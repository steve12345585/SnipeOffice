/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sal/config.h>

#include <string_view>

#include <config_features.h>
#include <rtl/character.hxx>
#include <rtl/malformeduriexception.hxx>
#include <rtl/uri.hxx>
#include <sot/exchange.hxx>
#include <svl/eitem.hxx>
#include <basic/sbstar.hxx>
#include <svl/stritem.hxx>
#include <svl/svdde.hxx>
#include <sfx2/lnkbase.hxx>
#include <sfx2/linkmgr.hxx>

#include <tools/debug.hxx>
#include <tools/urlobj.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <unotools/pathoptions.hxx>
#include <vcl/svapp.hxx>

#include <sfx2/app.hxx>
#include <appdata.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/docfile.hxx>
#include <ucbhelper/content.hxx>
#include <comphelper/processfactory.hxx>

#if defined(_WIN32)

static OUString SfxDdeServiceName_Impl( const OUString& sIn )
{
    OUStringBuffer sReturn(sIn.getLength());

    for ( sal_uInt16 n = sIn.getLength(); n; --n )
    {
        sal_Unicode cChar = sIn[n-1];
        if (rtl::isAsciiAlphanumeric(cChar))
            sReturn.append(cChar);
    }

    return sReturn.makeStringAndClear();
}

namespace {

class ImplDdeService : public DdeService
{
public:
    explicit ImplDdeService( const OUString& rNm )
        : DdeService( rNm )
    {}
    virtual bool MakeTopic( const OUString& );

    virtual OUString  Topics();

    virtual bool SysTopicExecute( const OUString* pStr );
};

    bool lcl_IsDocument( std::u16string_view rContent )
    {
        using namespace com::sun::star;

        bool bRet = false;
        INetURLObject aObj( rContent );
        DBG_ASSERT( aObj.GetProtocol() != INetProtocol::NotValid, "Invalid URL!" );

        try
        {
            ::ucbhelper::Content aCnt( aObj.GetMainURL( INetURLObject::DecodeMechanism::NONE ), uno::Reference< ucb::XCommandEnvironment >(), comphelper::getProcessComponentContext() );
            bRet = aCnt.isDocument();
        }
        catch( const uno::Exception& )
        {
            TOOLS_WARN_EXCEPTION( "sfx.appl", "" );
        }

        return bRet;
    }
}

bool ImplDdeService::MakeTopic( const OUString& rNm )
{
    // Workaround for Event after Main() under OS/2
    // happens when exiting starts the App again
    if ( !Application::IsInExecute() )
        return false;

    // The Topic rNm is sought, do we have it?
    // First only loop over the ObjectShells to find those
    // with the specific name:
    bool bRet = false;
    OUString sNm( rNm.toAsciiLowerCase() );
    SfxObjectShell* pShell = SfxObjectShell::GetFirst();
    while( pShell )
    {
        OUString sTmp( pShell->GetTitle(SFX_TITLE_FULLNAME) );
        if( sNm == sTmp.toAsciiLowerCase() )
        {
            SfxGetpApp()->AddDdeTopic( pShell );
            bRet = true;
            break;
        }
        pShell = SfxObjectShell::GetNext( *pShell );
    }

    if( !bRet )
    {
        bool abs;
        OUString url;
        try {
            url = rtl::Uri::convertRelToAbs(SvtPathOptions().GetWorkPath(), rNm);
            abs = true;
        } catch (rtl::MalformedUriException &) {
            abs = false;
        }
        if ( abs && lcl_IsDocument( url ) )
        {
            // File exists? then try to load it:
            SfxStringItem aName( SID_FILE_NAME, url );
            SfxBoolItem aNewView(SID_OPEN_NEW_VIEW, true);

            SfxBoolItem aSilent(SID_SILENT, true);
            const SfxPoolItemHolder aResult(SfxGetpApp()->GetDispatcher_Impl()->ExecuteList(SID_OPENDOC,
                SfxCallMode::SYNCHRON,
                { &aName, &aNewView, &aSilent }));

            if( auto const item = dynamic_cast< const SfxViewFrameItem *>(aResult.getItem());
                item &&
                item->GetFrame() &&
                nullptr != ( pShell = item->GetFrame()->GetObjectShell() ) )
            {
                SfxGetpApp()->AddDdeTopic( pShell );
                bRet = true;
            }
        }
    }
    return bRet;
}

OUString ImplDdeService::Topics()
{
    OUString sRet;
    if( GetSysTopic() )
        sRet += GetSysTopic()->GetName();

    SfxObjectShell* pShell = SfxObjectShell::GetFirst();
    while( pShell )
    {
        if( SfxViewFrame::GetFirst( pShell ) )
        {
            if( !sRet.isEmpty() )
                sRet += "\t";
            sRet += pShell->GetTitle(SFX_TITLE_FULLNAME);
        }
        pShell = SfxObjectShell::GetNext( *pShell );
    }
    if( !sRet.isEmpty() )
        sRet += "\r\n";
    return sRet;
}

bool ImplDdeService::SysTopicExecute( const OUString* pStr )
{
    return SfxApplication::DdeExecute( *pStr );
}
#endif

class SfxDdeDocTopic_Impl : public DdeTopic
{
#if defined(_WIN32)
public:
    SfxObjectShell* pSh;
    DdeData aData;
    css::uno::Sequence< sal_Int8 > aSeq;

    explicit SfxDdeDocTopic_Impl( SfxObjectShell* pShell )
        : DdeTopic( pShell->GetTitle(SFX_TITLE_FULLNAME) ), pSh( pShell )
    {}

    virtual DdeData* Get( SotClipboardFormatId ) override;
    virtual bool Put( const DdeData* ) override;
    virtual bool Execute( const OUString* ) override;
    virtual bool StartAdviseLoop() override;
    virtual bool MakeItem( const OUString& rItem ) override;
#endif
};


#if defined(_WIN32)

namespace {

/*  [Description]

    Checks if 'rCmd' of the event 'rEvent' is (without '(') and then assemble
    this data into a <ApplicationEvent>, which is then executed through
    <Application::AppEvent()>. If 'rCmd' is the given event 'rEvent', then
    TRUE is returned, otherwise FALSE.

    [Example]

    rCmd = "Open(\"d:\doc\doc.sdw\")"
    rEvent = "Open"
*/
bool SfxAppEvent_Impl( const OUString& rCmd, std::u16string_view rEvent,
                           ApplicationEvent::Type eType )
{
    OUString sEvent(OUString::Concat(rEvent) + "(");
    if (rCmd.startsWithIgnoreAsciiCase(sEvent))
    {
        sal_Int32 start = sEvent.getLength();
        if ( rCmd.getLength() - start >= 2 )
        {
            // Transform into the ApplicationEvent Format
            //TODO: I /assume/ that rCmd should match the syntax of
            // <http://msdn.microsoft.com/en-us/library/ms648995.aspx>
            // "WM_DDE_EXECUTE message" but does not (handle commands enclosed
            // in [...]; handle commas separating multiple arguments; handle
            // double "", ((, )), [[, ]] in quoted arguments); see also the mail
            // thread starting at <http://lists.freedesktop.org/archives/
            // libreoffice/2013-July/054779.html> "DDE on Windows."
            std::vector<OUString> aData;
            for ( sal_Int32 n = start; n < rCmd.getLength() - 1; )
            {
                // Resiliently read arguments either starting with " and
                // spanning to the next " (if any; TODO: do we need to undo any
                // escaping within the string?) or with neither " nor SPC and
                // spanning to the next SPC (if any; TODO: is this from not
                // wrapped in "..." relevant? it would have been parsed by the
                // original code even if that was only by accident, so I left it
                // in), with runs of SPCs treated like single ones:
                switch ( rCmd[n] )
                {
                case '"':
                    {
                        sal_Int32 i = rCmd.indexOf('"', ++n);
                        if (i < 0 || i > rCmd.getLength() - 1) {
                            i = rCmd.getLength() - 1;
                        }
                        aData.push_back(rCmd.copy(n, i - n));
                        n = i + 1;
                        break;
                    }
                case ' ':
                    ++n;
                    break;
                default:
                    {
                        sal_Int32 i = rCmd.indexOf(' ', n);
                        if (i < 0 || i > rCmd.getLength() - 1) {
                            i = rCmd.getLength() - 1;
                        }
                        aData.push_back(rCmd.copy(n, i - n));
                        n = i + 1;
                        break;
                    }
                }
            }

            GetpApp()->AppEvent( ApplicationEvent(eType, std::move(aData)) );
            return true;
        }
    }

    return false;
}

}

/*  Description]

    This method can be overridden by application developers, to receive
    DDE-commands directed to their SfxApplication subclass.

    The base implementation understands the API functionality of the
    relevant SfxApplication subclass in BASIC syntax. Return values can
    not be transferred, unfortunately.
*/
bool SfxApplication::DdeExecute( const OUString&   rCmd )  // Expressed in our BASIC-Syntax
{
    // Print or Open-Event?
    if ( !( SfxAppEvent_Impl( rCmd, u"Print", ApplicationEvent::Type::Print ) ||
            SfxAppEvent_Impl( rCmd, u"Open", ApplicationEvent::Type::Open ) ) )
    {
        // all others are BASIC
        StarBASIC* pBasic = GetBasic();
        DBG_ASSERT( pBasic, "Where is the Basic???" );
        SbxVariable* pRet = pBasic->Execute( rCmd );
        if( !pRet )
        {
            SbxBase::ResetError();
            return false;
        }
    }
    return true;
}

/*  [Description]

    This method can be overridden by application developers, to receive
    DDE-commands directed to the their SfxApplication subclass.

    The base implementation does nothing and returns 0.
*/
bool SfxObjectShell::DdeExecute( const OUString&   rCmd )  // Expressed in our BASIC-Syntax
{
#if !HAVE_FEATURE_SCRIPTING
    (void) rCmd;
#else
    StarBASIC* pBasic = GetBasic();
    DBG_ASSERT( pBasic, "Where is the Basic???" ) ;
    SbxVariable* pRet = pBasic->Execute( rCmd );
    if( !pRet )
    {
        SbxBase::ResetError();
        return false;
    }
#endif
    return true;
}

/*  [Description]

    This method can be overridden by application developers, to receive
    DDE-data-requests directed to their SfxApplication subclass.

    The base implementation provides no data and returns false.
*/
bool SfxObjectShell::DdeGetData( const OUString&,              // the Item to be addressed
                                 const OUString&,              // in: Format
                                 css::uno::Any& )// out: requested data
{
    return false;
}


/*  [Description]

    This method can be overridden by application developers, to receive
    DDE-data directed to their SfxApplication subclass.

    The base implementation is not receiving any data and returns false.
*/
bool SfxObjectShell::DdeSetData( const OUString&,                    // the Item to be addressed
                                 const OUString&,                    // in: Format
                                 const css::uno::Any& )// out: requested data
{
    return false;
}

#endif

/*  [Description]

    This method can be overridden by application developers, to establish
    a DDE-hotlink to their SfxApplication subclass.

    The base implementation is not generate a link and returns 0.
*/
::sfx2::SvLinkSource* SfxObjectShell::DdeCreateLinkSource( const OUString& ) // the Item to be addressed
{
    return nullptr;
}

void SfxObjectShell::ReconnectDdeLink(SfxObjectShell& /*rServer*/)
{
}

void SfxObjectShell::ReconnectDdeLinks(SfxObjectShell& rServer)
{
    SfxObjectShell* p = GetFirst(nullptr, false);
    while (p)
    {
        if (&rServer != p)
            p->ReconnectDdeLink(rServer);

        p = GetNext(*p, nullptr, false);
    }
}

bool SfxApplication::InitializeDde()
{
    int nError = 0;
#if defined(_WIN32)
    DBG_ASSERT( !pImpl->pDdeService,
                "Dde can not be initialized multiple times" );

    pImpl->pDdeService.reset(new ImplDdeService( Application::GetAppName() ));
    nError = pImpl->pDdeService->GetError();
    if( !nError )
    {
        // we certainly want to support RTF!
        pImpl->pDdeService->AddFormat( SotClipboardFormatId::RTF );
        pImpl->pDdeService->AddFormat( SotClipboardFormatId::RICHTEXT );

        // Config path as a topic because of multiple starts
        INetURLObject aOfficeLockFile( SvtPathOptions().GetUserConfigPath() );
        aOfficeLockFile.insertName( u"soffice.lck" );
        OUString aService( SfxDdeServiceName_Impl(
                    aOfficeLockFile.GetMainURL(INetURLObject::DecodeMechanism::ToIUri) ) );
        aService = aService.toAsciiUpperCase();
        pImpl->pDdeService2.reset( new ImplDdeService( aService ));
        pImpl->pTriggerTopic.reset(new SfxDdeTriggerTopic_Impl);
        pImpl->pDdeService2->AddTopic( *pImpl->pTriggerTopic );
    }
#endif
    return !nError;
}

void SfxAppData_Impl::DeInitDDE()
{
    pTriggerTopic.reset();
    pDdeService2.reset();
    maDocTopics.clear();
    pDdeService.reset();
}

#if defined(_WIN32)
void SfxApplication::AddDdeTopic( SfxObjectShell* pSh )
{
    //OV: DDE is disconnected in server mode!
    if( pImpl->maDocTopics.empty() )
        return;

    // prevent double submit
    OUString sShellNm;
    bool bFnd = false;
    for (size_t n = pImpl->maDocTopics.size(); n;)
    {
        if( pImpl->maDocTopics[ --n ]->pSh == pSh )
        {
            // If the document is untitled, is still a new Topic is created!
            if( !bFnd )
            {
                bFnd = true;
                sShellNm = pSh->GetTitle(SFX_TITLE_FULLNAME).toAsciiLowerCase();
            }
            OUString sNm( pImpl->maDocTopics[ n ]->GetName() );
            if( sShellNm == sNm.toAsciiLowerCase() )
                return ;
        }
    }

    SfxDdeDocTopic_Impl *const pTopic = new SfxDdeDocTopic_Impl(pSh);
    pImpl->maDocTopics.push_back(pTopic);
    pImpl->pDdeService->AddTopic( *pTopic );
}
#endif

void SfxApplication::RemoveDdeTopic( SfxObjectShell const * pSh )
{
#if defined(_WIN32)
    //OV: DDE is disconnected in server mode!
    if( pImpl->maDocTopics.empty() )
        return;

    for (size_t n = pImpl->maDocTopics.size(); n; )
    {
        SfxDdeDocTopic_Impl *const pTopic = pImpl->maDocTopics[ --n ];
        if (pTopic->pSh == pSh)
        {
            pImpl->pDdeService->RemoveTopic( *pTopic );
            delete pTopic;
            pImpl->maDocTopics.erase( pImpl->maDocTopics.begin() + n );
        }
    }
#else
    (void) pSh;
#endif
}

const DdeService* SfxApplication::GetDdeService() const
{
    return pImpl->pDdeService.get();
}

DdeService* SfxApplication::GetDdeService()
{
    return pImpl->pDdeService.get();
}

#if defined(_WIN32)

DdeData* SfxDdeDocTopic_Impl::Get(SotClipboardFormatId nFormat)
{
    OUString sMimeType( SotExchange::GetFormatMimeType( nFormat ));
    css::uno::Any aValue;
    bool bRet = pSh->DdeGetData( GetCurItem(), sMimeType, aValue );
    if( bRet && aValue.hasValue() && ( aValue >>= aSeq ) )
    {
        aData = DdeData( aSeq.getConstArray(), aSeq.getLength(), nFormat );
        return &aData;
    }
    aSeq.realloc( 0 );
    return nullptr;
}

bool SfxDdeDocTopic_Impl::Put( const DdeData* pData )
{
    aSeq = css::uno::Sequence< sal_Int8 >(
                            static_cast<sal_Int8 const *>(pData->getData()), pData->getSize() );
    bool bRet;
    if( aSeq.getLength() )
    {
        css::uno::Any aValue;
        aValue <<= aSeq;
        OUString sMimeType( SotExchange::GetFormatMimeType( pData->GetFormat() ));
        bRet = pSh->DdeSetData( GetCurItem(), sMimeType, aValue );
    }
    else
        bRet = false;
    return bRet;
}

bool SfxDdeDocTopic_Impl::Execute( const OUString* pStr )
{
    return pStr && pSh->DdeExecute( *pStr );
}

bool SfxDdeDocTopic_Impl::MakeItem( const OUString& rItem )
{
    AddItem( DdeItem( rItem ) );
    return true;
}

bool SfxDdeDocTopic_Impl::StartAdviseLoop()
{
    bool bRet = false;
    ::sfx2::SvLinkSource* pNewObj = pSh->DdeCreateLinkSource( GetCurItem() );
    if( pNewObj )
    {
        // then we also establish a corresponding SvBaseLink
        OUString sNm, sTmp( Application::GetAppName() );
        ::sfx2::MakeLnkName( sNm, &sTmp, pSh->GetTitle(SFX_TITLE_FULLNAME), GetCurItem() );
        new ::sfx2::SvBaseLink( sNm, sfx2::SvBaseLinkObjectType::DdeExternal, pNewObj );
        bRet = true;
    }
    return bRet;
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <config_features.h>

#include <macroloader.hxx>

#include <com/sun/star/frame/DispatchResultState.hpp>
#include <basic/basmgr.hxx>
#include <basic/sbuno.hxx>
#include <basic/sberrors.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/weakref.hxx>
#include <framework/documentundoguard.hxx>
#include <sfx2/app.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/objsh.hxx>
#include <tools/urlobj.hxx>
#include <vcl/svapp.hxx>

#include <memory>

using namespace ::com::sun::star;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;

SfxMacroLoader::SfxMacroLoader(const css::uno::Sequence< css::uno::Any >& aArguments)
{
    Reference < XFrame > xFrame;
    if ( aArguments.hasElements() )
    {
        aArguments[0] >>= xFrame;
        m_xFrame = xFrame;
    }
}

OUString SAL_CALL SfxMacroLoader::getImplementationName()
{
    return u"com.sun.star.comp.sfx2.SfxMacroLoader"_ustr;
}

sal_Bool SAL_CALL SfxMacroLoader::supportsService(OUString const & ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> SAL_CALL SfxMacroLoader::getSupportedServiceNames()
{
    return { u"com.sun.star.frame.ProtocolHandler"_ustr };
}

SfxObjectShell* SfxMacroLoader::GetObjectShell(const Reference <XFrame>& xFrame)
{
    SfxObjectShell* pDocShell = nullptr;

    if ( xFrame.is() )
    {
        SfxFrame* pFrame=nullptr;
        for ( pFrame = SfxFrame::GetFirst(); pFrame; pFrame = SfxFrame::GetNext( *pFrame ) )
        {
            if ( pFrame->GetFrameInterface() == xFrame )
                break;
        }

        if ( pFrame )
            pDocShell = pFrame->GetCurrentDocument();
    }

    return pDocShell;
}

SfxObjectShell* SfxMacroLoader::GetObjectShell_Impl()
{
    Reference < XFrame > xFrame( m_xFrame.get(), UNO_QUERY );
    return SfxMacroLoader::GetObjectShell(xFrame);
}

uno::Reference<frame::XDispatch> SAL_CALL SfxMacroLoader::queryDispatch(
    const util::URL&   aURL            ,
    const OUString&               /*sTargetFrameName*/,
    sal_Int32                            /*nSearchFlags*/    )
{
    uno::Reference<frame::XDispatch> xDispatcher;
    if(aURL.Complete.startsWith("macro:"))
        xDispatcher = this;
    return xDispatcher;
}


uno::Sequence< uno::Reference<frame::XDispatch> > SAL_CALL
                SfxMacroLoader::queryDispatches( const uno::Sequence < frame::DispatchDescriptor >& seqDescriptor )
{
    sal_Int32 nCount = seqDescriptor.getLength();
    uno::Sequence< uno::Reference<frame::XDispatch> > lDispatcher(nCount);
    std::transform(seqDescriptor.begin(), seqDescriptor.end(), lDispatcher.getArray(),
        [this](const frame::DispatchDescriptor& rDescr) -> uno::Reference<frame::XDispatch> {
            return queryDispatch(rDescr.FeatureURL, rDescr.FrameName, rDescr.SearchFlags); });
    return lDispatcher;
}


void SAL_CALL SfxMacroLoader::dispatchWithNotification(
    const util::URL& aURL, const uno::Sequence<beans::PropertyValue>& /*lArgs*/,
    const uno::Reference<frame::XDispatchResultListener>& xListener )
{
    SolarMutexGuard aGuard;

    uno::Any aAny;
    ErrCode nErr = loadMacro( aURL.Complete, aAny, GetObjectShell_Impl() );
    if( !xListener.is() )
        return;

    // always call dispatchFinished(), because we didn't load a document but
    // executed a macro instead!
    frame::DispatchResultEvent aEvent;

    aEvent.Source = getXWeak();
    if( nErr == ERRCODE_NONE )
        aEvent.State = frame::DispatchResultState::SUCCESS;
    else
        aEvent.State = frame::DispatchResultState::FAILURE;

    xListener->dispatchFinished( aEvent ) ;
}

uno::Any SAL_CALL SfxMacroLoader::dispatchWithReturnValue(
    const util::URL& aURL, const uno::Sequence<beans::PropertyValue>& )
{
    uno::Any aRet;
    ErrCode nErr = loadMacro( aURL.Complete, aRet, GetObjectShell_Impl() );

    // aRet gets set to a different value only if nErr == ERRCODE_NONE
    // Return it in such case to preserve the original behaviour

    // In all other cases (nErr != ERRCODE_NONE), the calling code gets
    // the actual error code back
    if ( nErr != ERRCODE_NONE )
    {
        beans::PropertyValue aErrorCode;

        aErrorCode.Name = "ErrorCode";
        aErrorCode.Value <<= sal_uInt32(nErr);

        aRet <<= aErrorCode;
    }

    return aRet;
}

void SAL_CALL SfxMacroLoader::dispatch(
    const util::URL& aURL, const uno::Sequence<beans::PropertyValue>& /*lArgs*/ )
{
    SolarMutexGuard aGuard;

    uno::Any aAny;
    loadMacro( aURL.Complete, aAny, GetObjectShell_Impl() );
}

void SAL_CALL SfxMacroLoader::addStatusListener(
    const uno::Reference< frame::XStatusListener >& ,
    const util::URL&                                                    )
{
    /* TODO
            How we can handle different listener for further coming or currently running dispatch() jobs
            without any inconsistency!
     */
}


void SAL_CALL SfxMacroLoader::removeStatusListener(
    const uno::Reference< frame::XStatusListener >&,
    const util::URL&                                                  )
{
}

ErrCode SfxMacroLoader::loadMacro( const OUString& rURL, css::uno::Any& rRetval, SfxObjectShell* pSh )
{
#if !HAVE_FEATURE_SCRIPTING
    (void) rURL;
    (void) rRetval;
    (void) pSh;
    return ERRCODE_BASIC_PROC_UNDEFINED;
#else
    SfxObjectShell* pCurrent = pSh;
    if ( !pCurrent )
        // all not full qualified names use the BASIC of the given or current document
        pCurrent = SfxObjectShell::Current();

    // 'macro:///lib.mod.proc(args)' => macro of App-BASIC
    // 'macro://[docname|.]/lib.mod.proc(args)' => macro of current or qualified document
    // 'macro://obj.method(args)' => direct API call, execute it via App-BASIC
    const OUString& aMacro( rURL );
    sal_Int32 nThirdSlashPos = aMacro.indexOf( '/', 8 );
    sal_Int32 nArgsPos = aMacro.indexOf( '(' );
    BasicManager *pAppMgr = SfxApplication::GetBasicManager();
    BasicManager *pBasMgr = nullptr;
    ErrCode nErr = ERRCODE_NONE;

    // should a macro function be executed ( no direct API call)?
    if ( -1 != nThirdSlashPos && ( -1 == nArgsPos || nThirdSlashPos < nArgsPos ) )
    {
        // find BasicManager
        SfxObjectShell* pDoc = nullptr;
        OUString aBasMgrName( INetURLObject::decode(aMacro.subView( 8, nThirdSlashPos-8 ), INetURLObject::DecodeMechanism::WithCharset) );
        if ( aBasMgrName.isEmpty() )
            pBasMgr = pAppMgr;
        else if ( aBasMgrName == "." )
        {
            // current/actual document
            pDoc = pCurrent;
            if (pDoc)
                pBasMgr = pDoc->GetBasicManager();
        }
        else
        {
            // full qualified name, find document by name
            for ( SfxObjectShell *pObjSh = SfxObjectShell::GetFirst();
                    pObjSh && !pBasMgr;
                    pObjSh = SfxObjectShell::GetNext(*pObjSh) )
                if ( aBasMgrName == pObjSh->GetTitle(SFX_TITLE_APINAME) )
                {
                    pDoc = pObjSh;
                    pBasMgr = pDoc->GetBasicManager();
                }
        }

        if ( pBasMgr )
        {
            const bool bIsAppBasic = ( pBasMgr == pAppMgr );
            const bool bIsDocBasic = ( pBasMgr != pAppMgr );

            if ( pDoc )
            {
                // security check for macros from document basic if an SFX doc is given
                if ( !pDoc->AdjustMacroMode() )
                    // check forbids execution
                    return ERRCODE_IO_ACCESSDENIED;
            }

            // find BASIC method
            OUString aQualifiedMethod( INetURLObject::decode(aMacro.subView( nThirdSlashPos+1 ), INetURLObject::DecodeMechanism::WithCharset) );
            OUString aArgs;
            if ( -1 != nArgsPos )
            {
                // remove arguments from macro name
                aArgs = aQualifiedMethod.copy( nArgsPos - nThirdSlashPos - 1 );
                aQualifiedMethod = aQualifiedMethod.copy( 0, nArgsPos - nThirdSlashPos - 1 );
            }

            if ( pBasMgr->HasMacro( aQualifiedMethod ) )
            {
                Any aOldThisComponent;
                const bool bSetDocMacroMode = ( pDoc != nullptr ) && bIsDocBasic;
                const bool bSetGlobalThisComponent = ( pDoc != nullptr ) && bIsAppBasic;
                if ( bSetDocMacroMode )
                {
                    // mark document: it executes an own macro, so it's in a modal mode
                    pDoc->SetMacroMode_Impl();
                }

                if ( bSetGlobalThisComponent )
                {
                    // document is executed via AppBASIC, adjust ThisComponent variable
                    pAppMgr->SetGlobalUNOConstant( u"ThisComponent"_ustr, Any( pDoc->GetModel() ), &aOldThisComponent );
                }

                // just to let the shell be alive
                SfxObjectShellRef xKeepDocAlive = pDoc;

                {
                    // attempt to protect the document against the script tampering with its Undo Context
                    std::optional< ::framework::DocumentUndoGuard > pUndoGuard;
                    if ( bIsDocBasic )
                        pUndoGuard.emplace( pDoc->GetModel() );

                    // execute the method
                    SbxVariableRef retValRef = new SbxVariable;
                    nErr = pBasMgr->ExecuteMacro( aQualifiedMethod, aArgs, retValRef.get() );
                    if ( nErr == ERRCODE_NONE )
                        rRetval = sbxToUnoValue( retValRef.get() );
                }

                if ( bSetGlobalThisComponent )
                {
                    pAppMgr->SetGlobalUNOConstant( u"ThisComponent"_ustr, aOldThisComponent );
                }

                if ( bSetDocMacroMode )
                {
                    // remove flag for modal mode
                    pDoc->SetMacroMode_Impl( false );
                }
            }
            else
                nErr = ERRCODE_BASIC_PROC_UNDEFINED;
        }
        else
            nErr = ERRCODE_IO_NOTEXISTS;
    }
    else
    {
        // direct API call on a specified object
        OUString aCall =
            "[" +
            INetURLObject::decode(aMacro.subView(6),
                    INetURLObject::DecodeMechanism::WithCharset) +
            "]";
        pAppMgr->GetLib(0)->Execute(aCall);
        nErr = SbxBase::GetError();
    }

    SbxBase::ResetError();
    return nErr;
#endif
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_sfx2_SfxMacroLoader_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &arguments)
{
    return cppu::acquire(new SfxMacroLoader(arguments));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XEnumeration.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/container/XContainerQuery.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <comphelper/sequenceashashmap.hxx>

#include <sot/exchange.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <svl/stritem.hxx>

#include <comphelper/processfactory.hxx>

#include <sal/types.h>
#include <com/sun/star/uno/Reference.hxx>
#include <unotools/moduleoptions.hxx>
#include <unotools/mediadescriptor.hxx>
#include <tools/urlobj.hxx>

#include <unotools/syslocale.hxx>
#include <unotools/charclass.hxx>

#include <sfx2/docfilt.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfxtypes.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/strings.hrc>
#include <sfx2/sfxresid.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/sfxsids.hrc>
#include "fltlst.hxx"
#include <arrdecl.hxx>

#include <vector>
#include <memory>

#if defined(DBG_UTIL)
unsigned SfxStack::nLevel = 0;
#endif

using namespace com::sun::star;

static SfxFilterList_Impl* pFilterArr = nullptr;
static bool bFirstRead = true;

static void CreateFilterArr()
{
    static SfxFilterList_Impl theSfxFilterArray;
    pFilterArr = &theSfxFilterArray;
    static SfxFilterListener theSfxFilterListener;
}

static OUString ToUpper_Impl( const OUString &rStr )
{
    return SvtSysLocale().GetCharClass().uppercase( rStr );
}

class SfxFilterContainer_Impl
{
public:
    OUString            aName;

    explicit SfxFilterContainer_Impl( OUString _aName )
        : aName(std::move( _aName ))
    {
    }
};

std::shared_ptr<const SfxFilter> SfxFilterContainer::GetFilter4EA(const OUString& rEA, SfxFilterFlags nMust, SfxFilterFlags nDont) const
{
    SfxFilterMatcher aMatch(pImpl->aName);
    return aMatch.GetFilter4EA(rEA, nMust, nDont);
}

std::shared_ptr<const SfxFilter> SfxFilterContainer::GetFilter4Extension(const OUString& rExt, SfxFilterFlags nMust, SfxFilterFlags nDont) const
{
    SfxFilterMatcher aMatch(pImpl->aName);
    return aMatch.GetFilter4Extension(rExt, nMust, nDont);
}

std::shared_ptr<const SfxFilter> SfxFilterContainer::GetFilter4FilterName(const OUString& rName, SfxFilterFlags nMust, SfxFilterFlags nDont) const
{
    SfxFilterMatcher aMatch(pImpl->aName);
    return aMatch.GetFilter4FilterName(rName, nMust, nDont);
}

std::shared_ptr<const SfxFilter> SfxFilterContainer::GetAnyFilter( SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    SfxFilterMatcher aMatch( pImpl->aName );
    return aMatch.GetAnyFilter( nMust, nDont );
}


SfxFilterContainer::SfxFilterContainer( const OUString& rName )
   : pImpl( new SfxFilterContainer_Impl( rName ) )
{
}


SfxFilterContainer::~SfxFilterContainer()
{
}


OUString const & SfxFilterContainer::GetName() const
{
    return pImpl->aName;
}

std::shared_ptr<const SfxFilter> SfxFilterContainer::GetDefaultFilter_Impl( std::u16string_view rName )
{
    // Try to find out the type of factory.
    // Interpret given name as Service- and ShortName!
    SvtModuleOptions aOpt;
    SvtModuleOptions::EFactory eFactory = SvtModuleOptions::ClassifyFactoryByServiceName(rName);
    if (eFactory == SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
        eFactory = SvtModuleOptions::ClassifyFactoryByShortName(rName);

    // could not classify factory by its service nor by its short name.
    // Must be an unknown factory! => return NULL
    if (eFactory == SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
        return nullptr;

    // For the following code we need some additional information.
    const OUString& sServiceName   = aOpt.GetFactoryName(eFactory);
    OUString sDefaultFilter = aOpt.GetFactoryDefaultFilter(eFactory);

    // Try to get the default filter. Don't forget to verify it.
    // May the set default filter does not exists any longer or
    // does not fit the given factory.
    const SfxFilterMatcher aMatcher;
    std::shared_ptr<const SfxFilter> pFilter = aMatcher.GetFilter4FilterName(sDefaultFilter);

    if (
        pFilter &&
        !pFilter->GetServiceName().equalsIgnoreAsciiCase(sServiceName)
       )
    {
        pFilter = nullptr;
    }

    // If at least no default filter could be located - use any filter of this
    // factory.
    if (!pFilter)
    {
        if ( bFirstRead )
            ReadFilters_Impl();

        for (const std::shared_ptr<const SfxFilter>& pCheckFilter : *pFilterArr)
        {
            if ( pCheckFilter->GetServiceName().equalsIgnoreAsciiCase(sServiceName) )
            {
                pFilter = pCheckFilter;
                break;
            }
        }
    }

    return pFilter;
}


// Impl-Data is shared between all FilterMatchers of the same factory
class SfxFilterMatcher_Impl
{
public:
    OUString                    aName;
    mutable SfxFilterList_Impl* pList;      // is created on demand

    void InitForIterating() const;
    void Update() const;
    explicit SfxFilterMatcher_Impl(OUString _aName)
        : aName(std::move(_aName))
        , pList(nullptr)
    {
    }
    ~SfxFilterMatcher_Impl()
    {
        // SfxFilterMatcher_Impl::InitForIterating() will set pList to
        // either the global filter array matcher pFilterArr, or to
        // a new SfxFilterList_Impl.
        if (pList != pFilterArr)
            delete pList;
    }
};

namespace
{
    std::vector<std::unique_ptr<SfxFilterMatcher_Impl> > aImplArr;
    int nSfxFilterMatcherCount;

    SfxFilterMatcher_Impl & getSfxFilterMatcher_Impl(const OUString &rName)
    {
        OUString aName;

        if (!rName.isEmpty())
            aName = SfxObjectShell::GetServiceNameFromFactory(rName);

        // find the impl-Data of any comparable FilterMatcher that was created
        // previously
        for (std::unique_ptr<SfxFilterMatcher_Impl>& aImpl : aImplArr)
            if (aImpl->aName == aName)
                return *aImpl;

        // first Matcher created for this factory
        aImplArr.push_back(std::make_unique<SfxFilterMatcher_Impl>(aName));
        return *aImplArr.back();
    }
}

SfxFilterMatcher::SfxFilterMatcher( const OUString& rName )
    : m_rImpl( getSfxFilterMatcher_Impl(rName) )
{
    ++nSfxFilterMatcherCount;
}

SfxFilterMatcher::SfxFilterMatcher()
    : m_rImpl( getSfxFilterMatcher_Impl(OUString()) )
{
    // global FilterMatcher always uses global filter array (also created on
    // demand)
    ++nSfxFilterMatcherCount;
}

SfxFilterMatcher::~SfxFilterMatcher()
{
    --nSfxFilterMatcherCount;
    if (nSfxFilterMatcherCount == 0)
        aImplArr.clear();
}

void SfxFilterMatcher_Impl::Update() const
{
    if ( pList )
    {
        // this List was already used
        pList->clear();
        for (const std::shared_ptr<const SfxFilter>& pFilter : *pFilterArr)
        {
            if ( pFilter->GetServiceName() == aName )
                pList->push_back( pFilter );
        }
    }
}

void SfxFilterMatcher_Impl::InitForIterating() const
{
    if ( pList )
        return;

    if ( bFirstRead )
        // global filter array has not been created yet
        SfxFilterContainer::ReadFilters_Impl();

    if ( !aName.isEmpty() )
    {
        // matcher of factory: use only filters of that document type
        pList = new SfxFilterList_Impl;
        Update();
    }
    else
    {
        // global matcher: use global filter array
        pList = pFilterArr;
    }
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetAnyFilter( SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    m_rImpl.InitForIterating();
    for (const std::shared_ptr<const SfxFilter>& pFilter : *m_rImpl.pList)
    {
        SfxFilterFlags nFlags = pFilter->GetFilterFlags();
        if ( (nFlags & nMust) == nMust && !(nFlags & nDont ) )
            return pFilter;
    }

    return nullptr;
}


ErrCode  SfxFilterMatcher::GuessFilterIgnoringContent(
    SfxMedium const & rMedium,
    std::shared_ptr<const SfxFilter>& rpFilter ) const
{
    uno::Reference<document::XTypeDetection> xDetection(
        comphelper::getProcessServiceFactory()->createInstance(u"com.sun.star.document.TypeDetection"_ustr), uno::UNO_QUERY);

    OUString sTypeName;
    try
    {
        sTypeName = xDetection->queryTypeByURL( rMedium.GetURLObject().GetMainURL( INetURLObject::DecodeMechanism::NONE ) );
    }
    catch (uno::Exception&)
    {
    }

    rpFilter = nullptr;
    if ( !sTypeName.isEmpty() )
    {
        // make sure filter list is initialized
        m_rImpl.InitForIterating();
        rpFilter = GetFilter4EA( sTypeName );
    }

    return rpFilter ? ERRCODE_NONE : ERRCODE_ABORT;
}


ErrCode  SfxFilterMatcher::GuessFilter( SfxMedium& rMedium, std::shared_ptr<const SfxFilter>& rpFilter, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    return GuessFilterControlDefaultUI( rMedium, rpFilter, nMust, nDont );
}


ErrCode  SfxFilterMatcher::GuessFilterControlDefaultUI( SfxMedium& rMedium, std::shared_ptr<const SfxFilter>& rpFilter, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    std::shared_ptr<const SfxFilter> pOldFilter = rpFilter;

    // no detection service -> nothing to do !
    uno::Reference<document::XTypeDetection> xDetection(
        comphelper::getProcessServiceFactory()->createInstance(u"com.sun.star.document.TypeDetection"_ustr), uno::UNO_QUERY);

    if (!xDetection.is())
        return ERRCODE_ABORT;

    try
    {
        // open the stream one times only ...
        // Otherwise it will be tried more than once and show the same interaction more than once ...

        OUString sURL( rMedium.GetURLObject().GetMainURL( INetURLObject::DecodeMechanism::NONE ) );
        uno::Reference< io::XInputStream > xInStream = rMedium.GetInputStream();
        OUString aFilterName;
        OUString sTypeName;

        // stream exists => deep detection (with preselection ... if possible)
        if (xInStream.is())
        {
            utl::MediaDescriptor aDescriptor;

            aDescriptor[utl::MediaDescriptor::PROP_URL               ] <<= sURL;
            aDescriptor[utl::MediaDescriptor::PROP_INPUTSTREAM       ] <<= xInStream;
            aDescriptor[utl::MediaDescriptor::PROP_INTERACTIONHANDLER] <<= rMedium.GetInteractionHandler();
            SfxStringItem const * it = rMedium.GetItemSet().GetItem(SID_REFERER);
            if (it != nullptr) {
                aDescriptor[utl::MediaDescriptor::PROP_REFERRER]
                    <<= it->GetValue();
            }

            if ( !m_rImpl.aName.isEmpty() )
                aDescriptor[utl::MediaDescriptor::PROP_DOCUMENTSERVICE] <<= m_rImpl.aName;

            if ( pOldFilter )
            {
                aDescriptor[utl::MediaDescriptor::PROP_TYPENAME  ] <<= pOldFilter->GetTypeName();
                aDescriptor[utl::MediaDescriptor::PROP_FILTERNAME] <<= pOldFilter->GetFilterName();
            }

            uno::Sequence< beans::PropertyValue > lDescriptor = aDescriptor.getAsConstPropertyValueList();
            sTypeName = xDetection->queryTypeByDescriptor(lDescriptor, true); // lDescriptor is used as In/Out param ... don't use aDescriptor.getAsConstPropertyValueList() directly!

            for (const auto& rProp : lDescriptor)
            {
                if (rProp.Name == "FilterName")
                    // Type detection picked a preferred filter for this format.
                    aFilterName = rProp.Value.get<OUString>();
            }
        }
        // no stream exists => try flat detection without preselection as fallback
        else
            sTypeName = xDetection->queryTypeByURL(sURL);

        if (!sTypeName.isEmpty())
        {
            std::shared_ptr<const SfxFilter> xNewFilter;
            if (!aFilterName.isEmpty())
                // Type detection returned a suitable filter for this.  Use it.
                xNewFilter = SfxFilter::GetFilterByName(aFilterName);

            // fdo#78742 respect requested document service if set
            if (!xNewFilter || (!m_rImpl.aName.isEmpty()
                             && m_rImpl.aName != xNewFilter->GetServiceName()))
            {
                // detect filter by given type
                // In case of this matcher is bound to a particular document type:
                // If there is no acceptable type for this document at all, the type detection has possibly returned something else.
                // The DocumentService property is only a preselection, and all preselections are considered as optional!
                // This "wrong" type will be sorted out now because we match only allowed filters to the detected type
                uno::Sequence< beans::NamedValue > lQuery { { u"Name"_ustr, css::uno::Any(sTypeName) } };

                xNewFilter = GetFilterForProps(lQuery, nMust, nDont);
            }

            if (xNewFilter)
            {
                rpFilter = std::move(xNewFilter);
                return ERRCODE_NONE;
            }
        }
    }
    catch (const uno::Exception&)
    {}

    return ERRCODE_ABORT;
}


bool SfxFilterMatcher::IsFilterInstalled_Impl( const std::shared_ptr<const SfxFilter>& pFilter )
{
    if ( pFilter->GetFilterFlags() & SfxFilterFlags::MUSTINSTALL )
    {
        // Here could a  re-installation be offered
        OUString aText( SfxResId(STR_FILTER_NOT_INSTALLED) );
        aText = aText.replaceFirst( "$(FILTER)", pFilter->GetUIName() );
        std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(nullptr,
                                                       VclMessageType::Question, VclButtonsType::YesNo,
                                                       aText));
        xQueryBox->set_default_response(RET_YES);

        short nRet = xQueryBox->run();
        if ( nRet == RET_YES )
        {
#ifdef DBG_UTIL
            // Start Setup
            std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(nullptr,
                                                          VclMessageType::Info, VclButtonsType::Ok,
                                                          u"Here should the Setup now be starting!"_ustr));
            xInfoBox->run();
#endif
            // Installation must still give feedback if it worked or not,
            // then the  Filterflag be deleted
        }

        return ( !(pFilter->GetFilterFlags() & SfxFilterFlags::MUSTINSTALL) );
    }
    else if ( pFilter->GetFilterFlags() & SfxFilterFlags::CONSULTSERVICE )
    {
        OUString aText( SfxResId(STR_FILTER_CONSULT_SERVICE) );
        aText = aText.replaceFirst( "$(FILTER)", pFilter->GetUIName() );
        std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(nullptr,
                                                      VclMessageType::Info, VclButtonsType::Ok,
                                                      aText));
        xInfoBox->run();
        return false;
    }
    else
        return true;
}


ErrCode SfxFilterMatcher::DetectFilter( SfxMedium& rMedium, std::shared_ptr<const SfxFilter>& rpFilter ) const
/*  [Description]

    Here the Filter selection box is pulled up. Otherwise GuessFilter
 */

{
    std::shared_ptr<const SfxFilter> pFilter = rMedium.GetFilter();
    if ( pFilter )
    {
        if( !IsFilterInstalled_Impl( pFilter ) )
            pFilter = nullptr;
        else
        {
            const SfxStringItem* pSalvageItem = rMedium.GetItemSet().GetItem(SID_DOC_SALVAGE, false);
            if ( ( pFilter->GetFilterFlags() & SfxFilterFlags::PACKED ) && pSalvageItem )
                // Salvage is always done without packing
                pFilter = nullptr;
        }
    }

    bool bPreview = rMedium.IsPreview_Impl();
    const SfxStringItem* pReferer = rMedium.GetItemSet().GetItem(SID_REFERER, false);
    if ( bPreview && rMedium.IsRemote() && ( !pReferer || !pReferer->GetValue().match("private:searchfolder:") ) )
        return ERRCODE_ABORT;

    ErrCode nErr = GuessFilter( rMedium, pFilter );
    if ( nErr == ERRCODE_ABORT )
        return nErr;

    if ( nErr == ERRCODE_IO_PENDING )
    {
        rpFilter = pFilter;
        return nErr;
    }

    if ( !pFilter )
    {
        std::shared_ptr<const SfxFilter> pInstallFilter;

        // Now test the filter which are not installed (ErrCode is irrelevant)
        GuessFilter( rMedium, pInstallFilter, SfxFilterFlags::IMPORT, SfxFilterFlags::CONSULTSERVICE );
        if ( pInstallFilter )
        {
            if ( IsFilterInstalled_Impl( pInstallFilter ) )
            {
                // Maybe the filter was installed afterwards.
                pFilter = std::move(pInstallFilter);
            }
        }
        else
        {
          // Now test the filter, which first must be obtained by Star
          // (ErrCode is irrelevant)
            GuessFilter( rMedium, pInstallFilter, SfxFilterFlags::IMPORT, SfxFilterFlags::NONE );
            if ( pInstallFilter )
                IsFilterInstalled_Impl( pInstallFilter );
        }
    }

    bool bHidden = bPreview;
    const SfxStringItem* pFlags = rMedium.GetItemSet().GetItem(SID_OPTIONS, false);
    if ( !bHidden && pFlags )
    {
        OUString aFlags( pFlags->GetValue() );
        aFlags = aFlags.toAsciiUpperCase();
        if( -1 != aFlags.indexOf( 'H' ) )
            bHidden = true;
    }
    rpFilter = pFilter;

    if ( bHidden )
        nErr = pFilter ? ERRCODE_NONE : ERRCODE_ABORT;
    return nErr;
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilterForProps( const css::uno::Sequence < beans::NamedValue >& aSeq, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    uno::Reference< lang::XMultiServiceFactory > xServiceManager = ::comphelper::getProcessServiceFactory();
    if( !xServiceManager )
        return nullptr;

    static constexpr OUStringLiteral sTypeDetection = u"com.sun.star.document.TypeDetection";
    uno::Reference< container::XContainerQuery > xTypeCFG( xServiceManager->createInstance( sTypeDetection ), uno::UNO_QUERY );
    if ( !xTypeCFG )
        return nullptr;

    // make query for all types matching the properties
    uno::Reference < css::container::XEnumeration > xEnum = xTypeCFG->createSubSetEnumerationByProperties( aSeq );
    uno::Sequence<beans::PropertyValue> aProps;
    while ( xEnum->hasMoreElements() )
    {
        static constexpr OUStringLiteral sPreferredFilter = u"PreferredFilter";
        static constexpr OUStringLiteral sName = u"Name";

        xEnum->nextElement() >>= aProps;
        OUString aValue, aName;
        for( const auto & rPropVal : aProps)
        {
            if (rPropVal.Name == sPreferredFilter)
                rPropVal.Value >>= aValue;
            else if (rPropVal.Name == sName)
                rPropVal.Value >>= aName;
        }

        // try to get the preferred filter (works without loading all filters!)
        if ( !aValue.isEmpty() )
        {
            std::shared_ptr<const SfxFilter> pFilter = SfxFilter::GetFilterByName( aValue );
            if ( !pFilter || (pFilter->GetFilterFlags() & nMust) != nMust || (pFilter->GetFilterFlags() & nDont ) )
                // check for filter flags
                // pFilter == 0: if preferred filter is a Writer filter, but Writer module is not installed
                continue;

            if ( !m_rImpl.aName.isEmpty() )
            {
                // if this is not the global FilterMatcher: check if filter matches the document type
                if ( pFilter->GetServiceName() != m_rImpl.aName )
                {
                    // preferred filter belongs to another document type; now we must search the filter
                    m_rImpl.InitForIterating();
                    pFilter = GetFilter4EA( aName, nMust, nDont );
                    if ( pFilter )
                        return pFilter;
                }
                else
                    return pFilter;
            }
            else
                return pFilter;
        }
    }

    return nullptr;
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4Mime( const OUString& rMediaType, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    if ( m_rImpl.pList )
    {
        for (const std::shared_ptr<const SfxFilter>& pFilter : *m_rImpl.pList)
        {
            SfxFilterFlags nFlags = pFilter->GetFilterFlags();
            if ( (nFlags & nMust) == nMust && !(nFlags & nDont ) && pFilter->GetMimeType() == rMediaType )
                return pFilter;
        }

        return nullptr;
    }

    css::uno::Sequence < css::beans::NamedValue > aSeq { { u"MediaType"_ustr, css::uno::Any(rMediaType) } };
    return GetFilterForProps( aSeq, nMust, nDont );
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4EA( const OUString& rType, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    if ( m_rImpl.pList )
    {
        std::shared_ptr<const SfxFilter> pFirst;
        for (const std::shared_ptr<const SfxFilter>& pFilter : *m_rImpl.pList)
        {
            SfxFilterFlags nFlags = pFilter->GetFilterFlags();
            if ( (nFlags & nMust) == nMust && !(nFlags & nDont ) && pFilter->GetTypeName() == rType )
            {
                if (nFlags & SfxFilterFlags::PREFERED)
                    return pFilter;
                if (!pFirst)
                    pFirst = pFilter;
            }
        }
        if (pFirst)
            return pFirst;

        return nullptr;
    }

    css::uno::Sequence < css::beans::NamedValue > aSeq { { u"Name"_ustr, css::uno::Any(rType) } };
    return GetFilterForProps( aSeq, nMust, nDont );
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4Extension( const OUString& rExt, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    if ( m_rImpl.pList )
    {
        if (OUString sExt = ToUpper_Impl(rExt); !sExt.isEmpty())
        {
            if (sExt[0] != '.')
                sExt = "." + sExt;

            for (const std::shared_ptr<const SfxFilter>& pFilter : *m_rImpl.pList)
            {
                SfxFilterFlags nFlags = pFilter->GetFilterFlags();
                if ((nFlags & nMust) == nMust && !(nFlags & nDont))
                {
                    OUString sWildCard = ToUpper_Impl(pFilter->GetWildcard().getGlob());

                    WildCard aCheck(sWildCard, ';');
                    if (aCheck.Matches(sExt))
                        return pFilter;
                }
            }
        }

        return nullptr;
    }

    // Use extension without dot!
    OUString sExt( rExt );
    if ( sExt.startsWith(".") )
        sExt = sExt.copy(1);

    css::uno::Sequence < css::beans::NamedValue > aSeq
        { { u"Extensions"_ustr, css::uno::Any(uno::Sequence < OUString > { sExt } ) } };
    return GetFilterForProps( aSeq, nMust, nDont );
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4ClipBoardId( SotClipboardFormatId nId, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    if (nId == SotClipboardFormatId::NONE)
        return nullptr;

    css::uno::Sequence < css::beans::NamedValue > aSeq
        { { u"ClipboardFormat"_ustr, css::uno::Any(SotExchange::GetFormatName( nId )) } };
    return GetFilterForProps( aSeq, nMust, nDont );
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4UIName( std::u16string_view rName, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    m_rImpl.InitForIterating();
    std::shared_ptr<const SfxFilter> pFirstFilter;
    for (const std::shared_ptr<const SfxFilter>& pFilter : *m_rImpl.pList)
    {
        SfxFilterFlags nFlags = pFilter->GetFilterFlags();
        if ( (nFlags & nMust) == nMust &&
             !(nFlags & nDont ) && pFilter->GetUIName() == rName )
        {
            if ( pFilter->GetFilterFlags() & SfxFilterFlags::PREFERED )
                return pFilter;
            else if ( !pFirstFilter )
                pFirstFilter = pFilter;
        }
    }
    return pFirstFilter;
}

std::shared_ptr<const SfxFilter> SfxFilterMatcher::GetFilter4FilterName( const OUString& rName, SfxFilterFlags nMust, SfxFilterFlags nDont ) const
{
    std::u16string_view aName( rName );
    sal_Int32 nIndex = rName.indexOf(": ");
    if (  nIndex != -1 )
    {
        SAL_WARN( "sfx.bastyp", "Old filter name used!");
        aName = rName.subView( nIndex + 2 );
    }

    if ( bFirstRead )
    {
        uno::Reference< lang::XMultiServiceFactory > xServiceManager = ::comphelper::getProcessServiceFactory();
        uno::Reference< container::XNameAccess >     xFilterCFG                                                ;
        uno::Reference< container::XNameAccess >     xTypeCFG                                                  ;
        if( xServiceManager.is() )
        {
            static constexpr OUStringLiteral sFilterFactory = u"com.sun.star.document.FilterFactory";
            static constexpr OUStringLiteral sTypeDetection = u"com.sun.star.document.TypeDetection";
            xFilterCFG.set( xServiceManager->createInstance(  sFilterFactory ), uno::UNO_QUERY );
            xTypeCFG.set( xServiceManager->createInstance(  sTypeDetection ), uno::UNO_QUERY );
        }

        if( xFilterCFG.is() && xTypeCFG.is() )
        {
            if ( !pFilterArr )
                CreateFilterArr();
            else
            {
                for (const std::shared_ptr<const SfxFilter>& pFilter : *pFilterArr)
                {
                    SfxFilterFlags nFlags = pFilter->GetFilterFlags();
                    if ((nFlags & nMust) == nMust && !(nFlags & nDont) && pFilter->GetFilterName().equalsIgnoreAsciiCase(aName))
                        return pFilter;
                }
            }

            SfxFilterContainer::ReadSingleFilter_Impl( rName, xTypeCFG, xFilterCFG, false );
        }
    }

    SfxFilterList_Impl* pList = m_rImpl.pList;
    if ( !pList )
        pList = pFilterArr;

    for (const std::shared_ptr<const SfxFilter>& pFilter : *pList)
    {
        SfxFilterFlags nFlags = pFilter->GetFilterFlags();
        if ( (nFlags & nMust) == nMust && !(nFlags & nDont ) && pFilter->GetFilterName().equalsIgnoreAsciiCase(aName))
            return pFilter;
    }

    return nullptr;
}

IMPL_LINK( SfxFilterMatcher, MaybeFileHdl_Impl, OUString*, pString, bool )
{
    std::shared_ptr<const SfxFilter> pFilter = GetFilter4Extension( *pString );
    return pFilter &&
        !pFilter->GetWildcard().Matches(u"") &&
        !pFilter->GetWildcard().Matches(u"*.*") &&
        !pFilter->GetWildcard().Matches(u"*");
}


SfxFilterMatcherIter::SfxFilterMatcherIter(
    const SfxFilterMatcher& rMatcher,
    SfxFilterFlags nOrMaskP, SfxFilterFlags nAndMaskP )
    : nOrMask( nOrMaskP ), nAndMask( nAndMaskP ),
      nCurrent(0), m_rMatch(rMatcher.m_rImpl)
{
    if( nOrMask == static_cast<SfxFilterFlags>(0xffff) ) //Due to faulty build on s
        nOrMask = SfxFilterFlags::NONE;
    m_rMatch.InitForIterating();
}


std::shared_ptr<const SfxFilter> SfxFilterMatcherIter::Find_Impl()
{
    std::shared_ptr<const SfxFilter> pFilter;
    while( nCurrent < m_rMatch.pList->size() )
    {
        pFilter = (*m_rMatch.pList)[nCurrent++];
        SfxFilterFlags nFlags = pFilter->GetFilterFlags();
        if( ((nFlags & nOrMask) == nOrMask ) && !(nFlags & nAndMask ) )
            break;
        pFilter = nullptr;
    }

    return pFilter;
}

std::shared_ptr<const SfxFilter> SfxFilterMatcherIter::First()
{
    nCurrent = 0;
    return Find_Impl();
}


std::shared_ptr<const SfxFilter> SfxFilterMatcherIter::Next()
{
    return Find_Impl();
}

/*---------------------------------------------------------------
    helper to build own formatted string from given stringlist by
    using given separator
  ---------------------------------------------------------------*/
static OUString implc_convertStringlistToString( const uno::Sequence< OUString >& lList     ,
                                                 sal_Unicode                                        cSeparator,
                                                 std::u16string_view                                sPrefix   )
{
    OUStringBuffer   sString ( 1000 )           ;
    sal_Int32               nCount  = lList.getLength();
    sal_Int32               nItem   = 0                ;
    for( nItem=0; nItem<nCount; ++nItem )
    {
        if( !sPrefix.empty() )
        {
            sString.append( sPrefix );
        }
        sString.append( lList[nItem] );
        if( nItem+1<nCount )
        {
            sString.append( cSeparator );
        }
    }
    return sString.makeStringAndClear();
}


void SfxFilterContainer::ReadSingleFilter_Impl(
    const OUString& rName,
    const uno::Reference< container::XNameAccess >& xTypeCFG,
    const uno::Reference< container::XNameAccess >& xFilterCFG,
    bool bUpdate
    )
{
    OUString sFilterName( rName );
    SfxFilterList_Impl& rList = *pFilterArr;
    uno::Sequence< beans::PropertyValue > lFilterProperties;
    uno::Any aResult;
    try
    {
        aResult = xFilterCFG->getByName( sFilterName );
    }
    catch( container::NoSuchElementException& )
    {
        aResult = uno::Any();
    }

    if( !(aResult >>= lFilterProperties) )
        return;

    // collect information to add filter to container
    // (attention: some information aren't available on filter directly ... you must search for corresponding type too!)
    SfxFilterFlags       nFlags          = SfxFilterFlags::NONE;
    SotClipboardFormatId nClipboardId    = SotClipboardFormatId::NONE;
    sal_Int32       nFormatVersion  = 0 ;
    OUString sMimeType           ;
    OUString sType               ;
    OUString sUIName             ;
    OUString sHumanName          ;
    OUString sDefaultTemplate    ;
    OUString sUserData           ;
    OUString sExtension          ;
    OUString sServiceName        ;
    bool bEnabled = true         ;

    // first get directly available properties
    for (const auto& rFilterProperty : lFilterProperties)
    {
        if ( rFilterProperty.Name == "FileFormatVersion" )
        {
            rFilterProperty.Value >>= nFormatVersion;
        }
        else if ( rFilterProperty.Name == "TemplateName" )
        {
            rFilterProperty.Value >>= sDefaultTemplate;
        }
        else if ( rFilterProperty.Name == "Flags" )
        {
            sal_Int32 nTmp(0);
            rFilterProperty.Value >>= nTmp;
            assert((nTmp & ~o3tl::typed_flags<SfxFilterFlags>::mask) == 0);
            nFlags = static_cast<SfxFilterFlags>(nTmp);
        }
        else if ( rFilterProperty.Name == "UIName" )
        {
            rFilterProperty.Value >>= sUIName;
        }
        else if ( rFilterProperty.Name == "UserData" )
        {
            uno::Sequence< OUString > lUserData;
            rFilterProperty.Value >>= lUserData;
            sUserData = implc_convertStringlistToString( lUserData, ',', u"" );
        }
        else if ( rFilterProperty.Name == "DocumentService" )
        {
            rFilterProperty.Value >>= sServiceName;
        }
        else if (rFilterProperty.Name == "ExportExtension")
        {
            // Extension preferred by the filter.  This takes precedence
            // over those that are given in the file format type.
            rFilterProperty.Value >>= sExtension;
            sExtension = "*." + sExtension;
        }
        else if ( rFilterProperty.Name == "Type" )
        {
            rFilterProperty.Value >>= sType;
            // Try to get filter .. but look for any exceptions!
            // May be filter was deleted by another thread ...
            try
            {
                aResult = xTypeCFG->getByName( sType );
            }
            catch (const container::NoSuchElementException&)
            {
                aResult = uno::Any();
            }

            uno::Sequence< beans::PropertyValue > lTypeProperties;
            if( aResult >>= lTypeProperties )
            {
                // get indirect available properties then (types)
                for (const auto& rTypeProperty : lTypeProperties)
                {
                    if ( rTypeProperty.Name == "ClipboardFormat" )
                    {
                        rTypeProperty.Value >>= sHumanName;
                    }
                    else if ( rTypeProperty.Name == "MediaType" )
                    {
                        rTypeProperty.Value >>= sMimeType;
                    }
                    else if ( rTypeProperty.Name == "Extensions" )
                    {
                        if (sExtension.isEmpty())
                        {
                            uno::Sequence< OUString > lExtensions;
                            rTypeProperty.Value >>= lExtensions;
                            sExtension = implc_convertStringlistToString( lExtensions, ';', u"*." );
                        }
                    }
                }
            }
        }
        else if ( rFilterProperty.Name == "Enabled" )
        {
            rFilterProperty.Value >>= bEnabled;
        }

    }

    if ( sServiceName.isEmpty() )
        return;

    // old formats are found ... using HumanPresentableName!
    if( !sHumanName.isEmpty() )
    {
        nClipboardId = SotExchange::RegisterFormatName( sHumanName );

        // For external filters ignore clipboard IDs
        if(nFlags & SfxFilterFlags::STARONEFILTER)
        {
            nClipboardId = SotClipboardFormatId::NONE;
        }
    }
    // register SfxFilter
    // first erase module name from old filter names!
    // e.g: "scalc: DIF" => "DIF"
    sal_Int32 nStartRealName = sFilterName.indexOf( ": " );
    if( nStartRealName != -1 )
    {
        SAL_WARN( "sfx.bastyp", "Old format, not supported!");
        sFilterName = sFilterName.copy( nStartRealName+2 );
    }

    std::shared_ptr<const SfxFilter> pFilter = bUpdate ? SfxFilter::GetFilterByName( sFilterName ) : nullptr;
    if (!pFilter)
    {
        pFilter = std::make_shared<SfxFilter>( sFilterName             ,
                                 sExtension              ,
                                 nFlags                  ,
                                 nClipboardId            ,
                                 sType                   ,
                                 sMimeType               ,
                                 sUserData               ,
                                 sServiceName            ,
                                 bEnabled );
        rList.push_back( pFilter );
    }
    else
    {
        SfxFilter* pFilt = const_cast<SfxFilter*>(pFilter.get());
        pFilt->maFilterName  = sFilterName;
        pFilt->aWildCard    = WildCard(sExtension, ';');
        pFilt->nFormatType  = nFlags;
        pFilt->lFormat      = nClipboardId;
        pFilt->aTypeName    = sType;
        pFilt->aMimeType    = sMimeType;
        pFilt->aUserData    = sUserData;
        pFilt->aServiceName = sServiceName;
        pFilt->mbEnabled    = bEnabled;
    }

    SfxFilter* pFilt = const_cast<SfxFilter*>(pFilter.get());

    // Don't forget to set right UIName!
    // Otherwise internal name is used as fallback ...
    pFilt->SetUIName( sUIName );
    pFilt->SetDefaultTemplate( sDefaultTemplate );
    if( nFormatVersion )
    {
        pFilt->SetVersion( nFormatVersion );
    }
}

void SfxFilterContainer::ReadFilters_Impl( bool bUpdate )
{
    if ( !pFilterArr )
    {
        CreateFilterArr();
        assert(pFilterArr);
    }

    bFirstRead = false;
    SfxFilterList_Impl& rList = *pFilterArr;

    try
    {
        // get the FilterFactory service to access the registered filters ... and types!
        uno::Reference< lang::XMultiServiceFactory > xServiceManager = ::comphelper::getProcessServiceFactory();
        uno::Reference< container::XNameAccess >     xFilterCFG                                                ;
        uno::Reference< container::XNameAccess >     xTypeCFG                                                  ;
        if( xServiceManager.is() )
        {
            xFilterCFG.set( xServiceManager->createInstance(  u"com.sun.star.document.FilterFactory"_ustr ), uno::UNO_QUERY );
            xTypeCFG.set( xServiceManager->createInstance(  u"com.sun.star.document.TypeDetection"_ustr ), uno::UNO_QUERY );
        }

        if( xFilterCFG.is() && xTypeCFG.is() )
        {
            // select right query to get right set of filters for search module
            const uno::Sequence< OUString > lFilterNames = xFilterCFG->getElementNames();
            if ( lFilterNames.hasElements() )
            {
                // If list of filters already exist ...
                // ReadExternalFilters must work in update mode.
                // Best way seems to mark all filters NOT_INSTALLED
                // and change it back for all valid filters afterwards.
                if( !rList.empty() )
                {
                    bUpdate = true;
                    for (const std::shared_ptr<const SfxFilter>& pFilter : rList)
                    {
                        SfxFilter* pNonConstFilter = const_cast<SfxFilter*>(pFilter.get());
                        pNonConstFilter->nFormatType |= SFX_FILTER_NOTINSTALLED;
                    }
                }

                // get all properties of filters ... put it into the filter container
                for( const OUString& sFilterName : lFilterNames )
                {
                    // Try to get filter .. but look for any exceptions!
                    // May be filter was deleted by another thread ...
                    ReadSingleFilter_Impl( sFilterName, xTypeCFG, xFilterCFG, bUpdate );
                }
            }
        }
    }
    catch(const uno::Exception&)
    {
        SAL_WARN( "sfx.bastyp", "SfxFilterContainer::ReadFilter()\nException detected. Possible not all filters could be cached." );
    }

    if ( bUpdate )
    {
        // global filter array was modified, factory specific ones might need an
        // update too
        for (const auto& aImpl : aImplArr)
            aImpl->Update();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/frame/ModuleManager.hpp>
#include <com/sun/star/frame/XLoadable.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <unotools/moduleoptions.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/configurationhelper.hxx>

#include <sfx2/docfilt.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/viewfac.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/module.hxx>
#include "syspath.hxx"
#include <osl/file.hxx>
#include <osl/security.hxx>

#include <sal/log.hxx>
#include <tools/debug.hxx>
#include <tools/globname.hxx>

#include <memory>
#include <utility>

using namespace ::com::sun::star;


struct SfxObjectFactory_Impl
{
    std::vector<SfxViewFactory*> aViewFactoryArr;// List of <SfxViewFactory>s
    OUString                     aServiceName;
    SfxFilterContainer*          pFilterContainer;
    SfxModule*                   pModule;
    SvGlobalName                 aClassName;

    SfxObjectFactory_Impl() :
        pFilterContainer    ( nullptr ),
        pModule             ( nullptr )
        {}
};

SfxFilterContainer* SfxObjectFactory::GetFilterContainer() const
{
    return pImpl->pFilterContainer;
}

SfxObjectFactory::SfxObjectFactory
(
    const SvGlobalName&     rName,
    OUString          sName
) :    m_sFactoryName(std::move( sName )),
       pImpl( new SfxObjectFactory_Impl )
{
    pImpl->pFilterContainer = new SfxFilterContainer( m_sFactoryName );
    pImpl->aClassName = rName;
}

SfxObjectFactory::~SfxObjectFactory()
{
    delete pImpl->pFilterContainer;
}


void SfxObjectFactory::RegisterViewFactory
(
    SfxViewFactory &rFactory
)
{
#if OSL_DEBUG_LEVEL > 0
    {
        const OUString sViewName( rFactory.GetAPIViewName() );
        for (auto const& viewFactory : pImpl->aViewFactoryArr)
        {
            if ( viewFactory->GetAPIViewName() != sViewName )
                continue;
            SAL_WARN( "sfx", "SfxObjectFactory::RegisterViewFactory: duplicate view name: " << sViewName );
            break;
        }
    }
#endif
    auto it = std::find_if(pImpl->aViewFactoryArr.begin(), pImpl->aViewFactoryArr.end(),
        [&rFactory](SfxViewFactory* pFactory) { return pFactory->GetOrdinal() > rFactory.GetOrdinal(); });
    pImpl->aViewFactoryArr.insert(it, &rFactory);
}


sal_uInt16 SfxObjectFactory::GetViewFactoryCount() const
{
    return pImpl->aViewFactoryArr.size();
}


SfxViewFactory& SfxObjectFactory::GetViewFactory(sal_uInt16 i) const
{
    return *pImpl->aViewFactoryArr[i];
}


SfxModule* SfxObjectFactory::GetModule() const
{
    return pImpl->pModule;
}

void SfxObjectFactory::SetModule_Impl( SfxModule *pMod )
{
    pImpl->pModule = pMod;
}

void SfxObjectFactory::SetSystemTemplate( const OUString& rServiceName, const OUString& rTemplateName )
{
    static const int nMaxPathSize = 16000;

    const OUString sConfPath = "Office/Factories/" + rServiceName;
    static constexpr OUString PROP_DEF_TEMPL_CHANGED
        = u"ooSetupFactorySystemDefaultTemplateChanged"_ustr;

    static const char DEF_TPL_STR[] = "/soffice.";

    OUString sUserTemplateURL;
    OUString sPath;
    sal_Unicode aPathBuffer[nMaxPathSize];
    if ( SystemPath::GetUserTemplateLocation( aPathBuffer, nMaxPathSize ))
        sPath = OUString( aPathBuffer );
    osl::FileBase::getFileURLFromSystemPath( sPath, sUserTemplateURL );

    if ( sUserTemplateURL.isEmpty())
        return;

    try
    {
        uno::Reference< lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
        uno::Reference< uno::XInterface > xConfig = ::comphelper::ConfigurationHelper::openConfig(
            ::comphelper::getProcessComponentContext(), u"/org.openoffice.Setup"_ustr, ::comphelper::EConfigurationModes::Standard );

        OUString aActualFilter;
        ::comphelper::ConfigurationHelper::readRelativeKey( xConfig, sConfPath, u"ooSetupFactoryActualFilter"_ustr ) >>= aActualFilter;
        bool bChanged(false);
        ::comphelper::ConfigurationHelper::readRelativeKey( xConfig, sConfPath, PROP_DEF_TEMPL_CHANGED ) >>= bChanged;

        uno::Reference< container::XNameAccess > xFilterFactory(
            xFactory->createInstance( u"com.sun.star.document.FilterFactory"_ustr ), uno::UNO_QUERY_THROW );
        uno::Reference< container::XNameAccess > xTypeDetection(
            xFactory->createInstance( u"com.sun.star.document.TypeDetection"_ustr ), uno::UNO_QUERY_THROW );

        OUString aActualFilterTypeName;
        uno::Sequence< beans::PropertyValue > aActuralFilterData;
        xFilterFactory->getByName( aActualFilter ) >>= aActuralFilterData;
        for (const auto& rProp : aActuralFilterData)
            if ( rProp.Name == "Type" )
                rProp.Value >>= aActualFilterTypeName;
        ::comphelper::SequenceAsHashMap aProps1( xTypeDetection->getByName( aActualFilterTypeName ) );
        uno::Sequence< OUString > aAllExt =
            aProps1.getUnpackedValueOrDefault(u"Extensions"_ustr, uno::Sequence< OUString >() );
        //To-do: check if aAllExt is empty first
        const OUString aExt = DEF_TPL_STR + aAllExt[0];

        sUserTemplateURL += aExt;

        uno::Reference<ucb::XSimpleFileAccess3> xSimpleFileAccess(
            ucb::SimpleFileAccess::create( ::comphelper::getComponentContext(xFactory) ) );

        OUString aBackupURL;
        ::osl::Security().getConfigDir(aBackupURL);
        aBackupURL += "/temp";

        if ( !xSimpleFileAccess->exists( aBackupURL ) )
            xSimpleFileAccess->createFolder( aBackupURL );

        aBackupURL += aExt;

        if ( !rTemplateName.isEmpty() )
        {
            if ( xSimpleFileAccess->exists( sUserTemplateURL ) && !bChanged )
                xSimpleFileAccess->copy( sUserTemplateURL, aBackupURL );

            uno::Reference< document::XTypeDetection > xTypeDetector( xTypeDetection, uno::UNO_QUERY );
            ::comphelper::SequenceAsHashMap aProps2( xTypeDetection->getByName( xTypeDetector->queryTypeByURL( rTemplateName ) ) );
            OUString aFilterName =
                aProps2.getUnpackedValueOrDefault(u"PreferredFilter"_ustr, OUString() );

            uno::Sequence< beans::PropertyValue > aArgs{
                comphelper::makePropertyValue(u"FilterName"_ustr, aFilterName),
                comphelper::makePropertyValue(u"AsTemplate"_ustr, true),
                comphelper::makePropertyValue(u"URL"_ustr, rTemplateName)
            };

            uno::Reference< frame::XLoadable > xLoadable( xFactory->createInstance( rServiceName ), uno::UNO_QUERY );
            xLoadable->load( aArgs );

            aArgs.realloc( 2 );
            auto pArgs = aArgs.getArray();
            pArgs[1].Name = "Overwrite";
            pArgs[1].Value <<= true;

            uno::Reference< frame::XStorable > xStorable( xLoadable, uno::UNO_QUERY );
            xStorable->storeToURL( sUserTemplateURL, aArgs );
            ::comphelper::ConfigurationHelper::writeRelativeKey( xConfig, sConfPath, PROP_DEF_TEMPL_CHANGED, uno::Any( true ));
            ::comphelper::ConfigurationHelper::flush( xConfig );
        }
        else
        {
            DBG_ASSERT( bChanged, "invalid ooSetupFactorySystemDefaultTemplateChanged value!" );

            xSimpleFileAccess->copy( aBackupURL, sUserTemplateURL );
            xSimpleFileAccess->kill( aBackupURL );
            ::comphelper::ConfigurationHelper::writeRelativeKey( xConfig, sConfPath, PROP_DEF_TEMPL_CHANGED, uno::Any( false ));
            ::comphelper::ConfigurationHelper::flush( xConfig );
        }
    }
    catch(const uno::Exception&)
    {
    }
}

void SfxObjectFactory::SetStandardTemplate( const OUString& rServiceName, const OUString& rTemplate )
{
    SvtModuleOptions::EFactory eFac = SvtModuleOptions::ClassifyFactoryByServiceName(rServiceName);
    if (eFac == SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
        eFac = SvtModuleOptions::ClassifyFactoryByShortName(rServiceName);
    if (eFac != SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
    {
        SetSystemTemplate( rServiceName, rTemplate );
        SvtModuleOptions().SetFactoryStandardTemplate(eFac, rTemplate);
    }
}

OUString SfxObjectFactory::GetStandardTemplate( std::u16string_view rServiceName )
{
    SvtModuleOptions::EFactory eFac = SvtModuleOptions::ClassifyFactoryByServiceName(rServiceName);
    if (eFac == SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
        eFac = SvtModuleOptions::ClassifyFactoryByShortName(rServiceName);

    if (eFac != SvtModuleOptions::EFactory::UNKNOWN_FACTORY)
        return SvtModuleOptions().GetFactoryStandardTemplate(eFac);

    return OUString();
}

std::shared_ptr<const SfxFilter> SfxObjectFactory::GetTemplateFilter() const
{
    sal_uInt16 nVersion=0;
    SfxFilterMatcher aMatcher ( m_sFactoryName );
    SfxFilterMatcherIter aIter( aMatcher );
    std::shared_ptr<const SfxFilter> pFilter;
    std::shared_ptr<const SfxFilter> pTemp = aIter.First();
    while ( pTemp )
    {
        if( pTemp->IsOwnFormat() && pTemp->IsOwnTemplateFormat() && ( pTemp->GetVersion() > nVersion ) )
        {
            pFilter = pTemp;
            nVersion = static_cast<sal_uInt16>(pTemp->GetVersion());
        }

        pTemp = aIter.Next();
    }

    return pFilter;
}

void SfxObjectFactory::SetDocumentServiceName( const OUString& rServiceName )
{
    pImpl->aServiceName = rServiceName;
}

const OUString& SfxObjectFactory::GetDocumentServiceName() const
{
    return pImpl->aServiceName;
}

const SvGlobalName& SfxObjectFactory::GetClassId() const
{
    return pImpl->aClassName;
}

OUString SfxObjectFactory::GetFactoryURL() const
{
    return "private:factory/" + m_sFactoryName;
}

OUString SfxObjectFactory::GetModuleName() const
{
    try
    {
        const css::uno::Reference< css::uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();

        css::uno::Reference< css::frame::XModuleManager2 > xModuleManager(
            css::frame::ModuleManager::create(xContext));

        ::comphelper::SequenceAsHashMap aPropSet( xModuleManager->getByName(GetDocumentServiceName()) );
        return aPropSet.getUnpackedValueOrDefault(u"ooSetupFactoryUIName"_ustr, OUString());
    }
    catch(const css::uno::RuntimeException&)
    {
        throw;
    }
    catch(const css::uno::Exception&)
    {
    }

    return OUString();
}


sal_uInt16 SfxObjectFactory::GetViewNo_Impl( const SfxInterfaceId i_nViewId, const sal_uInt16 i_nFallback ) const
{
    for ( sal_uInt16 curViewNo = 0; curViewNo < GetViewFactoryCount(); ++curViewNo )
    {
        const SfxInterfaceId curViewId = GetViewFactory( curViewNo ).GetOrdinal();
        if ( i_nViewId == curViewId )
           return curViewNo;
    }
    return i_nFallback;
}

SfxViewFactory* SfxObjectFactory::GetViewFactoryByViewName( std::u16string_view i_rViewName ) const
{
    for (   sal_uInt16 nViewNo = 0;
            nViewNo < GetViewFactoryCount();
            ++nViewNo
        )
    {
        SfxViewFactory& rViewFac( GetViewFactory( nViewNo ) );
        if  (   ( rViewFac.GetAPIViewName() == i_rViewName )
            ||  ( rViewFac.GetLegacyViewName() == i_rViewName )
            )
            return &rViewFac;
    }
    return nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


/**************************************************************************
                                TODO
 **************************************************************************

 - XInitialization::initialize does not work any longer!

 *************************************************************************/
#include <osl/diagnose.h>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <com/sun/star/util/theOfficeInstallationDirectories.hpp>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/weak.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/macros.hxx>
#include "hierarchyprovider.hxx"
#include "hierarchycontent.hxx"
#include "hierarchyuri.hxx"

#include "../inc/urihelper.hxx"

using namespace com::sun::star;
using namespace hierarchy_ucp;


// HierarchyContentProvider Implementation.


HierarchyContentProvider::HierarchyContentProvider(
            const uno::Reference< uno::XComponentContext >& rxContext )
: HierarchyContentProvider_Base( rxContext )
{
}


// virtual
HierarchyContentProvider::~HierarchyContentProvider()
{
}

// XServiceInfo methods.

OUString SAL_CALL HierarchyContentProvider::getImplementationName()                       \
{
    return u"com.sun.star.comp.ucb.HierarchyContentProvider"_ustr;
}
sal_Bool SAL_CALL HierarchyContentProvider::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}
css::uno::Sequence< OUString > HierarchyContentProvider::getSupportedServiceNames()
{
    return { u"com.sun.star.ucb.HierarchyContentProvider"_ustr };
}

// Service factory implementation.

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
ucb_HierarchyContentProvider_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new HierarchyContentProvider(context));
}

// XContentProvider methods.


// virtual
uno::Reference< ucb::XContent > SAL_CALL
HierarchyContentProvider::queryContent(
        const uno::Reference< ucb::XContentIdentifier >& Identifier )
{
    HierarchyUri aUri( Identifier->getContentIdentifier() );
    if ( !aUri.isValid() )
        throw ucb::IllegalIdentifierException();

    // Encode URL and create new Id. This may "correct" user-typed-in URL's.
    uno::Reference< ucb::XContentIdentifier > xCanonicId
        = new ::ucbhelper::ContentIdentifier( ::ucb_impl::urihelper::encodeURI( aUri.getUri() ) );
    osl::MutexGuard aGuard( m_aMutex );

    // Check, if a content with given id already exists...
    uno::Reference< ucb::XContent > xContent
        = queryExistingContent( xCanonicId );
    if ( xContent.is() )
        return xContent;

    // Create a new content.
    xContent = HierarchyContent::create( m_xContext, this, xCanonicId );
    registerNewContent( xContent );

    if ( xContent.is() && !xContent->getIdentifier().is() )
        throw ucb::IllegalIdentifierException();

    return xContent;
}


// XInitialization methods.


// virtual
void SAL_CALL HierarchyContentProvider::initialize(
                                const uno::Sequence< uno::Any >& aArguments )
{
    if ( aArguments.hasElements() )
        OSL_FAIL( "HierarchyContentProvider::initialize : not supported!" );
}


//  Non-interface methods.


uno::Reference< lang::XMultiServiceFactory >
HierarchyContentProvider::getConfigProvider(
                                const OUString & rServiceSpecifier )
{
    osl::MutexGuard aGuard( m_aMutex );
    ConfigProviderMap::iterator it = m_aConfigProviderMap.find(
                                                    rServiceSpecifier );
    if ( it == m_aConfigProviderMap.end() )
    {
        try
        {
            ConfigProviderMapEntry aEntry;
            aEntry.xConfigProvider.set(
                                m_xContext->getServiceManager()->createInstanceWithContext(rServiceSpecifier, m_xContext),
                                uno::UNO_QUERY );

            if ( aEntry.xConfigProvider.is() )
            {
                m_aConfigProviderMap[ rServiceSpecifier ] = aEntry;
                return aEntry.xConfigProvider;
            }
        }
        catch ( uno::Exception const & )
        {
//            OSL_FAIL( //                        "HierarchyContentProvider::getConfigProvider - "
//                        "caught exception!" );
        }

        OSL_FAIL( "HierarchyContentProvider::getConfigProvider - "
                    "No config provider!" );

        return uno::Reference< lang::XMultiServiceFactory >();
    }

    return (*it).second.xConfigProvider;
}

uno::Reference< container::XHierarchicalNameAccess >
HierarchyContentProvider::getRootConfigReadNameAccess(
                                const OUString & rServiceSpecifier )
{
    osl::MutexGuard aGuard( m_aMutex );
    ConfigProviderMap::iterator it = m_aConfigProviderMap.find(
                                                    rServiceSpecifier );
    if (it == m_aConfigProviderMap.end())
        return uno::Reference< container::XHierarchicalNameAccess >();

    if ( !( (*it).second.xRootReadAccess.is() ) )
    {
        if ( (*it).second.bTriedToGetRootReadAccess )
        {
            OSL_FAIL( "HierarchyContentProvider::getRootConfigReadNameAccess - "
                "Unable to read any config data! -> #82494#" );
            return uno::Reference< container::XHierarchicalNameAccess >();
        }

        try
        {
            uno::Reference< lang::XMultiServiceFactory > xConfigProv
                = getConfigProvider( rServiceSpecifier );

            if ( xConfigProv.is() )
            {
                beans::PropertyValue      aProperty;
                aProperty.Name = "nodepath" ;
                aProperty.Value <<= OUString(); // root path
                uno::Sequence< uno::Any > aArguments{ uno::Any(aProperty) };

                (*it).second.bTriedToGetRootReadAccess = true;

                (*it).second.xRootReadAccess.set(
                        xConfigProv->createInstanceWithArguments(
                            u"com.sun.star.ucb.HierarchyDataReadAccess"_ustr,
                            aArguments ),
                        uno::UNO_QUERY );
            }
        }
        catch ( uno::RuntimeException const & )
        {
            throw;
        }
        catch ( uno::Exception const & )
        {
            // createInstance, createInstanceWithArguments

            OSL_FAIL( "HierarchyContentProvider::getRootConfigReadNameAccess - "
                "caught Exception!" );
        }
    }

    return (*it).second.xRootReadAccess;
}

uno::Reference< util::XOfficeInstallationDirectories >
HierarchyContentProvider::getOfficeInstallationDirectories()
{
    if ( !m_xOfficeInstDirs.is() )
    {
        osl::MutexGuard aGuard( m_aMutex );
        if ( !m_xOfficeInstDirs.is() )
        {
            OSL_ENSURE( m_xContext.is(), "No service manager!" );

            m_xOfficeInstDirs = util::theOfficeInstallationDirectories::get(m_xContext);
        }
    }
    return m_xOfficeInstDirs;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

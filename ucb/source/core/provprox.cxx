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

#include <osl/diagnose.h>
#include <osl/thread.h>
#include <rtl/strbuf.hxx>
#include <comphelper/diagnose_ex.hxx>
#include "provprox.hxx"
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/weak.hxx>
#include <ucbhelper/macros.hxx>
#include <utility>

using namespace com::sun::star::lang;
using namespace com::sun::star::ucb;
using namespace com::sun::star::uno;

// UcbContentProviderProxyFactory Implementation.


UcbContentProviderProxyFactory::UcbContentProviderProxyFactory(
                        const Reference< XComponentContext >& rxContext )
: m_xContext( rxContext )
{
}


// virtual
UcbContentProviderProxyFactory::~UcbContentProviderProxyFactory()
{
}

// XServiceInfo methods.

OUString SAL_CALL UcbContentProviderProxyFactory::getImplementationName()
{
    return u"com.sun.star.comp.ucb.UcbContentProviderProxyFactory"_ustr;
}
sal_Bool SAL_CALL UcbContentProviderProxyFactory::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}
css::uno::Sequence< OUString > SAL_CALL UcbContentProviderProxyFactory::getSupportedServiceNames()
{
    return { u"com.sun.star.ucb.ContentProviderProxyFactory"_ustr };
}

// Service factory implementation.


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
ucb_UcbContentProviderProxyFactory_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new UcbContentProviderProxyFactory(context));
}


// XContentProviderFactory methods.


// virtual
Reference< XContentProvider > SAL_CALL
UcbContentProviderProxyFactory::createContentProvider(
                                                const OUString& Service )
{
    return Reference< XContentProvider >(
                        new UcbContentProviderProxy( m_xContext, Service ) );
}


// UcbContentProviderProxy Implementation.


UcbContentProviderProxy::UcbContentProviderProxy(
                        const Reference< XComponentContext >& rxContext,
                        OUString Service )
: m_aService(std::move( Service )),
  m_bReplace( false ),
  m_bRegister( false ),
  m_xContext( rxContext )
{
}


// virtual
UcbContentProviderProxy::~UcbContentProviderProxy()
{
}


// XInterface methods.

// virtual
Any SAL_CALL
UcbContentProviderProxy::queryInterface( const Type & rType )
{
    Any aRet = UcbContentProviderProxy_BASE::queryInterface(rType);

    if ( !aRet.hasValue() )
    {
        // Get original provider and forward the call...
        Reference< XContentProvider > xProvider = getContentProvider();
        if ( xProvider.is() )
            aRet = xProvider->queryInterface( rType );
    }

    return aRet;
}


// XTypeProvider methods.

Sequence< Type > SAL_CALL UcbContentProviderProxy::getTypes()
{
    // Get original provider and forward the call...
    if (Reference<XTypeProvider> xProvider{ getContentProvider(), UNO_QUERY })
        return xProvider->getTypes();

    return UcbContentProviderProxy_BASE::getTypes();
}


// XServiceInfo methods.

OUString SAL_CALL UcbContentProviderProxy::getImplementationName()
{
    return u"com.sun.star.comp.ucb.UcbContentProviderProxy"_ustr;
}

sal_Bool SAL_CALL UcbContentProviderProxy::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

css::uno::Sequence< OUString > SAL_CALL UcbContentProviderProxy::getSupportedServiceNames()
{
    return { u"com.sun.star.ucb.ContentProviderProxy"_ustr };
}


// XContentProvider methods.


// virtual
Reference< XContent > SAL_CALL UcbContentProviderProxy::queryContent(
                        const Reference< XContentIdentifier >& Identifier )
{
    // Get original provider and forward the call...

    Reference< XContentProvider > xProvider = getContentProvider();
    if ( xProvider.is() )
        return xProvider->queryContent( Identifier );

    return Reference< XContent >();
}


// virtual
sal_Int32 SAL_CALL UcbContentProviderProxy::compareContentIds(
                       const Reference< XContentIdentifier >& Id1,
                       const Reference< XContentIdentifier >& Id2 )
{
    // Get original provider and forward the call...

    Reference< XContentProvider > xProvider = getContentProvider();
    if ( xProvider.is() )
        return xProvider->compareContentIds( Id1, Id2 );

    // OSL_FAIL( // "UcbContentProviderProxy::compareContentIds - No provider!" );

    // @@@ What else?
    return 0;
}


// XParameterizedContentProvider methods.


// virtual
Reference< XContentProvider > SAL_CALL
UcbContentProviderProxy::registerInstance( const OUString& Template,
                                             const OUString& Arguments,
                                             sal_Bool ReplaceExisting )
{
    // Just remember that this method was called ( and the params ).

    std::scoped_lock aGuard( m_aMutex );

    if ( !m_bRegister )
    {
//      m_xTargetProvider = 0;
        m_aTemplate  = Template;
        m_aArguments = Arguments;
        m_bReplace   = ReplaceExisting;

        m_bRegister  = true;
    }
    return this;
}


// virtual
Reference< XContentProvider > SAL_CALL
UcbContentProviderProxy::deregisterInstance( const OUString& Template,
                                             const OUString& Arguments )
{
    std::scoped_lock aGuard( m_aMutex );

    // registerInstance called at proxy and at original?
    if ( m_bRegister && m_xTargetProvider.is() )
    {
        m_bRegister       = false;
        m_xTargetProvider = nullptr;

        Reference< XParameterizedContentProvider >
                                xParamProvider( m_xProvider, UNO_QUERY );
        if ( xParamProvider.is() )
        {
            try
            {
                xParamProvider->deregisterInstance( Template, Arguments );
            }
            catch ( IllegalIdentifierException const & )
            {
                OSL_FAIL( "UcbContentProviderProxy::deregisterInstance - "
                    "Caught IllegalIdentifierException!" );
            }
        }
    }

    return this;
}


// XContentProviderSupplier methods.


// virtual
Reference< XContentProvider > SAL_CALL
UcbContentProviderProxy::getContentProvider()
{
    std::scoped_lock aGuard( m_aMutex );
    if ( !m_xProvider.is() )
    {
        try
        {
            m_xProvider.set( m_xContext->getServiceManager()->createInstanceWithContext( m_aService,m_xContext ), UNO_QUERY );
            if ( m_aArguments == "NoConfig" )
            {
                Reference<XInitialization> xInit(m_xProvider,UNO_QUERY);
                if(xInit.is()) {
                    xInit->initialize({ Any(m_aArguments) });
                }
            }
        }
        catch ( RuntimeException const & )
        {
            throw;
        }
        catch ( Exception const & )
        {
            TOOLS_INFO_EXCEPTION( "ucb.core", "Exception getting content provider");
        }

        // registerInstance called at proxy, but not yet at original?
        if ( m_xProvider.is() && m_bRegister )
        {
            Reference< XParameterizedContentProvider >
                xParamProvider( m_xProvider, UNO_QUERY );
            if ( xParamProvider.is() )
            {
                try
                {
                    m_xTargetProvider
                        = xParamProvider->registerInstance( m_aTemplate,
                                                            m_aArguments,
                                                            m_bReplace );
                }
                catch ( IllegalIdentifierException const & )
                {
                    OSL_FAIL( "UcbContentProviderProxy::getContentProvider - "
                        "Caught IllegalIdentifierException!" );
                }

                OSL_ENSURE( m_xTargetProvider.is(),
                    "UcbContentProviderProxy::getContentProvider - "
                    "No provider!" );
            }
        }
        if ( !m_xTargetProvider.is() )
            m_xTargetProvider = m_xProvider;
    }

    OSL_ENSURE( m_xProvider.is(),
        OStringBuffer("UcbContentProviderProxy::getContentProvider - No provider for '" +
            OUStringToOString(m_aService, osl_getThreadTextEncoding()) +
            ".").getStr() );
    return m_xTargetProvider;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

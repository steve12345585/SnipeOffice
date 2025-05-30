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

#pragma once

#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ucb/XContentProviderFactory.hpp>
#include <com/sun/star/ucb/XContentProvider.hpp>
#include <com/sun/star/ucb/XParameterizedContentProvider.hpp>
#include <com/sun/star/ucb/XContentProviderSupplier.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/compbase.hxx>
#include <cppuhelper/weak.hxx>



using UcbContentProviderProxyFactory_Base = comphelper::WeakComponentImplHelper <
                                                css::lang::XServiceInfo,
                                                css::ucb::XContentProviderFactory >;
class UcbContentProviderProxyFactory : public UcbContentProviderProxyFactory_Base
{
    css::uno::Reference< css::uno::XComponentContext > m_xContext;

public:
    explicit UcbContentProviderProxyFactory(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext );
    virtual ~UcbContentProviderProxyFactory() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XContentProviderFactory
    virtual css::uno::Reference< css::ucb::XContentProvider > SAL_CALL
    createContentProvider( const OUString& Service ) override;
};



using UcbContentProviderProxy_BASE = comphelper::WeakImplHelper<css::lang::XServiceInfo,
                                                                css::ucb::XContentProviderSupplier,
                                                                css::ucb::XContentProvider,
                                                                css::ucb::XParameterizedContentProvider>;
class UcbContentProviderProxy : public UcbContentProviderProxy_BASE
{
    OUString m_aService;
    OUString m_aTemplate;
    OUString m_aArguments;
    bool        m_bReplace;
    bool        m_bRegister;

    css::uno::Reference< css::uno::XComponentContext >
                                m_xContext;
    css::uno::Reference< css::ucb::XContentProvider >
                                m_xProvider;
    css::uno::Reference< css::ucb::XContentProvider >
                                m_xTargetProvider;

public:
    UcbContentProviderProxy(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
            OUString Service );
    virtual ~UcbContentProviderProxy() override;

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;

    // XTypeProvider
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XContentProviderSupplier
    virtual css::uno::Reference<
        css::ucb::XContentProvider > SAL_CALL
    getContentProvider() override;

    // XContentProvider
    virtual css::uno::Reference<
        css::ucb::XContent > SAL_CALL
    queryContent( const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier ) override;
    virtual sal_Int32 SAL_CALL
    compareContentIds( const css::uno::Reference< css::ucb::XContentIdentifier >& Id1,
                       const css::uno::Reference< css::ucb::XContentIdentifier >& Id2 ) override;

    // XParameterizedContentProvider
    virtual css::uno::Reference< css::ucb::XContentProvider > SAL_CALL
    registerInstance( const OUString& Template,
                      const OUString& Arguments,
                      sal_Bool ReplaceExisting ) override;
    virtual css::uno::Reference< css::ucb::XContentProvider > SAL_CALL
    deregisterInstance( const OUString& Template,
                        const OUString& Arguments ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

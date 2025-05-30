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

#include <sal/config.h>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/connection/XAcceptor.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/bridge/XInstanceProvider.hpp>
#include <com/sun/star/bridge/XBridgeFactory2.hpp>
#include <cppuhelper/implbase.hxx>

#include <comphelper/weakbag.hxx>
#include <osl/conditn.hxx>
#include <osl/thread.hxx>

#include <mutex>

namespace com::sun::star::uno { class XComponentContext; }

namespace desktop {

class  Acceptor
    : public ::cppu::WeakImplHelper<css::lang::XServiceInfo, css::lang::XInitialization>
{
private:
    std::mutex m_aMutex;

    oslThread m_thread;
    comphelper::WeakBag< css::bridge::XBridge > m_bridges;

    ::osl::Condition m_cEnable;

    css::uno::Reference< css::uno::XComponentContext >     m_rContext;
    css::uno::Reference< css::connection::XAcceptor >      m_rAcceptor;
    css::uno::Reference< css::bridge::XBridgeFactory2 >    m_rBridgeFactory;

    OUString m_aAcceptString;
    OUString m_aConnectString;
    OUString m_aProtocol;

    bool m_bInit;
    bool m_bDying;

public:
    explicit Acceptor( const css::uno::Reference< css::uno::XComponentContext >& rxContext );
    virtual ~Acceptor() override;

    void run();

    // XService info
    virtual OUString SAL_CALL getImplementationName() override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& aName ) override;

    // XInitialize
    virtual void SAL_CALL initialize( const css::uno::Sequence<css::uno::Any>& aArguments ) override;
};

class AccInstanceProvider : public ::cppu::WeakImplHelper<css::bridge::XInstanceProvider>
{
private:
    css::uno::Reference<css::uno::XComponentContext> m_rContext;

public:
    AccInstanceProvider(const css::uno::Reference< css::uno::XComponentContext >& rxContext);
    virtual ~AccInstanceProvider() override;

    // XInstanceProvider
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL getInstance (const OUString& aName ) override;
};


} //namespace desktop

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

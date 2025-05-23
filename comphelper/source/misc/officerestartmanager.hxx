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

#include <com/sun/star/task/XRestartManager.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/awt/XCallback.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <mutex>
#include <cppuhelper/implbase.hxx>
#include <utility>

namespace comphelper
{

class OOfficeRestartManager : public ::cppu::WeakImplHelper< css::task::XRestartManager
                                                           , css::awt::XCallback
                                                           , css::lang::XServiceInfo >
{
    std::mutex m_aMutex;
    css::uno::Reference< css::uno::XComponentContext > m_xContext;

    bool m_bOfficeInitialized;
    bool m_bRestartRequested;

public:
    explicit OOfficeRestartManager( css::uno::Reference< css::uno::XComponentContext > xContext )
    : m_xContext(std::move( xContext ))
    , m_bOfficeInitialized( false )
    , m_bRestartRequested( false )
    {}

// XRestartManager
    virtual void SAL_CALL requestRestart( const css::uno::Reference< css::task::XInteractionHandler >& xInteractionHandler ) override;
    virtual sal_Bool SAL_CALL isRestartRequested( sal_Bool bInitialized ) override;

// XCallback
    virtual void SAL_CALL notify( const css::uno::Any& aData ) override;

// XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

};

} // namespace comphelper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

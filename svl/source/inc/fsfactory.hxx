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

#ifndef INCLUDED_SVL_SOURCE_INC_FSFACTORY_HXX
#define INCLUDED_SVL_SOURCE_INC_FSFACTORY_HXX

#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/implbase.hxx>
#include <osl/diagnose.h>

class FSStorageFactory final : public ::cppu::WeakImplHelper< css::lang::XSingleServiceFactory,
                                                        css::lang::XServiceInfo >
{
    css::uno::Reference< css::uno::XComponentContext > m_xContext;

public:
    FSStorageFactory( const css::uno::Reference< css::uno::XComponentContext >& xContext )
    : m_xContext( xContext )
    {
        OSL_ENSURE( xContext.is(), "No service manager is provided!" );
    }

    // XSingleServiceFactory
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstance() override;
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstanceWithArguments( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

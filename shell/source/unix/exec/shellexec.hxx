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

#ifndef INCLUDED_SHELL_SOURCE_UNIX_EXEC_SHELLEXEC_HXX
#define INCLUDED_SHELL_SOURCE_UNIX_EXEC_SHELLEXEC_HXX

#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <com/sun/star/system/XSystemShellExecute.hpp>




class ShellExec : public ::cppu::WeakImplHelper< css::system::XSystemShellExecute, css::lang::XServiceInfo >
{
    css::uno::Reference< css::uno::XComponentContext > m_xContext;

public:
    explicit ShellExec(const css::uno::Reference< css::uno::XComponentContext >& xContext);


    // XSystemShellExecute


    virtual void SAL_CALL execute( const OUString& aCommand, const OUString& aParameter, sal_Int32 nFlags ) override;


    // XServiceInfo


    virtual OUString SAL_CALL getImplementationName(  ) override;

    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;

    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

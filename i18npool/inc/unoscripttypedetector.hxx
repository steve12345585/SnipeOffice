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

#include <com/sun/star/lang/XServiceInfo.hpp>

#include <com/sun/star/i18n/XScriptTypeDetector.hpp>
#include <cppuhelper/implbase.hxx>



class UnoScriptTypeDetector final : public cppu::WeakImplHelper
<
    css::i18n::XScriptTypeDetector,
    css::lang::XServiceInfo
>
{
public:
    // Methods
    virtual sal_Int32 SAL_CALL beginOfScriptDirection( const OUString& Text, sal_Int32 nPos, sal_Int16 scriptDirection ) override;
    virtual sal_Int32 SAL_CALL endOfScriptDirection( const OUString& Text, sal_Int32 nPos, sal_Int16 scriptDirection ) override;
    virtual sal_Int16 SAL_CALL getScriptDirection( const OUString& Text, sal_Int32 nPos, sal_Int16 defaultScriptDirection ) override;
    virtual sal_Int32 SAL_CALL beginOfCTLScriptType( const OUString& Text, sal_Int32 nPos ) override;
    virtual sal_Int32 SAL_CALL endOfCTLScriptType( const OUString& Text, sal_Int32 nPos ) override;
    virtual sal_Int16 SAL_CALL getCTLScriptType( const OUString& Text, sal_Int32 nPos ) override;


    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

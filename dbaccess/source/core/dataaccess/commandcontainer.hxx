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

#include <definitioncontainer.hxx>

#include <com/sun/star/lang/XSingleServiceFactory.hpp>

#include <cppuhelper/implbase1.hxx>

namespace dbaccess
{
// OCommandContainer

typedef ::cppu::ImplHelper1 <   css::lang::XSingleServiceFactory
                            >   OCommandContainer_BASE;

class OCommandContainer : public ODefinitionContainer
                         ,public OCommandContainer_BASE
{
    bool m_bTables;

public:
    /** constructs the container.<BR>
    */
    OCommandContainer(
         const css::uno::Reference< css::uno::XComponentContext >& _xORB
        ,const css::uno::Reference< css::uno::XInterface >&   _xParentContainer
        ,const TContentPtr& _pImpl
        ,bool _bTables
        );

    DECLARE_XINTERFACE( )

    virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
    virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;

    // XSingleServiceFactory
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstance( ) override;
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstanceWithArguments( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

protected:
    virtual ~OCommandContainer() override;

    // ODefinitionContainer
    virtual css::uno::Reference< css::ucb::XContent > createObject(const OUString& _rName) override;

protected:
    // OContentHelper overridables
    virtual OUString determineContentType() const override;
};

}   // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

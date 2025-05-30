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

#include <comphelper/uno3.hxx>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/propertycontainer.hxx>
#include <comphelper/proparrhlp.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/implbase.hxx>


#define RM_PROPERTY_ID_LABEL        1
#define RM_PROPERTY_ID_ID           2
#define RM_PROPERTY_ID_ENABLED      4
#define RM_PROPERTY_ID_INTERACTIVE  5

typedef cppu::WeakImplHelper< css::lang::XServiceInfo > ORoadmapEntry_Base;

class ORoadmapEntry final : public ORoadmapEntry_Base
            ,public ::comphelper::OMutexAndBroadcastHelper
            ,public ::comphelper::OPropertyContainer
            ,public ::comphelper::OPropertyArrayUsageHelper< ORoadmapEntry >
{

public:
       ORoadmapEntry();

    DECLARE_XINTERFACE()        // merge XInterface implementations
    DECLARE_XTYPEPROVIDER()     // merge XTypeProvider implementations

private:
    /// @see css::beans::XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo >
        SAL_CALL getPropertySetInfo() override;

    // OPropertySetHelper
    virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

    // OPropertyArrayUsageHelper
    virtual ::cppu::IPropertyArrayHelper* createArrayHelper() const override;

    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    OUString        m_sLabel;
    sal_Int32       m_nID;
    bool            m_bEnabled;
    bool            m_bInteractive;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

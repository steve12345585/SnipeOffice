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

#include <helper/propertysetcontainer.hxx>
#include <cppuhelper/compbase.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>

inline constexpr OUString SERVICENAME_ACTIONTRIGGERCONTAINER = u"com.sun.star.ui.ActionTriggerContainer"_ustr;
inline constexpr OUString IMPLEMENTATIONNAME_ACTIONTRIGGERCONTAINER = u"com.sun.star.comp.ui.ActionTriggerContainer"_ustr;

namespace framework
{

class ActionTriggerContainer final : public cppu::ImplInheritanceHelper<PropertySetContainer,
                                                                        css::lang::XMultiServiceFactory,
                                                                        css::lang::XServiceInfo>
{
    public:
        ActionTriggerContainer();
        virtual ~ActionTriggerContainer() override;

        // XMultiServiceFactory
        virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstance( const OUString& aServiceSpecifier ) override;
        virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstanceWithArguments( const OUString& ServiceSpecifier, const css::uno::Sequence< css::uno::Any >& Arguments ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getAvailableServiceNames() override;

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName(  ) override;
        virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

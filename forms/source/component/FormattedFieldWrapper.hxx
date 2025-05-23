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
#include <cppuhelper/implbase3.hxx>
#include <com/sun/star/io/XPersistObject.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <rtl/ref.hxx>

namespace frm
{

class OEditModel;
class OFormattedModel;
class OEditBaseModel;

//= OFormattedFieldWrapper

typedef ::cppu::WeakAggImplHelper3  <   css::io::XPersistObject
                                    ,   css::lang::XServiceInfo
                                    ,   css::util::XCloneable
                                    >   OFormattedFieldWrapper_Base;

class OFormattedFieldWrapper final : public OFormattedFieldWrapper_Base
{
    css::uno::Reference< css::uno::XComponentContext> m_xContext;
    OUString m_implementationName;

    rtl::Reference< OEditBaseModel >      m_xAggregate; // either OEditModel or OFormattedModel

    rtl::Reference< OEditModel > m_pEditPart;
    // if we act as formatted this is used to write the EditModel part
    rtl::Reference< OEditBaseModel >     m_xFormattedPart;

    OFormattedFieldWrapper(const css::uno::Reference< css::uno::XComponentContext>& _rxFactory,
                           OUString const & implementationName);

    virtual ~OFormattedFieldWrapper() override;

public:
    // if we act as formatted, this is the PersistObject interface of our aggregate, used
    // to read and write the FormattedModel part
    // if bActAsFormatted is false, the state is undetermined until somebody calls
    // ::read or does anything which requires a living aggregate
    static css::uno::Reference<css::uno::XInterface> createFormattedFieldWrapper(const css::uno::Reference< css::uno::XComponentContext>& _rxFactory, bool bActAsFormatted, OUString const & implementationName);

    // UNO
    DECLARE_UNO3_AGG_DEFAULTS(OFormattedFieldWrapper, OWeakAggObject)
    virtual css::uno::Any SAL_CALL queryAggregation(const css::uno::Type& _rType) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // XPersistObject
    virtual OUString SAL_CALL getServiceName() override;
    virtual void SAL_CALL write(const css::uno::Reference< css::io::XObjectOutputStream>& _rxOutStream) override;
    virtual void SAL_CALL read(const css::uno::Reference< css::io::XObjectInputStream>& _rxInStream) override;

    // XCloneable
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone(  ) override;

private:
    /// ensure we're in a defined state, which means a FormattedModel _OR_ an EditModel
    void ensureAggregate();
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

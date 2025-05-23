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

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weakref.hxx>
#include <comphelper/uno3.hxx>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <OPropertySet.hxx>
#include <ModifyListenerHelper.hxx>

namespace chart
{

namespace impl
{
typedef ::cppu::WeakImplHelper<
        css::container::XChild,
        css::util::XCloneable,
        css::util::XModifyBroadcaster,
        css::util::XModifyListener,
        css::lang::XServiceInfo >
    DataPoint_Base;
}

class DataPoint final :
        public impl::DataPoint_Base,
        public ::property::OPropertySet
{
public:
    explicit DataPoint( const css::uno::Reference< css::beans::XPropertySet > & rParentProperties );
    virtual ~DataPoint() override;

    /// merge XInterface implementations
     DECLARE_XINTERFACE()
    /// XServiceInfo declarations
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

private:
    explicit DataPoint( const DataPoint & rOther );

    // ____ OPropertySet ____
    virtual void GetDefaultValue( sal_Int32 nHandle, css::uno::Any& rAny ) const override;
    virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper() override;
    virtual void SAL_CALL setFastPropertyValue_NoBroadcast
        ( sal_Int32 nHandle,
          const css::uno::Any& rValue ) override;

    // ____ XPropertySet ____
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
        getPropertySetInfo() override;

    // ____ XCloneable ____
    // Note: m_xParentProperties are not cloned!
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone() override;

    // ____ XChild ____
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL getParent() override;
    virtual void SAL_CALL setParent(
        const css::uno::Reference< css::uno::XInterface >& Parent ) override;

    // ____ XModifyBroadcaster ____
    virtual void SAL_CALL addModifyListener(
        const css::uno::Reference< css::util::XModifyListener >& aListener ) override;
    virtual void SAL_CALL removeModifyListener(
        const css::uno::Reference< css::util::XModifyListener >& aListener ) override;

    // ____ XModifyListener ____
    virtual void SAL_CALL modified(
        const css::lang::EventObject& aEvent ) override;

    // ____ XEventListener (base of XModifyListener) ____
    virtual void SAL_CALL disposing(
        const css::lang::EventObject& Source ) override;

    // ____ OPropertySet ____
    virtual void firePropertyChangeEvent() override;
    using OPropertySet::disposing;

    css::uno::WeakReference< css::beans::XPropertySet >   m_xParentProperties;

    rtl::Reference<ModifyEventForwarder> m_xModifyEventForwarder;
    bool m_bNoParentPropAllowed;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

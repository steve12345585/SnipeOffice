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

#include <memory>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyState.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <svl/lstner.hxx>
#include <comphelper/servicehelper.hxx>

#include <cppuhelper/implbase.hxx>
#include <editeng/unoipset.hxx>

class SdDrawDocument;
class SdrModel;
class SfxItemSet;
class SvxItemPropertySet;
struct SfxItemPropertyMapEntry;

const SvxItemPropertySet* ImplGetPageBackgroundPropertySet();

class SdUnoPageBackground final : public ::cppu::WeakImplHelper<
                                    css::beans::XPropertySet,
                                    css::lang::XServiceInfo,
                                    css::beans::XPropertyState>,
                            public SfxListener
{
    const SvxItemPropertySet*  mpPropSet;
    SvxItemPropertySetUsrAnys maUsrAnys;
    std::unique_ptr<SfxItemSet> mpSet;
    SdrModel*           mpDoc;

    const SfxItemPropertyMapEntry* getPropertyMapEntry( std::u16string_view rPropertyName ) const noexcept;
public:
    SdUnoPageBackground( SdDrawDocument* pDoc = nullptr, const SfxItemSet* pSet = nullptr);
    virtual ~SdUnoPageBackground() noexcept override;

    // internal
    void fillItemSet( SdDrawDocument* pDoc, SfxItemSet& rSet );
    virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue( const OUString& aPropertyName, const css::uno::Any& aValue ) override;
    virtual css::uno::Any SAL_CALL getPropertyValue( const OUString& PropertyName ) override;
    virtual void SAL_CALL addPropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener ) override;
    virtual void SAL_CALL removePropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& aListener ) override;
    virtual void SAL_CALL addVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;
    virtual void SAL_CALL removeVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;

    // XPropertyState
    virtual css::beans::PropertyState SAL_CALL getPropertyState( const OUString& PropertyName ) override;
    virtual css::uno::Sequence< css::beans::PropertyState > SAL_CALL getPropertyStates( const css::uno::Sequence< OUString >& aPropertyName ) override;
    virtual void SAL_CALL setPropertyToDefault( const OUString& PropertyName ) override;
    virtual css::uno::Any SAL_CALL getPropertyDefault( const OUString& aPropertyName ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

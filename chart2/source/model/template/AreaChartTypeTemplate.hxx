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

#include <ChartTypeTemplate.hxx>
#include <StackMode.hxx>

#include <OPropertySet.hxx>
#include <comphelper/uno3.hxx>

namespace chart
{

class AreaChartTypeTemplate :
        public ChartTypeTemplate,
        public ::property::OPropertySet
{
public:
    explicit AreaChartTypeTemplate(
        css::uno::Reference< css::uno::XComponentContext > const & xContext,
        const OUString & rServiceName,
        StackMode eStackMode,
        sal_Int32 nDim = 2 );
    virtual ~AreaChartTypeTemplate() override;

    /// merge XInterface implementations
     DECLARE_XINTERFACE()
    /// merge XTypeProvider implementations
     DECLARE_XTYPEPROVIDER()

protected:
    // ____ OPropertySet ____
    virtual void GetDefaultValue( sal_Int32 nHandle, css::uno::Any& rAny ) const override;
    virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper() override;

    // ____ XPropertySet ____
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
        getPropertySetInfo() override;

    // ____ ChartTypeTemplate ____
    virtual rtl::Reference< ::chart::ChartType >
        getChartTypeForNewSeries2( const std::vector<
            rtl::Reference< ::chart::ChartType > >& aFormerlyUsedChartTypes ) override;
    virtual void applyStyle2(
        const rtl::Reference< ::chart::DataSeries >& xSeries,
        ::sal_Int32 nChartTypeGroupIndex,
        ::sal_Int32 nSeriesIndex,
        ::sal_Int32 nSeriesCount ) override;
    virtual void resetStyles2(
        const rtl::Reference< ::chart::Diagram >& xDiagram ) override;

    // ____ ChartTypeTemplate ____
    virtual rtl::Reference< ::chart::ChartType >
                getChartTypeForIndex( sal_Int32 nChartTypeIndex ) override;
    virtual sal_Int32 getDimension() const override;
    virtual StackMode getStackMode( sal_Int32 nChartTypeIndex ) const override;

private:
    StackMode          m_eStackMode;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

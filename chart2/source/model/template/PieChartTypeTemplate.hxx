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

#include <OPropertySet.hxx>
#include <comphelper/uno3.hxx>

#include <ChartTypeTemplate.hxx>
#include <com/sun/star/chart2/PieChartOffsetMode.hpp>
#include <com/sun/star/chart2/PieChartSubType.hpp>

namespace chart
{

class PieChartTypeTemplate :
        public ChartTypeTemplate,
        public ::property::OPropertySet
{
public:
    PieChartTypeTemplate(
        css::uno::Reference< css::uno::XComponentContext > const & xContext,
        const OUString & rServiceName,
        css::chart2::PieChartOffsetMode eMode,
        bool bRings,
        css::chart2::PieChartSubType eSubType,
        sal_Int32 nCompositeSize,
        sal_Int32 nDim );
    virtual ~PieChartTypeTemplate() override;

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
    virtual bool matchesTemplate2(
        const rtl::Reference< ::chart::Diagram >& xDiagram,
        bool bAdaptProperties ) override;
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
    virtual sal_Int32 getDimension() const override;

    virtual void adaptDiagram(
        const rtl::Reference< ::chart::Diagram > & xDiagram ) override;

    virtual sal_Int32 getAxisCountByDimension( sal_Int32 nDimension ) override;

    virtual void adaptAxes(
        const std::vector< rtl::Reference< ::chart::BaseCoordinateSystem > > & rCoordSys ) override;

    virtual void adaptScales(
        const std::vector< rtl::Reference< ::chart::BaseCoordinateSystem > > & aCooSysSeq,
        const css::uno::Reference< css::chart2::data::XLabeledDataSequence > & xCategories ) override;

    virtual void createChartTypes(
            const std::vector<
                std::vector<
                    rtl::Reference<
                        ::chart::DataSeries > > >& aSeriesSeq,
            const std::vector<
                rtl::Reference<
                    ::chart::BaseCoordinateSystem > > & rCoordSys,
            const std::vector< rtl::Reference< ChartType > > & aOldChartTypesSeq
            ) override;

    virtual rtl::Reference< ::chart::ChartType >
                getChartTypeForIndex( sal_Int32 nChartTypeIndex ) override;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

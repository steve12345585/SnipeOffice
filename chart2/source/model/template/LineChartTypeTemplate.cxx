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

#include "LineChartTypeTemplate.hxx"
#include "LineChartType.hxx"
#include <Diagram.hxx>
#include <DataSeries.hxx>
#include <DataSeriesHelper.hxx>
#include <PropertyHelper.hxx>
#include <ChartType.hxx>
#include <unonames.hxx>

#include <com/sun/star/chart2/CurveStyle.hpp>
#include <com/sun/star/chart2/SymbolStyle.hpp>
#include <com/sun/star/chart2/Symbol.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/diagnose_ex.hxx>

#include <algorithm>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::beans::Property;

namespace
{

enum
{
    PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE,
    PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION,
    PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER

};

::chart::tPropertyValueMap& StaticLineChartTypeTemplateDefaults()
{
    static ::chart::tPropertyValueMap aStaticDefaults =
        []()
        {
            ::chart::tPropertyValueMap aOutMap;
            ::chart::PropertyHelper::setPropertyValueDefault( aOutMap, PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE, chart2::CurveStyle_LINES );
            ::chart::PropertyHelper::setPropertyValueDefault< sal_Int32 >( aOutMap, PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION, 20 );

            // todo: check whether order 3 means polygons of order 3 or 2. (see
            // http://www.people.nnov.ru/fractal/Splines/Basis.htm )
            ::chart::PropertyHelper::setPropertyValueDefault< sal_Int32 >( aOutMap, PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER, 3 );
            return aOutMap;
        }();
    return aStaticDefaults;
}

::cppu::OPropertyArrayHelper& StaticLineChartTypeTemplateInfoHelper()
{
    static ::cppu::OPropertyArrayHelper aPropHelper(
        []()
        {
            std::vector< css::beans::Property > aProperties {
                { CHART_UNONAME_CURVE_STYLE,
                  PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE,
                  cppu::UnoType<chart2::CurveStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT },
                { CHART_UNONAME_CURVE_RESOLUTION,
                  PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT },
                { CHART_UNONAME_SPLINE_ORDER,
                  PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT } };
            std::sort( aProperties.begin(), aProperties.end(),
                         ::chart::PropertyNameLess() );

            return comphelper::containerToSequence( aProperties );
        }());
    return aPropHelper;
}

uno::Reference< beans::XPropertySetInfo >& StaticLineChartTypeTemplateInfo()
{
    static uno::Reference< beans::XPropertySetInfo > xPropertySetInfo(
        ::cppu::OPropertySetHelper::createPropertySetInfo(StaticLineChartTypeTemplateInfoHelper() ) );
    return xPropertySetInfo;
}

} // anonymous namespace

namespace chart
{

LineChartTypeTemplate::LineChartTypeTemplate(
    uno::Reference<
        uno::XComponentContext > const & xContext,
    const OUString & rServiceName,
    StackMode eStackMode,
    bool bSymbols,
    bool bHasLines /* = true */,
    sal_Int32 nDim /* = 2 */ ) :
        ChartTypeTemplate( xContext, rServiceName ),
        m_eStackMode( eStackMode ),
        m_bHasSymbols( bSymbols ),
        m_bHasLines( bHasLines ),
        m_nDim( nDim )
{
    if( nDim == 3 )
        m_bHasSymbols = false;
}

LineChartTypeTemplate::~LineChartTypeTemplate()
{}

// ____ OPropertySet ____
void LineChartTypeTemplate::GetDefaultValue( sal_Int32 nHandle, uno::Any& rAny ) const
{
    const tPropertyValueMap& rStaticDefaults = StaticLineChartTypeTemplateDefaults();
    tPropertyValueMap::const_iterator aFound( rStaticDefaults.find( nHandle ) );
    if( aFound == rStaticDefaults.end() )
        rAny.clear();
    else
        rAny = (*aFound).second;
}

::cppu::IPropertyArrayHelper & SAL_CALL LineChartTypeTemplate::getInfoHelper()
{
    return StaticLineChartTypeTemplateInfoHelper();
}

// ____ XPropertySet ____
uno::Reference< beans::XPropertySetInfo > SAL_CALL LineChartTypeTemplate::getPropertySetInfo()
{
    return StaticLineChartTypeTemplateInfo();
}

sal_Int32 LineChartTypeTemplate::getDimension() const
{
    return m_nDim;
}

StackMode LineChartTypeTemplate::getStackMode( sal_Int32 /* nChartTypeIndex */ ) const
{
    return m_eStackMode;
}

// ____ ChartTypeTemplate ____
bool LineChartTypeTemplate::matchesTemplate2(
    const rtl::Reference< ::chart::Diagram >& xDiagram,
    bool bAdaptProperties )
{
    bool bResult = ChartTypeTemplate::matchesTemplate2( xDiagram, bAdaptProperties );

    // check symbol-style and line-style
    // for a template with symbols (or with lines) it is ok, if there is at least one series
    // with symbols (or with lines)
    if( bResult )
    {
        bool bSymbolFound = false;
        bool bLineFound = false;

        std::vector< rtl::Reference< DataSeries > > aSeriesVec =
            xDiagram->getDataSeries();

        for (auto const& series : aSeriesVec)
        {
            try
            {
                chart2::Symbol aSymbProp;
                drawing::LineStyle eLineStyle;

                bool bCurrentHasSymbol = (series->getPropertyValue( u"Symbol"_ustr) >>= aSymbProp) &&
                    (aSymbProp.Style != chart2::SymbolStyle_NONE);

                if( bCurrentHasSymbol )
                    bSymbolFound = true;

                if( bCurrentHasSymbol && (!m_bHasSymbols) )
                {
                    bResult = false;
                    break;
                }

                bool bCurrentHasLine = (series->getPropertyValue( u"LineStyle"_ustr) >>= eLineStyle) &&
                    ( eLineStyle != drawing::LineStyle_NONE );

                if( bCurrentHasLine )
                    bLineFound = true;

                if( bCurrentHasLine && (!m_bHasLines) )
                {
                    bResult = false;
                    break;
                }
            }
            catch( const uno::Exception & )
            {
                DBG_UNHANDLED_EXCEPTION("chart2");
            }
        }

        if(bResult)
        {
            if( !bLineFound && m_bHasLines && bSymbolFound )
                bResult = false;
            else if( !bSymbolFound && m_bHasSymbols && bLineFound )
                bResult = false;
            else if( !bLineFound && !bSymbolFound )
                return m_bHasLines && m_bHasSymbols;
        }
    }

    // adapt curve style, spline order and resolution
    if( bResult && bAdaptProperties )
    {
        try
        {
            rtl::Reference< ChartType > xChartType = xDiagram->getChartTypeByIndex( 0 );
            setFastPropertyValue_NoBroadcast( PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE, xChartType->getPropertyValue(CHART_UNONAME_CURVE_STYLE) );
            setFastPropertyValue_NoBroadcast( PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION, xChartType->getPropertyValue(CHART_UNONAME_CURVE_RESOLUTION) );
            setFastPropertyValue_NoBroadcast( PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER, xChartType->getPropertyValue(CHART_UNONAME_SPLINE_ORDER) );
        }
        catch( const uno::Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }

    return bResult;
}

rtl::Reference< ChartType > LineChartTypeTemplate::getChartTypeForIndex( sal_Int32 /*nChartTypeIndex*/ )
{
    rtl::Reference< ChartType > xResult;

    try
    {
        xResult = new LineChartType();

        xResult->setPropertyValue(
            CHART_UNONAME_CURVE_STYLE, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE ));
        xResult->setPropertyValue(
            CHART_UNONAME_CURVE_RESOLUTION, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION ));
        xResult->setPropertyValue(
            CHART_UNONAME_SPLINE_ORDER, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER ));
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }

    return xResult;
}

rtl::Reference< ChartType > LineChartTypeTemplate::getChartTypeForNewSeries2(
        const std::vector< rtl::Reference< ChartType > >& aFormerlyUsedChartTypes )
{
    rtl::Reference< ChartType > xResult;

    try
    {
        xResult = new LineChartType();

        ChartTypeTemplate::copyPropertiesFromOldToNewCoordinateSystem( aFormerlyUsedChartTypes, xResult );

        xResult->setPropertyValue(
            CHART_UNONAME_CURVE_STYLE, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_CURVE_STYLE ));
        xResult->setPropertyValue(
            CHART_UNONAME_CURVE_RESOLUTION, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_CURVE_RESOLUTION ));
        xResult->setPropertyValue(
            CHART_UNONAME_SPLINE_ORDER, getFastPropertyValue( PROP_LINECHARTTYPE_TEMPLATE_SPLINE_ORDER ));
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }

    return xResult;
}

void LineChartTypeTemplate::applyStyle2(
    const rtl::Reference< DataSeries >& xSeries,
    ::sal_Int32 nChartTypeIndex,
    ::sal_Int32 nSeriesIndex,
    ::sal_Int32 nSeriesCount )
{
    ChartTypeTemplate::applyStyle2( xSeries, nChartTypeIndex, nSeriesIndex, nSeriesCount );

    try
    {
        xSeries->switchSymbolsOnOrOff( m_bHasSymbols, nSeriesIndex );
        xSeries->switchLinesOnOrOff( m_bHasLines );
        xSeries->makeLinesThickOrThin( m_nDim==2 );
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
}

IMPLEMENT_FORWARD_XINTERFACE2( LineChartTypeTemplate, ChartTypeTemplate, OPropertySet )
IMPLEMENT_FORWARD_XTYPEPROVIDER2( LineChartTypeTemplate, ChartTypeTemplate, OPropertySet )

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

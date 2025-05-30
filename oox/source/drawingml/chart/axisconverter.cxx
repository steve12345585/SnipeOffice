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

#include <drawingml/chart/axisconverter.hxx>
#include <ooxresid.hxx>
#include <strings.hrc>

#include <com/sun/star/chart/ChartAxisArrangeOrderType.hpp>
#include <com/sun/star/chart/ChartAxisLabelPosition.hpp>
#include <com/sun/star/chart/ChartAxisMarkPosition.hpp>
#include <com/sun/star/chart/ChartAxisPosition.hpp>
#include <com/sun/star/chart/TimeInterval.hpp>
#include <com/sun/star/chart/TimeUnit.hpp>
#include <com/sun/star/chart2/AxisType.hpp>
#include <com/sun/star/chart2/TickmarkStyle.hpp>
#include <com/sun/star/chart2/LinearScaling.hpp>
#include <com/sun/star/chart2/LogarithmicScaling.hpp>
#include <com/sun/star/chart2/XAxis.hpp>
#include <com/sun/star/chart2/XCoordinateSystem.hpp>
#include <com/sun/star/chart2/XTitled.hpp>
#include <drawingml/chart/axismodel.hxx>
#include <drawingml/chart/titleconverter.hxx>
#include <drawingml/chart/typegroupconverter.hxx>
#include <drawingml/lineproperties.hxx>
#include <drawingml/textbody.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/properties.hxx>
#include <oox/token/tokens.hxx>
#include <comphelper/processfactory.hxx>
#include <osl/diagnose.h>

namespace oox::drawingml::chart {

using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::chart2;
using namespace ::com::sun::star::uno;

namespace {

void lclSetValueOrClearAny( Any& orAny, const std::optional< double >& rofValue )
{
    if( rofValue.has_value() ) orAny <<= rofValue.value(); else orAny.clear();
}

bool lclIsLogarithmicScale( const AxisModel& rAxisModel )
{
    return rAxisModel.mofLogBase.has_value() && (2.0 <= rAxisModel.mofLogBase.value()) && (rAxisModel.mofLogBase.value() <= 1000.0);
}

sal_Int32 lclGetApiTimeUnit( sal_Int32 nTimeUnit )
{
    using namespace ::com::sun::star::chart;
    switch( nTimeUnit )
    {
        case XML_days:      return TimeUnit::DAY;
        case XML_months:    return TimeUnit::MONTH;
        case XML_years:     return TimeUnit::YEAR;
        default:            OSL_ENSURE( false, "lclGetApiTimeUnit - unexpected time unit" );
    }
    return TimeUnit::DAY;
}

void lclConvertTimeInterval( Any& orInterval, const std::optional< double >& rofUnit, sal_Int32 nTimeUnit )
{
    if( rofUnit.has_value() && (1.0 <= rofUnit.value()) && (rofUnit.value() <= SAL_MAX_INT32) )
        orInterval <<= css::chart::TimeInterval( static_cast< sal_Int32 >( rofUnit.value() ), lclGetApiTimeUnit( nTimeUnit ) );
    else
        orInterval.clear();
}

css::chart::ChartAxisLabelPosition lclGetLabelPosition( sal_Int32 nToken )
{
    using namespace ::com::sun::star::chart;
    switch( nToken )
    {
        case XML_high:      return ChartAxisLabelPosition_OUTSIDE_END;
        case XML_low:       return ChartAxisLabelPosition_OUTSIDE_START;
        case XML_nextTo:    return ChartAxisLabelPosition_NEAR_AXIS;
    }
    return ChartAxisLabelPosition_NEAR_AXIS;
}

sal_Int32 lclGetTickMark( sal_Int32 nToken )
{
    using namespace ::com::sun::star::chart2::TickmarkStyle;
    switch( nToken )
    {
        case XML_in:    return INNER;
        case XML_out:   return OUTER;
        case XML_cross: return INNER | OUTER;
    }
    return css::chart2::TickmarkStyle::NONE;
}

/**
 * The groups is of percent type only when all of its members are of percent
 * type.
 */
bool isPercent( const RefVector<TypeGroupConverter>& rTypeGroups )
{
    if (rTypeGroups.empty())
        return false;

    for (auto const& typeGroup : rTypeGroups)
    {
        TypeGroupConverter& rConv = *typeGroup;
        if (!rConv.isPercent())
            return false;
    }

    return true;
}

} // namespace

AxisConverter::AxisConverter( const ConverterRoot& rParent, AxisModel& rModel ) :
    ConverterBase< AxisModel >( rParent, rModel )
{
}

AxisConverter::~AxisConverter()
{
}

void AxisConverter::convertFromModel(const Reference<XCoordinateSystem>& rxCoordSystem,
                                     RefVector<TypeGroupConverter>& rTypeGroups,
                                     const AxisModel* pCrossingAxis, sal_Int32 nAxesSetIdx,
                                     sal_Int32 nAxisIdx, bool bUseFixedInnerSize)
{
    if (rTypeGroups.empty())
        return;

    Reference< XAxis > xAxis;
    try
    {
        namespace cssc = css::chart;
        namespace cssc2 = css::chart2;

        const TypeGroupInfo& rTypeInfo = rTypeGroups.front()->getTypeInfo();
        ObjectFormatter& rFormatter = getFormatter();

        // create the axis object (always)
        xAxis.set( createInstance( u"com.sun.star.chart2.Axis"_ustr ), UNO_QUERY_THROW );
        PropertySet aAxisProp( xAxis );
        // #i58688# axis enabled
        aAxisProp.setProperty( PROP_Show, !mrModel.mbDeleted );

        // axis line, tick, and gridline properties ---------------------------

        // show axis labels
        aAxisProp.setProperty( PROP_DisplayLabels, mrModel.mnTickLabelPos != XML_none );
        aAxisProp.setProperty( PROP_LabelPosition, lclGetLabelPosition( mrModel.mnTickLabelPos ) );
        // no X axis line in radar charts
        if( (nAxisIdx == API_X_AXIS) && (rTypeInfo.meTypeCategory == TYPECATEGORY_RADAR) )
            mrModel.mxShapeProp.getOrCreate().getLineProperties().maLineFill.moFillType = XML_noFill;
        // axis line and tick label formatting
        rFormatter.convertFormatting( aAxisProp, mrModel.mxShapeProp, mrModel.mxTextProp, OBJECTTYPE_AXIS );
        // tick label rotation
        ObjectFormatter::convertTextRotation( aAxisProp, mrModel.mxTextProp, true );

        // tick mark style
        aAxisProp.setProperty( PROP_MajorTickmarks, lclGetTickMark( mrModel.mnMajorTickMark ) );
        aAxisProp.setProperty( PROP_MinorTickmarks, lclGetTickMark( mrModel.mnMinorTickMark ) );
        aAxisProp.setProperty( PROP_MarkPosition, cssc::ChartAxisMarkPosition_AT_AXIS );

        // main grid
        PropertySet aGridProp( xAxis->getGridProperties() );
        aGridProp.setProperty( PROP_Show, mrModel.mxMajorGridLines.is() );
        if( mrModel.mxMajorGridLines.is() )
            rFormatter.convertFrameFormatting( aGridProp, mrModel.mxMajorGridLines, OBJECTTYPE_MAJORGRIDLINE );

        // sub grid
        Sequence< Reference< XPropertySet > > aSubGridPropSeq = xAxis->getSubGridProperties();
        if( aSubGridPropSeq.hasElements() )
        {
            PropertySet aSubGridProp( aSubGridPropSeq[ 0 ] );
            aSubGridProp.setProperty( PROP_Show, mrModel.mxMinorGridLines.is() );
            if( mrModel.mxMinorGridLines.is() )
                rFormatter.convertFrameFormatting( aSubGridProp, mrModel.mxMinorGridLines, OBJECTTYPE_MINORGRIDLINE );
        }

        // axis type and X axis categories ------------------------------------

        ScaleData aScaleData = xAxis->getScaleData();
        // set axis type
        switch( nAxisIdx )
        {
            case API_X_AXIS:
                if( rTypeInfo.mbCategoryAxis )
                {
                    OSL_ENSURE( (mrModel.mnTypeId == C_TOKEN( catAx )) || (mrModel.mnTypeId == C_TOKEN( dateAx )),
                        "AxisConverter::convertFromModel - unexpected axis model type (must: c:catAx or c:dateAx)" );
                    bool bDateAxis = mrModel.mnTypeId == C_TOKEN( dateAx );
                    // tdf#132076: set axis type to date, if it is a date axis!
                    aScaleData.AxisType = bDateAxis ? cssc2::AxisType::DATE : cssc2::AxisType::CATEGORY;
                    aScaleData.AutoDateAxis = mrModel.mbAuto;
                    /* TODO: create main category axis labels once, while InternalDataProvider
                    can not handle different category names on the primary and secondary category axis. */
                    if( nAxesSetIdx == 0 )
                        aScaleData.Categories = rTypeGroups.front()->createCategorySequence();
                    /* set default ShiftedCategoryPosition values for some charttype,
                       because the XML can contain wrong CrossBetween value, if came from MSO */
                    if( rTypeGroups.front()->is3dChart() && (rTypeInfo.meTypeId == TYPEID_BAR || rTypeInfo.meTypeId == TYPEID_HORBAR || rTypeInfo.meTypeId == TYPEID_STOCK) )
                        aScaleData.ShiftedCategoryPosition = true;
                    else if( rTypeInfo.meTypeId == TYPEID_RADARLINE || rTypeInfo.meTypeId == TYPEID_RADARAREA )
                        aScaleData.ShiftedCategoryPosition = false;
                    else if( pCrossingAxis->mnCrossBetween != -1 ) /*because of backwards compatibility*/
                        aScaleData.ShiftedCategoryPosition = pCrossingAxis->mnCrossBetween == XML_between;
                    else if( rTypeInfo.meTypeCategory == TYPECATEGORY_BAR || rTypeInfo.meTypeId == TYPEID_LINE || rTypeInfo.meTypeId == TYPEID_STOCK )
                        aScaleData.ShiftedCategoryPosition = true;
                }
                else
                {
                    OSL_ENSURE( mrModel.mnTypeId == C_TOKEN( valAx ), "AxisConverter::convertFromModel - unexpected axis model type (must: c:valAx)" );
                    aScaleData.AxisType = cssc2::AxisType::REALNUMBER;
                }
            break;
            case API_Y_AXIS:
                OSL_ENSURE( mrModel.mnTypeId == C_TOKEN( valAx ), "AxisConverter::convertFromModel - unexpected axis model type (must: c:valAx)" );
                aScaleData.AxisType = isPercent(rTypeGroups) ? cssc2::AxisType::PERCENT : cssc2::AxisType::REALNUMBER;
            break;
            case API_Z_AXIS:
                OSL_ENSURE( mrModel.mnTypeId == C_TOKEN( serAx ), "AxisConverter::convertFromModel - unexpected axis model type (must: c:serAx)" );
                OSL_ENSURE( rTypeGroups.front()->isDeep3dChart(), "AxisConverter::convertFromModel - series axis not supported by this chart type" );
                aScaleData.AxisType = cssc2::AxisType::SERIES;
            break;
        }

        // axis scaling and increment -----------------------------------------

        switch( aScaleData.AxisType )
        {
            case cssc2::AxisType::CATEGORY:
            case cssc2::AxisType::SERIES:
            case cssc2::AxisType::DATE:
            {
                /*  Determine date axis type from XML type identifier, and not
                    via aScaleData.AxisType, as this value sticks to CATEGORY
                    for automatic category/date axes). */
                if( mrModel.mnTypeId == C_TOKEN( dateAx ) )
                {
                    // scaling algorithm
                    aScaleData.Scaling = LinearScaling::create( comphelper::getProcessComponentContext() );
                    // min/max
                    lclSetValueOrClearAny( aScaleData.Minimum, mrModel.mofMin );
                    lclSetValueOrClearAny( aScaleData.Maximum, mrModel.mofMax );
                    // major/minor increment
                    lclConvertTimeInterval( aScaleData.TimeIncrement.MajorTimeInterval, mrModel.mofMajorUnit, mrModel.mnMajorTimeUnit );
                    lclConvertTimeInterval( aScaleData.TimeIncrement.MinorTimeInterval, mrModel.mofMinorUnit, mrModel.mnMinorTimeUnit );
                    // base time unit
                    if( mrModel.monBaseTimeUnit.has_value() )
                        aScaleData.TimeIncrement.TimeResolution <<= lclGetApiTimeUnit( mrModel.monBaseTimeUnit.value() );
                    else
                        aScaleData.TimeIncrement.TimeResolution.clear();
                }
                else
                {
                    // do not overlap text unless the rotation is 0 in xml
                    bool bTextOverlap = false;
                    if (mrModel.mxTextProp.is()
                        && mrModel.mxTextProp->getTextProperties().moTextAreaRotation.has_value())
                        bTextOverlap
                            = mrModel.mxTextProp->getTextProperties().moTextAreaRotation.value() == 0;
                    aAxisProp.setProperty(PROP_TextOverlap, bTextOverlap);
                    /* do not break text into several lines unless the rotation is 0 degree,
                       or the rotation is 90 degree and the inner size of the chart is not fixed,
                       or the rotation is 270 degree and the inner size of the chart is not fixed */
                    bool bTextBreak = true;
                    double fRotationAngle = 0.0;
                    if (aAxisProp.getProperty(fRotationAngle, PROP_TextRotation)
                        && fRotationAngle != 0.0)
                        bTextBreak = !bUseFixedInnerSize
                                     && (fRotationAngle == 90.0 || fRotationAngle == 270.0);
                    aAxisProp.setProperty(PROP_TextBreak, bTextBreak);
                    // do not stagger labels in two lines
                    aAxisProp.setProperty( PROP_ArrangeOrder, cssc::ChartAxisArrangeOrderType_SIDE_BY_SIDE );
                    //! TODO #i58731# show n-th category
                }
            }
            break;
            case cssc2::AxisType::REALNUMBER:
            case cssc2::AxisType::PERCENT:
            {
                // scaling algorithm
                const bool bLogScale = lclIsLogarithmicScale( mrModel );
                if( bLogScale )
                    aScaleData.Scaling = LogarithmicScaling::create( comphelper::getProcessComponentContext() );
                else
                    aScaleData.Scaling = LinearScaling::create( comphelper::getProcessComponentContext() );
                // min/max
                lclSetValueOrClearAny( aScaleData.Minimum, mrModel.mofMin );
                lclSetValueOrClearAny( aScaleData.Maximum, mrModel.mofMax );
                // major increment
                IncrementData& rIncrementData = aScaleData.IncrementData;
                if( mrModel.mofMajorUnit.has_value() && aScaleData.Scaling.is() )
                    rIncrementData.Distance <<= aScaleData.Scaling->doScaling( mrModel.mofMajorUnit.value() );
                else
                    lclSetValueOrClearAny( rIncrementData.Distance, mrModel.mofMajorUnit );
                // minor increment
                Sequence< SubIncrement >& rSubIncrementSeq = rIncrementData.SubIncrements;
                rSubIncrementSeq.realloc( 1 );
                Any& rIntervalCount = rSubIncrementSeq.getArray()[ 0 ].IntervalCount;
                rIntervalCount.clear();
                if( bLogScale )
                {
                    if( mrModel.mofMinorUnit.has_value() )
                        rIntervalCount <<= sal_Int32( 9 );
                }
                else if( mrModel.mofMajorUnit.has_value() && mrModel.mofMinorUnit.has_value() && (0.0 < mrModel.mofMinorUnit.value()) && (mrModel.mofMinorUnit.value() <= mrModel.mofMajorUnit.value()) )
                {
                    double fCount = mrModel.mofMajorUnit.value() / mrModel.mofMinorUnit.value() + 0.5;
                    if( (1.0 <= fCount) && (fCount < 1001.0) )
                        rIntervalCount <<= static_cast< sal_Int32 >( fCount );
                }
                else if( !mrModel.mofMinorUnit.has_value() )
                {
                    // tdf#114168 If minor unit is not set then set interval to 5, as MS Excel do.
                    rIntervalCount <<= static_cast< sal_Int32 >( 5 );
                }
            }
            break;
            default:
                OSL_FAIL( "AxisConverter::convertFromModel - unknown axis type" );
        }

        /*  Do not set a value to the Origin member anymore (already done via
            new axis properties 'CrossoverPosition' and 'CrossoverValue'). */
        aScaleData.Origin.clear();

        // axis orientation ---------------------------------------------------

        // #i85167# pie/donut charts need opposite direction at Y axis
        // #i87747# radar charts need opposite direction at X axis
        bool bMirrorDirection =
            ((nAxisIdx == API_Y_AXIS) && (rTypeInfo.meTypeCategory == TYPECATEGORY_PIE)) ||
            ((nAxisIdx == API_X_AXIS) && (rTypeInfo.meTypeCategory == TYPECATEGORY_RADAR));
        bool bReverse = (mrModel.mnOrientation == XML_maxMin) != bMirrorDirection;
        aScaleData.Orientation = bReverse ? cssc2::AxisOrientation_REVERSE : cssc2::AxisOrientation_MATHEMATICAL;

        // write back scaling data
        xAxis->setScaleData( aScaleData );

        // number format ------------------------------------------------------
        if( !mrModel.mbDeleted && aScaleData.AxisType != cssc2::AxisType::SERIES )
        {
            getFormatter().convertNumberFormat(aAxisProp, mrModel.maNumberFormat, true);
        }

        // position of crossing axis ------------------------------------------

        bool bManualCrossing = mrModel.mofCrossesAt.has_value();
        cssc::ChartAxisPosition eAxisPos = cssc::ChartAxisPosition_VALUE;
        if( !bManualCrossing ) switch( mrModel.mnCrossMode )
        {
            case XML_min:       eAxisPos = cssc::ChartAxisPosition_START;   break;
            case XML_max:       eAxisPos = cssc::ChartAxisPosition_END;     break;
            case XML_autoZero:  eAxisPos = cssc::ChartAxisPosition_ZERO;   break;
        }

        aAxisProp.setProperty( PROP_CrossoverPosition, eAxisPos );

        // calculate automatic origin depending on scaling mode of crossing axis
        bool bCrossingLogScale = pCrossingAxis && lclIsLogarithmicScale( *pCrossingAxis );
        double fCrossingPos = bManualCrossing ? mrModel.mofCrossesAt.value() : (bCrossingLogScale ? 1.0 : 0.0);
        aAxisProp.setProperty( PROP_CrossoverValue, fCrossingPos );

        // axis title ---------------------------------------------------------

        // in radar charts, title objects may exist, but are not shown
        if( mrModel.mxTitle.is() && (rTypeGroups.front()->getTypeInfo().meTypeCategory != TYPECATEGORY_RADAR) )
        {
            Reference< XTitled > xTitled( xAxis, UNO_QUERY_THROW );
            if (((nAxisIdx == API_X_AXIS && rTypeInfo.meTypeId != TYPEID_HORBAR)
                || (nAxisIdx == API_Y_AXIS && rTypeInfo.meTypeId == TYPEID_HORBAR))
                && (mrModel.mnAxisPos == XML_l || mrModel.mnAxisPos == XML_r))
                mrModel.mxTitle->mnDefaultRotation = 0;
            TitleConverter aTitleConv( *this, *mrModel.mxTitle );
            aTitleConv.convertFromModel( xTitled, OoxResId(STR_DIAGRAM_AXISTITLE), OBJECTTYPE_AXISTITLE, nAxesSetIdx, nAxisIdx );
        }

        // axis data unit label -----------------------------------------------
        AxisDispUnitsConverter axisDispUnitsConverter (*this, mrModel.mxDispUnits.getOrCreate());
        axisDispUnitsConverter.convertFromModel(xAxis);
    }
    catch( Exception& )
    {
    }

    if( xAxis.is() && rxCoordSystem.is() ) try
    {
        // insert axis into coordinate system
        rxCoordSystem->setAxisByDimension( nAxisIdx, xAxis, nAxesSetIdx );
    }
    catch( Exception& )
    {
        OSL_FAIL( "AxisConverter::convertFromModel - cannot insert axis into coordinate system" );
    }
}

AxisDispUnitsConverter::AxisDispUnitsConverter( const ConverterRoot& rParent, AxisDispUnitsModel& rModel ) :
    ConverterBase< AxisDispUnitsModel >( rParent, rModel )
{
}

AxisDispUnitsConverter::~AxisDispUnitsConverter()
{
}

void AxisDispUnitsConverter::convertFromModel( const Reference< XAxis >& rxAxis )
{
    PropertySet aPropSet( rxAxis );
    if (!mrModel.mnBuiltInUnit.isEmpty() )
    {
        aPropSet.setProperty(PROP_DisplayUnits, true);
        aPropSet.setProperty( PROP_BuiltInUnit, mrModel.mnBuiltInUnit );
    }
}

} // namespace oox::drawingml::chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

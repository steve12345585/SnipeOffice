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

#include "DataPointProperties.hxx"
#include <LinePropertiesHelper.hxx>
#include <FillProperties.hxx>
#include <unonames.hxx>

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/LineDash.hpp>
#include <com/sun/star/drawing/LineCap.hpp>
#include <com/sun/star/drawing/BitmapMode.hpp>
#include <com/sun/star/drawing/RectanglePoint.hpp>
#include <com/sun/star/chart2/RelativePosition.hpp>
#include <com/sun/star/chart2/RelativeSize.hpp>
#include <com/sun/star/chart2/XDataPointCustomLabelField.hpp>
#include <com/sun/star/chart2/DataPointGeometry3D.hpp>
#include <com/sun/star/chart2/DataPointLabel.hpp>
#include <com/sun/star/chart2/Symbol.hpp>

#include <tools/color.hxx>

using namespace ::com::sun::star;

using ::com::sun::star::beans::Property;

namespace chart
{

void DataPointProperties::AddPropertiesToVector(
    std::vector< Property > & rOutProperties )
{
    // DataPointProperties

    // Common

    rOutProperties.emplace_back( "Color",
                  PROP_DATAPOINT_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID         // "maybe auto"
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillColor",
                  PROP_DATAPOINT_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID         // "maybe auto"
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "Transparency",
                  PROP_DATAPOINT_TRANSPARENCY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillTransparence",
                  PROP_DATAPOINT_TRANSPARENCY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    // Fill Properties
    rOutProperties.emplace_back( "FillStyle",
                  PROP_DATAPOINT_FILL_STYLE,
                  cppu::UnoType<drawing::FillStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "TransparencyGradientName",
                  PROP_DATAPOINT_TRANSPARENCY_GRADIENT_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "FillTransparenceGradientName",
                  PROP_DATAPOINT_TRANSPARENCY_GRADIENT_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "GradientName",
                  PROP_DATAPOINT_GRADIENT_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "FillGradientName",
                  PROP_DATAPOINT_GRADIENT_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "GradientStepCount",
                  PROP_DATAPOINT_GRADIENT_STEPCOUNT,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "FillGradientStepCount",
                  PROP_DATAPOINT_GRADIENT_STEPCOUNT,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "HatchName",
                  PROP_DATAPOINT_HATCH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "FillHatchName",
                  PROP_DATAPOINT_HATCH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "FillBitmapName",
                  PROP_DATAPOINT_FILL_BITMAP_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID  );

    rOutProperties.emplace_back( "FillBackground",
                  PROP_DATAPOINT_FILL_BACKGROUND,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT
                  | beans::PropertyAttribute::MAYBEVOID );

    // border for filled objects
    rOutProperties.emplace_back( "BorderColor",
                  PROP_DATAPOINT_BORDER_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID         // "maybe auto"
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "BorderStyle",
                  PROP_DATAPOINT_BORDER_STYLE,
                  cppu::UnoType<drawing::LineStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "BorderWidth",
                  PROP_DATAPOINT_BORDER_WIDTH,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "BorderDashName",
                  PROP_DATAPOINT_BORDER_DASH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "BorderTransparency",
                  PROP_DATAPOINT_BORDER_TRANSPARENCY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    // Line Properties

    rOutProperties.emplace_back( "LineColor",
                  PROP_DATAPOINT_BORDER_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "LineStyle",
                  LinePropertiesHelper::PROP_LINE_STYLE,
                  cppu::UnoType<drawing::LineStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "LineWidth",
                  LinePropertiesHelper::PROP_LINE_WIDTH,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "LineDash",
                   LinePropertiesHelper::PROP_LINE_DASH,
                   cppu::UnoType<drawing::LineDash>::get(),
                   beans::PropertyAttribute::BOUND
                   | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "LineDashName",
                  LinePropertiesHelper::PROP_LINE_DASH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "LineTransparence",
                  PROP_DATAPOINT_BORDER_TRANSPARENCY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "LineCap",
                  ::chart::LinePropertiesHelper::PROP_LINE_CAP,
                  cppu::UnoType<drawing::LineCap>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    // FillProperties
    // bitmap properties
    rOutProperties.emplace_back( "FillBitmapOffsetX",
                  FillProperties::PROP_FILL_BITMAP_OFFSETX,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapOffsetY",
                  FillProperties::PROP_FILL_BITMAP_OFFSETY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapPositionOffsetX",
                  FillProperties::PROP_FILL_BITMAP_POSITION_OFFSETX,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapPositionOffsetY",
                  FillProperties::PROP_FILL_BITMAP_POSITION_OFFSETY,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapRectanglePoint",
                  FillProperties::PROP_FILL_BITMAP_RECTANGLEPOINT,
                  cppu::UnoType<drawing::RectanglePoint>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapLogicalSize",
                  FillProperties::PROP_FILL_BITMAP_LOGICALSIZE,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapSizeX",
                  FillProperties::PROP_FILL_BITMAP_SIZEX,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapSizeY",
                  FillProperties::PROP_FILL_BITMAP_SIZEY,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "FillBitmapMode",
                  FillProperties::PROP_FILL_BITMAP_MODE,
                  cppu::UnoType<drawing::BitmapMode>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    // others
    rOutProperties.emplace_back( "Symbol",
                  PROP_DATAPOINT_SYMBOL_PROP,
                  cppu::UnoType<chart2::Symbol>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( "Offset",
                  PROP_DATAPOINT_OFFSET,
                  cppu::UnoType<double>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( "Geometry3D",
                  PROP_DATAPOINT_GEOMETRY3D,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( CHART_UNONAME_NUMFMT,
                  PROP_DATAPOINT_NUMBER_FORMAT,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( CHART_UNONAME_LINK_TO_SRC_NUMFMT,
                  PROP_DATAPOINT_LINK_NUMBERFORMAT_TO_SOURCE,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    //additional 'PercentageNumberFormat'
    rOutProperties.emplace_back( "PercentageNumberFormat",
                  PROP_DATAPOINT_PERCENTAGE_NUMBER_FORMAT,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "LabelPlacement",
                  PROP_DATAPOINT_LABEL_PLACEMENT,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "ReferencePageSize",
                  PROP_DATAPOINT_REFERENCE_DIAGRAM_SIZE,
                  cppu::UnoType<awt::Size>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back( "TextRotation",
                  PROP_DATAPOINT_TEXT_ROTATION,
                  cppu::UnoType<double>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "InvertNegative",
                  PROP_DATAPOINT_INVERT_NEGATIVE,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    // statistics
    rOutProperties.emplace_back( CHART_UNONAME_ERRORBAR_X,
                  PROP_DATAPOINT_ERROR_BAR_X,
                  // XPropertySet supporting service ErrorBar
                  cppu::UnoType<beans::XPropertySet>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );
    rOutProperties.emplace_back( CHART_UNONAME_ERRORBAR_Y,
                  PROP_DATAPOINT_ERROR_BAR_Y,
                  // XPropertySet supporting service ErrorBar
                  cppu::UnoType<beans::XPropertySet>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );
    rOutProperties.emplace_back( "ShowErrorBox",
                  PROP_DATAPOINT_SHOW_ERROR_BOX,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );
    rOutProperties.emplace_back( "PercentDiagonal",
                  PROP_DATAPOINT_PERCENT_DIAGONAL,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    // Properties specific to data label.

    rOutProperties.emplace_back( CHART_UNONAME_LABEL,
                  PROP_DATAPOINT_LABEL,
                  cppu::UnoType<chart2::DataPointLabel>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( "TextWordWrap",
                  PROP_DATAPOINT_TEXT_WORD_WRAP,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( CHART_UNONAME_LABEL_SEP,
                  PROP_DATAPOINT_LABEL_SEPARATOR,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_STYLE,
                  PROP_DATAPOINT_LABEL_BORDER_STYLE,
                  cppu::UnoType<drawing::LineStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_COLOR,
                  PROP_DATAPOINT_LABEL_BORDER_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID         // "maybe auto"
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_FILL_STYLE,
                  PROP_DATAPOINT_LABEL_FILL_STYLE,
                  cppu::UnoType<drawing::FillStyle>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_FILL_COLOR,
                  PROP_DATAPOINT_LABEL_FILL_COLOR,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_FILL_BACKGROUND,
                  PROP_DATAPOINT_LABEL_FILL_BACKGROUND,
                  cppu::UnoType<sal_Bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_FILL_HATCH_NAME,
                  PROP_DATAPOINT_LABEL_FILL_HATCH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_WIDTH,
                  PROP_DATAPOINT_LABEL_BORDER_WIDTH,
                  cppu::UnoType<sal_Int32>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_DASH,
                   PROP_DATAPOINT_LABEL_BORDER_DASH,
                   cppu::UnoType<drawing::LineDash>::get(),
                   beans::PropertyAttribute::BOUND
                   | beans::PropertyAttribute::MAYBEVOID );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_DASHNAME,
                  PROP_DATAPOINT_LABEL_BORDER_DASH_NAME,
                  cppu::UnoType<OUString>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );
    rOutProperties.emplace_back( CHART_UNONAME_LABEL_BORDER_TRANS,
                  PROP_DATAPOINT_LABEL_BORDER_TRANS,
                  cppu::UnoType<sal_Int16>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT );

    rOutProperties.emplace_back( CHART_UNONAME_CUSTOM_LABEL_FIELDS,
                  PROP_DATAPOINT_CUSTOM_LABEL_FIELDS,
                  cppu::UnoType<uno::Sequence<uno::Reference<chart2::XDataPointCustomLabelField>>>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT);

    rOutProperties.emplace_back( "CustomLabelPosition",
                  PROP_DATAPOINT_LABEL_CUSTOM_POS,
                  cppu::UnoType<chart2::RelativePosition>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );

    rOutProperties.emplace_back("CustomLabelSize", PROP_DATAPOINT_LABEL_CUSTOM_SIZE,
                                cppu::UnoType<chart2::RelativeSize>::get(),
                                beans::PropertyAttribute::BOUND
                                    | beans::PropertyAttribute::MAYBEVOID);
}

void DataPointProperties::AddDefaultsToMap(
    ::chart::tPropertyValueMap & rOutMap )
{
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_COLOR, Color(0x99, 0xcc, 0xff) ); // blue 8
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_TRANSPARENCY, sal_Int16(0) );

    //fill
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_FILL_STYLE, drawing::FillStyle_SOLID );
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, PROP_DATAPOINT_TRANSPARENCY_GRADIENT_NAME );
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, PROP_DATAPOINT_GRADIENT_NAME );
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, PROP_DATAPOINT_HATCH_NAME );
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, PROP_DATAPOINT_FILL_BITMAP_NAME );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_FILL_BACKGROUND, false );

    //border
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_BORDER_COLOR, COL_BLACK );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_BORDER_STYLE, drawing::LineStyle_SOLID ); // drawing::LineStyle_NONE
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_BORDER_WIDTH, sal_Int32(0) );
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, PROP_DATAPOINT_BORDER_DASH_NAME );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_BORDER_TRANSPARENCY, sal_Int16(0) );

    //line
    PropertyHelper::setPropertyValueDefault( rOutMap, LinePropertiesHelper::PROP_LINE_STYLE, drawing::LineStyle_SOLID );
    PropertyHelper::setPropertyValueDefault( rOutMap, LinePropertiesHelper::PROP_LINE_WIDTH, sal_Int32(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, LinePropertiesHelper::PROP_LINE_DASH, drawing::LineDash());
    PropertyHelper::setEmptyPropertyValueDefault( rOutMap, LinePropertiesHelper::PROP_LINE_DASH_NAME );
    PropertyHelper::setPropertyValueDefault( rOutMap, LinePropertiesHelper::PROP_LINE_CAP, drawing::LineCap_BUTT);

    //fill bitmap
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_OFFSETX, sal_Int16(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_OFFSETY, sal_Int16(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_POSITION_OFFSETX, sal_Int16(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_POSITION_OFFSETY, sal_Int16(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_RECTANGLEPOINT, drawing::RectanglePoint_MIDDLE_MIDDLE );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_LOGICALSIZE, true );

    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_SIZEX, sal_Int32(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_SIZEY, sal_Int32(0) );
    PropertyHelper::setPropertyValueDefault( rOutMap, FillProperties::PROP_FILL_BITMAP_MODE, drawing::BitmapMode_REPEAT );

    //others
    chart2::Symbol aSymbProp;
    aSymbProp.Style = chart2::SymbolStyle_NONE;
    aSymbProp.StandardSymbol = 0;
    aSymbProp.Size = awt::Size( 250, 250 ); // ca. 7pt x 7pt (7pt=246.94)
    aSymbProp.BorderColor = sal_Int32(COL_BLACK);
    aSymbProp.FillColor = 0xee4000;         // OrangeRed2
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_SYMBOL_PROP, aSymbProp );

    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_OFFSET, 0.0 );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_GEOMETRY3D, chart2::DataPointGeometry3D::CUBOID );

    //@todo maybe choose a different one here -> should be dynamically that of the attached axis
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_ERROR_BAR_X, uno::Reference< beans::XPropertySet >());
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_ERROR_BAR_Y, uno::Reference< beans::XPropertySet >());
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_PERCENT_DIAGONAL, sal_Int16(0) );

    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_TEXT_ROTATION, 0.0 );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_INVERT_NEGATIVE, false );

    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_LINK_NUMBERFORMAT_TO_SOURCE, true);

    // data label
    PropertyHelper::setPropertyValueDefault(
        rOutMap, PROP_DATAPOINT_LABEL,
        chart2::DataPointLabel(
            false, // ShowNumber
            false, // ShowNumberInPercent
            false, // ShowCategoryName
            false, // ShowLegendSymbol
            false, // ShowCustomLabel
            false // ShowSeriesName
            ));

    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_TEXT_WORD_WRAP, false );
    PropertyHelper::setPropertyValueDefault( rOutMap, PROP_DATAPOINT_LABEL_SEPARATOR, u" "_ustr );
    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_STYLE, drawing::LineStyle_NONE);
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_COLOR);
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_FILL_STYLE);
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_FILL_COLOR);
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_FILL_BACKGROUND);
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_FILL_HATCH_NAME);
    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_WIDTH, sal_Int32(0));
    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_DASH, drawing::LineDash());
    PropertyHelper::setEmptyPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_DASH_NAME);
    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_LABEL_BORDER_TRANS, sal_Int16(0));

    uno::Sequence<uno::Reference<chart2::XDataPointCustomLabelField>> aFields(0);
    PropertyHelper::setPropertyValueDefault(rOutMap, PROP_DATAPOINT_CUSTOM_LABEL_FIELDS, aFields);
}

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

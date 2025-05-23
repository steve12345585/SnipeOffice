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

#include <utility>
#include <xlchart.hxx>

#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/awt/Gradient.hpp>
#include <com/sun/star/drawing/Hatch.hpp>
#include <com/sun/star/drawing/LineDash.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/drawing/BitmapMode.hpp>
#include <com/sun/star/chart/DataLabelPlacement.hpp>
#include <com/sun/star/chart/XAxisXSupplier.hpp>
#include <com/sun/star/chart/XAxisYSupplier.hpp>
#include <com/sun/star/chart/XAxisZSupplier.hpp>
#include <com/sun/star/chart/XChartDocument.hpp>
#include <com/sun/star/chart/XSecondAxisTitleSupplier.hpp>
#include <com/sun/star/chart2/Symbol.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <o3tl/string_view.hxx>
#include <sal/macros.h>
#include <sal/mathconf.h>
#include <svl/itemset.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xflclit.hxx>
#include <svx/xfltrit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/unomid.hxx>
#include <filter/msfilter/escherex.hxx>
#include <xlroot.hxx>
#include <xlstyle.hxx>
#include <xltools.hxx>

using namespace css;

// Common =====================================================================

XclChRectangle::XclChRectangle() :
    mnX( 0 ),
    mnY( 0 ),
    mnWidth( 0 ),
    mnHeight( 0 )
{
}

XclChDataPointPos::XclChDataPointPos( sal_uInt16 nSeriesIdx, sal_uInt16 nPointIdx ) :
    mnSeriesIdx( nSeriesIdx ),
    mnPointIdx( nPointIdx )
{
}

bool operator<( const XclChDataPointPos& rL, const XclChDataPointPos& rR )
{
    return (rL.mnSeriesIdx < rR.mnSeriesIdx) ||
        ((rL.mnSeriesIdx == rR.mnSeriesIdx) && (rL.mnPointIdx < rR.mnPointIdx));
}

XclChFrBlock::XclChFrBlock( sal_uInt16 nType ) :
    mnType( nType ),
    mnContext( 0 ),
    mnValue1( 0 ),
    mnValue2( 0 )
{
}

// Frame formatting ===========================================================

XclChFramePos::XclChFramePos() :
    mnTLMode( EXC_CHFRAMEPOS_PARENT ),
    mnBRMode( EXC_CHFRAMEPOS_PARENT )
{
}

XclChLineFormat::XclChLineFormat() :
    maColor( COL_BLACK ),
    mnPattern( EXC_CHLINEFORMAT_SOLID ),
    mnWeight( EXC_CHLINEFORMAT_SINGLE ),
    mnFlags( EXC_CHLINEFORMAT_AUTO )
{
}

XclChAreaFormat::XclChAreaFormat() :
    maPattColor( COL_WHITE ),
    maBackColor( COL_BLACK ),
    mnPattern( EXC_PATT_SOLID ),
    mnFlags( EXC_CHAREAFORMAT_AUTO )
{
}

XclChEscherFormat::XclChEscherFormat()
{
}

XclChEscherFormat::~XclChEscherFormat()
{
}

XclChPicFormat::XclChPicFormat() :
    mnBmpMode( EXC_CHPICFORMAT_NONE ),
    mnFlags( EXC_CHPICFORMAT_TOPBOTTOM | EXC_CHPICFORMAT_FRONTBACK | EXC_CHPICFORMAT_LEFTRIGHT ),
    mfScale( 0.5 )
{
}

XclChFrame::XclChFrame() :
    mnFormat( EXC_CHFRAME_STANDARD ),
    mnFlags( EXC_CHFRAME_AUTOSIZE | EXC_CHFRAME_AUTOPOS )
{
}

// Source links ===============================================================

XclChSourceLink::XclChSourceLink() :
    mnDestType( EXC_CHSRCLINK_TITLE ),
    mnLinkType( EXC_CHSRCLINK_DEFAULT ),
    mnFlags( 0 ),
    mnNumFmtIdx( 0 )
{
}

// Text =======================================================================

XclChObjectLink::XclChObjectLink() :
    mnTarget( EXC_CHOBJLINK_NONE )
{
}

XclChFrLabelProps::XclChFrLabelProps() :
    mnFlags( 0 )
{
}

XclChText::XclChText() :
    mnHAlign( EXC_CHTEXT_ALIGN_CENTER ),
    mnVAlign( EXC_CHTEXT_ALIGN_CENTER ),
    mnBackMode( EXC_CHTEXT_TRANSPARENT ),
    mnFlags( EXC_CHTEXT_AUTOCOLOR | EXC_CHTEXT_AUTOFILL ),
    mnFlags2( EXC_CHTEXT_POS_DEFAULT ),
    mnRotation( EXC_ROT_NONE )
{
    maTextComplexColor.setColor(COL_BLACK);
}

// Data series ================================================================

XclChMarkerFormat::XclChMarkerFormat() :
    maLineColor( COL_BLACK ),
    maFillColor( COL_WHITE ),
    mnMarkerSize( EXC_CHMARKERFORMAT_SINGLESIZE ),
    mnMarkerType( EXC_CHMARKERFORMAT_NOSYMBOL ),
    mnFlags( EXC_CHMARKERFORMAT_AUTO )
{
};

XclCh3dDataFormat::XclCh3dDataFormat() :
    mnBase( EXC_CH3DDATAFORMAT_RECT ),
    mnTop( EXC_CH3DDATAFORMAT_STRAIGHT )
{
}

XclChDataFormat::XclChDataFormat() :
    mnFormatIdx( EXC_CHDATAFORMAT_DEFAULT ),
    mnFlags( 0 )
{
}

XclChSerTrendLine::XclChSerTrendLine() :
    mfForecastFor( 0.0 ),
    mfForecastBack( 0.0 ),
    mnLineType( EXC_CHSERTREND_POLYNOMIAL ),
    mnOrder( 1 ),
    mnShowEquation( 0 ),
    mnShowRSquared( 0 )
{
    /*  Set all bits in mfIntercept to 1 (that is -1.#NAN) to indicate that
        there is no interception point. Cannot use ::rtl::math::setNan() here
        cause it misses the sign bit. */
    reinterpret_cast<sal_math_Double*>(&mfIntercept)->intrep = 0xFFFFFFFFFFFFFFFF;
}

XclChSerErrorBar::XclChSerErrorBar() :
    mfValue( 0.0 ),
    mnValueCount( 1 ),
    mnBarType( EXC_CHSERERR_NONE ),
    mnSourceType( EXC_CHSERERR_FIXED ),
    mnLineEnd( EXC_CHSERERR_END_TSHAPE )
{
}

XclChSeries::XclChSeries() :
    mnCategType( EXC_CHSERIES_NUMERIC ),
    mnValueType( EXC_CHSERIES_NUMERIC ),
    mnBubbleType( EXC_CHSERIES_NUMERIC ),
    mnCategCount( 0 ),
    mnValueCount( 0 ),
    mnBubbleCount( 0 )
{
}

// Chart type groups ==========================================================

XclChType::XclChType() :
    mnOverlap( 0 ),
    mnGap( 150 ),
    mnRotation( 0 ),
    mnPieHole( 0 ),
    mnBubbleSize( 100 ),
    mnBubbleType( EXC_CHSCATTER_AREA ),
    mnFlags( 0 )
{
}

XclChChart3d::XclChChart3d() :
    mnRotation( 20 ),
    mnElevation( 15 ),
    mnEyeDist( 30 ),
    mnRelHeight( 100 ),
    mnRelDepth( 100 ),
    mnDepthGap( 150 ),
    mnFlags( EXC_CHCHART3D_AUTOHEIGHT )
{
}

XclChLegend::XclChLegend() :
    mnDockMode( EXC_CHLEGEND_RIGHT ),
    mnSpacing( EXC_CHLEGEND_MEDIUM ),
    mnFlags( EXC_CHLEGEND_DOCKED | EXC_CHLEGEND_AUTOSERIES |
        EXC_CHLEGEND_AUTOPOSX | EXC_CHLEGEND_AUTOPOSY | EXC_CHLEGEND_STACKED )
{
}

XclChTypeGroup::XclChTypeGroup() :
    mnFlags( 0 ),
    mnGroupIdx( EXC_CHSERGROUP_NONE )
{
}

XclChProperties::XclChProperties() :
    mnFlags( 0 ),
    mnEmptyMode( EXC_CHPROPS_EMPTY_SKIP )
{
}

// Axes =======================================================================

XclChLabelRange::XclChLabelRange() :
    mnCross( 1 ),
    mnLabelFreq( 1 ),
    mnTickFreq( 1 ),
    mnFlags( 0 )
{
}

XclChDateRange::XclChDateRange() :
    mnMinDate( 0 ),
    mnMaxDate( 0 ),
    mnMajorStep( 0 ),
    mnMajorUnit( EXC_CHDATERANGE_DAYS ),
    mnMinorStep( 0 ),
    mnMinorUnit( EXC_CHDATERANGE_DAYS ),
    mnBaseUnit( EXC_CHDATERANGE_DAYS ),
    mnCross( 0 ),
    mnFlags( EXC_CHDATERANGE_AUTOMIN | EXC_CHDATERANGE_AUTOMAX |
        EXC_CHDATERANGE_AUTOMAJOR | EXC_CHDATERANGE_AUTOMINOR |
        EXC_CHDATERANGE_AUTOBASE | EXC_CHDATERANGE_AUTOCROSS |
        EXC_CHDATERANGE_AUTODATE )
{
}

XclChValueRange::XclChValueRange() :
    mfMin( 0.0 ),
    mfMax( 0.0 ),
    mfMajorStep( 0.0 ),
    mfMinorStep( 0.0 ),
    mfCross( 0.0 ),
    mnFlags( EXC_CHVALUERANGE_AUTOMIN | EXC_CHVALUERANGE_AUTOMAX |
        EXC_CHVALUERANGE_AUTOMAJOR | EXC_CHVALUERANGE_AUTOMINOR |
        EXC_CHVALUERANGE_AUTOCROSS | EXC_CHVALUERANGE_BIT8 )
{
}

XclChTick::XclChTick() :
    mnMajor( EXC_CHTICK_INSIDE | EXC_CHTICK_OUTSIDE ),
    mnMinor( 0 ),
    mnLabelPos( EXC_CHTICK_NEXT ),
    mnBackMode( EXC_CHTICK_TRANSPARENT ),
    mnFlags( EXC_CHTICK_AUTOCOLOR | EXC_CHTICK_AUTOROT ),
    mnRotation( EXC_ROT_NONE )
{
    maTextComplexColor.setColor(COL_BLACK);
}

XclChAxis::XclChAxis() :
    mnType( EXC_CHAXIS_NONE )
{
}

sal_Int32 XclChAxis::GetApiAxisDimension() const
{
    sal_Int32 nApiAxisDim = EXC_CHART_AXIS_NONE;
    switch( mnType )
    {
        case EXC_CHAXIS_X:  nApiAxisDim = EXC_CHART_AXIS_X; break;
        case EXC_CHAXIS_Y:  nApiAxisDim = EXC_CHART_AXIS_Y; break;
        case EXC_CHAXIS_Z:  nApiAxisDim = EXC_CHART_AXIS_Z; break;
    }
    return nApiAxisDim;
}

XclChAxesSet::XclChAxesSet() :
    mnAxesSetId( EXC_CHAXESSET_PRIMARY )
{
}

sal_Int32 XclChAxesSet::GetApiAxesSetIndex() const
{
    sal_Int32 nApiAxesSetIdx = EXC_CHART_AXESSET_NONE;
    switch( mnAxesSetId )
    {
        case EXC_CHAXESSET_PRIMARY:     nApiAxesSetIdx = EXC_CHART_AXESSET_PRIMARY;     break;
        case EXC_CHAXESSET_SECONDARY:   nApiAxesSetIdx = EXC_CHART_AXESSET_SECONDARY;   break;
    }
    return nApiAxesSetIdx;
}

// Static helper functions ====================================================

sal_uInt16 XclChartHelper::GetSeriesLineAutoColorIdx( sal_uInt16 nFormatIdx )
{
    static const sal_uInt16 spnLineColors[] =
    {
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62,  8,
         9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 63
    };
    return spnLineColors[ nFormatIdx % SAL_N_ELEMENTS( spnLineColors ) ];
}

sal_uInt16 XclChartHelper::GetSeriesFillAutoColorIdx( sal_uInt16 nFormatIdx )
{
    static const sal_uInt16 spnFillColors[] =
    {
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63,
         8,  9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23
    };
    return spnFillColors[ nFormatIdx % SAL_N_ELEMENTS( spnFillColors ) ];
}

sal_uInt8 XclChartHelper::GetSeriesFillAutoTransp( sal_uInt16 nFormatIdx )
{
    static const sal_uInt8 spnTrans[] = { 0x00, 0x40, 0x20, 0x60, 0x70 };
    return spnTrans[ (nFormatIdx / 56) % SAL_N_ELEMENTS( spnTrans ) ];
}

sal_uInt16 XclChartHelper::GetAutoMarkerType( sal_uInt16 nFormatIdx )
{
    static const sal_uInt16 spnSymbols[] = {
        EXC_CHMARKERFORMAT_DIAMOND, EXC_CHMARKERFORMAT_SQUARE, EXC_CHMARKERFORMAT_TRIANGLE,
        EXC_CHMARKERFORMAT_CROSS, EXC_CHMARKERFORMAT_STAR, EXC_CHMARKERFORMAT_CIRCLE,
        EXC_CHMARKERFORMAT_PLUS, EXC_CHMARKERFORMAT_DOWJ, EXC_CHMARKERFORMAT_STDDEV };
    return spnSymbols[ nFormatIdx % SAL_N_ELEMENTS( spnSymbols ) ];
}

bool XclChartHelper::HasMarkerFillColor( sal_uInt16 nMarkerType )
{
    static const bool spbFilled[] = {
        false, true, true, true, false, false, false, false, true, false };
    return (nMarkerType < SAL_N_ELEMENTS( spbFilled )) && spbFilled[ nMarkerType ];
}

const OUString & XclChartHelper::GetErrorBarValuesRole( sal_uInt8 nBarType )
{
    switch( nBarType )
    {
        case EXC_CHSERERR_XPLUS:    return EXC_CHPROP_ROLE_ERRORBARS_POSX;
        case EXC_CHSERERR_XMINUS:   return EXC_CHPROP_ROLE_ERRORBARS_NEGX;
        case EXC_CHSERERR_YPLUS:    return EXC_CHPROP_ROLE_ERRORBARS_POSY;
        case EXC_CHSERERR_YMINUS:   return EXC_CHPROP_ROLE_ERRORBARS_NEGY;
        default:    OSL_FAIL( "XclChartHelper::GetErrorBarValuesRole - unknown bar type" );
    }
    return EMPTY_OUSTRING;
}

// Chart formatting info provider =============================================

namespace {

const XclChFormatInfo spFmtInfos[] =
{
    // object type                  property mode                auto line color         auto line weight         auto pattern color      missing frame type         create delete isframe
    { EXC_CHOBJTYPE_BACKGROUND,     EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, true,  true,  true  },
    { EXC_CHOBJTYPE_PLOTFRAME,      EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, true,  true,  true  },
    { EXC_CHOBJTYPE_WALL3D,         EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_AUTO,      true,  false, true  },
    { EXC_CHOBJTYPE_FLOOR3D,        EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   23,                     EXC_CHFRAMETYPE_AUTO,      true,  false, true  },
    { EXC_CHOBJTYPE_TEXT,           EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, true,  true  },
    { EXC_CHOBJTYPE_LEGEND,         EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_AUTO,      true,  true,  true  },
    { EXC_CHOBJTYPE_LINEARSERIES,   EXC_CHPROPMODE_LINEARSERIES, 0xFFFF,                 EXC_CHLINEFORMAT_SINGLE, EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_AUTO,      false, false, false },
    { EXC_CHOBJTYPE_FILLEDSERIES,   EXC_CHPROPMODE_FILLEDSERIES, EXC_COLOR_CHBORDERAUTO, EXC_CHLINEFORMAT_SINGLE, 0xFFFF,                 EXC_CHFRAMETYPE_AUTO,      false, false, true  },
    { EXC_CHOBJTYPE_AXISLINE,       EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_AUTO,      false, false, false },
    { EXC_CHOBJTYPE_GRIDLINE,       EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, true,  false  },
    { EXC_CHOBJTYPE_TRENDLINE,      EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_DOUBLE, EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, false, false },
    { EXC_CHOBJTYPE_ERRORBAR,       EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_SINGLE, EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, false, false },
    { EXC_CHOBJTYPE_CONNECTLINE,    EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, false, false },
    { EXC_CHOBJTYPE_HILOLINE,       EXC_CHPROPMODE_LINEARSERIES, EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, false, false },
    { EXC_CHOBJTYPE_WHITEDROPBAR,   EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWBACK, EXC_CHFRAMETYPE_INVISIBLE, false, false, true  },
    { EXC_CHOBJTYPE_BLACKDROPBAR,   EXC_CHPROPMODE_COMMON,       EXC_COLOR_CHWINDOWTEXT, EXC_CHLINEFORMAT_HAIR,   EXC_COLOR_CHWINDOWTEXT, EXC_CHFRAMETYPE_INVISIBLE, false, false, true  }
};

}

XclChFormatInfoProvider::XclChFormatInfoProvider()
{
    for(auto const &rIt : spFmtInfos)
        maInfoMap[ rIt.meObjType ] = &rIt;
}

const XclChFormatInfo& XclChFormatInfoProvider::GetFormatInfo( XclChObjectType eObjType ) const
{
    XclFmtInfoMap::const_iterator aIt = maInfoMap.find( eObjType );
    OSL_ENSURE( aIt != maInfoMap.end(), "XclChFormatInfoProvider::GetFormatInfo - unknown object type" );
    return (aIt == maInfoMap.end()) ? *spFmtInfos : *aIt->second;
}

// Chart type info provider ===================================================

namespace {

// chart type service names
const char SERVICE_CHART2_AREA[]      = "com.sun.star.chart2.AreaChartType";
const char SERVICE_CHART2_CANDLE[]    = "com.sun.star.chart2.CandleStickChartType";
const char SERVICE_CHART2_COLUMN[]    = "com.sun.star.chart2.ColumnChartType";
const char SERVICE_CHART2_LINE[]      = "com.sun.star.chart2.LineChartType";
const char SERVICE_CHART2_NET[]       = "com.sun.star.chart2.NetChartType";
const char SERVICE_CHART2_FILLEDNET[] = "com.sun.star.chart2.FilledNetChartType";
const char SERVICE_CHART2_PIE[]       = "com.sun.star.chart2.PieChartType";
const char SERVICE_CHART2_SCATTER[]   = "com.sun.star.chart2.ScatterChartType";
const char SERVICE_CHART2_BUBBLE[]    = "com.sun.star.chart2.BubbleChartType";
const char SERVICE_CHART2_SURFACE[]   = "com.sun.star.chart2.ColumnChartType";    // Todo

namespace csscd = css::chart::DataLabelPlacement;

const XclChTypeInfo spTypeInfos[] =
{
    // chart type             chart type category      record id           service                   varied point color     def label pos         comb2d 3d     polar  area2d area3d 1stvis xcateg swap   stack  revers betw
    { EXC_CHTYPEID_BAR,       EXC_CHTYPECATEG_BAR,     EXC_ID_CHBAR,       SERVICE_CHART2_COLUMN,    EXC_CHVARPOINT_SINGLE, csscd::OUTSIDE,       true,  true,  false, true,  true,  false, true,  false, true,  false, true  },
    { EXC_CHTYPEID_HORBAR,    EXC_CHTYPECATEG_BAR,     EXC_ID_CHBAR,       SERVICE_CHART2_COLUMN,    EXC_CHVARPOINT_SINGLE, csscd::OUTSIDE,       false, true,  false, true,  true,  false, true,  true,  true,  false, true  },
    { EXC_CHTYPEID_LINE,      EXC_CHTYPECATEG_LINE,    EXC_ID_CHLINE,      SERVICE_CHART2_LINE,      EXC_CHVARPOINT_SINGLE, csscd::RIGHT,         true,  true,  false, false, true,  false, true,  false, true,  false, false },
    { EXC_CHTYPEID_AREA,      EXC_CHTYPECATEG_LINE,    EXC_ID_CHAREA,      SERVICE_CHART2_AREA,      EXC_CHVARPOINT_NONE,   csscd::CENTER,        true,  true,  false, true,  true,  false, true,  false, true,  true,  false },
    { EXC_CHTYPEID_STOCK,     EXC_CHTYPECATEG_LINE,    EXC_ID_CHLINE,      SERVICE_CHART2_CANDLE,    EXC_CHVARPOINT_NONE,   csscd::RIGHT,         true,  false, false, false, false, false, true,  false, true,  false, false },
    { EXC_CHTYPEID_RADARLINE, EXC_CHTYPECATEG_RADAR,   EXC_ID_CHRADARLINE, SERVICE_CHART2_NET,       EXC_CHVARPOINT_SINGLE, csscd::TOP,           false, false, true,  false, true,  false, true,  false, false, false, false },
    { EXC_CHTYPEID_RADARAREA, EXC_CHTYPECATEG_RADAR,   EXC_ID_CHRADARAREA, SERVICE_CHART2_FILLEDNET, EXC_CHVARPOINT_NONE,   csscd::TOP,           false, false, true,  true,  true,  false, true,  false, false, true,  false },
    { EXC_CHTYPEID_PIE,       EXC_CHTYPECATEG_PIE,     EXC_ID_CHPIE,       SERVICE_CHART2_PIE,       EXC_CHVARPOINT_MULTI,  csscd::AVOID_OVERLAP, false, true,  true,  true,  true,  true,  true,  false, false, false, false },
    { EXC_CHTYPEID_DONUT,     EXC_CHTYPECATEG_PIE,     EXC_ID_CHPIE,       SERVICE_CHART2_PIE,       EXC_CHVARPOINT_MULTI,  csscd::AVOID_OVERLAP, false, true,  true,  true,  true,  false, true,  false, false, false, false },
    { EXC_CHTYPEID_PIEEXT,    EXC_CHTYPECATEG_PIE,     EXC_ID_CHPIEEXT,    SERVICE_CHART2_PIE,       EXC_CHVARPOINT_MULTI,  csscd::AVOID_OVERLAP, false, false, true,  true,  true,  true,  true,  false, false, false, false },
    { EXC_CHTYPEID_SCATTER,   EXC_CHTYPECATEG_SCATTER, EXC_ID_CHSCATTER,   SERVICE_CHART2_SCATTER,   EXC_CHVARPOINT_SINGLE, csscd::RIGHT,         true,  false, false, false, true,  false, false, false, false, false, false },
    { EXC_CHTYPEID_BUBBLES,   EXC_CHTYPECATEG_SCATTER, EXC_ID_CHSCATTER,   SERVICE_CHART2_BUBBLE,    EXC_CHVARPOINT_SINGLE, csscd::RIGHT,         false, false, false, true,  true,  false, false, false, false, false, false },
    { EXC_CHTYPEID_SURFACE,   EXC_CHTYPECATEG_SURFACE, EXC_ID_CHSURFACE,   SERVICE_CHART2_SURFACE,   EXC_CHVARPOINT_NONE,   csscd::RIGHT,         false, true,  false, true,  true,  false, true,  false, false, false, false },
    { EXC_CHTYPEID_UNKNOWN,   EXC_CHTYPECATEG_BAR,     EXC_ID_CHBAR,       SERVICE_CHART2_COLUMN,    EXC_CHVARPOINT_SINGLE, csscd::OUTSIDE,       true,  true,  false, true,  true,  false, true,  false, true,  false, true  }
};

} // namespace

XclChExtTypeInfo::XclChExtTypeInfo( const XclChTypeInfo& rTypeInfo ) :
    XclChTypeInfo( rTypeInfo ),
    mb3dChart( false ),
    mbSpline( false )
{
}

void XclChExtTypeInfo::Set( const XclChTypeInfo& rTypeInfo, bool b3dChart, bool bSpline )
{
    static_cast< XclChTypeInfo& >( *this ) = rTypeInfo;
    mb3dChart = mbSupports3d && b3dChart;
    mbSpline = bSpline;
}

XclChTypeInfoProvider::XclChTypeInfoProvider()
{
    for(const auto &rIt : spTypeInfos)
        maInfoMap[ rIt.meTypeId ] = &rIt;
}

const XclChTypeInfo& XclChTypeInfoProvider::GetTypeInfo( XclChTypeId eTypeId ) const
{
    XclChTypeInfoMap::const_iterator aIt = maInfoMap.find( eTypeId );
    OSL_ENSURE( aIt != maInfoMap.end(), "XclChTypeInfoProvider::GetTypeInfo - unknown chart type" );
    return (aIt == maInfoMap.end()) ? *maInfoMap.rbegin()->second : *aIt->second;
}

const XclChTypeInfo& XclChTypeInfoProvider::GetTypeInfoFromRecId( sal_uInt16 nRecId ) const
{
    for(const auto &rIt : spTypeInfos)
    {
        if(rIt.mnRecId == nRecId)
            return rIt;
    }
    OSL_FAIL( "XclChTypeInfoProvider::GetTypeInfoFromRecId - unknown record id" );
    return GetTypeInfo( EXC_CHTYPEID_UNKNOWN );
}

const XclChTypeInfo& XclChTypeInfoProvider::GetTypeInfoFromService( std::u16string_view rServiceName ) const
{
    for(auto const &rIt : spTypeInfos)
        if( o3tl::equalsAscii( rServiceName, rIt.mpcServiceName ) )
            return rIt;
    OSL_FAIL( "XclChTypeInfoProvider::GetTypeInfoFromService - unknown service name" );
    return GetTypeInfo( EXC_CHTYPEID_UNKNOWN );
}

// Property helpers ===========================================================

XclChObjectTable::XclChObjectTable(uno::Reference<lang::XMultiServiceFactory> xFactory,
        OUString aServiceName, OUString aObjNameBase ) :
    mxFactory(std::move( xFactory )),
    maServiceName(std::move( aServiceName )),
    maObjNameBase(std::move( aObjNameBase )),
    mnIndex( 0 )
{
}

uno::Any XclChObjectTable::GetObject( const OUString& rObjName )
{
    // get object table
    if( !mxContainer.is() )
        mxContainer.set(ScfApiHelper::CreateInstance( mxFactory, maServiceName ), uno::UNO_QUERY);
    OSL_ENSURE( mxContainer.is(), "XclChObjectTable::GetObject - container not found" );

    uno::Any aObj;
    if( mxContainer.is() )
    {
        // get object from container
        try
        {
            aObj = mxContainer->getByName( rObjName );
        }
        catch (uno::Exception &)
        {
            OSL_FAIL( "XclChObjectTable::GetObject - object not found" );
        }
    }
    return aObj;
}

OUString XclChObjectTable::InsertObject(const uno::Any& rObj)
{

    // create object table
    if( !mxContainer.is() )
        mxContainer.set(ScfApiHelper::CreateInstance( mxFactory, maServiceName ), uno::UNO_QUERY);
    OSL_ENSURE( mxContainer.is(), "XclChObjectTable::InsertObject - container not found" );

    OUString aObjName;
    if( mxContainer.is() )
    {
        // create new unused identifier
        do
        {
            aObjName = maObjNameBase + OUString::number( ++mnIndex );
        }
        while( mxContainer->hasByName( aObjName ) );

        // insert object
        try
        {
            mxContainer->insertByName( aObjName, rObj );
        }
        catch (uno::Exception &)
        {
            OSL_FAIL( "XclChObjectTable::InsertObject - cannot insert object" );
            aObjName.clear();
        }
    }
    return aObjName;
}

// Property names -------------------------------------------------------------

namespace {

/** Property names for line style in common objects. */
const char* const sppcLineNamesCommon[] =
    { "LineStyle", "LineWidth", "LineColor", "LineTransparence", "LineDashName", nullptr };
/** Property names for line style in linear series objects. */
const char* const sppcLineNamesLinear[] =
    { "LineStyle", "LineWidth", "Color", "Transparency", "LineDashName", nullptr };
/** Property names for line style in filled series objects. */
const char* const sppcLineNamesFilled[] =
    { "BorderStyle", "BorderWidth", "BorderColor", "BorderTransparency", "BorderDashName", nullptr };

/** Property names for solid area style in common objects. */
const char* const sppcAreaNamesCommon[] = { "FillStyle", "FillColor", "FillTransparence", nullptr };
/** Property names for solid area style in filled series objects. */
const char* const sppcAreaNamesFilled[] = { "FillStyle", "Color", "Transparency", nullptr };
/** Property names for gradient area style in common objects. */
const char* const sppcGradNamesCommon[] = {  "FillStyle", "FillGradientName", nullptr };
/** Property names for gradient area style in filled series objects. */
const char* const sppcGradNamesFilled[] = {  "FillStyle", "GradientName", nullptr };
/** Property names for hatch area style in common objects. */
const char* const sppcHatchNamesCommon[] = { "FillStyle", "FillHatchName", "FillColor", "FillBackground", nullptr };
/** Property names for hatch area style in filled series objects. */
const char* const sppcHatchNamesFilled[] = { "FillStyle", "HatchName", "Color", "FillBackground", nullptr };
/** Property names for bitmap area style. */
const char* const sppcBitmapNames[] = { "FillStyle", "FillBitmapName", "FillBitmapMode", nullptr };

} // namespace

XclChPropSetHelper::XclChPropSetHelper() :
    maLineHlpCommon( sppcLineNamesCommon ),
    maLineHlpLinear( sppcLineNamesLinear ),
    maLineHlpFilled( sppcLineNamesFilled ),
    maAreaHlpCommon( sppcAreaNamesCommon ),
    maAreaHlpFilled( sppcAreaNamesFilled ),
    maGradHlpCommon( sppcGradNamesCommon ),
    maGradHlpFilled( sppcGradNamesFilled ),
    maHatchHlpCommon( sppcHatchNamesCommon ),
    maHatchHlpFilled( sppcHatchNamesFilled ),
    maBitmapHlp( sppcBitmapNames )
{
}

// read properties ------------------------------------------------------------

void XclChPropSetHelper::ReadLineProperties(
        XclChLineFormat& rLineFmt, XclChObjectTable& rDashTable,
        const ScfPropertySet& rPropSet, XclChPropertyMode ePropMode )
{
    // read properties from property set
    drawing::LineStyle eApiStyle = drawing::LineStyle_NONE;
    sal_Int32 nApiWidth = 0;
    sal_Int16 nApiTrans = 0;
    uno::Any aDashNameAny;

    ScfPropSetHelper& rLineHlp = GetLineHelper( ePropMode );
    rLineHlp.ReadFromPropertySet( rPropSet );
    rLineHlp >> eApiStyle >> nApiWidth >> rLineFmt.maColor >> nApiTrans >> aDashNameAny;

    // clear automatic flag
    ::set_flag( rLineFmt.mnFlags, EXC_CHLINEFORMAT_AUTO, false );

    // line width
    if( nApiWidth <= 0 )        rLineFmt.mnWeight = EXC_CHLINEFORMAT_HAIR;
    else if( nApiWidth <= 35 )  rLineFmt.mnWeight = EXC_CHLINEFORMAT_SINGLE;
    else if( nApiWidth <= 70 )  rLineFmt.mnWeight = EXC_CHLINEFORMAT_DOUBLE;
    else                        rLineFmt.mnWeight = EXC_CHLINEFORMAT_TRIPLE;

    // line style
    switch( eApiStyle )
    {
        case drawing::LineStyle_NONE:
            rLineFmt.mnPattern = EXC_CHLINEFORMAT_NONE;
        break;
        case drawing::LineStyle_SOLID:
        {
            if( nApiTrans < 13 )        rLineFmt.mnPattern = EXC_CHLINEFORMAT_SOLID;
            else if( nApiTrans < 38 )   rLineFmt.mnPattern = EXC_CHLINEFORMAT_DARKTRANS;
            else if( nApiTrans < 63 )   rLineFmt.mnPattern = EXC_CHLINEFORMAT_MEDTRANS;
            else if( nApiTrans < 100 )  rLineFmt.mnPattern = EXC_CHLINEFORMAT_LIGHTTRANS;
            else                        rLineFmt.mnPattern = EXC_CHLINEFORMAT_NONE;
        }
        break;
        case drawing::LineStyle_DASH:
        {
            rLineFmt.mnPattern = EXC_CHLINEFORMAT_SOLID;
            OUString aDashName;
            drawing::LineDash aApiDash;
            if( (aDashNameAny >>= aDashName) && (rDashTable.GetObject( aDashName ) >>= aApiDash) )
            {
                // reorder dashes that are shorter than dots
                if( (aApiDash.Dashes == 0) || (aApiDash.DashLen < aApiDash.DotLen) )
                {
                    ::std::swap( aApiDash.Dashes, aApiDash.Dots );
                    ::std::swap( aApiDash.DashLen, aApiDash.DotLen );
                }
                // ignore dots that are nearly equal to dashes
                if( aApiDash.DotLen * 3 > aApiDash.DashLen * 2 )
                    aApiDash.Dots = 0;

                // convert line dash to predefined Excel dash types
                if( (aApiDash.Dashes == 1) && (aApiDash.Dots >= 1) )
                    // one dash and one or more dots
                    rLineFmt.mnPattern = (aApiDash.Dots == 1) ?
                        EXC_CHLINEFORMAT_DASHDOT : EXC_CHLINEFORMAT_DASHDOTDOT;
                else if( aApiDash.Dashes >= 1 )
                    // one or more dashes and no dots (also: dash-dash-dot)
                    rLineFmt.mnPattern = (aApiDash.DashLen < 250) ?
                        EXC_CHLINEFORMAT_DOT : EXC_CHLINEFORMAT_DASH;
            }
        }
        break;
        default:
            OSL_FAIL( "XclChPropSetHelper::ReadLineProperties - unknown line style" );
            rLineFmt.mnPattern = EXC_CHLINEFORMAT_SOLID;
    }
}

bool XclChPropSetHelper::ReadAreaProperties( XclChAreaFormat& rAreaFmt,
        const ScfPropertySet& rPropSet, XclChPropertyMode ePropMode )
{
    // read properties from property set
    drawing::FillStyle eApiStyle = drawing::FillStyle_NONE;
    sal_Int16 nTransparency = 0;

    ScfPropSetHelper& rAreaHlp = GetAreaHelper( ePropMode );
    rAreaHlp.ReadFromPropertySet( rPropSet );
    rAreaHlp >> eApiStyle >> rAreaFmt.maPattColor >> nTransparency;

    // clear automatic flag
    ::set_flag( rAreaFmt.mnFlags, EXC_CHAREAFORMAT_AUTO, false );

    // set fill style transparent or solid (set solid for anything but transparent)
    rAreaFmt.mnPattern = (eApiStyle == drawing::FillStyle_NONE) ? EXC_PATT_NONE : EXC_PATT_SOLID;

    // return true to indicate complex fill (gradient, bitmap, solid transparency)
    return (eApiStyle != drawing::FillStyle_NONE) && ((eApiStyle != drawing::FillStyle_SOLID) || (nTransparency > 0));
}

void XclChPropSetHelper::ReadEscherProperties(
        XclChEscherFormat& rEscherFmt, XclChPicFormat& rPicFmt,
        XclChObjectTable& rGradientTable, XclChObjectTable& rHatchTable, XclChObjectTable& rBitmapTable,
        const ScfPropertySet& rPropSet, XclChPropertyMode ePropMode )
{
    // read style and transparency properties from property set
    drawing::FillStyle eApiStyle = drawing::FillStyle_NONE;
    Color aColor;
    sal_Int16 nTransparency = 0;

    ScfPropSetHelper& rAreaHlp = GetAreaHelper( ePropMode );
    rAreaHlp.ReadFromPropertySet( rPropSet );
    rAreaHlp >> eApiStyle >> aColor >> nTransparency;

    switch( eApiStyle )
    {
        case drawing::FillStyle_SOLID:
        {
            OSL_ENSURE( nTransparency > 0, "XclChPropSetHelper::ReadEscherProperties - unexpected solid area without transparency" );
            if( (0 < nTransparency) && (nTransparency <= 100) )
            {
                // convert to Escher properties
                sal_uInt32 nEscherColor = 0x02000000;
                ::insert_value( nEscherColor, aColor.GetBlue(), 16, 8 );
                ::insert_value( nEscherColor, aColor.GetGreen(), 8, 8 );
                ::insert_value( nEscherColor, aColor.GetRed(), 0, 8 );
                sal_uInt32 nEscherOpacity = static_cast< sal_uInt32 >( (100 - nTransparency) * 655.36 );
                rEscherFmt.mxEscherSet = std::make_shared<EscherPropertyContainer>();
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fillType, ESCHER_FillSolid );
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fillColor, nEscherColor );
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fillOpacity, nEscherOpacity );
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fillBackColor, 0x02FFFFFF );
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fillBackOpacity, 0x00010000 );
                rEscherFmt.mxEscherSet->AddOpt( ESCHER_Prop_fNoFillHitTest, 0x001F001C );
            }
        }
        break;
        case drawing::FillStyle_GRADIENT:
        {
            // extract gradient from global gradient table
            OUString aGradientName;
            ScfPropSetHelper& rGradHlp = GetGradientHelper( ePropMode );
            rGradHlp.ReadFromPropertySet( rPropSet );
            rGradHlp >> eApiStyle >> aGradientName;
            awt::Gradient aGradient;
            if( rGradientTable.GetObject( aGradientName ) >>= aGradient )
            {
                // convert to Escher properties
                rEscherFmt.mxEscherSet = std::make_shared<EscherPropertyContainer>();
                rEscherFmt.mxEscherSet->CreateGradientProperties( aGradient );
            }
        }
        break;
        case drawing::FillStyle_HATCH:
        {
            // extract hatch from global hatch table
            OUString aHatchName;
            bool bFillBackground;
            ScfPropSetHelper& rHatchHlp = GetHatchHelper( ePropMode );
            rHatchHlp.ReadFromPropertySet( rPropSet );
            rHatchHlp >> eApiStyle >> aHatchName >> aColor >> bFillBackground;
            drawing::Hatch aHatch;
            if( rHatchTable.GetObject( aHatchName ) >>= aHatch )
            {
                // convert to Escher properties
                rEscherFmt.mxEscherSet = std::make_shared<EscherPropertyContainer>();
                rEscherFmt.mxEscherSet->CreateEmbeddedHatchProperties( aHatch, aColor, bFillBackground );
                rPicFmt.mnBmpMode = EXC_CHPICFORMAT_STACK;
            }
        }
        break;
        case drawing::FillStyle_BITMAP:
        {
            // extract bitmap URL from global bitmap table
            OUString aBitmapName;
            drawing::BitmapMode eApiBmpMode;
            maBitmapHlp.ReadFromPropertySet( rPropSet );
            maBitmapHlp >> eApiStyle >> aBitmapName >> eApiBmpMode;
            uno::Reference<awt::XBitmap> xBitmap;
            if (rBitmapTable.GetObject( aBitmapName ) >>= xBitmap)
            {
                // convert to Escher properties
                rEscherFmt.mxEscherSet = std::make_shared<EscherPropertyContainer>();
                rEscherFmt.mxEscherSet->CreateEmbeddedBitmapProperties( xBitmap, eApiBmpMode );
                rPicFmt.mnBmpMode = (eApiBmpMode == drawing::BitmapMode_REPEAT) ?
                    EXC_CHPICFORMAT_STACK : EXC_CHPICFORMAT_STRETCH;
            }
        }
        break;
        default:
            OSL_FAIL( "XclChPropSetHelper::ReadEscherProperties - unknown fill style" );
    }
}

void XclChPropSetHelper::ReadMarkerProperties(
        XclChMarkerFormat& rMarkerFmt, const ScfPropertySet& rPropSet, sal_uInt16 nFormatIdx )
{
    chart2::Symbol aApiSymbol;
    if( !rPropSet.GetProperty( aApiSymbol, EXC_CHPROP_SYMBOL ) )
        return;

    // clear automatic flag
    ::set_flag( rMarkerFmt.mnFlags, EXC_CHMARKERFORMAT_AUTO, false );

    // symbol style
    switch( aApiSymbol.Style )
    {
        case chart2::SymbolStyle_NONE:
            rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_NOSYMBOL;
        break;
        case chart2::SymbolStyle_STANDARD:
            switch( aApiSymbol.StandardSymbol )
            {
                case 0:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_SQUARE;    break;  // square
                case 1:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_DIAMOND;   break;  // diamond
                case 2:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_STDDEV;    break;  // arrow down
                case 3:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_TRIANGLE;  break;  // arrow up
                case 4:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_DOWJ;      break;  // arrow right, same as import
                case 5:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_PLUS;      break;  // arrow left
                case 6:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_CROSS;     break;  // bow tie
                case 7:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_STAR;      break;  // sand glass
                case 8:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_CIRCLE;    break;  // circle new in LibO3.5
                case 9:     rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_DIAMOND;   break;  // star new in LibO3.5
                case 10:    rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_CROSS;     break;  // X new in LibO3.5
                case 11:    rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_PLUS;      break;  // plus new in LibO3.5
                case 12:    rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_STAR;      break;  // asterisk new in LibO3.5
                case 13:    rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_STDDEV;    break;  // horizontal bar new in LibO3.5
                case 14:    rMarkerFmt.mnMarkerType = EXC_CHMARKERFORMAT_STAR;      break;  // vertical bar new in LibO3.5
                default:    rMarkerFmt.mnMarkerType = XclChartHelper::GetAutoMarkerType( nFormatIdx );
            }
        break;
        default:
            rMarkerFmt.mnMarkerType = XclChartHelper::GetAutoMarkerType( nFormatIdx );
    }
    bool bHasFillColor = XclChartHelper::HasMarkerFillColor( rMarkerFmt.mnMarkerType );
    ::set_flag( rMarkerFmt.mnFlags, EXC_CHMARKERFORMAT_NOFILL, !bHasFillColor );

    // symbol size
    sal_Int32 nApiSize = (aApiSymbol.Size.Width + aApiSymbol.Size.Height + 1) / 2;
    rMarkerFmt.mnMarkerSize = XclTools::GetTwipsFromHmm( nApiSize );

    // symbol colors
    rMarkerFmt.maLineColor = Color( ColorTransparency, aApiSymbol.BorderColor );
    rMarkerFmt.maFillColor = Color( ColorTransparency, aApiSymbol.FillColor );
}

sal_uInt16 XclChPropSetHelper::ReadRotationProperties( const ScfPropertySet& rPropSet, bool bSupportsStacked )
{
    // chart2 handles rotation as double in the range [0,360)
    double fAngle = 0.0;
    rPropSet.GetProperty( fAngle, EXC_CHPROP_TEXTROTATION );
    bool bStacked = bSupportsStacked && rPropSet.GetBoolProperty( EXC_CHPROP_STACKCHARACTERS );
    return bStacked ? EXC_ROT_STACKED :
        XclTools::GetXclRotation( Degree100(static_cast< sal_Int32 >( fAngle * 100.0 + 0.5 )) );
}

// write properties -----------------------------------------------------------

void XclChPropSetHelper::WriteLineProperties(
        ScfPropertySet& rPropSet, XclChObjectTable& rDashTable,
        const XclChLineFormat& rLineFmt, XclChPropertyMode ePropMode )
{
    // line width
    sal_Int32 nApiWidth = 0;    // 0 is the width of a hair line
    switch( rLineFmt.mnWeight )
    {
        case EXC_CHLINEFORMAT_SINGLE:   nApiWidth = 35;     break;
        case EXC_CHLINEFORMAT_DOUBLE:   nApiWidth = 70;     break;
        case EXC_CHLINEFORMAT_TRIPLE:   nApiWidth = 105;    break;
    }

    // line style
    drawing::LineStyle eApiStyle = drawing::LineStyle_NONE;
    sal_Int16 nApiTrans = 0;
    sal_Int32 nDotLen = ::std::min< sal_Int32 >( rLineFmt.mnWeight + 105, 210 );
    drawing::LineDash aApiDash( drawing::DashStyle_RECT, 0, nDotLen, 0, 4 * nDotLen, nDotLen );

    switch( rLineFmt.mnPattern )
    {
        case EXC_CHLINEFORMAT_SOLID:
            eApiStyle = drawing::LineStyle_SOLID;
        break;
        case EXC_CHLINEFORMAT_DARKTRANS:
            eApiStyle = drawing::LineStyle_SOLID; nApiTrans = 25;
        break;
        case EXC_CHLINEFORMAT_MEDTRANS:
            eApiStyle = drawing::LineStyle_SOLID; nApiTrans = 50;
        break;
        case EXC_CHLINEFORMAT_LIGHTTRANS:
            eApiStyle = drawing::LineStyle_SOLID; nApiTrans = 75;
        break;
        case EXC_CHLINEFORMAT_DASH:
            eApiStyle = drawing::LineStyle_DASH; aApiDash.Dashes = 1;
        break;
        case EXC_CHLINEFORMAT_DOT:
            eApiStyle = drawing::LineStyle_DASH; aApiDash.Dots = 1;
        break;
        case EXC_CHLINEFORMAT_DASHDOT:
            eApiStyle = drawing::LineStyle_DASH; aApiDash.Dashes = aApiDash.Dots = 1;
        break;
        case EXC_CHLINEFORMAT_DASHDOTDOT:
            eApiStyle = drawing::LineStyle_DASH; aApiDash.Dashes = 1; aApiDash.Dots = 2;
        break;
    }

    // line color
    sal_Int32 nApiColor = sal_Int32( rLineFmt.maColor );

    // try to insert the dash style and receive its name
    uno::Any aDashNameAny;
    if( eApiStyle == drawing::LineStyle_DASH )
    {
        OUString aDashName = rDashTable.InsertObject( uno::Any( aApiDash ) );
        if( !aDashName.isEmpty() )
            aDashNameAny <<= aDashName;
    }

    // write the properties
    ScfPropSetHelper& rLineHlp = GetLineHelper( ePropMode );
    rLineHlp.InitializeWrite();
    rLineHlp << eApiStyle << nApiWidth << nApiColor << nApiTrans << aDashNameAny;
    rLineHlp.WriteToPropertySet( rPropSet );
}

void XclChPropSetHelper::WriteAreaProperties( ScfPropertySet& rPropSet,
        const XclChAreaFormat& rAreaFmt, XclChPropertyMode ePropMode )
{
    drawing::FillStyle eFillStyle = drawing::FillStyle_NONE;
    Color aColor;

    // fill color
    if( rAreaFmt.mnPattern != EXC_PATT_NONE )
    {
        eFillStyle = drawing::FillStyle_SOLID;
        aColor = XclTools::GetPatternColor( rAreaFmt.maPattColor, rAreaFmt.maBackColor, rAreaFmt.mnPattern );
    }

    // write the properties
    ScfPropSetHelper& rAreaHlp = GetAreaHelper( ePropMode );
    rAreaHlp.InitializeWrite();
    rAreaHlp << eFillStyle << aColor << sal_Int16(0)/*nTransparency*/;
    rAreaHlp.WriteToPropertySet( rPropSet );
}

void XclChPropSetHelper::WriteEscherProperties( ScfPropertySet& rPropSet,
        XclChObjectTable& rGradientTable, XclChObjectTable& rBitmapTable,
        const XclChEscherFormat& rEscherFmt, const XclChPicFormat* pPicFmt,
        sal_uInt32 nDffFillType, XclChPropertyMode ePropMode )
{
    if( !rEscherFmt.mxItemSet )
        return;

    const XFillStyleItem* pStyleItem = rEscherFmt.mxItemSet->GetItem<XFillStyleItem>( XATTR_FILLSTYLE, false );
    if( !pStyleItem )
        return;

    switch( pStyleItem->GetValue() )
    {
        case drawing::FillStyle_SOLID:
            // #i84812# Excel 2007 writes Escher properties for solid fill
            if( const XFillColorItem* pColorItem = rEscherFmt.mxItemSet->GetItem<XFillColorItem>( XATTR_FILLCOLOR, false ) )
            {
                // get solid transparence too
                const XFillTransparenceItem* pTranspItem = rEscherFmt.mxItemSet->GetItem<XFillTransparenceItem>( XATTR_FILLTRANSPARENCE, false );
                sal_uInt16 nTransp = pTranspItem ? pTranspItem->GetValue() : 0;
                ScfPropSetHelper& rAreaHlp = GetAreaHelper( ePropMode );
                rAreaHlp.InitializeWrite();
                rAreaHlp << drawing::FillStyle_SOLID << pColorItem->GetColorValue() << static_cast< sal_Int16 >( nTransp );
                rAreaHlp.WriteToPropertySet( rPropSet );
            }
        break;
        case drawing::FillStyle_GRADIENT:
            if( const XFillGradientItem* pGradItem = rEscherFmt.mxItemSet->GetItem<XFillGradientItem>( XATTR_FILLGRADIENT, false ) )
            {
                uno::Any aGradientAny;
                if( pGradItem->QueryValue( aGradientAny, MID_FILLGRADIENT ) )
                {
                    OUString aGradName = rGradientTable.InsertObject( aGradientAny );
                    if( !aGradName.isEmpty() )
                    {
                        ScfPropSetHelper& rGradHlp = GetGradientHelper( ePropMode );
                        rGradHlp.InitializeWrite();
                        rGradHlp << drawing::FillStyle_GRADIENT << aGradName;
                        rGradHlp.WriteToPropertySet( rPropSet );
                    }
                }
            }
        break;
        case drawing::FillStyle_BITMAP:
            if( const XFillBitmapItem* pBmpItem = rEscherFmt.mxItemSet->GetItem<XFillBitmapItem>( XATTR_FILLBITMAP, false ) )
            {
                uno::Any aBitmapAny;
                if (pBmpItem->QueryValue(aBitmapAny, MID_BITMAP))
                {
                    OUString aBmpName = rBitmapTable.InsertObject( aBitmapAny );
                    if( !aBmpName.isEmpty() )
                    {
                        /*  #i71810# Caller decides whether to use a CHPICFORMAT record for bitmap mode.
                            If not passed, detect fill mode from the DFF property 'fill-type'. */
                        bool bStretch = pPicFmt ? (pPicFmt->mnBmpMode == EXC_CHPICFORMAT_STRETCH) : (nDffFillType == mso_fillPicture);
                        drawing::BitmapMode eApiBmpMode = bStretch ? drawing::BitmapMode_STRETCH : drawing::BitmapMode_REPEAT;
                        maBitmapHlp.InitializeWrite();
                        maBitmapHlp << drawing::FillStyle_BITMAP << aBmpName << eApiBmpMode;
                        maBitmapHlp.WriteToPropertySet( rPropSet );
                    }
                }
            }
        break;
        default:
            OSL_FAIL( "XclChPropSetHelper::WriteEscherProperties - unknown fill mode" );
    }
}

void XclChPropSetHelper::WriteMarkerProperties(
        ScfPropertySet& rPropSet, const XclChMarkerFormat& rMarkerFmt )
{
    // symbol style
    chart2::Symbol aApiSymbol;
    aApiSymbol.Style = chart2::SymbolStyle_STANDARD;
    switch( rMarkerFmt.mnMarkerType )
    {
        case EXC_CHMARKERFORMAT_NOSYMBOL:   aApiSymbol.Style = chart2::SymbolStyle_NONE;  break;
        case EXC_CHMARKERFORMAT_SQUARE:     aApiSymbol.StandardSymbol = 0;              break;  // square
        case EXC_CHMARKERFORMAT_DIAMOND:    aApiSymbol.StandardSymbol = 1;              break;  // diamond
        case EXC_CHMARKERFORMAT_TRIANGLE:   aApiSymbol.StandardSymbol = 3;              break;  // arrow up
        case EXC_CHMARKERFORMAT_CROSS:      aApiSymbol.StandardSymbol = 10;             break;  // X, legacy bow tie
        case EXC_CHMARKERFORMAT_STAR:       aApiSymbol.StandardSymbol = 12;             break;  // asterisk, legacy sand glass
        case EXC_CHMARKERFORMAT_DOWJ:       aApiSymbol.StandardSymbol = 4;              break;  // arrow right, same as export
        case EXC_CHMARKERFORMAT_STDDEV:     aApiSymbol.StandardSymbol = 13;             break;  // horizontal bar, legacy arrow down
        case EXC_CHMARKERFORMAT_CIRCLE:     aApiSymbol.StandardSymbol = 8;              break;  // circle, legacy arrow right
        case EXC_CHMARKERFORMAT_PLUS:       aApiSymbol.StandardSymbol = 11;             break;  // plus, legacy arrow left
        default: break;
    }

    // symbol size
    sal_Int32 nApiSize = XclTools::GetHmmFromTwips( rMarkerFmt.mnMarkerSize );
    aApiSymbol.Size = awt::Size( nApiSize, nApiSize );

    // symbol colors
    aApiSymbol.FillColor = sal_Int32( rMarkerFmt.maFillColor );
    aApiSymbol.BorderColor = ::get_flag( rMarkerFmt.mnFlags, EXC_CHMARKERFORMAT_NOLINE ) ?
        aApiSymbol.FillColor : sal_Int32( rMarkerFmt.maLineColor );

    // set the property
    rPropSet.SetProperty( EXC_CHPROP_SYMBOL, aApiSymbol );
}

void XclChPropSetHelper::WriteRotationProperties(
        ScfPropertySet& rPropSet, sal_uInt16 nRotation, bool bSupportsStacked )
{
    if( nRotation != EXC_CHART_AUTOROTATION )
    {
        // chart2 handles rotation as double in the range [0,360)
        double fAngle = XclTools::GetScRotation( nRotation, 0_deg100 ).get() / 100.0;
        rPropSet.SetProperty( EXC_CHPROP_TEXTROTATION, fAngle );
        if( bSupportsStacked )
            rPropSet.SetProperty( EXC_CHPROP_STACKCHARACTERS, nRotation == EXC_ROT_STACKED );
    }
}

// private --------------------------------------------------------------------

ScfPropSetHelper& XclChPropSetHelper::GetLineHelper( XclChPropertyMode ePropMode )
{
    switch( ePropMode )
    {
        case EXC_CHPROPMODE_COMMON:         return maLineHlpCommon;
        case EXC_CHPROPMODE_LINEARSERIES:   return maLineHlpLinear;
        case EXC_CHPROPMODE_FILLEDSERIES:   return maLineHlpFilled;
        default: OSL_FAIL( "XclChPropSetHelper::GetLineHelper - unknown property mode" );
    }
    return maLineHlpCommon;
}

ScfPropSetHelper& XclChPropSetHelper::GetAreaHelper( XclChPropertyMode ePropMode )
{
    switch( ePropMode )
    {
        case EXC_CHPROPMODE_COMMON:         return maAreaHlpCommon;
        case EXC_CHPROPMODE_FILLEDSERIES:   return maAreaHlpFilled;
        default:    OSL_FAIL( "XclChPropSetHelper::GetAreaHelper - unknown property mode" );
    }
    return maAreaHlpCommon;
}

ScfPropSetHelper& XclChPropSetHelper::GetGradientHelper( XclChPropertyMode ePropMode )
{
    switch( ePropMode )
    {
        case EXC_CHPROPMODE_COMMON:         return maGradHlpCommon;
        case EXC_CHPROPMODE_FILLEDSERIES:   return maGradHlpFilled;
        default:    OSL_FAIL( "XclChPropSetHelper::GetGradientHelper - unknown property mode" );
    }
    return maGradHlpCommon;
}

ScfPropSetHelper& XclChPropSetHelper::GetHatchHelper( XclChPropertyMode ePropMode )
{
    switch( ePropMode )
    {
        case EXC_CHPROPMODE_COMMON:         return maHatchHlpCommon;
        case EXC_CHPROPMODE_FILLEDSERIES:   return maHatchHlpFilled;
        default:    OSL_FAIL( "XclChPropSetHelper::GetHatchHelper - unknown property mode" );
    }
    return maHatchHlpCommon;
}

namespace {

/*  The following local functions implement getting the XShape interface of all
    supported title objects (chart and axes). This needs some effort due to the
    design of the old Chart1 API used to access these objects. */

/** Returns the drawing shape of the main title, if existing. */
uno::Reference<drawing::XShape> lclGetMainTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc)
{
    ScfPropertySet aPropSet(rxChart1Doc);
    if (rxChart1Doc.is() && aPropSet.GetBoolProperty(u"HasMainTitle"_ustr))
        return rxChart1Doc->getTitle();
    return uno::Reference<drawing::XShape>();
}

uno::Reference<drawing::XShape> lclGetXAxisTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc)
{
    uno::Reference<chart::XAxisXSupplier> xAxisSupp(rxChart1Doc->getDiagram(), uno::UNO_QUERY);
    ScfPropertySet aPropSet(xAxisSupp);
    if (xAxisSupp.is() && aPropSet.GetBoolProperty(u"HasXAxisTitle"_ustr))
        return xAxisSupp->getXAxisTitle();
    return uno::Reference<drawing::XShape>();
}

uno::Reference<drawing::XShape> lclGetYAxisTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc )
{
    uno::Reference<chart::XAxisYSupplier> xAxisSupp(rxChart1Doc->getDiagram(), uno::UNO_QUERY);
    ScfPropertySet aPropSet(xAxisSupp);
    if (xAxisSupp.is() && aPropSet.GetBoolProperty(u"HasYAxisTitle"_ustr))
        return xAxisSupp->getYAxisTitle();
    return uno::Reference<drawing::XShape>();
}

uno::Reference<drawing::XShape> lclGetZAxisTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc )
{
    uno::Reference<chart::XAxisZSupplier> xAxisSupp(rxChart1Doc->getDiagram(), uno::UNO_QUERY);
    ScfPropertySet aPropSet(xAxisSupp);
    if (xAxisSupp.is() && aPropSet.GetBoolProperty(u"HasZAxisTitle"_ustr))
        return xAxisSupp->getZAxisTitle();
    return uno::Reference<drawing::XShape>();
}

uno::Reference<drawing::XShape> lclGetSecXAxisTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc)
{
    uno::Reference<chart::XSecondAxisTitleSupplier> xAxisSupp(rxChart1Doc->getDiagram(), uno::UNO_QUERY);
    ScfPropertySet aPropSet(xAxisSupp);
    if (xAxisSupp.is() && aPropSet.GetBoolProperty(u"HasSecondaryXAxisTitle"_ustr))
        return xAxisSupp->getSecondXAxisTitle();
    return uno::Reference<drawing::XShape>();
}

uno::Reference<drawing::XShape> lclGetSecYAxisTitleShape(const uno::Reference<chart::XChartDocument> & rxChart1Doc)
{
    uno::Reference<chart::XSecondAxisTitleSupplier> xAxisSupp(rxChart1Doc->getDiagram(), uno::UNO_QUERY);
    ScfPropertySet aPropSet(xAxisSupp);
    if (xAxisSupp.is() && aPropSet.GetBoolProperty(u"HasSecondaryYAxisTitle"_ustr))
        return xAxisSupp->getSecondYAxisTitle();
    return uno::Reference<drawing::XShape>();
}

} // namespace

XclChRootData::XclChRootData()
    : mxTypeInfoProv(std::make_shared<XclChTypeInfoProvider>())
    , mxFmtInfoProv(std::make_shared<XclChFormatInfoProvider>())
    , mnBorderGapX(0)
    , mnBorderGapY(0)
    , mfUnitSizeX(0.0)
    , mfUnitSizeY(0.0)
{
    // remember some title shape getter functions
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_TITLE ) ] = lclGetMainTitleShape;
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_AXISTITLE, EXC_CHAXESSET_PRIMARY, EXC_CHAXIS_X ) ] = lclGetXAxisTitleShape;
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_AXISTITLE, EXC_CHAXESSET_PRIMARY, EXC_CHAXIS_Y ) ] = lclGetYAxisTitleShape;
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_AXISTITLE, EXC_CHAXESSET_PRIMARY, EXC_CHAXIS_Z ) ] = lclGetZAxisTitleShape;
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_AXISTITLE, EXC_CHAXESSET_SECONDARY, EXC_CHAXIS_X ) ] = lclGetSecXAxisTitleShape;
    maGetShapeFuncs[ XclChTextKey( EXC_CHTEXTTYPE_AXISTITLE, EXC_CHAXESSET_SECONDARY, EXC_CHAXIS_Y ) ] = lclGetSecYAxisTitleShape;
}

XclChRootData::~XclChRootData()
{
}

void XclChRootData::InitConversion(const XclRoot& rRoot, const uno::Reference<chart2::XChartDocument> & rxChartDoc, const tools::Rectangle& rChartRect)
{
    // remember chart document reference and chart shape position/size
    OSL_ENSURE( rxChartDoc.is(), "XclChRootData::InitConversion - missing chart document" );
    mxChartDoc = rxChartDoc;
    maChartRect = rChartRect;

    // Excel excludes a border of 5 pixels in each direction from chart area
    mnBorderGapX = rRoot.GetHmmFromPixelX( 5.0 );
    mnBorderGapY = rRoot.GetHmmFromPixelY( 5.0 );

    // size of a chart unit in 1/100 mm
    mfUnitSizeX = std::max<double>( maChartRect.GetWidth()  - 2 * mnBorderGapX, mnBorderGapX ) / EXC_CHART_TOTALUNITS;
    mfUnitSizeY = std::max<double>( maChartRect.GetHeight() - 2 * mnBorderGapY, mnBorderGapY ) / EXC_CHART_TOTALUNITS;

    // create object tables
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxChartDoc, uno::UNO_QUERY);
    mxLineDashTable = std::make_shared<XclChObjectTable>(xFactory, SERVICE_DRAWING_DASHTABLE, "Excel line dash ");
    mxGradientTable = std::make_shared<XclChObjectTable>(xFactory, SERVICE_DRAWING_GRADIENTTABLE, "Excel gradient ");
    mxHatchTable = std::make_shared<XclChObjectTable>(xFactory, SERVICE_DRAWING_HATCHTABLE, "Excel hatch ");
    mxBitmapTable = std::make_shared<XclChObjectTable>(xFactory, SERVICE_DRAWING_BITMAPTABLE, "Excel bitmap ");
}

void XclChRootData::FinishConversion()
{
    // forget formatting object tables
    mxBitmapTable.reset();
    mxHatchTable.reset();
    mxGradientTable.reset();
    mxLineDashTable.reset();
    // forget chart document reference
    mxChartDoc.clear();
}

uno::Reference<drawing::XShape> XclChRootData::GetTitleShape(const XclChTextKey& rTitleKey) const
{
    XclChGetShapeFuncMap::const_iterator aIt = maGetShapeFuncs.find( rTitleKey );
    OSL_ENSURE( aIt != maGetShapeFuncs.end(), "XclChRootData::GetTitleShape - invalid title key" );
    uno::Reference<chart::XChartDocument> xChart1Doc( mxChartDoc, uno::UNO_QUERY );
    uno::Reference<drawing::XShape> xTitleShape;
    if (xChart1Doc.is() && (aIt != maGetShapeFuncs.end()))
        xTitleShape = (aIt->second)(xChart1Doc);
    return xTitleShape;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

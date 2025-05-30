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

#include "BubbleChart.hxx"
#include <PlottingPositionHelper.hxx>
#include <ShapeFactory.hxx>
#include <ObjectIdentifier.hxx>
#include <LabelPositionHelper.hxx>
#include <ChartType.hxx>

#include <com/sun/star/chart/DataLabelPlacement.hpp>
#include <sal/log.hxx>
#include <osl/diagnose.h>

#include <limits>

namespace chart
{
using namespace ::com::sun::star;

BubbleChart::BubbleChart( const rtl::Reference<ChartType>& xChartTypeModel
                     , sal_Int32 nDimensionCount )
        : VSeriesPlotter( xChartTypeModel, nDimensionCount, false )
        , m_fMaxLogicBubbleSize( 0.0 )
        , m_fBubbleSizeFactorToScreen( 1.0 )
{
    // We only support 2 dimensional bubble charts
    assert(nDimensionCount == 2);

    if( !m_pMainPosHelper )
        m_pMainPosHelper = new PlottingPositionHelper();
    PlotterBase::m_pPosHelper = m_pMainPosHelper;
}

BubbleChart::~BubbleChart()
{
    delete m_pMainPosHelper;
}

void BubbleChart::calculateMaximumLogicBubbleSize()
{
    double fMaxSize = 0.0;

    sal_Int32 nEndIndex = VSeriesPlotter::getPointCount();
    for( sal_Int32 nIndex = 0; nIndex < nEndIndex; nIndex++ )
    {
        for( auto const& rZSlot : m_aZSlots )
        {
            for( auto const& rXSlot : rZSlot )
            {
                for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
                {
                    if(!pSeries)
                        continue;

                    double fSize = pSeries->getBubble_Size( nIndex );
                    if( fSize > fMaxSize )
                        fMaxSize = fSize;
                }
            }
        }
    }

    m_fMaxLogicBubbleSize = fMaxSize;
}

void BubbleChart::calculateBubbleSizeScalingFactor()
{
    double fLogicZ=1.0;
    drawing::Position3D aSceneMinPos( m_pMainPosHelper->transformLogicToScene( m_pMainPosHelper->getLogicMinX(),m_pMainPosHelper->getLogicMinY(),fLogicZ, false ) );
    drawing::Position3D aSceneMaxPos( m_pMainPosHelper->transformLogicToScene( m_pMainPosHelper->getLogicMaxX(),m_pMainPosHelper->getLogicMaxY(),fLogicZ, false ) );

    awt::Point aScreenMinPos( LabelPositionHelper(m_nDimension,m_xLogicTarget).transformSceneToScreenPosition( aSceneMinPos ) );
    awt::Point aScreenMaxPos( LabelPositionHelper(m_nDimension,m_xLogicTarget).transformSceneToScreenPosition( aSceneMaxPos ) );

    sal_Int32 nWidth = abs( aScreenMaxPos.X - aScreenMinPos.X );
    sal_Int32 nHeight = abs( aScreenMaxPos.Y - aScreenMinPos.Y );

    sal_Int32 nMinExtend = std::min( nWidth, nHeight );
    m_fBubbleSizeFactorToScreen = nMinExtend * 0.25;//max bubble size is 25 percent of diagram size
}

drawing::Direction3D BubbleChart::transformToScreenBubbleSize( double fLogicSize )
{
    drawing::Direction3D aRet(0,0,0);

    if( std::isnan(fLogicSize) || std::isinf(fLogicSize) )
        return aRet;

    double fMaxSize = m_fMaxLogicBubbleSize;

    double fMaxRadius = sqrt( fMaxSize / M_PI );
    double fRadius = sqrt( fLogicSize / M_PI );

    aRet.DirectionX = m_fBubbleSizeFactorToScreen * fRadius / fMaxRadius;
    aRet.DirectionY = aRet.DirectionX;

    return aRet;
}

bool BubbleChart::isExpandIfValuesCloseToBorder( sal_Int32 /*nDimensionIndex*/ )
{
    return true;
}

bool BubbleChart::isSeparateStackingForDifferentSigns( sal_Int32 /*nDimensionIndex*/ )
{
    return false;
}

LegendSymbolStyle BubbleChart::getLegendSymbolStyle()
{
    return LegendSymbolStyle::Circle;
}

drawing::Direction3D BubbleChart::getPreferredDiagramAspectRatio() const
{
    return drawing::Direction3D(-1,-1,-1);
}

namespace {

//better performance for big data
struct FormerPoint
{
    FormerPoint( double fX, double fY, double fZ )
        : m_fX(fX), m_fY(fY), m_fZ(fZ)
        {}
    FormerPoint()
        : m_fX(std::numeric_limits<double>::quiet_NaN())
        , m_fY(std::numeric_limits<double>::quiet_NaN())
        , m_fZ(std::numeric_limits<double>::quiet_NaN())
    {
    }

    double m_fX;
    double m_fY;
    double m_fZ;
};

}

void BubbleChart::createShapes()
{
    if( m_aZSlots.empty() ) //no series
        return;

    OSL_ENSURE(m_xLogicTarget.is()&&m_xFinalTarget.is(),"BubbleChart is not proper initialized");
    if(!(m_xLogicTarget.is()&&m_xFinalTarget.is()))
        return;

    //therefore create an own group for the texts and the error bars to move them to front
    //(because the text group is created after the series group the texts are displayed on top)
    rtl::Reference<SvxShapeGroupAnyD> xSeriesTarget = createGroupShape( m_xLogicTarget );
    rtl::Reference< SvxShapeGroup > xTextTarget = ShapeFactory::createGroup2D( m_xFinalTarget );

    //update/create information for current group
    double fLogicZ = 1.0;//as defined

    sal_Int32 const nStartIndex = 0; // inclusive       ;..todo get somehow from x scale
    sal_Int32 nEndIndex = VSeriesPlotter::getPointCount();
    if(nEndIndex<=0)
        nEndIndex=1;

    //better performance for big data
    std::map< VDataSeries*, FormerPoint > aSeriesFormerPointMap;
    m_bPointsWereSkipped = false;
    sal_Int32 nSkippedPoints = 0;
    sal_Int32 nCreatedPoints = 0;

    calculateMaximumLogicBubbleSize();
    calculateBubbleSizeScalingFactor();
    if( m_fMaxLogicBubbleSize <= 0 || m_fBubbleSizeFactorToScreen <= 0 )
        return;

    //iterate through all x values per indices
    for( sal_Int32 nIndex = nStartIndex; nIndex < nEndIndex; nIndex++ )
    {
        for( auto const& rZSlot : m_aZSlots )
        {
            for( auto const& rXSlot : rZSlot )
            {
                //iterate through all series
                for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
                {
                    if(!pSeries)
                        continue;

                    bool bHasFillColorMapping = pSeries->hasPropertyMapping(u"FillColor"_ustr);
                    bool bHasBorderColorMapping = pSeries->hasPropertyMapping(u"LineColor"_ustr);

                    rtl::Reference<SvxShapeGroupAnyD> xSeriesGroupShape_Shapes = getSeriesGroupShape(pSeries.get(), xSeriesTarget);

                    sal_Int32 nAttachedAxisIndex = pSeries->getAttachedAxisIndex();
                    PlottingPositionHelper& rPosHelper
                        = getPlottingPositionHelper(nAttachedAxisIndex);
                    m_pPosHelper = &rPosHelper;

                    //collect data point information (logic coordinates, style ):
                    double fLogicX = pSeries->getXValue(nIndex);
                    double fLogicY = pSeries->getYValue(nIndex);
                    double fBubbleSize = pSeries->getBubble_Size( nIndex );

                    bool bInvertNeg(false);
                    uno::Reference< beans::XPropertySet > xPointProperties =
                        pSeries->getPropertiesOfPoint(nIndex);

                    // check point properties, and if none then series
                    // properties
                    try {
                        xPointProperties->getPropertyValue(u"InvertNegative"_ustr) >>= bInvertNeg;
                    } catch (const uno::Exception&)
                    {
                        uno::Reference< beans::XPropertySet > xSeriesProperties =
                            pSeries->getPropertiesOfSeries();
                        try {
                            xSeriesProperties->getPropertyValue(u"InvertNegative"_ustr) >>= bInvertNeg;
                        } catch (const uno::Exception&)
                        {}
                    }

                    if( fBubbleSize<0.0 ) {
                        if (bInvertNeg) {
                            fBubbleSize = -fBubbleSize;
                        } else {
                            continue;
                        }
                    }

                    if( fBubbleSize == 0.0 || std::isnan(fBubbleSize) )
                        continue;

                    if(    std::isnan(fLogicX) || std::isinf(fLogicX)
                        || std::isnan(fLogicY) || std::isinf(fLogicY) )
                        continue;

                    bool bIsVisible = rPosHelper.isLogicVisible(fLogicX, fLogicY, fLogicZ);

                    drawing::Position3D aUnscaledLogicPosition( fLogicX, fLogicY, fLogicZ );
                    drawing::Position3D aScaledLogicPosition(aUnscaledLogicPosition);
                    rPosHelper.doLogicScaling(aScaledLogicPosition);

                    //transformation 3) -> 4)
                    drawing::Position3D aScenePosition(
                        rPosHelper.transformLogicToScene(fLogicX, fLogicY, fLogicZ, false));

                    //better performance for big data
                    uno::Reference< beans::XPropertySet > xProps(pSeries->getPropertiesOfPoint( nIndex ));
                    sal_Int16 nFillTransparency(0);
                    xProps->getPropertyValue(u"FillTransparence"_ustr) >>= nFillTransparency;
                    const bool bIsTransparent(nFillTransparency != 0);

                    FormerPoint aFormerPoint( aSeriesFormerPointMap[pSeries.get()] );
                    rPosHelper.setCoordinateSystemResolution(m_aCoordinateSystemResolution);
                    if (!pSeries->isAttributedDataPoint(nIndex)
                        && !bIsTransparent  // don't short-cut if there's transparency
                        && rPosHelper.isSameForGivenResolution(
                               aFormerPoint.m_fX, aFormerPoint.m_fY, aFormerPoint.m_fZ,
                               aScaledLogicPosition.PositionX, aScaledLogicPosition.PositionY,
                               aScaledLogicPosition.PositionZ))
                    {
                        nSkippedPoints++;
                        m_bPointsWereSkipped = true;
                        continue;
                    }
                    aSeriesFormerPointMap[pSeries.get()] = FormerPoint(aScaledLogicPosition.PositionX, aScaledLogicPosition.PositionY, aScaledLogicPosition.PositionZ);

                    //create a single datapoint if point is visible
                    if( !bIsVisible )
                        continue;

                    //create a group shape for this point and add to the series shape:
                    OUString aPointCID = ObjectIdentifier::createPointCID(
                        pSeries->getPointCID_Stub(), nIndex );
                    rtl::Reference<SvxShapeGroupAnyD> xPointGroupShape_Shapes(
                        createGroupShape(xSeriesGroupShape_Shapes,aPointCID) );

                    {
                        nCreatedPoints++;

                        //create data point
                        drawing::Direction3D aSymbolSize = transformToScreenBubbleSize( fBubbleSize );
                        rtl::Reference<SvxShapeCircle> xShape = ShapeFactory::createCircle2D( xPointGroupShape_Shapes
                                , aScenePosition, aSymbolSize );

                        PropertyMapper::setMappedProperties( *xShape
                                , pSeries->getPropertiesOfPoint( nIndex )
                                , PropertyMapper::getPropertyNameMapForFilledSeriesProperties() );

                        if(bHasFillColorMapping)
                        {
                            double nPropVal = pSeries->getValueByProperty(nIndex, u"FillColor"_ustr);
                            if(!std::isnan(nPropVal))
                            {
                                xShape->SvxShape::setPropertyValue(u"FillColor"_ustr, uno::Any(static_cast<sal_Int32>(nPropVal)));
                            }
                        }
                        if(bHasBorderColorMapping)
                        {
                            double nPropVal = pSeries->getValueByProperty(nIndex, u"LineColor"_ustr);
                            if(!std::isnan(nPropVal))
                            {
                                xShape->SvxShape::setPropertyValue(u"LineColor"_ustr, uno::Any(static_cast<sal_Int32>(nPropVal)));
                            }
                        }

                        ::chart::ShapeFactory::setShapeName( xShape, u"MarkHandles"_ustr );

                        //create data point label
                        if( pSeries->getDataPointLabelIfLabel(nIndex) )
                        {
                            LabelAlignment eAlignment = LABEL_ALIGN_TOP;
                            drawing::Position3D aScenePosition3D( aScenePosition.PositionX
                                        , aScenePosition.PositionY
                                        , aScenePosition.PositionZ+getTransformedDepth() );

                            sal_Int32 nLabelPlacement = pSeries->getLabelPlacement(
                                nIndex, m_xChartTypeModel, rPosHelper.isSwapXAndY());

                            switch(nLabelPlacement)
                            {
                            case css::chart::DataLabelPlacement::TOP:
                                aScenePosition3D.PositionY -= (aSymbolSize.DirectionY/2+1);
                                eAlignment = LABEL_ALIGN_TOP;
                                break;
                            case css::chart::DataLabelPlacement::BOTTOM:
                                aScenePosition3D.PositionY += (aSymbolSize.DirectionY/2+1);
                                eAlignment = LABEL_ALIGN_BOTTOM;
                                break;
                            case css::chart::DataLabelPlacement::LEFT:
                                aScenePosition3D.PositionX -= (aSymbolSize.DirectionX/2+1);
                                eAlignment = LABEL_ALIGN_LEFT;
                                break;
                            case css::chart::DataLabelPlacement::RIGHT:
                                aScenePosition3D.PositionX += (aSymbolSize.DirectionX/2+1);
                                eAlignment = LABEL_ALIGN_RIGHT;
                                break;
                            case css::chart::DataLabelPlacement::CENTER:
                                eAlignment = LABEL_ALIGN_CENTER;
                                break;
                            default:
                                OSL_FAIL("this label alignment is not implemented yet");
                                aScenePosition3D.PositionY -= (aSymbolSize.DirectionY/2+1);
                                eAlignment = LABEL_ALIGN_TOP;
                                break;
                            }

                            awt::Point aScreenPosition2D( LabelPositionHelper(m_nDimension,m_xLogicTarget)
                                .transformSceneToScreenPosition( aScenePosition3D ) );
                            sal_Int32 nOffset = 0;
                            if(eAlignment!=LABEL_ALIGN_CENTER)
                                nOffset = 100;//add some spacing //@todo maybe get more intelligent values
                            createDataLabel( xTextTarget, *pSeries, nIndex
                                            , fBubbleSize, fBubbleSize, aScreenPosition2D, eAlignment, nOffset );
                        }
                    }

                    //remove PointGroupShape if empty
                    if(!xPointGroupShape_Shapes->getCount())
                        xSeriesGroupShape_Shapes->remove(xPointGroupShape_Shapes);

                }//next series in x slot (next y slot)
            }//next x slot
        }//next z slot
    }//next category
    SAL_INFO(
        "chart2",
        "skipped points: " << nSkippedPoints << " created points: "
            << nCreatedPoints);
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

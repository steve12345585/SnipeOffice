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

#include "CandleStickChart.hxx"
#include <ChartType.hxx>
#include <ShapeFactory.hxx>
#include <CommonConverters.hxx>
#include <ExplicitCategoriesProvider.hxx>
#include <ObjectIdentifier.hxx>
#include "BarPositionHelper.hxx"
#include <DateHelper.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <osl/diagnose.h>

namespace chart
{
using namespace ::com::sun::star;

CandleStickChart::CandleStickChart( const rtl::Reference<ChartType>& xChartTypeModel
                                    , sal_Int32 nDimensionCount )
        : VSeriesPlotter( xChartTypeModel, nDimensionCount )
{
    PlotterBase::m_pPosHelper = &m_aMainPosHelper;
    VSeriesPlotter::m_pMainPosHelper = &m_aMainPosHelper;
}

CandleStickChart::~CandleStickChart()
{
}

// MinimumAndMaximumSupplier

bool CandleStickChart::isSeparateStackingForDifferentSigns( sal_Int32 /* nDimensionIndex */ )
{
    return false;
}

LegendSymbolStyle CandleStickChart::getLegendSymbolStyle()
{
    return LegendSymbolStyle::Line;
}

drawing::Direction3D CandleStickChart::getPreferredDiagramAspectRatio() const
{
    return drawing::Direction3D(-1,-1,-1);
}

void CandleStickChart::addSeries( std::unique_ptr<VDataSeries> pSeries, sal_Int32 /* zSlot */, sal_Int32 xSlot, sal_Int32 ySlot )
{
    //ignore y stacking for candle stick chart
    VSeriesPlotter::addSeries( std::move(pSeries), 0, xSlot, ySlot );
}

void CandleStickChart::createShapes()
{
    if( m_aZSlots.empty() ) //no series
        return;

    if( m_nDimension!=2 )
        return;

    OSL_ENSURE(m_xLogicTarget.is()&&m_xFinalTarget.is(),"CandleStickChart is not proper initialized");
    if(!(m_xLogicTarget.is()&&m_xFinalTarget.is()))
        return;

    //the text labels should be always on top of the other series shapes
    //therefore create an own group for the texts to move them to front
    //(because the text group is created after the series group the texts are displayed on top)

    rtl::Reference<SvxShapeGroupAnyD> xSeriesTarget =
        createGroupShape( m_xLogicTarget );
    rtl::Reference<SvxShapeGroupAnyD> xLossTarget =
        createGroupShape( m_xLogicTarget, ObjectIdentifier::createClassifiedIdentifier(
            OBJECTTYPE_DATA_STOCK_LOSS, u"" ));
    rtl::Reference<SvxShapeGroupAnyD> xGainTarget =
        createGroupShape( m_xLogicTarget, ObjectIdentifier::createClassifiedIdentifier(
            OBJECTTYPE_DATA_STOCK_GAIN, u"" ));
    rtl::Reference< SvxShapeGroup > xTextTarget =
        ShapeFactory::createGroup2D( m_xFinalTarget );

    //check necessary here that different Y axis can not be stacked in the same group? ... hm?

    bool bJapaneseStyle=true;//@todo is this the correct default?
    bool bShowFirst = true;//is only important if bJapaneseStyle == false
    tNameSequence aWhiteBox_Names, aBlackBox_Names;
    tAnySequence  aWhiteBox_Values, aBlackBox_Values;
    try
    {
        if( m_xChartTypeModel.is() )
        {
            m_xChartTypeModel->getPropertyValue( u"ShowFirst"_ustr ) >>= bShowFirst;

            uno::Reference< beans::XPropertySet > xWhiteDayProps;
            uno::Reference< beans::XPropertySet > xBlackDayProps;
            m_xChartTypeModel->getPropertyValue( u"Japanese"_ustr ) >>= bJapaneseStyle;
            m_xChartTypeModel->getPropertyValue( u"WhiteDay"_ustr ) >>= xWhiteDayProps;
            m_xChartTypeModel->getPropertyValue( u"BlackDay"_ustr ) >>= xBlackDayProps;

            tPropertyNameValueMap aWhiteBox_Map;
            PropertyMapper::getValueMap( aWhiteBox_Map, PropertyMapper::getPropertyNameMapForFillAndLineProperties(), xWhiteDayProps );
            PropertyMapper::getMultiPropertyListsFromValueMap( aWhiteBox_Names, aWhiteBox_Values, aWhiteBox_Map );

            tPropertyNameValueMap aBlackBox_Map;
            PropertyMapper::getValueMap( aBlackBox_Map, PropertyMapper::getPropertyNameMapForFillAndLineProperties(), xBlackDayProps );
            PropertyMapper::getMultiPropertyListsFromValueMap( aBlackBox_Names, aBlackBox_Values, aBlackBox_Map );
        }
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }

    //(@todo maybe different iteration for breaks in axis ?)
    sal_Int32 nEndIndex = VSeriesPlotter::getPointCount();
    double fLogicZ = 1.5;//as defined
    //iterate through all x values per indices
    for( sal_Int32 nIndex = 0; nIndex < nEndIndex; nIndex++ )
    {
        for( auto const& rZSlot : m_aZSlots )
        {
            BarPositionHelper* pPosHelper = &m_aMainPosHelper;
            if( !rZSlot.empty() )
            {
                sal_Int32 nAttachedAxisIndex = rZSlot.front().getAttachedAxisIndexForFirstSeries();
                //2ND_AXIS_IN_BARS so far one can assume to have the same plotter for each z slot
                pPosHelper = dynamic_cast<BarPositionHelper*>(&( getPlottingPositionHelper( nAttachedAxisIndex ) ) );
                if(!pPosHelper)
                    pPosHelper = &m_aMainPosHelper;
            }
            PlotterBase::m_pPosHelper = pPosHelper;

            //update/create information for current group
            pPosHelper->updateSeriesCount( rZSlot.size() );
            double fSlotX=0;
            //iterate through all x slots in this category
            for( auto const& rXSlot : rZSlot )
            {
                //iterate through all series in this x slot
                for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
                {
                    //collect data point information (logic coordinates, style ):
                    double fUnscaledX = pSeries->getXValue( nIndex );
                    if( m_pExplicitCategoriesProvider && m_pExplicitCategoriesProvider->isDateAxis() )
                        fUnscaledX = DateHelper::RasterizeDateValue( fUnscaledX, m_aNullDate, m_nTimeResolution );
                    if(fUnscaledX<pPosHelper->getLogicMinX() || fUnscaledX>pPosHelper->getLogicMaxX())
                        continue;//point not visible
                    double fScaledX = pPosHelper->getScaledSlotPos( fUnscaledX, fSlotX );

                    double fUnscaledY_First = pSeries->getY_First( nIndex );
                    double fUnscaledY_Last = pSeries->getY_Last( nIndex );
                    double fUnscaledY_Min = pSeries->getY_Min( nIndex );
                    double fUnscaledY_Max = pSeries->getY_Max( nIndex );

                    bool bBlack=false;
                    if(fUnscaledY_Last<=fUnscaledY_First)
                    {
                        std::swap(fUnscaledY_First,fUnscaledY_Last);
                        bBlack=true;
                    }
                    if(fUnscaledY_Max<fUnscaledY_Min)
                        std::swap(fUnscaledY_Min,fUnscaledY_Max);
                    //transformation 3) -> 4)
                    double fHalfScaledWidth = pPosHelper->getScaledSlotWidth()/2.0;

                    double fScaledY_First(fUnscaledY_First);
                    double fScaledY_Last(fUnscaledY_Last);
                    double fScaledY_Min(fUnscaledY_Min);
                    double fScaledY_Max(fUnscaledY_Max);
                    pPosHelper->clipLogicValues( nullptr,&fScaledY_First,nullptr );
                    pPosHelper->clipLogicValues( nullptr,&fScaledY_Last,nullptr );
                    pPosHelper->clipLogicValues( nullptr,&fScaledY_Min,nullptr );
                    pPosHelper->clipLogicValues( nullptr,&fScaledY_Max,nullptr );
                    pPosHelper->doLogicScaling( nullptr,&fScaledY_First,nullptr );
                    pPosHelper->doLogicScaling( nullptr,&fScaledY_Last,nullptr );
                    pPosHelper->doLogicScaling( nullptr,&fScaledY_Min,nullptr );
                    pPosHelper->doLogicScaling( nullptr,&fScaledY_Max,nullptr );

                    drawing::Position3D aPosLeftFirst( pPosHelper->transformScaledLogicToScene( fScaledX-fHalfScaledWidth, fScaledY_First ,0 ,true ) );
                    drawing::Position3D aPosRightLast( pPosHelper->transformScaledLogicToScene( fScaledX+fHalfScaledWidth, fScaledY_Last  ,0 ,true ) );
                    drawing::Position3D aPosMiddleFirst( pPosHelper->transformScaledLogicToScene( fScaledX, fScaledY_First ,0 ,true ) );
                    drawing::Position3D aPosMiddleLast( pPosHelper->transformScaledLogicToScene( fScaledX, fScaledY_Last  ,0 ,true ) );
                    drawing::Position3D aPosMiddleMinimum( pPosHelper->transformScaledLogicToScene( fScaledX, fScaledY_Min ,0 ,true ) );
                    drawing::Position3D aPosMiddleMaximum( pPosHelper->transformScaledLogicToScene( fScaledX, fScaledY_Max ,0 ,true ) );

                    rtl::Reference<SvxShapeGroupAnyD> xLossGainTarget(bBlack ? xLossTarget : xGainTarget);

                    uno::Reference< beans::XPropertySet > xPointProp( pSeries->getPropertiesOfPoint( nIndex ));
                    rtl::Reference<SvxShapeGroupAnyD> xPointGroupShape_Shapes;
                    {
                        OUString aPointCID = ObjectIdentifier::createPointCID( pSeries->getPointCID_Stub(), nIndex );
                        rtl::Reference<SvxShapeGroupAnyD> xSeriesGroupShape_Shapes( getSeriesGroupShape(pSeries.get(), xSeriesTarget) );
                        xPointGroupShape_Shapes = createGroupShape(xSeriesGroupShape_Shapes,aPointCID);
                    }

                    //create min-max line
                    if( isValidPosition(aPosMiddleMinimum) && isValidPosition(aPosMiddleMaximum) )
                    {
                        std::vector<std::vector<css::drawing::Position3D>> aPoly
                        {
                            { aPosMiddleMinimum, aPosMiddleMaximum }
                        };

                        rtl::Reference<SvxShapePolyPolygon> xShape =
                            ShapeFactory::createLine2D( xPointGroupShape_Shapes, aPoly);
                        PropertyMapper::setMappedProperties( *xShape, xPointProp, PropertyMapper::getPropertyNameMapForLineSeriesProperties() );
                    }

                    //create first-last shape
                    if(bJapaneseStyle && isValidPosition(aPosLeftFirst) && isValidPosition(aPosRightLast) )
                    {
                        drawing::Direction3D aDiff = aPosRightLast-aPosLeftFirst;
                        awt::Size aAWTSize( Direction3DToAWTSize( aDiff ));
                        // workaround for bug in drawing: if height is 0 the box gets infinitely large
                        if( aAWTSize.Height == 0 )
                            aAWTSize.Height = 1;

                        tNameSequence aNames;
                        tAnySequence aValues;

                        rtl::Reference<SvxShapeRect> xShape =
                            ShapeFactory::createRectangle( xLossGainTarget,
                                    aAWTSize, Position3DToAWTPoint( aPosLeftFirst ),
                                    aNames, aValues);

                        if(bBlack)
                            PropertyMapper::setMultiProperties( aBlackBox_Names, aBlackBox_Values, *xShape );
                        else
                            PropertyMapper::setMultiProperties( aWhiteBox_Names, aWhiteBox_Values, *xShape );
                    }
                    else
                    {
                        std::vector<std::vector<css::drawing::Position3D>> aPoly;

                        sal_Int32 nLineIndex = 0;
                        if( bShowFirst &&  pPosHelper->isLogicVisible( fUnscaledX, fUnscaledY_First ,fLogicZ )
                            && isValidPosition(aPosLeftFirst) && isValidPosition(aPosMiddleFirst) )
                        {
                            AddPointToPoly( aPoly, aPosLeftFirst, nLineIndex );
                            AddPointToPoly( aPoly, aPosMiddleFirst, nLineIndex++ );
                        }
                        if( pPosHelper->isLogicVisible( fUnscaledX, fUnscaledY_Last ,fLogicZ )
                            && isValidPosition(aPosMiddleLast) && isValidPosition(aPosRightLast) )
                        {
                            AddPointToPoly( aPoly, aPosMiddleLast, nLineIndex );
                            AddPointToPoly( aPoly, aPosRightLast, nLineIndex );
                        }

                        if( !aPoly.empty() )
                        {
                            rtl::Reference<SvxShapePolyPolygon> xShape =
                                ShapeFactory::createLine2D( xPointGroupShape_Shapes, aPoly );
                            PropertyMapper::setMappedProperties( *xShape, xPointProp, PropertyMapper::getPropertyNameMapForLineSeriesProperties() );
                        }
                    }

                    //create data point label
                    if( pSeries->getDataPointLabelIfLabel(nIndex) )
                    {
                        if(isValidPosition(aPosMiddleFirst))
                            createDataLabel( xTextTarget, *pSeries, nIndex
                                        , fUnscaledY_First, 1.0, Position3DToAWTPoint(aPosMiddleFirst), LABEL_ALIGN_LEFT_BOTTOM );
                        if(isValidPosition(aPosMiddleLast))
                            createDataLabel( xTextTarget, *pSeries, nIndex
                                        , fUnscaledY_Last, 1.0, Position3DToAWTPoint(aPosMiddleLast), LABEL_ALIGN_RIGHT_TOP );
                        if(isValidPosition(aPosMiddleMinimum))
                            createDataLabel( xTextTarget, *pSeries, nIndex
                                        , fUnscaledY_Min, 1.0, Position3DToAWTPoint(aPosMiddleMinimum), LABEL_ALIGN_BOTTOM );
                        if(isValidPosition(aPosMiddleMaximum))
                            createDataLabel( xTextTarget, *pSeries, nIndex
                                        , fUnscaledY_Max, 1.0, Position3DToAWTPoint(aPosMiddleMaximum), LABEL_ALIGN_TOP );
                    }
                }//next series in x slot (next y slot)
                fSlotX+=1.0;
            }//next x slot
        }//next z slot
    }//next category
    /* @todo remove series shapes if empty
    //remove and delete point-group-shape if empty
    if(!xSeriesGroupShape_Shapes->getCount())
    {
        pSeries->m_xShape.set(NULL);
        m_xLogicTarget->remove(xSeriesGroupShape_Shape);
    }
    */

    //remove and delete series-group-shape if empty

    //... todo
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

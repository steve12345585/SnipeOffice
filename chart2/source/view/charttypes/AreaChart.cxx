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

#include "AreaChart.hxx"
#include <PlottingPositionHelper.hxx>
#include <ShapeFactory.hxx>
#include <CommonConverters.hxx>
#include <ExplicitCategoriesProvider.hxx>
#include <ObjectIdentifier.hxx>
#include "Splines.hxx"
#include <ChartType.hxx>
#include <ChartTypeHelper.hxx>
#include <LabelPositionHelper.hxx>
#include <Clipping.hxx>
#include <Stripe.hxx>
#include <DateHelper.hxx>
#include <unonames.hxx>

#include <com/sun/star/chart2/Symbol.hpp>
#include <com/sun/star/chart/DataLabelPlacement.hpp>
#include <com/sun/star/chart/MissingValueTreatment.hpp>

#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <officecfg/Office/Compatibility.hxx>
#include <officecfg/Office/Chart.hxx>

#include <limits>

namespace chart
{
using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;

AreaChart::AreaChart( const rtl::Reference<ChartType>& xChartTypeModel
                     , sal_Int32 nDimensionCount
                     , bool bCategoryXAxis
                     , bool bNoArea
                     )
        : VSeriesPlotter( xChartTypeModel, nDimensionCount, bCategoryXAxis )
        , m_bArea(!bNoArea)
        , m_bLine(bNoArea)
        , m_bSymbol(xChartTypeModel.is() ? xChartTypeModel->isSupportingSymbolProperties(nDimensionCount) : false)
        , m_eCurveStyle(CurveStyle_LINES)
        , m_nCurveResolution(20)
        , m_nSplineOrder(3)
{
    PlotterBase::m_pPosHelper = &m_aMainPosHelper;
    VSeriesPlotter::m_pMainPosHelper = &m_aMainPosHelper;

    m_pMainPosHelper->AllowShiftXAxisPos(true);
    m_pMainPosHelper->AllowShiftZAxisPos(true);

    try
    {
        if( m_xChartTypeModel.is() )
        {
            m_xChartTypeModel->getPropertyValue(CHART_UNONAME_CURVE_STYLE) >>= m_eCurveStyle;
            m_xChartTypeModel->getPropertyValue(CHART_UNONAME_CURVE_RESOLUTION) >>= m_nCurveResolution;
            m_xChartTypeModel->getPropertyValue(CHART_UNONAME_SPLINE_ORDER) >>= m_nSplineOrder;
        }
    }
    catch( uno::Exception& e )
    {
        //the above properties are not supported by all charttypes supported by this class (e.g. area or net chart)
        //in that cases this exception is ok
        e.Context.is();//to have debug information without compilation warnings
    }
}

AreaChart::~AreaChart()
{
}

bool AreaChart::isSeparateStackingForDifferentSigns( sal_Int32 /*nDimensionIndex*/ )
{
    // no separate stacking in all types of line/area charts
    return false;
}

LegendSymbolStyle AreaChart::getLegendSymbolStyle()
{
    if( m_bArea || m_nDimension == 3 )
        return LegendSymbolStyle::Box;
    return LegendSymbolStyle::Line;
}

uno::Any AreaChart::getExplicitSymbol( const VDataSeries& rSeries, sal_Int32 nPointIndex )
{
    uno::Any aRet;

    Symbol* pSymbolProperties = rSeries.getSymbolProperties( nPointIndex );
    if( pSymbolProperties )
    {
        aRet <<= *pSymbolProperties;
    }

    return aRet;
}

drawing::Direction3D AreaChart::getPreferredDiagramAspectRatio() const
{
    drawing::Direction3D aRet(1,-1,1);
    if( m_nDimension == 2 )
        aRet = drawing::Direction3D(-1,-1,-1);
    else if (m_pPosHelper)
    {
        drawing::Direction3D aScale( m_pPosHelper->getScaledLogicWidth() );
        aRet.DirectionZ = aScale.DirectionZ*0.2;
        if(aRet.DirectionZ>1.0)
            aRet.DirectionZ=1.0;
        if(aRet.DirectionZ>10)
            aRet.DirectionZ=10;
    }
    return aRet;
}

void AreaChart::addSeries( std::unique_ptr<VDataSeries> pSeries, sal_Int32 zSlot, sal_Int32 xSlot, sal_Int32 ySlot )
{
    if( m_bArea && pSeries )
    {
        sal_Int32 nMissingValueTreatment = pSeries->getMissingValueTreatment();
        if( nMissingValueTreatment == css::chart::MissingValueTreatment::LEAVE_GAP  )
            pSeries->setMissingValueTreatment( css::chart::MissingValueTreatment::USE_ZERO );
    }
    if( m_nDimension == 3 && !m_bCategoryXAxis )
    {
        //3D xy always deep
        OSL_ENSURE( zSlot==-1,"3D xy charts should be deep stacked in model also" );
        zSlot=-1;
        xSlot=0;
        ySlot=0;
    }
    VSeriesPlotter::addSeries( std::move(pSeries), zSlot, xSlot, ySlot );
}

static void lcl_removeDuplicatePoints( std::vector<std::vector<css::drawing::Position3D>>& rPolyPoly, PlottingPositionHelper& rPosHelper )
{
    sal_Int32 nPolyCount = rPolyPoly.size();
    if(!nPolyCount)
        return;

    // TODO we could do with without a temporary array
    std::vector<std::vector<css::drawing::Position3D>> aTmp;
    aTmp.resize(nPolyCount);

    for( sal_Int32 nPolygonIndex = 0; nPolygonIndex<nPolyCount; nPolygonIndex++ )
    {
        std::vector<css::drawing::Position3D>* pOuterSource = &rPolyPoly[nPolygonIndex];
        std::vector<css::drawing::Position3D>* pOuterTarget = &aTmp[nPolygonIndex];

        sal_Int32 nPointCount = pOuterSource->size();
        if( !nPointCount )
            continue;

        pOuterTarget->resize(nPointCount);

        css::drawing::Position3D* pSource = pOuterSource->data();
        css::drawing::Position3D* pTarget = pOuterTarget->data();

        //copy first point
        *pTarget=*pSource++;
        sal_Int32 nTargetPointCount=1;

        for( sal_Int32 nSource=1; nSource<nPointCount; nSource++ )
        {
            if( !rPosHelper.isSameForGivenResolution( pTarget->PositionX, pTarget->PositionY, pTarget->PositionZ
                                                   , pSource->PositionX, pSource->PositionY, pSource->PositionZ ) )
            {
                pTarget++;
                *pTarget=*pSource;
                nTargetPointCount++;
            }
            pSource++;
        }

        //free unused space
        if( nTargetPointCount<nPointCount )
        {
            pOuterTarget->resize(nTargetPointCount);
        }

        pOuterSource->clear();
    }

    //free space
    rPolyPoly.resize(nPolyCount);

    rPolyPoly = std::move(aTmp);
}

bool AreaChart::create_stepped_line(
        std::vector<std::vector<css::drawing::Position3D>> aStartPoly,
        chart2::CurveStyle eCurveStyle,
        PlottingPositionHelper const * pPosHelper,
        std::vector<std::vector<css::drawing::Position3D>> &aPoly )
{
    sal_uInt32 nOuterCount = aStartPoly.size();
    if ( !nOuterCount )
        return false;

    std::vector<std::vector<css::drawing::Position3D>> aSteppedPoly;
    aSteppedPoly.resize(nOuterCount);

    auto pSequence = aSteppedPoly.data();

    for( sal_uInt32 nOuter = 0; nOuter < nOuterCount; ++nOuter )
    {
        if( aStartPoly[nOuter].size() <= 1 )
            continue; //we need at least two points

        sal_uInt32 nMaxIndexPoints = aStartPoly[nOuter].size()-1; // is >1
        sal_uInt32 nNewIndexPoints = 0;
        if ( eCurveStyle==CurveStyle_STEP_START || eCurveStyle==CurveStyle_STEP_END)
            nNewIndexPoints = nMaxIndexPoints * 2 + 1;
        else
            nNewIndexPoints = nMaxIndexPoints * 3 + 1;

        const css::drawing::Position3D* pOld = aStartPoly[nOuter].data();

        pSequence[nOuter].resize( nNewIndexPoints );

        css::drawing::Position3D* pNew = pSequence[nOuter].data();

        pNew[0] = pOld[0];
        for( sal_uInt32 oi = 0; oi < nMaxIndexPoints; oi++ )
        {
            switch ( eCurveStyle )
            {
                case CurveStyle_STEP_START:
                     /**           O
                                   |
                                   |
                                   |
                             O-----+
                     */
                    // create the intermediate point
                    pNew[1+oi*2].PositionX = pOld[oi+1].PositionX;
                    pNew[1+oi*2].PositionY = pOld[oi].PositionY;
                    pNew[1+oi*2].PositionZ = pOld[oi].PositionZ;
                    // and now the normal one
                    pNew[1+oi*2+1] = pOld[oi+1];
                    break;
                case CurveStyle_STEP_END:
                     /**    +------O
                            |
                            |
                            |
                            O
                     */
                    // create the intermediate point
                    pNew[1+oi*2].PositionX = pOld[oi].PositionX;
                    pNew[1+oi*2].PositionY = pOld[oi+1].PositionY;
                    pNew[1+oi*2].PositionZ = pOld[oi].PositionZ;
                    // and now the normal one
                    pNew[1+oi*2+1] = pOld[oi+1];
                    break;
                case CurveStyle_STEP_CENTER_X:
                     /**        +--O
                                |
                                |
                                |
                             O--+
                     */
                    // create the first intermediate point
                    pNew[1+oi*3].PositionX = (pOld[oi].PositionX + pOld[oi+1].PositionX) / 2;
                    pNew[1+oi*3].PositionY = pOld[oi].PositionY;
                    pNew[1+oi*3].PositionZ = pOld[oi].PositionZ;
                    // create the second intermediate point
                    pNew[1+oi*3+1].PositionX = (pOld[oi].PositionX + pOld[oi+1].PositionX) / 2;
                    pNew[1+oi*3+1].PositionY = pOld[oi+1].PositionY;
                    pNew[1+oi*3+1].PositionZ = pOld[oi].PositionZ;
                    // and now the normal one
                    pNew[1+oi*3+2] = pOld[oi+1];
                    break;
                case CurveStyle_STEP_CENTER_Y:
                     /**           O
                                   |
                             +-----+
                             |
                             O
                     */
                    // create the first intermediate point
                    pNew[1+oi*3].PositionX = pOld[oi].PositionX;
                    pNew[1+oi*3].PositionY = (pOld[oi].PositionY + pOld[oi+1].PositionY) / 2;
                    pNew[1+oi*3].PositionZ = pOld[oi].PositionZ;
                    // create the second intermediate point
                    pNew[1+oi*3+1].PositionX = pOld[oi+1].PositionX;
                    pNew[1+oi*3+1].PositionY = (pOld[oi].PositionY + pOld[oi+1].PositionY) / 2;
                    pNew[1+oi*3+1].PositionZ = pOld[oi].PositionZ;
                    // and now the normal one
                    pNew[1+oi*3+2] = pOld[oi+1];
                    break;
                default:
                    // this should never be executed
                    OSL_FAIL("Unknown curvestyle in AreaChart::create_stepped_line");
            }
        }
    }
    Clipping::clipPolygonAtRectangle( aSteppedPoly, pPosHelper->getScaledLogicClipDoubleRect(), aPoly );

    return true;
}

bool AreaChart::impl_createLine( VDataSeries* pSeries
                , std::vector<std::vector<css::drawing::Position3D>> const * pSeriesPoly
                , PlottingPositionHelper* pPosHelper )
{
    //return true if a line was created successfully
    rtl::Reference<SvxShapeGroupAnyD> xSeriesGroupShape_Shapes = getSeriesGroupShapeBackChild(pSeries, m_xSeriesTarget);

    std::vector<std::vector<css::drawing::Position3D>> aPoly;
    if(m_eCurveStyle==CurveStyle_CUBIC_SPLINES)
    {
        std::vector<std::vector<css::drawing::Position3D>> aSplinePoly;
        SplineCalculater::CalculateCubicSplines( *pSeriesPoly, aSplinePoly, m_nCurveResolution );
        lcl_removeDuplicatePoints( aSplinePoly, *pPosHelper );
        Clipping::clipPolygonAtRectangle( aSplinePoly, pPosHelper->getScaledLogicClipDoubleRect(), aPoly );
    }
    else if(m_eCurveStyle==CurveStyle_B_SPLINES)
    {
        std::vector<std::vector<css::drawing::Position3D>> aSplinePoly;
        SplineCalculater::CalculateBSplines( *pSeriesPoly, aSplinePoly, m_nCurveResolution, m_nSplineOrder );
        lcl_removeDuplicatePoints( aSplinePoly, *pPosHelper );
        Clipping::clipPolygonAtRectangle( aSplinePoly, pPosHelper->getScaledLogicClipDoubleRect(), aPoly );
    }
    else if (m_eCurveStyle==CurveStyle_STEP_START ||
             m_eCurveStyle==CurveStyle_STEP_END ||
             m_eCurveStyle==CurveStyle_STEP_CENTER_Y ||
             m_eCurveStyle==CurveStyle_STEP_CENTER_X
            )
    {
        if (!create_stepped_line(*pSeriesPoly, m_eCurveStyle, pPosHelper, aPoly))
        {
            return false;
        }
    }
    else
    { // default to creating a straight line
        SAL_WARN_IF(m_eCurveStyle != CurveStyle_LINES, "chart2.areachart", "Unknown curve style");
        Clipping::clipPolygonAtRectangle( *pSeriesPoly, pPosHelper->getScaledLogicClipDoubleRect(), aPoly );
    }

    if(!ShapeFactory::hasPolygonAnyLines(aPoly))
        return false;

    //transformation 3) -> 4)
    pPosHelper->transformScaledLogicToScene( aPoly );

    //create line:
    rtl::Reference< SvxShape > xShape;
    if(m_nDimension==3)
    {
        double fDepth = getTransformedDepth();
        sal_Int32 nPolyCount = aPoly.size();
        for(sal_Int32 nPoly=0;nPoly<nPolyCount;nPoly++)
        {
            sal_Int32 nPointCount = aPoly[nPoly].size();
            for(sal_Int32 nPoint=0;nPoint<nPointCount-1;nPoint++)
            {
                drawing::Position3D aPoint1, aPoint2;
                aPoint1 = aPoly[nPoly][nPoint+1];
                aPoint2 = aPoly[nPoly][nPoint];

                ShapeFactory::createStripe(xSeriesGroupShape_Shapes
                    , Stripe( aPoint1, aPoint2, fDepth )
                    , pSeries->getPropertiesOfSeries(), PropertyMapper::getPropertyNameMapForFilledSeriesProperties(), true, 1 );
            }
        }
    }
    else //m_nDimension!=3
    {
        xShape = ShapeFactory::createLine2D( xSeriesGroupShape_Shapes, aPoly );
        PropertyMapper::setMappedProperties( *xShape
                , pSeries->getPropertiesOfSeries()
                , PropertyMapper::getPropertyNameMapForLineSeriesProperties() );
        //because of this name this line will be used for marking
        ::chart::ShapeFactory::setShapeName(xShape, u"MarkHandles"_ustr);
    }
    return true;
}

bool AreaChart::impl_createArea( VDataSeries* pSeries
                , std::vector<std::vector<css::drawing::Position3D>> const * pSeriesPoly
                , std::vector<std::vector<css::drawing::Position3D>> const * pPreviousSeriesPoly
                , PlottingPositionHelper const * pPosHelper )
{
    //return true if an area was created successfully

    rtl::Reference<SvxShapeGroupAnyD> xSeriesGroupShape_Shapes = getSeriesGroupShapeBackChild(pSeries, m_xSeriesTarget);
    double zValue = pSeries->m_fLogicZPos;

    std::vector<std::vector<css::drawing::Position3D>> aPoly( *pSeriesPoly );
    //add second part to the polygon (grounding points or previous series points)
    if(!pPreviousSeriesPoly)
    {
        double fMinX = pSeries->m_fLogicMinX;
        double fMaxX = pSeries->m_fLogicMaxX;
        double fY = pPosHelper->getBaseValueY();//logic grounding
        if( m_nDimension==3 )
            fY = pPosHelper->getLogicMinY();

        //clip to scale
        if(fMaxX<pPosHelper->getLogicMinX() || fMinX>pPosHelper->getLogicMaxX())
            return false;//no visible shape needed
        pPosHelper->clipLogicValues( &fMinX, &fY, nullptr );
        pPosHelper->clipLogicValues( &fMaxX, nullptr, nullptr );

        //apply scaling
        {
            pPosHelper->doLogicScaling( &fMinX, &fY, &zValue );
            pPosHelper->doLogicScaling( &fMaxX, nullptr, nullptr );
        }

        AddPointToPoly( aPoly, drawing::Position3D( fMaxX,fY,zValue) );
        AddPointToPoly( aPoly, drawing::Position3D( fMinX,fY,zValue) );
    }
    else
    {
        appendPoly( aPoly, *pPreviousSeriesPoly );
    }
    ShapeFactory::closePolygon(aPoly);

    //apply clipping
    {
        std::vector<std::vector<css::drawing::Position3D>> aClippedPoly;
        Clipping::clipPolygonAtRectangle( aPoly, pPosHelper->getScaledLogicClipDoubleRect(), aClippedPoly, false );
        ShapeFactory::closePolygon(aClippedPoly); //again necessary after clipping
        aPoly = std::move(aClippedPoly);
    }

    if(!ShapeFactory::hasPolygonAnyLines(aPoly))
        return false;

    //transformation 3) -> 4)
    pPosHelper->transformScaledLogicToScene( aPoly );

    //create area:
    rtl::Reference< SvxShape > xShape;
    if(m_nDimension==3)
    {
        xShape = ShapeFactory::createArea3D( xSeriesGroupShape_Shapes
                , aPoly, getTransformedDepth() );
    }
    else //m_nDimension!=3
    {
        xShape = ShapeFactory::createArea2D( xSeriesGroupShape_Shapes
                , aPoly );
    }
    PropertyMapper::setMappedProperties( *xShape
                , pSeries->getPropertiesOfSeries()
                , PropertyMapper::getPropertyNameMapForFilledSeriesProperties() );
    //because of this name this line will be used for marking
    ::chart::ShapeFactory::setShapeName(xShape, u"MarkHandles"_ustr);
    return true;
}

void AreaChart::impl_createSeriesShapes()
{
    //the polygon shapes for each series need to be created before

    //iterate through all series again to create the series shapes
    for( auto const& rZSlot : m_aZSlots )
    {
        for( auto const& rXSlot : rZSlot )
        {
            std::map< sal_Int32, std::vector<std::vector<css::drawing::Position3D>>* > aPreviousSeriesPolyMap;//a PreviousSeriesPoly for each different nAttachedAxisIndex
            std::vector<std::vector<css::drawing::Position3D>>* pSeriesPoly = nullptr;

            //iterate through all series
            for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
            {
                sal_Int32 nAttachedAxisIndex = pSeries->getAttachedAxisIndex();
                PlottingPositionHelper& rPosHelper = getPlottingPositionHelper(nAttachedAxisIndex);
                m_pPosHelper = &rPosHelper;

                createRegressionCurvesShapes( *pSeries, m_xErrorBarTarget, m_xRegressionCurveEquationTarget,
                                              m_pPosHelper->maySkipPointsInRegressionCalculation());

                pSeriesPoly = &pSeries->m_aPolyPolygonShape3D;
                if( m_bArea )
                {
                    if (!impl_createArea(pSeries.get(), pSeriesPoly,
                                         aPreviousSeriesPolyMap[nAttachedAxisIndex], &rPosHelper))
                        continue;
                }
                if( m_bLine )
                {
                    if (!impl_createLine(pSeries.get(), pSeriesPoly, &rPosHelper))
                        continue;
                }
                aPreviousSeriesPolyMap[nAttachedAxisIndex] = pSeriesPoly;
            }//next series in x slot (next y slot)
        }//next x slot
    }//next z slot
}

namespace
{

void lcl_reorderSeries( std::vector< std::vector< VDataSeriesGroup > >&  rZSlots )
{
    std::vector< std::vector< VDataSeriesGroup > >  aRet;
    aRet.reserve( rZSlots.size() );

    std::vector< std::vector< VDataSeriesGroup > >::reverse_iterator aZIt( rZSlots.rbegin() );
    std::vector< std::vector< VDataSeriesGroup > >::reverse_iterator aZEnd( rZSlots.rend() );
    for( ; aZIt != aZEnd; ++aZIt )
    {
        std::vector< VDataSeriesGroup > aXSlot;
        aXSlot.reserve( aZIt->size() );

        std::vector< VDataSeriesGroup >::reverse_iterator aXIt( aZIt->rbegin() );
        std::vector< VDataSeriesGroup >::reverse_iterator aXEnd( aZIt->rend() );
        for( ; aXIt != aXEnd; ++aXIt )
            aXSlot.push_back(std::move(*aXIt));

        aRet.push_back(std::move(aXSlot));
    }

    rZSlots = std::move(aRet);
}

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

}//anonymous namespace

void AreaChart::createShapes()
{
    if( m_aZSlots.empty() ) //no series
        return;

    //tdf#127813 Don't reverse the series in OOXML-heavy environments
    if( officecfg::Office::Compatibility::View::ReverseSeriesOrderAreaAndNetChart::get() && m_nDimension == 2 && ( m_bArea || !m_bCategoryXAxis ) )
        lcl_reorderSeries( m_aZSlots );

    OSL_ENSURE(m_xLogicTarget.is()&&m_xFinalTarget.is(),"AreaChart is not proper initialized");
    if(!(m_xLogicTarget.is()&&m_xFinalTarget.is()))
        return;

    //the text labels should be always on top of the other series shapes
    //for area chart the error bars should be always on top of the other series shapes

    //therefore create an own group for the texts and the error bars to move them to front
    //(because the text group is created after the series group the texts are displayed on top)
    m_xSeriesTarget   = createGroupShape( m_xLogicTarget );
    if( m_bArea )
        m_xErrorBarTarget = createGroupShape( m_xLogicTarget );
    else
        m_xErrorBarTarget = m_xSeriesTarget;
    m_xTextTarget     = ShapeFactory::createGroup2D( m_xFinalTarget );
    m_xRegressionCurveEquationTarget = ShapeFactory::createGroup2D( m_xFinalTarget );

    //check necessary here that different Y axis can not be stacked in the same group? ... hm?

    //update/create information for current group
    double fLogicZ        = 1.0;//as defined

    sal_Int32 nStartIndex = 0; // inclusive       ;..todo get somehow from x scale
    sal_Int32 nEndIndex = VSeriesPlotter::getPointCount();
    if(nEndIndex<=0)
        nEndIndex=1;

    //better performance for big data
    std::map< VDataSeries*, FormerPoint > aSeriesFormerPointMap;
    m_bPointsWereSkipped = false;
    sal_Int32 nSkippedPoints = 0;
    sal_Int32 nCreatedPoints = 0;

    bool bDateCategory = (m_pExplicitCategoriesProvider && m_pExplicitCategoriesProvider->isDateAxis());

    // indexed by {nIndex, nAttachedAxisIndex}
    std::map< std::pair<sal_Int32, sal_Int32>, double > aLogicYSumMapByX;
    for( auto const& rZSlot : m_aZSlots )
    {
        //iterate through all x slots in this category to get 100percent sum
        for( auto const& rXSlot : rZSlot )
        {
            for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
            {
                if(!pSeries)
                    continue;

                if (bDateCategory)
                    pSeries->doSortByXValues();

                sal_Int32 nAttachedAxisIndex = pSeries->getAttachedAxisIndex();
                for( sal_Int32 nIndex = nStartIndex; nIndex < nEndIndex; nIndex++ )
                {
                    double fAdd = pSeries->getYValue( nIndex );
                    if( !std::isnan(fAdd) && !std::isinf(fAdd) )
                        aLogicYSumMapByX[ {nIndex, nAttachedAxisIndex} ] += fabs( fAdd );
                }
            }
        }
    }

    const bool bUseErrorRectangle = officecfg::Office::Chart::ErrorProperties::ErrorRectangle::get();

    sal_Int32 nZ=1;
    for( auto const& rZSlot : m_aZSlots )
    {
        //for the area chart there should be at most one x slot (no side by side stacking available)
        //attention different: xSlots are always interpreted as independent areas one behind the other: @todo this doesn't work why not???
        for( auto const& rXSlot : rZSlot )
        {
            std::vector<std::map< sal_Int32, double > > aLogicYForNextSeriesMapByX(nEndIndex); //one for each different nAttachedAxisIndex
            //iterate through all series
            for( std::unique_ptr<VDataSeries> const & pSeries : rXSlot.m_aSeriesVector )
            {
                if(!pSeries)
                    continue;

                rtl::Reference<SvxShapeGroupAnyD> xSeriesGroupShape_Shapes = getSeriesGroupShapeFrontChild(pSeries.get(), m_xSeriesTarget);

                sal_Int32 nAttachedAxisIndex = pSeries->getAttachedAxisIndex();
                double fXMin, fXMax;
                pSeries->getMinMaxXValue(fXMin, fXMax);
                PlottingPositionHelper& rPosHelper = getPlottingPositionHelper(nAttachedAxisIndex);
                m_pPosHelper = &rPosHelper;

                if(m_nDimension==3)
                    fLogicZ = nZ+0.5;
                pSeries->m_fLogicZPos = fLogicZ;

                for( sal_Int32 nIndex = nStartIndex; nIndex < nEndIndex; nIndex++ )
                {

                    /*  #i70133# ignore points outside of series length in standard area
                        charts. Stacked area charts will use missing points as zeros. In
                        standard charts, pSeriesList contains only one series. */
                    if( m_bArea && (rXSlot.m_aSeriesVector.size() == 1) && (nIndex >= pSeries->getTotalPointCount()) )
                        continue;

                    //collect data point information (logic coordinates, style ):
                    double fLogicX = pSeries->getXValue(nIndex);
                    if (bDateCategory)
                    {
                        if (std::isnan(fLogicX) || (fLogicX < fXMin || fLogicX > fXMax))
                            continue;

                        fLogicX = DateHelper::RasterizeDateValue( fLogicX, m_aNullDate, m_nTimeResolution );
                    }
                    double fLogicY = pSeries->getYValue(nIndex);

                    if( m_nDimension==3 && m_bArea && rXSlot.m_aSeriesVector.size()!=1 )
                        fLogicY = fabs( fLogicY );

                    double fLogicValueForLabeDisplay = fLogicY;
                    double fLogicSumForX = 0.0;
                    auto it = aLogicYSumMapByX.find({nIndex, nAttachedAxisIndex});
                    if (it != aLogicYSumMapByX.end())
                        fLogicSumForX = it->second;
                    if (rPosHelper.isPercentY() && fLogicSumForX != 0.0)
                        fLogicY = fabs( fLogicY ) / fLogicSumForX;

                    if(    std::isnan(fLogicX) || std::isinf(fLogicX)
                            || std::isnan(fLogicY) || std::isinf(fLogicY)
                            || std::isnan(fLogicZ) || std::isinf(fLogicZ) )
                    {
                        if( pSeries->getMissingValueTreatment() == css::chart::MissingValueTreatment::LEAVE_GAP )
                        {
                            std::vector<std::vector<css::drawing::Position3D>>& rPolygon = pSeries->m_aPolyPolygonShape3D;
                            sal_Int32& rIndex = pSeries->m_nPolygonIndex;
                            if( 0<= rIndex && o3tl::make_unsigned(rIndex) < rPolygon.size() )
                            {
                                if( !rPolygon[ rIndex ].empty() )
                                    rIndex++; //start a new polygon for the next point if the current poly is not empty
                            }
                        }
                        continue;
                    }

                    std::map< sal_Int32, double >& rLogicYForNextSeriesMap = aLogicYForNextSeriesMapByX[nIndex];
                    rLogicYForNextSeriesMap.try_emplace(nAttachedAxisIndex, 0.0);

                    double fPreviousYValue = rLogicYForNextSeriesMap[nAttachedAxisIndex];
                    fLogicY += rLogicYForNextSeriesMap[nAttachedAxisIndex];
                    rLogicYForNextSeriesMap[nAttachedAxisIndex] = fLogicY;

                    bool bIsVisible = rPosHelper.isLogicVisible(fLogicX, fLogicY, fLogicZ);

                    //remind minimal and maximal x values for area 'grounding' points
                    //only for filled area
                    {
                        double& rfMinX = pSeries->m_fLogicMinX;
                        if(!nIndex||fLogicX<rfMinX)
                            rfMinX=fLogicX;
                        double& rfMaxX = pSeries->m_fLogicMaxX;
                        if(!nIndex||fLogicX>rfMaxX)
                            rfMaxX=fLogicX;
                    }

                    drawing::Position3D aUnscaledLogicPosition( fLogicX, fLogicY, fLogicZ );
                    drawing::Position3D aScaledLogicPosition(aUnscaledLogicPosition);
                    rPosHelper.doLogicScaling(aScaledLogicPosition);

                    //transformation 3) -> 4)
                    drawing::Position3D aScenePosition(
                        rPosHelper.transformLogicToScene(fLogicX, fLogicY, fLogicZ, false));

                    //better performance for big data
                    FormerPoint aFormerPoint( aSeriesFormerPointMap[pSeries.get()] );
                    rPosHelper.setCoordinateSystemResolution(m_aCoordinateSystemResolution);
                    if (!pSeries->isAttributedDataPoint(nIndex)
                        && rPosHelper.isSameForGivenResolution(
                               aFormerPoint.m_fX, aFormerPoint.m_fY, aFormerPoint.m_fZ,
                               aScaledLogicPosition.PositionX, aScaledLogicPosition.PositionY,
                               aScaledLogicPosition.PositionZ))
                    {
                        ++nSkippedPoints;
                        m_bPointsWereSkipped = true;
                        continue;
                    }
                    aSeriesFormerPointMap[pSeries.get()] = FormerPoint(aScaledLogicPosition.PositionX, aScaledLogicPosition.PositionY, aScaledLogicPosition.PositionZ);

                    //store point information for series polygon
                    //for area and/or line (symbols only do not need this)
                    if( isValidPosition(aScaledLogicPosition) )
                    {
                        AddPointToPoly( pSeries->m_aPolyPolygonShape3D, aScaledLogicPosition, pSeries->m_nPolygonIndex );
                    }

                    //create a single datapoint if point is visible
                    //apply clipping:
                    if( !bIsVisible )
                        continue;

                    bool bCreateYErrorBar = false, bCreateXErrorBar = false;
                    {
                        uno::Reference< beans::XPropertySet > xErrorBarProp(pSeries->getYErrorBarProperties(nIndex));
                        if( xErrorBarProp.is() )
                        {
                            bool bShowPositive = false;
                            bool bShowNegative = false;
                            xErrorBarProp->getPropertyValue(u"ShowPositiveError"_ustr) >>= bShowPositive;
                            xErrorBarProp->getPropertyValue(u"ShowNegativeError"_ustr) >>= bShowNegative;
                            bCreateYErrorBar = bShowPositive || bShowNegative;
                        }

                        xErrorBarProp = pSeries->getXErrorBarProperties(nIndex);
                        if ( xErrorBarProp.is() )
                        {
                            bool bShowPositive = false;
                            bool bShowNegative = false;
                            xErrorBarProp->getPropertyValue(u"ShowPositiveError"_ustr) >>= bShowPositive;
                            xErrorBarProp->getPropertyValue(u"ShowNegativeError"_ustr) >>= bShowNegative;
                            bCreateXErrorBar = bShowPositive || bShowNegative;
                        }
                    }

                    Symbol* pSymbolProperties = m_bSymbol ? pSeries->getSymbolProperties( nIndex ) : nullptr;
                    bool bCreateSymbol = pSymbolProperties && (pSymbolProperties->Style != SymbolStyle_NONE);

                    if( !bCreateSymbol && !bCreateYErrorBar &&
                            !bCreateXErrorBar && !pSeries->getDataPointLabelIfLabel(nIndex) )
                        continue;

                    {
                        nCreatedPoints++;

                        //create data point
                        drawing::Direction3D aSymbolSize(0,0,0);
                        if( bCreateSymbol )
                        {
                            if(m_nDimension!=3)
                            {
                                //create a group shape for this point and add to the series shape:
                                OUString aPointCID = ObjectIdentifier::createPointCID(
                                        pSeries->getPointCID_Stub(), nIndex );
                                rtl::Reference<SvxShapeGroupAnyD> xPointGroupShape_Shapes;
                                if (pSymbolProperties->Style == SymbolStyle_STANDARD || pSymbolProperties->Style == SymbolStyle_GRAPHIC)
                                    xPointGroupShape_Shapes = createGroupShape(xSeriesGroupShape_Shapes,aPointCID);

                                if (pSymbolProperties->Style != SymbolStyle_NONE)
                                {
                                    aSymbolSize.DirectionX = pSymbolProperties->Size.Width;
                                    aSymbolSize.DirectionY = pSymbolProperties->Size.Height;
                                }

                                if (pSymbolProperties->Style == SymbolStyle_STANDARD)
                                {
                                    sal_Int32 nSymbol = pSymbolProperties->StandardSymbol;
                                    ShapeFactory::createSymbol2D(
                                        xPointGroupShape_Shapes, aScenePosition, aSymbolSize,
                                        nSymbol, pSymbolProperties->BorderColor,
                                        pSymbolProperties->FillColor);
                                }
                                else if (pSymbolProperties->Style == SymbolStyle_GRAPHIC)
                                {
                                    ShapeFactory::createGraphic2D(xPointGroupShape_Shapes,
                                                                     aScenePosition, aSymbolSize,
                                                                     pSymbolProperties->Graphic);
                                }
                                //@todo other symbol styles
                            }
                        }
                        //create error bars or rectangles, depending on configuration
                        if ( bUseErrorRectangle )
                        {
                            if ( bCreateXErrorBar || bCreateYErrorBar )
                            {
                                createErrorRectangle(
                                      aUnscaledLogicPosition,
                                      *pSeries,
                                      nIndex,
                                      m_xErrorBarTarget,
                                      bCreateXErrorBar,
                                      bCreateYErrorBar );
                            }
                        }
                        else
                        {
                            if (bCreateXErrorBar)
                                createErrorBar_X( aUnscaledLogicPosition, *pSeries, nIndex, m_xErrorBarTarget );

                            if (bCreateYErrorBar)
                                createErrorBar_Y( aUnscaledLogicPosition, *pSeries, nIndex, m_xErrorBarTarget, nullptr );
                        }

                        //create data point label
                        if( pSeries->getDataPointLabelIfLabel(nIndex) )
                        {
                            LabelAlignment eAlignment = LABEL_ALIGN_TOP;
                            sal_Int32 nLabelPlacement = pSeries->getLabelPlacement(
                                nIndex, m_xChartTypeModel, rPosHelper.isSwapXAndY());

                            if (m_bArea && nLabelPlacement == css::chart::DataLabelPlacement::CENTER)
                            {
                                if (fPreviousYValue)
                                    fLogicY -= (fLogicY - fPreviousYValue) / 2.0;
                                else
                                    fLogicY = (fLogicY + rPosHelper.getLogicMinY()) / 2.0;
                                aScenePosition = rPosHelper.transformLogicToScene(fLogicX, fLogicY, fLogicZ, false);
                            }

                            drawing::Position3D aScenePosition3D( aScenePosition.PositionX
                                    , aScenePosition.PositionY
                                    , aScenePosition.PositionZ+getTransformedDepth() );

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

                            awt::Point aScreenPosition2D;//get the screen position for the labels
                            sal_Int32 nOffset = 100; //todo maybe calculate this font height dependent
                            {
                                if(eAlignment==LABEL_ALIGN_CENTER || m_nDimension == 3 )
                                    nOffset = 0;
                                aScreenPosition2D = LabelPositionHelper(m_nDimension,m_xLogicTarget)
                                        .transformSceneToScreenPosition( aScenePosition3D );
                            }

                            createDataLabel( m_xTextTarget, *pSeries, nIndex
                                    , fLogicValueForLabeDisplay
                                    , fLogicSumForX, aScreenPosition2D, eAlignment, nOffset );
                        }
                    }
                }

            }//next series in x slot (next y slot)
        }//next x slot
        ++nZ;
    }//next z slot

    impl_createSeriesShapes();

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

    SAL_INFO(
        "chart2",
        "skipped points: " << nSkippedPoints << " created points: "
            << nCreatedPoints);
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

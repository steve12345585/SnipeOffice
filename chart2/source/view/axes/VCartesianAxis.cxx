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

#include "VCartesianAxis.hxx"
#include <PlottingPositionHelper.hxx>
#include <ShapeFactory.hxx>
#include <PropertyMapper.hxx>
#include <NumberFormatterWrapper.hxx>
#include <LabelPositionHelper.hxx>
#include <BaseGFXHelper.hxx>
#include <Axis.hxx>
#include <AxisHelper.hxx>
#include "Tickmarks_Equidistant.hxx"
#include <ExplicitCategoriesProvider.hxx>
#include <com/sun/star/chart2/AxisType.hpp>
#include <o3tl/safeint.hxx>
#include <rtl/math.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/color.hxx>
#include <svl/numuno.hxx>
#include <svx/unoshape.hxx>
#include <svx/unoshtxt.hxx>
#include <VSeriesPlotter.hxx>
#include <DataTableView.hxx>
#include <ChartModel.hxx>

#include <comphelper/scopeguard.hxx>

#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygonclipper.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/numeric/ftools.hxx>

#include <algorithm>
#include <limits>
#include <memory>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Reference;
using ::basegfx::B2DVector;
using ::basegfx::B2DPolygon;
using ::basegfx::B2DPolyPolygon;

namespace chart {

VCartesianAxis::VCartesianAxis( const AxisProperties& rAxisProperties
            , const rtl::Reference< SvNumberFormatsSupplierObj >& xNumberFormatsSupplier
            , sal_Int32 nDimensionIndex, sal_Int32 nDimensionCount
            , PlottingPositionHelper* pPosHelper )//takes ownership
            : VAxisBase( nDimensionIndex, nDimensionCount, rAxisProperties, xNumberFormatsSupplier )
{
    if( pPosHelper )
        m_pPosHelper = pPosHelper;
    else
        m_pPosHelper = new PlottingPositionHelper();
}

VCartesianAxis::~VCartesianAxis()
{
    delete m_pPosHelper;
    m_pPosHelper = nullptr;
}

static void lcl_ResizeTextShapeToFitAvailableSpace( SvxShapeText& rShape2DText,
                                             const AxisLabelProperties& rAxisLabelProperties,
                                             std::u16string_view rLabel,
                                             const tNameSequence& rPropNames,
                                             const tAnySequence& rPropValues,
                                             const bool bIsHorizontalAxis )
{
    bool bTextHorizontal = rAxisLabelProperties.m_fRotationAngleDegree != 0.0;
    bool bIsDirectionVertical = bIsHorizontalAxis && bTextHorizontal;
    const sal_Int32 nFullSize = bIsDirectionVertical ? rAxisLabelProperties.m_aFontReferenceSize.Height : rAxisLabelProperties.m_aFontReferenceSize.Width;

    if( !nFullSize || rLabel.empty() )
        return;

    const sal_Int32 nAvgCharWidth = rShape2DText.getSize().Width / rLabel.size();

    sal_Int32 nMaxLabelsSize = bIsDirectionVertical ? rAxisLabelProperties.m_aMaximumSpaceForLabels.Height : rAxisLabelProperties.m_aMaximumSpaceForLabels.Width;

    awt::Size aSizeAfterRotation = ShapeFactory::getSizeAfterRotation(rShape2DText, rAxisLabelProperties.m_fRotationAngleDegree);

    const sal_Int32 nTextSize = bIsDirectionVertical ? aSizeAfterRotation.Height : aSizeAfterRotation.Width;

    if( !nAvgCharWidth )
        return;

    static constexpr OUString sDots = u"..."_ustr;
    const sal_Int32 nCharsToRemove = ( nTextSize - nMaxLabelsSize ) / nAvgCharWidth + 1;
    sal_Int32 nNewLen = rLabel.size() - nCharsToRemove - sDots.getLength();
    // Prevent from showing only dots
    if (nNewLen < 0)
        nNewLen = ( sal_Int32(rLabel.size()) >= sDots.getLength() ) ? sDots.getLength() : rLabel.size();

    bool bCrop = nCharsToRemove > 0;
    if( !bCrop )
        return;

    OUString aNewLabel( rLabel.substr( 0, nNewLen ) );
    if( nNewLen > sDots.getLength() )
        aNewLabel += sDots;
    rShape2DText.setString( aNewLabel );

    PropertyMapper::setMultiProperties( rPropNames, rPropValues, rShape2DText );
}

static rtl::Reference<SvxShapeText> createSingleLabel(
            const rtl::Reference< SvxShapeGroupAnyD >& xTarget
          , const awt::Point& rAnchorScreenPosition2D
          , const OUString& rLabel
          , const AxisLabelProperties& rAxisLabelProperties
          , const AxisProperties& rAxisProperties
          , const tNameSequence& rPropNames
          , const tAnySequence& rPropValues
          , const bool bIsHorizontalAxis
          )
{
    if(rLabel.isEmpty())
        return nullptr;

    // #i78696# use mathematically correct rotation now
    const double fRotationAnglePi(-basegfx::deg2rad(rAxisLabelProperties.m_fRotationAngleDegree));
    uno::Any aATransformation = ShapeFactory::makeTransformation( rAnchorScreenPosition2D, fRotationAnglePi );
    OUString aLabel = ShapeFactory::getStackedString( rLabel, rAxisLabelProperties.m_bStackCharacters );

    rtl::Reference<SvxShapeText> xShape2DText =
                    ShapeFactory::createText( xTarget, aLabel, rPropNames, rPropValues, aATransformation );

    if( rAxisProperties.m_bLimitSpaceForLabels )
        lcl_ResizeTextShapeToFitAvailableSpace(*xShape2DText, rAxisLabelProperties, aLabel, rPropNames, rPropValues, bIsHorizontalAxis);

    LabelPositionHelper::correctPositionForRotation( xShape2DText
        , rAxisProperties.maLabelAlignment.meAlignment, rAxisLabelProperties.m_fRotationAngleDegree, rAxisProperties.m_bComplexCategories );

    return xShape2DText;
}

static bool lcl_doesShapeOverlapWithTickmark( SvxShape& rShape
                       , double fRotationAngleDegree
                       , const basegfx::B2DVector& rTickScreenPosition )
{
    ::basegfx::B2IRectangle aShapeRect = BaseGFXHelper::makeRectangle(rShape.getPosition(), ShapeFactory::getSizeAfterRotation( rShape, fRotationAngleDegree ));

    basegfx::B2IVector aPosition(
        static_cast<sal_Int32>( rTickScreenPosition.getX() )
        , static_cast<sal_Int32>( rTickScreenPosition.getY() ) );
    return aShapeRect.isInside(aPosition);
}

static void lcl_getRotatedPolygon( B2DPolygon &aPoly, const ::basegfx::B2DRectangle &aRect, const awt::Point &aPos, const double fRotationAngleDegree )
{
    aPoly = basegfx::utils::createPolygonFromRect( aRect );

    // For rotating the rectangle we use the opposite angle,
    // since `B2DHomMatrix` class used for
    // representing the transformation, performs rotations in the positive
    // direction (from the X axis to the Y axis). However since the coordinate
    // system used by the chart has the Y-axis pointing downward, a rotation in
    // the positive direction means a clockwise rotation. On the contrary text
    // labels are rotated counterclockwise.
    // The rotation is performed around the top-left vertex of the rectangle
    // which is then moved to its final position by using the top-left
    // vertex of the text label bounding box (aPos) as the translation vector.
    ::basegfx::B2DHomMatrix aMatrix;
    aMatrix.rotate(-basegfx::deg2rad(fRotationAngleDegree));
    aMatrix.translate( aPos.X, aPos.Y);
    aPoly.transform( aMatrix );
}

static bool doesOverlap( const rtl::Reference<SvxShapeText>& xShape1
                , const rtl::Reference<SvxShapeText>& xShape2
                , double fRotationAngleDegree )
{
    if( !xShape1.is() || !xShape2.is() )
        return false;

    ::basegfx::B2DRectangle aRect1( BaseGFXHelper::makeRectangle( awt::Point(0,0), xShape1->getSize()));
    ::basegfx::B2DRectangle aRect2( BaseGFXHelper::makeRectangle( awt::Point(0,0), xShape2->getSize()));

    B2DPolygon aPoly1;
    B2DPolygon aPoly2;
    lcl_getRotatedPolygon( aPoly1, aRect1, xShape1->getPosition(), fRotationAngleDegree );
    lcl_getRotatedPolygon( aPoly2, aRect2, xShape2->getPosition(), fRotationAngleDegree );

    B2DPolyPolygon aPolyPoly1, aPolyPoly2;
    aPolyPoly1.append( aPoly1 );
    aPolyPoly2.append( aPoly2 );
    B2DPolyPolygon overlapPoly = ::basegfx::utils::clipPolyPolygonOnPolyPolygon( aPolyPoly1, aPolyPoly2, true, false );

    return (overlapPoly.count() > 0);
}

static void removeShapesAtWrongRhythm( TickIter& rIter
                              , sal_Int32 nCorrectRhythm
                              , sal_Int32 nMaxTickToCheck
                              , const rtl::Reference< SvxShapeGroupAnyD >& xTarget )
{
    sal_Int32 nTick = 0;
    for( TickInfo* pTickInfo = rIter.firstInfo()
        ; pTickInfo && nTick <= nMaxTickToCheck
        ; pTickInfo = rIter.nextInfo(), nTick++ )
    {
        //remove labels which does not fit into the rhythm
        if( nTick%nCorrectRhythm != 0)
        {
            if(pTickInfo->xTextShape.is())
            {
                xTarget->remove(pTickInfo->xTextShape);
                pTickInfo->xTextShape = nullptr;
            }
        }
    }
}

namespace {

/**
 * If the labels are staggered and bInnerLine is true we iterate through
 * only those labels that are closer to the diagram.
 *
 * If the labels are staggered and bInnerLine is false we iterate through
 * only those that are farther from the diagram.
 *
 * If the labels are not staggered we iterate through all labels.
 */
class LabelIterator : public TickIter
{
public:
    LabelIterator( TickInfoArrayType& rTickInfoVector
            , const AxisLabelStaggering eAxisLabelStaggering
            , bool bInnerLine );

    virtual TickInfo*   firstInfo() override;
    virtual TickInfo*   nextInfo() override;

private: //member
    PureTickIter m_aPureTickIter;
    const AxisLabelStaggering   m_eAxisLabelStaggering;
    bool m_bInnerLine;
};

}

LabelIterator::LabelIterator( TickInfoArrayType& rTickInfoVector
            , const AxisLabelStaggering eAxisLabelStaggering
            , bool bInnerLine )
            : m_aPureTickIter( rTickInfoVector )
            , m_eAxisLabelStaggering(eAxisLabelStaggering)
            , m_bInnerLine(bInnerLine)
{
}

TickInfo* LabelIterator::firstInfo()
{
    TickInfo* pTickInfo = m_aPureTickIter.firstInfo();
    while( pTickInfo && !pTickInfo->xTextShape.is() )
        pTickInfo = m_aPureTickIter.nextInfo();
    if(!pTickInfo)
        return nullptr;
    if( (m_eAxisLabelStaggering==AxisLabelStaggering::StaggerEven && m_bInnerLine)
        ||
        (m_eAxisLabelStaggering==AxisLabelStaggering::StaggerOdd && !m_bInnerLine)
        )
    {
        //skip first label
        do
            pTickInfo = m_aPureTickIter.nextInfo();
        while( pTickInfo && !pTickInfo->xTextShape.is() );
    }
    if(!pTickInfo)
        return nullptr;
    return pTickInfo;
}

TickInfo* LabelIterator::nextInfo()
{
    TickInfo* pTickInfo = nullptr;
    //get next label
    do
        pTickInfo = m_aPureTickIter.nextInfo();
    while( pTickInfo && !pTickInfo->xTextShape.is() );

    if(  m_eAxisLabelStaggering==AxisLabelStaggering::StaggerEven
      || m_eAxisLabelStaggering==AxisLabelStaggering::StaggerOdd )
    {
        //skip one label
        do
            pTickInfo = m_aPureTickIter.nextInfo();
        while( pTickInfo && !pTickInfo->xTextShape.is() );
    }
    return pTickInfo;
}

static B2DVector lcl_getLabelsDistance( TickIter& rIter, const B2DVector& rDistanceTickToText, double fRotationAngleDegree )
{
    //calculates the height or width of a line of labels
    //thus a following line of labels can be shifted for that distance

    B2DVector aRet(0,0);

    sal_Int32 nDistanceTickToText = static_cast<sal_Int32>( rDistanceTickToText.getLength() );
    if( nDistanceTickToText==0.0)
        return aRet;

    B2DVector aStaggerDirection(rDistanceTickToText);
    aStaggerDirection.normalize();

    sal_Int32 nDistance=0;
    rtl::Reference< SvxShapeText >  xShape2DText;
    for( TickInfo* pTickInfo = rIter.firstInfo()
        ; pTickInfo
        ; pTickInfo = rIter.nextInfo() )
    {
        xShape2DText = pTickInfo->xTextShape;
        if( xShape2DText.is() )
        {
            awt::Size aSize = ShapeFactory::getSizeAfterRotation( *xShape2DText, fRotationAngleDegree );
            if(fabs(aStaggerDirection.getX())>fabs(aStaggerDirection.getY()))
                nDistance = std::max(nDistance,aSize.Width);
            else
                nDistance = std::max(nDistance,aSize.Height);
        }
    }

    aRet = aStaggerDirection*nDistance;

    //add extra distance for vertical distance
    if(fabs(aStaggerDirection.getX())>fabs(aStaggerDirection.getY()))
        aRet += rDistanceTickToText;

    return aRet;
}

static void lcl_shiftLabels( TickIter& rIter, const B2DVector& rStaggerDistance )
{
    if(rStaggerDistance.getLength()==0.0)
        return;
    for( TickInfo* pTickInfo = rIter.firstInfo()
        ; pTickInfo
        ; pTickInfo = rIter.nextInfo() )
    {
        const rtl::Reference<SvxShapeText>& xShape2DText = pTickInfo->xTextShape;
        if( xShape2DText.is() )
        {
            awt::Point aPos  = xShape2DText->getPosition();
            aPos.X += static_cast<sal_Int32>(rStaggerDistance.getX());
            aPos.Y += static_cast<sal_Int32>(rStaggerDistance.getY());
            xShape2DText->setPosition( aPos );
        }
    }
}

static bool lcl_hasWordBreak( const rtl::Reference<SvxShapeText>& xShape )
{
    if (!xShape.is())
        return false;

    SvxTextEditSource* pTextEditSource = dynamic_cast<SvxTextEditSource*>(xShape->GetEditSource());
    if (!pTextEditSource)
        return false;

    pTextEditSource->UpdateOutliner();
    SvxTextForwarder* pTextForwarder = pTextEditSource->GetTextForwarder();
    if (!pTextForwarder)
        return false;

    sal_Int32 nParaCount = pTextForwarder->GetParagraphCount();
    for ( sal_Int32 nPara = 0; nPara < nParaCount; ++nPara )
    {
        sal_Int32 nLineCount = pTextForwarder->GetLineCount( nPara );
        for ( sal_Int32 nLine = 0; nLine < nLineCount; ++nLine )
        {
            sal_Int32 nLineStart = 0;
            sal_Int32 nLineEnd = 0;
            pTextForwarder->GetLineBoundaries( nLineStart, nLineEnd, nPara, nLine );
            assert(nLineStart >= 0);
            sal_Int32 nWordStart = 0;
            sal_Int32 nWordEnd = 0;
            if ( pTextForwarder->GetWordIndices( nPara, nLineStart, nWordStart, nWordEnd ) &&
                 ( nWordStart != nLineStart ) )
            {
                return true;
            }
        }
    }

    return false;
}

static OUString getTextLabelString(
    const FixedNumberFormatter& rFixedNumberFormatter, const uno::Sequence<OUString>* pCategories,
    const TickInfo* pTickInfo, bool bComplexCat, Color& rExtraColor, bool& rHasExtraColor )
{
    if (pCategories)
    {
        // This is a normal category axis.  Get the label string from the
        // label string array.
        sal_Int32 nIndex = static_cast<sal_Int32>(pTickInfo->getUnscaledTickValue()) - 1; //first category (index 0) matches with real number 1.0
        if( nIndex>=0 && nIndex<pCategories->getLength() )
            return (*pCategories)[nIndex];

        return OUString();
    }
    else if (bComplexCat)
    {
        // This is a complex category axis.  The label is stored in the tick.
        return pTickInfo->aText;
    }

    // This is a numeric axis.  Format the original tick value per number format.
    return rFixedNumberFormatter.getFormattedString(pTickInfo->getUnscaledTickValue(), rExtraColor, rHasExtraColor);
}

static void getAxisLabelProperties(
    tNameSequence& rPropNames, tAnySequence& rPropValues, const AxisProperties& rAxisProp,
    const AxisLabelProperties& rAxisLabelProp,
    sal_Int32 nLimitedSpaceForText, bool bLimitedHeight )
{
    Reference<beans::XPropertySet> xProps(rAxisProp.m_xAxisModel);

    PropertyMapper::getTextLabelMultiPropertyLists(
        xProps, rPropNames, rPropValues, false, nLimitedSpaceForText, bLimitedHeight, false);

    LabelPositionHelper::doDynamicFontResize(
        rPropValues, rPropNames, xProps, rAxisLabelProp.m_aFontReferenceSize);

    LabelPositionHelper::changeTextAdjustment(
        rPropValues, rPropNames, rAxisProp.maLabelAlignment.meAlignment);
}

namespace {

/**
 * Iterate through only 3 ticks including the one that has the longest text
 * length.  When the first tick has the longest text, it iterates through
 * the first 3 ticks.  Otherwise it iterates through 3 ticks such that the
 * 2nd tick is the one with the longest text.
 */
class MaxLabelTickIter : public TickIter
{
public:
    MaxLabelTickIter( TickInfoArrayType& rTickInfoVector, size_t nLongestLabelIndex );

    virtual TickInfo* firstInfo() override;
    virtual TickInfo* nextInfo() override;

private:
    TickInfoArrayType& m_rTickInfoVector;
    std::vector<size_t> m_aValidIndices;
    size_t m_nCurrentIndex;
};

}

MaxLabelTickIter::MaxLabelTickIter(
    TickInfoArrayType& rTickInfoVector, size_t nLongestLabelIndex ) :
    m_rTickInfoVector(rTickInfoVector), m_nCurrentIndex(0)
{
    assert(!rTickInfoVector.empty()); // should be checked by the caller.
    assert(nLongestLabelIndex < rTickInfoVector.size());

    size_t nMaxIndex = m_rTickInfoVector.size()-1;
    if (nLongestLabelIndex >= nMaxIndex-1)
        nLongestLabelIndex = 0;

    if (nLongestLabelIndex > 0)
        m_aValidIndices.push_back(nLongestLabelIndex-1);

    m_aValidIndices.push_back(nLongestLabelIndex);

    while (m_aValidIndices.size() < 3)
    {
        ++nLongestLabelIndex;
        if (nLongestLabelIndex > nMaxIndex)
            break;

        m_aValidIndices.push_back(nLongestLabelIndex);
    }
}

TickInfo* MaxLabelTickIter::firstInfo()
{
    m_nCurrentIndex = 0;
    if (m_nCurrentIndex < m_aValidIndices.size())
        return &m_rTickInfoVector[m_aValidIndices[m_nCurrentIndex]];
    return nullptr;
}

TickInfo* MaxLabelTickIter::nextInfo()
{
    m_nCurrentIndex++;
    if (m_nCurrentIndex < m_aValidIndices.size())
        return &m_rTickInfoVector[m_aValidIndices[m_nCurrentIndex]];
    return nullptr;
}

bool VCartesianAxis::isBreakOfLabelsAllowed(
    const AxisLabelProperties& rAxisLabelProperties, bool bIsHorizontalAxis, bool bIsVerticalAxis) const
{
    if( m_aTextLabels.getLength() > 100 )
        return false;
    if( !rAxisLabelProperties.m_bLineBreakAllowed )
        return false;
    if( rAxisLabelProperties.m_bStackCharacters )
        return false;
    //no break for value axis
    if( !m_bUseTextLabels )
        return false;
    if( !( rAxisLabelProperties.m_fRotationAngleDegree == 0.0 ||
           rAxisLabelProperties.m_fRotationAngleDegree == 90.0 ||
           rAxisLabelProperties.m_fRotationAngleDegree == 270.0 ) )
        return false;
    //no break for complex vertical category axis
    if( !m_aAxisProperties.m_bSwapXAndY )
        return bIsHorizontalAxis;
    else if( m_aAxisProperties.m_bSwapXAndY && !m_aAxisProperties.m_bComplexCategories )
        return bIsVerticalAxis;
    else
        return false;
}
namespace{

bool canAutoAdjustLabelPlacement(
    const AxisLabelProperties& rAxisLabelProperties, bool bIsHorizontalAxis, bool bIsVerticalAxis)
{
    // joined prerequisite checks for auto rotate and auto stagger
    if( rAxisLabelProperties.m_bOverlapAllowed )
        return false;
    if( rAxisLabelProperties.m_bLineBreakAllowed ) // auto line break may conflict with...
        return false;
    if( rAxisLabelProperties.m_fRotationAngleDegree != 0.0 )
        return false;
    // automatic adjusting labels only works for
    // horizontal axis with horizontal text
    // or vertical axis with vertical text
    if( bIsHorizontalAxis )
        return !rAxisLabelProperties.m_bStackCharacters;
    if( bIsVerticalAxis )
        return rAxisLabelProperties.m_bStackCharacters;
    return false;
}

bool isAutoStaggeringOfLabelsAllowed(
    const AxisLabelProperties& rAxisLabelProperties, bool bIsHorizontalAxis, bool bIsVerticalAxis )
{
    if( rAxisLabelProperties.m_eStaggering != AxisLabelStaggering::StaggerAuto )
        return false;
    return canAutoAdjustLabelPlacement(rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis);
}

// make clear that we check for auto rotation prerequisites
const auto& isAutoRotatingOfLabelsAllowed = canAutoAdjustLabelPlacement;

} // namespace
void VCartesianAxis::createAllTickInfosFromComplexCategories( TickInfoArraysType& rAllTickInfos, bool bShiftedPosition )
{
    //no minor tickmarks will be generated!
    //order is: inner labels first , outer labels last (that is different to all other TickIter cases)
    if(!bShiftedPosition)
    {
        rAllTickInfos.clear();
        sal_Int32 nLevel=0;
        sal_Int32 nLevelCount = m_aAxisProperties.m_pExplicitCategoriesProvider->getCategoryLevelCount();
        for( ; nLevel<nLevelCount; nLevel++ )
        {
            TickInfoArrayType aTickInfoVector;
            const std::vector<ComplexCategory>* pComplexCategories =
                m_aAxisProperties.m_pExplicitCategoriesProvider->getCategoriesByLevel(nLevel);

            if (!pComplexCategories)
                continue;

            sal_Int32 nCatIndex = 0;

            for (auto const& complexCategory : *pComplexCategories)
            {
                TickInfo aTickInfo(nullptr);
                sal_Int32 nCount = complexCategory.Count;
                if( nCatIndex + 1.0 + nCount >= m_aScale.Maximum )
                {
                    nCount = static_cast<sal_Int32>(m_aScale.Maximum - 1.0 - nCatIndex);
                    if( nCount <= 0 )
                        nCount = 1;
                }
                aTickInfo.fScaledTickValue = nCatIndex + 1.0 + nCount/2.0;
                aTickInfo.nFactorForLimitedTextWidth = nCount;
                aTickInfo.aText = complexCategory.Text;
                aTickInfoVector.push_back(aTickInfo);
                nCatIndex += nCount;
                if( nCatIndex + 1.0 >= m_aScale.Maximum )
                    break;
            }
            rAllTickInfos.push_back(aTickInfoVector);
        }
    }
    else //bShiftedPosition==false
    {
        rAllTickInfos.clear();
        sal_Int32 nLevel=0;
        sal_Int32 nLevelCount = m_aAxisProperties.m_pExplicitCategoriesProvider->getCategoryLevelCount();
        for( ; nLevel<nLevelCount; nLevel++ )
        {
            TickInfoArrayType aTickInfoVector;
            const std::vector<ComplexCategory>* pComplexCategories =
                m_aAxisProperties.m_pExplicitCategoriesProvider->getCategoriesByLevel(nLevel);
            sal_Int32 nCatIndex = 0;
            if (pComplexCategories)
            {
                for (auto const& complexCategory : *pComplexCategories)
                {
                    TickInfo aTickInfo(nullptr);
                    aTickInfo.fScaledTickValue = nCatIndex + 1.0;
                    aTickInfoVector.push_back(aTickInfo);
                    nCatIndex += complexCategory.Count;
                    if( nCatIndex + 1.0 > m_aScale.Maximum )
                        break;
                }
            }

            //fill up with single ticks until maximum scale
            while( nCatIndex + 1.0 < m_aScale.Maximum )
            {
                TickInfo aTickInfo(nullptr);
                aTickInfo.fScaledTickValue = nCatIndex + 1.0;
                aTickInfoVector.push_back(aTickInfo);
                nCatIndex ++;
                if( nLevel>0 )
                    break;
            }
            //add an additional tick at the end
            {
                TickInfo aTickInfo(nullptr);
                aTickInfo.fScaledTickValue = m_aScale.Maximum;
                aTickInfoVector.push_back(aTickInfo);
            }
            rAllTickInfos.push_back(aTickInfoVector);
        }
    }
}

void VCartesianAxis::createAllTickInfos( TickInfoArraysType& rAllTickInfos )
{
    if( isComplexCategoryAxis() )
        createAllTickInfosFromComplexCategories( rAllTickInfos, false );
    else
        VAxisBase::createAllTickInfos(rAllTickInfos);
}

TickIter* VCartesianAxis::createLabelTickIterator( sal_Int32 nTextLevel )
{
    if( nTextLevel>=0 && o3tl::make_unsigned(nTextLevel) < m_aAllTickInfos.size() )
        return new PureTickIter( m_aAllTickInfos[nTextLevel] );
    return nullptr;
}

TickIter* VCartesianAxis::createMaximumLabelTickIterator( sal_Int32 nTextLevel )
{
    if( isComplexCategoryAxis() || isDateAxis() )
    {
        return createLabelTickIterator( nTextLevel ); //mmmm maybe todo: create less than all texts here
    }
    else
    {
        if(nTextLevel==0)
        {
            if( !m_aAllTickInfos.empty() )
            {
                size_t nLongestLabelIndex = m_bUseTextLabels ? getIndexOfLongestLabel(m_aTextLabels) : 0;
                if (nLongestLabelIndex >= m_aAllTickInfos[0].size())
                    return nullptr;

                return new MaxLabelTickIter( m_aAllTickInfos[0], nLongestLabelIndex );
            }
        }
    }
    return nullptr;
}

sal_Int32 VCartesianAxis::getTextLevelCount() const
{
    sal_Int32 nTextLevelCount = 1;
    if( isComplexCategoryAxis() )
        nTextLevelCount = m_aAxisProperties.m_pExplicitCategoriesProvider->getCategoryLevelCount();
    return nTextLevelCount;
}

bool VCartesianAxis::createTextShapes(
    const rtl::Reference< SvxShapeGroupAnyD >& xTarget, TickIter& rTickIter,
    AxisLabelProperties& rAxisLabelProperties, TickFactory2D const * pTickFactory,
    sal_Int32 nScreenDistanceBetweenTicks )
{
    const bool bIsHorizontalAxis = pTickFactory->isHorizontalAxis();
    const bool bIsVerticalAxis = pTickFactory->isVerticalAxis();

    if( m_bUseTextLabels && (m_aAxisProperties.m_eLabelPos == css::chart::ChartAxisLabelPosition_NEAR_AXIS ||
        m_aAxisProperties.m_eLabelPos == css::chart::ChartAxisLabelPosition_OUTSIDE_START))
    {
        if (bIsHorizontalAxis)
        {
            rAxisLabelProperties.m_aMaximumSpaceForLabels.Y = pTickFactory->getXaxisStartPos().getY();
            rAxisLabelProperties.m_aMaximumSpaceForLabels.Height = rAxisLabelProperties.m_aFontReferenceSize.Height - rAxisLabelProperties.m_aMaximumSpaceForLabels.Y;
        }
        else if (bIsVerticalAxis)
        {
            rAxisLabelProperties.m_aMaximumSpaceForLabels.X = 0;
            rAxisLabelProperties.m_aMaximumSpaceForLabels.Width = pTickFactory->getXaxisStartPos().getX();
        }
    }

    bool bIsBreakOfLabelsAllowed = isBreakOfLabelsAllowed( rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis );
    if (!bIsBreakOfLabelsAllowed &&
        !isAutoStaggeringOfLabelsAllowed(rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis) &&
        !rAxisLabelProperties.isStaggered())
    {
        return createTextShapesSimple(xTarget, rTickIter, rAxisLabelProperties, pTickFactory);
    }

    FixedNumberFormatter aFixedNumberFormatter(
                m_xNumberFormatsSupplier, rAxisLabelProperties.m_nNumberFormatKey );

    bool bIsStaggered = rAxisLabelProperties.isStaggered();
    B2DVector aTextToTickDistance = pTickFactory->getDistanceAxisTickToText(m_aAxisProperties, true);
    sal_Int32 nLimitedSpaceForText = -1;

    if (bIsBreakOfLabelsAllowed)
    {
        if (!m_aAxisProperties.m_bLimitSpaceForLabels)
        {
            basegfx::B2DVector nDeltaVector = pTickFactory->getXaxisEndPos() - pTickFactory->getXaxisStartPos();
            nLimitedSpaceForText = nDeltaVector.getX();
        }
        if (nScreenDistanceBetweenTicks > 0)
            nLimitedSpaceForText = nScreenDistanceBetweenTicks;

        if( bIsStaggered )
            nLimitedSpaceForText *= 2;

        if( nLimitedSpaceForText > 0 )
        { //reduce space for a small amount to have a visible distance between the labels:
            sal_Int32 nReduce = (nLimitedSpaceForText*5)/100;
            if(!nReduce)
                nReduce = 1;
            nLimitedSpaceForText -= nReduce;
        }

        // recalculate the nLimitedSpaceForText in case of 90 and 270 degree if the text break is true
        if ( rAxisLabelProperties.m_fRotationAngleDegree == 90.0 || rAxisLabelProperties.m_fRotationAngleDegree == 270.0 )
        {
            nLimitedSpaceForText = rAxisLabelProperties.m_aMaximumSpaceForLabels.Height;
            m_aAxisProperties.m_bLimitSpaceForLabels = false;
        }

        // recalculate the nLimitedSpaceForText in case of vertical category axis if the text break is true
        if ( m_aAxisProperties.m_bSwapXAndY && bIsVerticalAxis && rAxisLabelProperties.m_fRotationAngleDegree == 0.0 )
        {
            nLimitedSpaceForText = pTickFactory->getXaxisStartPos().getX();
            m_aAxisProperties.m_bLimitSpaceForLabels = false;
        }
    }

    // Stores an array of text label strings in case of a normal
    // (non-complex) category axis.
    const uno::Sequence<OUString>* pCategories = nullptr;
    if( m_bUseTextLabels && !m_aAxisProperties.m_bComplexCategories )
        pCategories = &m_aTextLabels;

    bool bLimitedHeight;
    if( !m_aAxisProperties.m_bSwapXAndY )
        bLimitedHeight = fabs(aTextToTickDistance.getX()) > fabs(aTextToTickDistance.getY());
    else
        bLimitedHeight = fabs(aTextToTickDistance.getX()) < fabs(aTextToTickDistance.getY());
    //prepare properties for multipropertyset-interface of shape
    tNameSequence aPropNames;
    tAnySequence aPropValues;
    getAxisLabelProperties(aPropNames, aPropValues, m_aAxisProperties, rAxisLabelProperties, nLimitedSpaceForText, bLimitedHeight);

    uno::Any* pColorAny = PropertyMapper::getValuePointer(aPropValues,aPropNames,u"CharColor");
    Color nColor = COL_AUTO;
    if(pColorAny)
        *pColorAny >>= nColor;

    uno::Any* pLimitedSpaceAny = PropertyMapper::getValuePointerForLimitedSpace(aPropValues,aPropNames,bLimitedHeight);

    const TickInfo* pPreviousVisibleTickInfo = nullptr;
    const TickInfo* pPREPreviousVisibleTickInfo = nullptr;
    sal_Int32 nTick = 0;
    for( TickInfo* pTickInfo = rTickIter.firstInfo()
        ; pTickInfo
        ; pTickInfo = rTickIter.nextInfo(), nTick++ )
    {
        const TickInfo* pLastVisibleNeighbourTickInfo = bIsStaggered ?
                    pPREPreviousVisibleTickInfo : pPreviousVisibleTickInfo;

        //don't create labels which does not fit into the rhythm
        if( nTick%rAxisLabelProperties.m_nRhythm != 0 )
            continue;

        //don't create labels for invisible ticks
        if( !pTickInfo->bPaintIt )
            continue;

        if( pLastVisibleNeighbourTickInfo && !rAxisLabelProperties.m_bOverlapAllowed )
        {
            // Overlapping is not allowed.  If the label overlaps with its
            // neighboring label, try increasing the tick interval (or rhythm
            // as it's called) and start over.

            if( lcl_doesShapeOverlapWithTickmark( *pLastVisibleNeighbourTickInfo->xTextShape
                       , rAxisLabelProperties.m_fRotationAngleDegree
                       , pTickInfo->aTickScreenPosition ) )
            {
                // This tick overlaps with its neighbor.  Try to stagger (if
                // auto staggering is allowed) to avoid overlapping.

                bool bOverlapsAfterAutoStagger = true;
                if( !bIsStaggered && isAutoStaggeringOfLabelsAllowed( rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis ) )
                {
                    bIsStaggered = true;
                    rAxisLabelProperties.m_eStaggering = AxisLabelStaggering::StaggerEven;
                    pLastVisibleNeighbourTickInfo = pPREPreviousVisibleTickInfo;
                    if( !pLastVisibleNeighbourTickInfo ||
                        !lcl_doesShapeOverlapWithTickmark( *pLastVisibleNeighbourTickInfo->xTextShape
                                , rAxisLabelProperties.m_fRotationAngleDegree
                                , pTickInfo->aTickScreenPosition ) )
                        bOverlapsAfterAutoStagger = false;
                }

                if (bOverlapsAfterAutoStagger)
                {
                    // Still overlaps with its neighbor even after staggering.
                    // Increment the visible tick intervals (if that's
                    // allowed) and start over.

                    rAxisLabelProperties.m_nRhythm++;
                    removeShapesAtWrongRhythm( rTickIter, rAxisLabelProperties.m_nRhythm, nTick, xTarget );
                    return false;
                }
            }
        }

        bool bHasExtraColor=false;
        Color nExtraColor;

        OUString aLabel = getTextLabelString(
            aFixedNumberFormatter, pCategories, pTickInfo, isComplexCategoryAxis(),
            nExtraColor, bHasExtraColor);

        if(pColorAny)
            *pColorAny <<= bHasExtraColor?nExtraColor:nColor;
        if(pLimitedSpaceAny)
            *pLimitedSpaceAny <<= sal_Int32(nLimitedSpaceForText*pTickInfo->nFactorForLimitedTextWidth);

        B2DVector aTickScreenPos2D = pTickInfo->aTickScreenPosition;
        aTickScreenPos2D += aTextToTickDistance;
        awt::Point aAnchorScreenPosition2D(
            static_cast<sal_Int32>(aTickScreenPos2D.getX())
            ,static_cast<sal_Int32>(aTickScreenPos2D.getY()));

        //create single label
        if(!pTickInfo->xTextShape.is())
        {
            pTickInfo->xTextShape = createSingleLabel( xTarget
                                    , aAnchorScreenPosition2D, aLabel
                                    , rAxisLabelProperties, m_aAxisProperties
                                    , aPropNames, aPropValues, bIsHorizontalAxis );
        }
        if(!pTickInfo->xTextShape.is())
            continue;

        recordMaximumTextSize( *pTickInfo->xTextShape, rAxisLabelProperties.m_fRotationAngleDegree );

        // Label has multiple lines and the words are broken
        if (nLimitedSpaceForText > 0
                && !rAxisLabelProperties.m_bOverlapAllowed
                && rAxisLabelProperties.m_fRotationAngleDegree == 0.0
                && nTick > 0
                && lcl_hasWordBreak(pTickInfo->xTextShape))
        {
            // Label has multiple lines and belongs to a complex category
            // axis. Rotate 90 degrees to try to avoid overlaps.
            if ( m_aAxisProperties.m_bComplexCategories )
            {
                rAxisLabelProperties.m_fRotationAngleDegree = 90;
            }
            rAxisLabelProperties.m_bLineBreakAllowed = false;
            m_aAxisLabelProperties.m_fRotationAngleDegree = rAxisLabelProperties.m_fRotationAngleDegree;
            removeTextShapesFromTicks();
            return false;
        }

        //if NO OVERLAP -> remove overlapping shapes
        if( pLastVisibleNeighbourTickInfo && !rAxisLabelProperties.m_bOverlapAllowed )
        {
            // Check if the label still overlaps with its neighbor.
            if( doesOverlap( pLastVisibleNeighbourTickInfo->xTextShape, pTickInfo->xTextShape, rAxisLabelProperties.m_fRotationAngleDegree ) )
            {
                // It overlaps.  Check if staggering helps.
                bool bOverlapsAfterAutoStagger = true;
                if( !bIsStaggered && isAutoStaggeringOfLabelsAllowed( rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis ) )
                {
                    // Compatibility option: starting from LibreOffice 5.1 the rotated
                    // layout is preferred to staggering for axis labels.
                    if( !isAutoRotatingOfLabelsAllowed(rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis)
                        || m_aAxisProperties.m_bTryStaggeringFirst )
                    {
                        bIsStaggered = true;
                        rAxisLabelProperties.m_eStaggering = AxisLabelStaggering::StaggerEven;
                        pLastVisibleNeighbourTickInfo = pPREPreviousVisibleTickInfo;
                        if( !pLastVisibleNeighbourTickInfo ||
                            !lcl_doesShapeOverlapWithTickmark( *pLastVisibleNeighbourTickInfo->xTextShape
                                , rAxisLabelProperties.m_fRotationAngleDegree
                                , pTickInfo->aTickScreenPosition ) )
                            bOverlapsAfterAutoStagger = false;
                    }
                }

                if (bOverlapsAfterAutoStagger)
                {
                    // Staggering didn't solve the overlap.
                    if( isAutoRotatingOfLabelsAllowed(rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis) )
                    {
                        // Try auto-rotating the labels at 45 degrees and
                        // start over.  This rotation angle will be stored for
                        // all future text shape creation runs.
                        // The nRhythm parameter is reset to 1 since the layout
                        // used for text labels is changed.
                        rAxisLabelProperties.autoRotate45();
                        m_aAxisLabelProperties.m_fRotationAngleDegree = rAxisLabelProperties.m_fRotationAngleDegree; // Store it for future runs.
                        removeTextShapesFromTicks();
                        rAxisLabelProperties.m_nRhythm = 1;
                        return false;
                    }

                    // Try incrementing the tick interval and start over.
                    rAxisLabelProperties.m_nRhythm++;
                    removeShapesAtWrongRhythm( rTickIter, rAxisLabelProperties.m_nRhythm, nTick, xTarget );
                    return false;
                }
            }
        }

        pPREPreviousVisibleTickInfo = pPreviousVisibleTickInfo;
        pPreviousVisibleTickInfo = pTickInfo;
    }
    return true;
}

bool VCartesianAxis::createTextShapesSimple(
    const rtl::Reference< SvxShapeGroupAnyD >& xTarget, TickIter& rTickIter,
    AxisLabelProperties& rAxisLabelProperties, TickFactory2D const * pTickFactory )
{
    FixedNumberFormatter aFixedNumberFormatter(
                m_xNumberFormatsSupplier, rAxisLabelProperties.m_nNumberFormatKey );

    const bool bIsHorizontalAxis = pTickFactory->isHorizontalAxis();
    const bool bIsVerticalAxis = pTickFactory->isVerticalAxis();
    B2DVector aTextToTickDistance = pTickFactory->getDistanceAxisTickToText(m_aAxisProperties, true);

     // Stores an array of text label strings in case of a normal
     // (non-complex) category axis.
    const uno::Sequence<OUString>* pCategories = nullptr;
    if( m_bUseTextLabels && !m_aAxisProperties.m_bComplexCategories )
        pCategories = &m_aTextLabels;

    bool bLimitedHeight = fabs(aTextToTickDistance.getX()) > fabs(aTextToTickDistance.getY());

    //prepare properties for multipropertyset-interface of shape
    tNameSequence aPropNames;
    tAnySequence aPropValues;
    getAxisLabelProperties(aPropNames, aPropValues, m_aAxisProperties, rAxisLabelProperties, -1, bLimitedHeight);

    uno::Any* pColorAny = PropertyMapper::getValuePointer(aPropValues,aPropNames,u"CharColor");
    Color nColor = COL_AUTO;
    if(pColorAny)
        *pColorAny >>= nColor;

    uno::Any* pLimitedSpaceAny = PropertyMapper::getValuePointerForLimitedSpace(aPropValues,aPropNames,bLimitedHeight);

    const TickInfo* pPreviousVisibleTickInfo = nullptr;
    sal_Int32 nTick = 0;
    for( TickInfo* pTickInfo = rTickIter.firstInfo()
        ; pTickInfo
        ; pTickInfo = rTickIter.nextInfo(), nTick++ )
    {
        const TickInfo* pLastVisibleNeighbourTickInfo = pPreviousVisibleTickInfo;

        //don't create labels which does not fit into the rhythm
        if( nTick%rAxisLabelProperties.m_nRhythm != 0 )
            continue;

        //don't create labels for invisible ticks
        if( !pTickInfo->bPaintIt )
            continue;

        if( pLastVisibleNeighbourTickInfo && !rAxisLabelProperties.m_bOverlapAllowed )
        {
            // Overlapping is not allowed.  If the label overlaps with its
            // neighboring label, try increasing the tick interval (or rhythm
            // as it's called) and start over.

            if( lcl_doesShapeOverlapWithTickmark( *pLastVisibleNeighbourTickInfo->xTextShape
                       , rAxisLabelProperties.m_fRotationAngleDegree
                       , pTickInfo->aTickScreenPosition ) )
            {
                // This tick overlaps with its neighbor. Increment the visible
                // tick intervals (if that's allowed) and start over.

                rAxisLabelProperties.m_nRhythm++;
                removeShapesAtWrongRhythm( rTickIter, rAxisLabelProperties.m_nRhythm, nTick, xTarget );
                return false;
            }
        }

        bool bHasExtraColor=false;
        Color nExtraColor;

        OUString aLabel = getTextLabelString(
            aFixedNumberFormatter, pCategories, pTickInfo, isComplexCategoryAxis(),
            nExtraColor, bHasExtraColor);

        if(pColorAny)
            *pColorAny <<= bHasExtraColor?nExtraColor:nColor;
        if(pLimitedSpaceAny)
            *pLimitedSpaceAny <<= sal_Int32(-1*pTickInfo->nFactorForLimitedTextWidth);

        B2DVector aTickScreenPos2D = pTickInfo->aTickScreenPosition;
        aTickScreenPos2D += aTextToTickDistance;
        awt::Point aAnchorScreenPosition2D(
            static_cast<sal_Int32>(aTickScreenPos2D.getX())
            ,static_cast<sal_Int32>(aTickScreenPos2D.getY()));

        //create single label
        if(!pTickInfo->xTextShape.is())
            pTickInfo->xTextShape = createSingleLabel( xTarget
                                    , aAnchorScreenPosition2D, aLabel
                                    , rAxisLabelProperties, m_aAxisProperties
                                    , aPropNames, aPropValues, bIsHorizontalAxis );
        if(!pTickInfo->xTextShape.is())
            continue;

        recordMaximumTextSize( *pTickInfo->xTextShape, rAxisLabelProperties.m_fRotationAngleDegree );

        //if NO OVERLAP -> remove overlapping shapes
        if( pLastVisibleNeighbourTickInfo && !rAxisLabelProperties.m_bOverlapAllowed )
        {
            // Check if the label still overlaps with its neighbor.
            if( doesOverlap( pLastVisibleNeighbourTickInfo->xTextShape, pTickInfo->xTextShape, rAxisLabelProperties.m_fRotationAngleDegree ) )
            {
                // It overlaps.
                if( isAutoRotatingOfLabelsAllowed(rAxisLabelProperties, bIsHorizontalAxis, bIsVerticalAxis) )
                {
                    // Try auto-rotating the labels at 45 degrees and
                    // start over.  This rotation angle will be stored for
                    // all future text shape creation runs.
                    // The nRhythm parameter is reset to 1 since the layout
                    // used for text labels is changed.
                    rAxisLabelProperties.autoRotate45();
                    m_aAxisLabelProperties.m_fRotationAngleDegree = rAxisLabelProperties.m_fRotationAngleDegree; // Store it for future runs.
                    removeTextShapesFromTicks();
                    rAxisLabelProperties.m_nRhythm = 1;
                    return false;
                }

                // Try incrementing the tick interval and start over.
                rAxisLabelProperties.m_nRhythm++;
                removeShapesAtWrongRhythm( rTickIter, rAxisLabelProperties.m_nRhythm, nTick, xTarget );
                return false;
            }
        }

        pPreviousVisibleTickInfo = pTickInfo;
    }
    return true;
}

double VCartesianAxis::getAxisIntersectionValue() const
{
    if (m_aAxisProperties.m_pfMainLinePositionAtOtherAxis)
        return *m_aAxisProperties.m_pfMainLinePositionAtOtherAxis;

    double fMin = (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMinX() : m_pPosHelper->getLogicMinY();
    double fMax = (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMaxX() : m_pPosHelper->getLogicMaxY();

    return (m_aAxisProperties.m_eCrossoverType == css::chart::ChartAxisPosition_END) ? fMax : fMin;
}

double VCartesianAxis::getLabelLineIntersectionValue() const
{
    if (m_aAxisProperties.m_eLabelPos == css::chart::ChartAxisLabelPosition_OUTSIDE_START)
        return (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMinX() : m_pPosHelper->getLogicMinY();

    if (m_aAxisProperties.m_eLabelPos == css::chart::ChartAxisLabelPosition_OUTSIDE_END)
        return (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMaxX() : m_pPosHelper->getLogicMaxY();

    return getAxisIntersectionValue();
}

double VCartesianAxis::getExtraLineIntersectionValue() const
{
    if( !m_aAxisProperties.m_pfExrtaLinePositionAtOtherAxis )
        return std::numeric_limits<double>::quiet_NaN();

    double fMin = (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMinX() : m_pPosHelper->getLogicMinY();
    double fMax = (m_nDimensionIndex==1) ? m_pPosHelper->getLogicMaxX() : m_pPosHelper->getLogicMaxY();

    if( *m_aAxisProperties.m_pfExrtaLinePositionAtOtherAxis <= fMin
        || *m_aAxisProperties.m_pfExrtaLinePositionAtOtherAxis >= fMax )
        return std::numeric_limits<double>::quiet_NaN();

    return *m_aAxisProperties.m_pfExrtaLinePositionAtOtherAxis;
}

B2DVector VCartesianAxis::getScreenPosition( double fLogicX, double fLogicY, double fLogicZ ) const
{
    B2DVector aRet(0,0);

    if( m_pPosHelper )
    {
        drawing::Position3D aScenePos = m_pPosHelper->transformLogicToScene( fLogicX, fLogicY, fLogicZ, true );
        if(m_nDimension==3)
        {
            if (m_xLogicTarget.is())
            {
                tPropertyNameMap aDummyPropertyNameMap;
                rtl::Reference<Svx3DExtrudeObject> xShape3DAnchor = ShapeFactory::createCube( m_xLogicTarget
                        , aScenePos,drawing::Direction3D(1,1,1), 0, nullptr, aDummyPropertyNameMap);
                awt::Point a2DPos = xShape3DAnchor->getPosition(); //get 2D position from xShape3DAnchor
                m_xLogicTarget->remove(xShape3DAnchor);
                aRet.setX( a2DPos.X );
                aRet.setY( a2DPos.Y );
            }
            else
            {
                OSL_FAIL("cannot calculate screen position in VCartesianAxis::getScreenPosition");
            }
        }
        else
        {
            aRet.setX( aScenePos.PositionX );
            aRet.setY( aScenePos.PositionY );
        }
    }

    return aRet;
}

VCartesianAxis::ScreenPosAndLogicPos VCartesianAxis::getScreenPosAndLogicPos( double fLogicX_, double fLogicY_, double fLogicZ_ ) const
{
    ScreenPosAndLogicPos aRet;
    aRet.fLogicX = fLogicX_;
    aRet.fLogicY = fLogicY_;
    aRet.fLogicZ = fLogicZ_;
    aRet.aScreenPos = getScreenPosition( fLogicX_, fLogicY_, fLogicZ_ );
    return aRet;
}

typedef std::vector< VCartesianAxis::ScreenPosAndLogicPos > tScreenPosAndLogicPosList;

namespace {

struct lcl_LessXPos
{
    bool operator() ( const VCartesianAxis::ScreenPosAndLogicPos& rPos1, const VCartesianAxis::ScreenPosAndLogicPos& rPos2 )
    {
        return ( rPos1.aScreenPos.getX() < rPos2.aScreenPos.getX() );
    }
};

struct lcl_GreaterYPos
{
    bool operator() ( const VCartesianAxis::ScreenPosAndLogicPos& rPos1, const VCartesianAxis::ScreenPosAndLogicPos& rPos2 )
    {
        return ( rPos1.aScreenPos.getY() > rPos2.aScreenPos.getY() );
    }
};

}

void VCartesianAxis::get2DAxisMainLine(
    B2DVector& rStart, B2DVector& rEnd, AxisLabelAlignment& rAlignment, double fCrossesOtherAxis ) const
{
    //m_aAxisProperties might get updated and changed here because
    //    the label alignment and inner direction sign depends exactly of the choice of the axis line position which is made here in this method

    double const fMinX = m_pPosHelper->getLogicMinX();
    double const fMinY = m_pPosHelper->getLogicMinY();
    double const fMinZ = m_pPosHelper->getLogicMinZ();
    double const fMaxX = m_pPosHelper->getLogicMaxX();
    double const fMaxY = m_pPosHelper->getLogicMaxY();
    double const fMaxZ = m_pPosHelper->getLogicMaxZ();

    double fXOnXPlane = fMinX;
    double fXOther = fMaxX;
    int nDifferentValue = !m_pPosHelper->isMathematicalOrientationX() ? -1 : 1;
    if( !m_pPosHelper->isSwapXAndY() )
        nDifferentValue *= (m_eLeftWallPos != CuboidPlanePosition_Left) ? -1 : 1;
    else
        nDifferentValue *= (m_eBottomPos != CuboidPlanePosition_Bottom) ? -1 : 1;
    if( nDifferentValue<0 )
    {
        fXOnXPlane = fMaxX;
        fXOther = fMinX;
    }

    double fYOnYPlane = fMinY;
    double fYOther = fMaxY;
    nDifferentValue = !m_pPosHelper->isMathematicalOrientationY() ? -1 : 1;
    if( !m_pPosHelper->isSwapXAndY() )
        nDifferentValue *= (m_eBottomPos != CuboidPlanePosition_Bottom) ? -1 : 1;
    else
        nDifferentValue *= (m_eLeftWallPos != CuboidPlanePosition_Left) ? -1 : 1;
    if( nDifferentValue<0 )
    {
        fYOnYPlane = fMaxY;
        fYOther = fMinY;
    }

    double fZOnZPlane = fMaxZ;
    double fZOther = fMinZ;
    nDifferentValue = !m_pPosHelper->isMathematicalOrientationZ() ? -1 : 1;
    nDifferentValue *= (m_eBackWallPos != CuboidPlanePosition_Back) ? -1 : 1;
    if( nDifferentValue<0 )
    {
        fZOnZPlane = fMinZ;
        fZOther = fMaxZ;
    }

    double fXStart = fMinX;
    double fYStart = fMinY;
    double fZStart = fMinZ;
    double fXEnd;
    double fYEnd;
    double fZEnd = fZStart;

    if( m_nDimensionIndex==0 ) //x-axis
    {
        if( fCrossesOtherAxis < fMinY )
            fCrossesOtherAxis = fMinY;
        else if( fCrossesOtherAxis > fMaxY )
            fCrossesOtherAxis = fMaxY;

        fYStart = fYEnd = fCrossesOtherAxis;
        fXEnd=m_pPosHelper->getLogicMaxX();

        if(m_nDimension==3)
        {
            if( AxisHelper::isAxisPositioningEnabled() )
            {
                if( ::rtl::math::approxEqual( fYOther, fYStart) )
                    fZStart = fZEnd = fZOnZPlane;
                else
                    fZStart = fZEnd = fZOther;
            }
            else
            {
                rStart = getScreenPosition( fXStart, fYStart, fZStart );
                rEnd = getScreenPosition( fXEnd, fYEnd, fZEnd );

                double fDeltaX = rEnd.getX() - rStart.getX();
                double fDeltaY = rEnd.getY() - rStart.getY();

                //only those points are candidates which are lying on exactly one wall as these are outer edges
                tScreenPosAndLogicPosList aPosList { getScreenPosAndLogicPos( fMinX, fYOnYPlane, fZOther ), getScreenPosAndLogicPos( fMinX, fYOther, fZOnZPlane ) };

                if( fabs(fDeltaY) > fabs(fDeltaX)  )
                {
                    rAlignment.meAlignment = LABEL_ALIGN_LEFT;
                    //choose most left positions
                    std::sort( aPosList.begin(), aPosList.end(), lcl_LessXPos() );
                    rAlignment.mfLabelDirection = (fDeltaY < 0) ? -1.0 : 1.0;
                }
                else
                {
                    rAlignment.meAlignment = LABEL_ALIGN_BOTTOM;
                    //choose most bottom positions
                    std::sort( aPosList.begin(), aPosList.end(), lcl_GreaterYPos() );
                    rAlignment.mfLabelDirection = (fDeltaX < 0) ? -1.0 : 1.0;
                }
                ScreenPosAndLogicPos aBestPos( aPosList[0] );
                fYStart = fYEnd = aBestPos.fLogicY;
                fZStart = fZEnd = aBestPos.fLogicZ;
                if( !m_pPosHelper->isMathematicalOrientationX() )
                    rAlignment.mfLabelDirection *= -1.0;
            }
        }//end 3D x axis
    }
    else if( m_nDimensionIndex==1 ) //y-axis
    {
        if( fCrossesOtherAxis < fMinX )
            fCrossesOtherAxis = fMinX;
        else if( fCrossesOtherAxis > fMaxX )
            fCrossesOtherAxis = fMaxX;

        fXStart = fXEnd = fCrossesOtherAxis;
        fYEnd=m_pPosHelper->getLogicMaxY();

        if(m_nDimension==3)
        {
            if( AxisHelper::isAxisPositioningEnabled() )
            {
                if( ::rtl::math::approxEqual( fXOther, fXStart) )
                    fZStart = fZEnd = fZOnZPlane;
                else
                    fZStart = fZEnd = fZOther;
            }
            else
            {
                rStart = getScreenPosition( fXStart, fYStart, fZStart );
                rEnd = getScreenPosition( fXEnd, fYEnd, fZEnd );

                double fDeltaX = rEnd.getX() - rStart.getX();
                double fDeltaY = rEnd.getY() - rStart.getY();

                //only those points are candidates which are lying on exactly one wall as these are outer edges
                tScreenPosAndLogicPosList aPosList { getScreenPosAndLogicPos( fXOnXPlane, fMinY, fZOther ), getScreenPosAndLogicPos( fXOther, fMinY, fZOnZPlane ) };

                if( fabs(fDeltaY) > fabs(fDeltaX)  )
                {
                    rAlignment.meAlignment = LABEL_ALIGN_LEFT;
                    //choose most left positions
                    std::sort( aPosList.begin(), aPosList.end(), lcl_LessXPos() );
                    rAlignment.mfLabelDirection = (fDeltaY < 0) ? -1.0 : 1.0;
                }
                else
                {
                    rAlignment.meAlignment = LABEL_ALIGN_BOTTOM;
                    //choose most bottom positions
                    std::sort( aPosList.begin(), aPosList.end(), lcl_GreaterYPos() );
                    rAlignment.mfLabelDirection = (fDeltaX < 0) ? -1.0 : 1.0;
                }
                ScreenPosAndLogicPos aBestPos( aPosList[0] );
                fXStart = fXEnd = aBestPos.fLogicX;
                fZStart = fZEnd = aBestPos.fLogicZ;
                if( !m_pPosHelper->isMathematicalOrientationY() )
                    rAlignment.mfLabelDirection *= -1.0;
            }
        }//end 3D y axis
    }
    else //z-axis
    {
        fZEnd = m_pPosHelper->getLogicMaxZ();
        if( AxisHelper::isAxisPositioningEnabled() )
        {
            if( !m_aAxisProperties.m_bSwapXAndY )
            {
                if( fCrossesOtherAxis < fMinY )
                    fCrossesOtherAxis = fMinY;
                else if( fCrossesOtherAxis > fMaxY )
                    fCrossesOtherAxis = fMaxY;
                fYStart = fYEnd = fCrossesOtherAxis;

                if( ::rtl::math::approxEqual( fYOther, fYStart) )
                    fXStart = fXEnd = fXOnXPlane;
                else
                    fXStart = fXEnd = fXOther;
            }
            else
            {
                if( fCrossesOtherAxis < fMinX )
                    fCrossesOtherAxis = fMinX;
                else if( fCrossesOtherAxis > fMaxX )
                    fCrossesOtherAxis = fMaxX;
                fXStart = fXEnd = fCrossesOtherAxis;

                if( ::rtl::math::approxEqual( fXOther, fXStart) )
                    fYStart = fYEnd = fYOnYPlane;
                else
                    fYStart = fYEnd = fYOther;
            }
        }
        else
        {
            if( !m_pPosHelper->isSwapXAndY() )
            {
                fXStart = fXEnd = m_pPosHelper->isMathematicalOrientationX() ? m_pPosHelper->getLogicMaxX() : m_pPosHelper->getLogicMinX();
                fYStart = fYEnd = m_pPosHelper->isMathematicalOrientationY() ? m_pPosHelper->getLogicMinY() : m_pPosHelper->getLogicMaxY();
            }
            else
            {
                fXStart = fXEnd = m_pPosHelper->isMathematicalOrientationX() ? m_pPosHelper->getLogicMinX() : m_pPosHelper->getLogicMaxX();
                fYStart = fYEnd = m_pPosHelper->isMathematicalOrientationY() ? m_pPosHelper->getLogicMaxY() : m_pPosHelper->getLogicMinY();
            }

            if(m_nDimension==3)
            {
                rStart = getScreenPosition( fXStart, fYStart, fZStart );
                rEnd = getScreenPosition( fXEnd, fYEnd, fZEnd );

                double fDeltaX = rEnd.getX() - rStart.getX();

                //only those points are candidates which are lying on exactly one wall as these are outer edges
                tScreenPosAndLogicPosList aPosList { getScreenPosAndLogicPos( fXOther, fYOnYPlane, fMinZ ), getScreenPosAndLogicPos( fXOnXPlane, fYOther, fMinZ ) };

                std::sort( aPosList.begin(), aPosList.end(), lcl_GreaterYPos() );
                ScreenPosAndLogicPos aBestPos( aPosList[0] );
                ScreenPosAndLogicPos aNotSoGoodPos( aPosList[1] );

                //choose most bottom positions
                if( fDeltaX != 0.0 ) // prefer left-right alignments
                {
                    if( aBestPos.aScreenPos.getX() > aNotSoGoodPos.aScreenPos.getX() )
                        rAlignment.meAlignment = LABEL_ALIGN_RIGHT;
                    else
                         rAlignment.meAlignment = LABEL_ALIGN_LEFT;
                }
                else
                {
                    if( aBestPos.aScreenPos.getY() > aNotSoGoodPos.aScreenPos.getY() )
                        rAlignment.meAlignment = LABEL_ALIGN_BOTTOM;
                    else
                        rAlignment.meAlignment = LABEL_ALIGN_TOP;
                }

                rAlignment.mfLabelDirection = (fDeltaX < 0) ? -1.0 : 1.0;
                if( !m_pPosHelper->isMathematicalOrientationZ() )
                    rAlignment.mfLabelDirection *= -1.0;

                fXStart = fXEnd = aBestPos.fLogicX;
                fYStart = fYEnd = aBestPos.fLogicY;
            }
        }//end 3D z axis
    }

    rStart = getScreenPosition( fXStart, fYStart, fZStart );
    rEnd = getScreenPosition( fXEnd, fYEnd, fZEnd );

    if(m_nDimension==3 && !AxisHelper::isAxisPositioningEnabled() )
        rAlignment.mfInnerTickDirection = rAlignment.mfLabelDirection;//to behave like before

    if(!(m_nDimension==3 && AxisHelper::isAxisPositioningEnabled()) )
        return;

    double fDeltaX = rEnd.getX() - rStart.getX();
    double fDeltaY = rEnd.getY() - rStart.getY();

    if( m_nDimensionIndex==2 )
    {
        if( m_eLeftWallPos != CuboidPlanePosition_Left )
        {
            rAlignment.mfLabelDirection *= -1.0;
            rAlignment.mfInnerTickDirection *= -1.0;
        }

        rAlignment.meAlignment =
            (rAlignment.mfLabelDirection < 0) ?
                LABEL_ALIGN_LEFT :  LABEL_ALIGN_RIGHT;

        if( ( fDeltaY<0 && m_aScale.Orientation == chart2::AxisOrientation_REVERSE ) ||
            ( fDeltaY>0 && m_aScale.Orientation == chart2::AxisOrientation_MATHEMATICAL ) )
            rAlignment.meAlignment =
                (rAlignment.meAlignment == LABEL_ALIGN_RIGHT) ?
                    LABEL_ALIGN_LEFT : LABEL_ALIGN_RIGHT;
    }
    else if( fabs(fDeltaY) > fabs(fDeltaX) )
    {
        if( m_eBackWallPos != CuboidPlanePosition_Back )
        {
            rAlignment.mfLabelDirection *= -1.0;
            rAlignment.mfInnerTickDirection *= -1.0;
        }

        rAlignment.meAlignment =
            (rAlignment.mfLabelDirection < 0) ?
                LABEL_ALIGN_LEFT : LABEL_ALIGN_RIGHT;

        if( ( fDeltaY<0 && m_aScale.Orientation == chart2::AxisOrientation_REVERSE ) ||
            ( fDeltaY>0 && m_aScale.Orientation == chart2::AxisOrientation_MATHEMATICAL ) )
            rAlignment.meAlignment =
                (rAlignment.meAlignment == LABEL_ALIGN_RIGHT) ?
                    LABEL_ALIGN_LEFT :  LABEL_ALIGN_RIGHT;
    }
    else
    {
        if( m_eBackWallPos != CuboidPlanePosition_Back )
        {
            rAlignment.mfLabelDirection *= -1.0;
            rAlignment.mfInnerTickDirection *= -1.0;
        }

        rAlignment.meAlignment =
            (rAlignment.mfLabelDirection < 0) ?
                LABEL_ALIGN_TOP : LABEL_ALIGN_BOTTOM;

        if( ( fDeltaX>0 && m_aScale.Orientation == chart2::AxisOrientation_REVERSE ) ||
            ( fDeltaX<0 && m_aScale.Orientation == chart2::AxisOrientation_MATHEMATICAL ) )
            rAlignment.meAlignment =
                (rAlignment.meAlignment == LABEL_ALIGN_TOP) ?
                    LABEL_ALIGN_BOTTOM : LABEL_ALIGN_TOP;
    }
}

TickFactory* VCartesianAxis::createTickFactory()
{
    return createTickFactory2D();
}

TickFactory2D* VCartesianAxis::createTickFactory2D()
{
    AxisLabelAlignment aLabelAlign = m_aAxisProperties.maLabelAlignment;
    B2DVector aStart, aEnd;
    get2DAxisMainLine(aStart, aEnd, aLabelAlign, getAxisIntersectionValue());

    B2DVector aLabelLineStart, aLabelLineEnd;
    get2DAxisMainLine(aLabelLineStart, aLabelLineEnd, aLabelAlign, getLabelLineIntersectionValue());
    m_aAxisProperties.maLabelAlignment = aLabelAlign;

    return new TickFactory2D( m_aScale, m_aIncrement, aStart, aEnd, aLabelLineStart-aStart );
}

static void lcl_hideIdenticalScreenValues( TickIter& rTickIter )
{
    TickInfo* pPrevTickInfo = rTickIter.firstInfo();
    if (!pPrevTickInfo)
        return;

    pPrevTickInfo->bPaintIt = true;
    for( TickInfo* pTickInfo = rTickIter.nextInfo(); pTickInfo; pTickInfo = rTickIter.nextInfo())
    {
        pTickInfo->bPaintIt = (pTickInfo->aTickScreenPosition != pPrevTickInfo->aTickScreenPosition);
        pPrevTickInfo = pTickInfo;
    }
}

//'hide' tickmarks with identical screen values in aAllTickInfos
void VCartesianAxis::hideIdenticalScreenValues( TickInfoArraysType& rTickInfos ) const
{
    if( isComplexCategoryAxis() || isDateAxis() )
    {
        sal_Int32 nCount = rTickInfos.size();
        for( sal_Int32 nN=0; nN<nCount; nN++ )
        {
            PureTickIter aTickIter( rTickInfos[nN] );
            lcl_hideIdenticalScreenValues( aTickIter );
        }
    }
    else
    {
        EquidistantTickIter aTickIter( rTickInfos, m_aIncrement, -1 );
        lcl_hideIdenticalScreenValues( aTickIter );
    }
}

sal_Int32 VCartesianAxis::estimateMaximumAutoMainIncrementCount()
{
    sal_Int32 nRet = 10;

    if( m_nMaximumTextWidthSoFar==0 && m_nMaximumTextHeightSoFar==0 )
        return nRet;

    B2DVector aStart, aEnd;
    AxisLabelAlignment aLabelAlign = m_aAxisProperties.maLabelAlignment;
    get2DAxisMainLine(aStart, aEnd, aLabelAlign, getAxisIntersectionValue());
    m_aAxisProperties.maLabelAlignment = aLabelAlign;

    sal_Int32 nMaxHeight = static_cast<sal_Int32>(fabs(aEnd.getY()-aStart.getY()));
    sal_Int32 nMaxWidth = static_cast<sal_Int32>(fabs(aEnd.getX()-aStart.getX()));

    sal_Int32 nTotalAvailable = nMaxHeight;
    sal_Int32 nSingleNeeded = m_nMaximumTextHeightSoFar;
    sal_Int32 nMaxSameLabel = 0;

    // tdf#48041: do not duplicate the value labels because of rounding
    if (m_aAxisProperties.m_nAxisType != css::chart2::AxisType::DATE)
    {
        FixedNumberFormatter aFixedNumberFormatterTest(m_xNumberFormatsSupplier, m_aAxisLabelProperties.m_nNumberFormatKey);
        OUString sPreviousValueLabel;
        sal_Int32 nSameLabel = 0;
        for (auto const & nLabel: m_aAllTickInfos[0])
        {
            Color nColor = COL_AUTO;
            bool bHasColor = false;
            OUString sValueLabel = aFixedNumberFormatterTest.getFormattedString(nLabel.fScaledTickValue, nColor, bHasColor);
            if (sValueLabel == sPreviousValueLabel)
            {
                nSameLabel++;
                if (nSameLabel > nMaxSameLabel)
                    nMaxSameLabel = nSameLabel;
            }
            else
                nSameLabel = 0;
            sPreviousValueLabel = sValueLabel;
        }
    }
    //for horizontal axis:
    if( (m_nDimensionIndex == 0 && !m_aAxisProperties.m_bSwapXAndY)
        || (m_nDimensionIndex == 1 && m_aAxisProperties.m_bSwapXAndY) )
    {
        nTotalAvailable = nMaxWidth;
        nSingleNeeded = m_nMaximumTextWidthSoFar;
    }

    if( nSingleNeeded>0 )
        nRet = nTotalAvailable/nSingleNeeded;

    if ( nMaxSameLabel > 0 )
    {
        sal_Int32 nRetNoSameLabel = m_aAllTickInfos[0].size() / (nMaxSameLabel + 1);
        if ( nRet > nRetNoSameLabel )
           nRet = nRetNoSameLabel;
    }

    return nRet;
}

void VCartesianAxis::doStaggeringOfLabels( const AxisLabelProperties& rAxisLabelProperties, TickFactory2D const * pTickFactory2D )
{
    if( !pTickFactory2D )
        return;

    if( isComplexCategoryAxis() )
    {
        sal_Int32 nTextLevelCount = getTextLevelCount();
        B2DVector aCumulatedLabelsDistance(0,0);
        for( sal_Int32 nTextLevel=0; nTextLevel<nTextLevelCount; nTextLevel++ )
        {
            std::unique_ptr<TickIter> apTickIter(createLabelTickIterator(nTextLevel));
            if (apTickIter)
            {
                double fRotationAngleDegree = m_aAxisLabelProperties.m_fRotationAngleDegree;
                if( nTextLevel>0 )
                {
                    lcl_shiftLabels(*apTickIter, aCumulatedLabelsDistance);
                    //multilevel labels: 0 or 90 by default
                    if( m_aAxisProperties.m_bSwapXAndY )
                        fRotationAngleDegree = 90.0;
                    else
                        fRotationAngleDegree = 0.0;
                }
                aCumulatedLabelsDistance += lcl_getLabelsDistance(
                    *apTickIter, pTickFactory2D->getDistanceAxisTickToText(m_aAxisProperties),
                    fRotationAngleDegree);
            }
        }
    }
    else if (rAxisLabelProperties.isStaggered())
    {
        if( !m_aAllTickInfos.empty() )
        {
            LabelIterator aInnerIter( m_aAllTickInfos[0], rAxisLabelProperties.m_eStaggering, true );
            LabelIterator aOuterIter( m_aAllTickInfos[0], rAxisLabelProperties.m_eStaggering, false );

            lcl_shiftLabels( aOuterIter
                , lcl_getLabelsDistance( aInnerIter
                    , pTickFactory2D->getDistanceAxisTickToText( m_aAxisProperties ), 0.0 ) );
        }
    }
}

void VCartesianAxis::createDataTableShape(std::unique_ptr<TickFactory2D> const& rpTickFactory2D)
{
    // Check if we can create the data table shape
    // Data table view and m_bDisplayDataTable must be true
    if (!m_pDataTableView || !m_aAxisProperties.m_bDisplayDataTable)
        return;

    m_pDataTableView->initializeShapes(m_xDataTableTarget);
    basegfx::B2DVector aStart = rpTickFactory2D->getXaxisStartPos();
    basegfx::B2DVector aEnd = rpTickFactory2D->getXaxisEndPos();

    rpTickFactory2D->updateScreenValues(m_aAllTickInfos);

    sal_Int32 nDistance = -1;

    std::unique_ptr<TickIter> apTickIter(createLabelTickIterator(0));
    if (apTickIter)
    {
        nDistance = TickFactory2D::getTickScreenDistance(*apTickIter);
        if (getTextLevelCount() > 1)
            nDistance *= 2;
    }

    if (nDistance <= 0)
    {
        // we only have one data series so we have no TickMarks, therefore calculate and use the table size
        auto rDelta = aEnd - aStart;
        nDistance = basegfx::fround(rDelta.getX());
    }

    if (nDistance > 0)
    {
        m_pDataTableView->createShapes(aStart, aEnd, nDistance);
    }
}

void VCartesianAxis::createLabels()
{
    if( !prepareShapeCreation() )
        return;

    std::unique_ptr<TickFactory2D> apTickFactory2D(createTickFactory2D()); // throws on failure

    createDataTableShape(apTickFactory2D);

    //create labels
    if (!m_aAxisProperties.m_bDisplayLabels)
        return;

    TickFactory2D* pTickFactory2D = apTickFactory2D.get();

    //get the transformed screen values for all tickmarks in aAllTickInfos
    pTickFactory2D->updateScreenValues( m_aAllTickInfos );
    //'hide' tickmarks with identical screen values in aAllTickInfos
    hideIdenticalScreenValues( m_aAllTickInfos );

    removeTextShapesFromTicks();

    //create tick mark text shapes
    sal_Int32 nTextLevelCount = getTextLevelCount();
    sal_Int32 nScreenDistanceBetweenTicks = -1;
    for( sal_Int32 nTextLevel=0; nTextLevel<nTextLevelCount; nTextLevel++ )
    {
        std::unique_ptr< TickIter > apTickIter(createLabelTickIterator( nTextLevel ));
        if(apTickIter)
        {
            if(nTextLevel==0)
            {
                nScreenDistanceBetweenTicks = TickFactory2D::getTickScreenDistance(*apTickIter);
                if( nTextLevelCount>1 )
                    nScreenDistanceBetweenTicks*=2; //the above used tick iter does contain also the sub ticks -> thus the given distance is only the half
            }

            AxisLabelProperties aComplexProps(m_aAxisLabelProperties);
            if( m_aAxisProperties.m_bComplexCategories )
            {
                aComplexProps.m_bLineBreakAllowed = true;
                aComplexProps.m_bOverlapAllowed = aComplexProps.m_fRotationAngleDegree != 0.0;
                if( nTextLevel > 0 )
                {
                    //multilevel labels: 0 or 90 by default
                    if( m_aAxisProperties.m_bSwapXAndY )
                        aComplexProps.m_fRotationAngleDegree = 90.0;
                    else
                        aComplexProps.m_fRotationAngleDegree = 0.0;
                }
            }
            AxisLabelProperties& rAxisLabelProperties =  m_aAxisProperties.m_bComplexCategories ? aComplexProps : m_aAxisLabelProperties;
            while (!createTextShapes(m_xTextTarget, *apTickIter, rAxisLabelProperties,
                                     pTickFactory2D, nScreenDistanceBetweenTicks))
            {
            };
        }
    }
    doStaggeringOfLabels( m_aAxisLabelProperties, pTickFactory2D );

    if (m_pDataTableView)
    {
        sal_Int32 x = m_xTextTarget->getPosition().X;
        sal_Int32 y = m_xTextTarget->getPosition().Y;
        sal_Int32 height = m_xTextTarget->getSize().Height;
        m_pDataTableView->changePosition(x, y + height);
    }
}

void VCartesianAxis::createMaximumLabels()
{
    m_bRecordMaximumTextSize = true;
    const comphelper::ScopeGuard aGuard([this]() { m_bRecordMaximumTextSize = false; });

    if( !prepareShapeCreation() )
        return;

    //create labels
    if (!m_aAxisProperties.m_bDisplayLabels)
        return;

    std::unique_ptr<TickFactory2D> apTickFactory2D(createTickFactory2D()); // throws on failure
    TickFactory2D* pTickFactory2D = apTickFactory2D.get();

    //get the transformed screen values for all tickmarks in aAllTickInfos
    pTickFactory2D->updateScreenValues( m_aAllTickInfos );

    //create tick mark text shapes
    //@todo: iterate through all tick depth which should be labeled

    AxisLabelProperties aAxisLabelProperties( m_aAxisLabelProperties );
    if( isAutoStaggeringOfLabelsAllowed( aAxisLabelProperties, pTickFactory2D->isHorizontalAxis(), pTickFactory2D->isVerticalAxis() ) )
        aAxisLabelProperties.m_eStaggering = AxisLabelStaggering::StaggerEven;

    aAxisLabelProperties.m_bOverlapAllowed = true;
    aAxisLabelProperties.m_bLineBreakAllowed = false;
    sal_Int32 nTextLevelCount = getTextLevelCount();
    for( sal_Int32 nTextLevel=0; nTextLevel<nTextLevelCount; nTextLevel++ )
    {
        std::unique_ptr< TickIter > apTickIter(createMaximumLabelTickIterator( nTextLevel ));
        if(apTickIter)
        {
            while (!createTextShapes(m_xTextTarget, *apTickIter, aAxisLabelProperties,
                                     pTickFactory2D, -1))
            {
            };
        }
    }
    doStaggeringOfLabels( aAxisLabelProperties, pTickFactory2D );
}

void VCartesianAxis::updatePositions()
{
    //update positions of labels
    if (!m_aAxisProperties.m_bDisplayLabels)
        return;

    std::unique_ptr<TickFactory2D> apTickFactory2D(createTickFactory2D()); // throws on failure
    TickFactory2D* pTickFactory2D = apTickFactory2D.get();

    //update positions of all existing text shapes
    pTickFactory2D->updateScreenValues( m_aAllTickInfos );

    sal_Int32 nDepth=0;
    for (auto const& tickInfos : m_aAllTickInfos)
    {
        for (auto const& tickInfo : tickInfos)
        {
            const rtl::Reference<SvxShapeText> & xShape2DText(tickInfo.xTextShape);
            if( xShape2DText.is() )
            {
                B2DVector aTextToTickDistance( pTickFactory2D->getDistanceAxisTickToText( m_aAxisProperties, true ) );
                B2DVector aTickScreenPos2D(tickInfo.aTickScreenPosition);
                aTickScreenPos2D += aTextToTickDistance;
                awt::Point aAnchorScreenPosition2D(
                    static_cast<sal_Int32>(aTickScreenPos2D.getX())
                    ,static_cast<sal_Int32>(aTickScreenPos2D.getY()));

                double fRotationAngleDegree = m_aAxisLabelProperties.m_fRotationAngleDegree;
                if( nDepth > 0 )
                {
                    //multilevel labels: 0 or 90 by default
                    if( pTickFactory2D->isHorizontalAxis() )
                        fRotationAngleDegree = 0.0;
                    else
                        fRotationAngleDegree = 90;
                }

                // #i78696# use mathematically correct rotation now
                const double fRotationAnglePi(-basegfx::deg2rad(fRotationAngleDegree));
                uno::Any aATransformation = ShapeFactory::makeTransformation(aAnchorScreenPosition2D, fRotationAnglePi);

                //set new position
                try
                {
                    xShape2DText->SvxShape::setPropertyValue( u"Transformation"_ustr, aATransformation );
                }
                catch( const uno::Exception& )
                {
                    TOOLS_WARN_EXCEPTION("chart2", "" );
                }

                //correctPositionForRotation
                LabelPositionHelper::correctPositionForRotation( xShape2DText
                    , m_aAxisProperties.maLabelAlignment.meAlignment, fRotationAngleDegree, m_aAxisProperties.m_bComplexCategories );
            }
        }
        ++nDepth;
    }

    doStaggeringOfLabels( m_aAxisLabelProperties, pTickFactory2D );
}

void VCartesianAxis::createTickMarkLineShapes( TickInfoArrayType& rTickInfos, const TickmarkProperties& rTickmarkProperties, TickFactory2D const & rTickFactory2D, bool bOnlyAtLabels )
{
    sal_Int32 nPointCount = rTickInfos.size();
    drawing::PointSequenceSequence aPoints(2*nPointCount);

    sal_Int32 nN = 0;
    for (auto const& tickInfo : rTickInfos)
    {
        if( !tickInfo.bPaintIt )
            continue;

        bool bTicksAtLabels = ( m_aAxisProperties.m_eTickmarkPos != css::chart::ChartAxisMarkPosition_AT_AXIS );
        double fInnerDirectionSign = m_aAxisProperties.maLabelAlignment.mfInnerTickDirection;
        if( bTicksAtLabels && m_aAxisProperties.m_eLabelPos == css::chart::ChartAxisLabelPosition_OUTSIDE_END )
            fInnerDirectionSign *= -1.0;
        bTicksAtLabels = bTicksAtLabels || bOnlyAtLabels;
        //add ticks at labels:
        rTickFactory2D.addPointSequenceForTickLine( aPoints, nN++, tickInfo.fScaledTickValue
            , fInnerDirectionSign , rTickmarkProperties, bTicksAtLabels );
        //add ticks at axis (without labels):
        if( !bOnlyAtLabels && m_aAxisProperties.m_eTickmarkPos == css::chart::ChartAxisMarkPosition_AT_LABELS_AND_AXIS )
            rTickFactory2D.addPointSequenceForTickLine( aPoints, nN++, tickInfo.fScaledTickValue
                , m_aAxisProperties.maLabelAlignment.mfInnerTickDirection, rTickmarkProperties, !bTicksAtLabels );
    }
    aPoints.realloc(nN);
    ShapeFactory::createLine2D( m_xGroupShape_Shapes, aPoints
                                , &rTickmarkProperties.aLineProperties );
}

void VCartesianAxis::createShapes()
{
    if( !prepareShapeCreation() )
        return;

    //create line shapes
    if(m_nDimension==2)
    {
        std::unique_ptr<TickFactory2D> apTickFactory2D(createTickFactory2D()); // throws on failure
        TickFactory2D* pTickFactory2D = apTickFactory2D.get();

        //create extra long ticks to separate complex categories (create them only there where the labels are)
        if( isComplexCategoryAxis() )
        {
            TickInfoArraysType aComplexTickInfos;
            createAllTickInfosFromComplexCategories( aComplexTickInfos, true );
            pTickFactory2D->updateScreenValues( aComplexTickInfos );
            hideIdenticalScreenValues( aComplexTickInfos );

            std::vector<TickmarkProperties> aTickmarkPropertiesList;
            static const bool bIncludeSpaceBetweenTickAndText = false;
            sal_Int32 nOffset = static_cast<sal_Int32>(pTickFactory2D->getDistanceAxisTickToText( m_aAxisProperties, false, bIncludeSpaceBetweenTickAndText ).getLength());
            sal_Int32 nTextLevelCount = getTextLevelCount();
            for( sal_Int32 nTextLevel=0; nTextLevel<nTextLevelCount; nTextLevel++ )
            {
                std::unique_ptr< TickIter > apTickIter(createLabelTickIterator( nTextLevel ));
                if( apTickIter )
                {
                    double fRotationAngleDegree = m_aAxisLabelProperties.m_fRotationAngleDegree;
                    if( nTextLevel > 0 )
                    {
                        //Multi-level Labels: default to 0 or 90
                        if( m_aAxisProperties.m_bSwapXAndY )
                            fRotationAngleDegree = 90.0;
                        else
                            fRotationAngleDegree = 0.0;
                    }
                    B2DVector aLabelsDistance(lcl_getLabelsDistance(
                        *apTickIter, pTickFactory2D->getDistanceAxisTickToText(m_aAxisProperties),
                        fRotationAngleDegree));
                    sal_Int32 nCurrentLength = static_cast<sal_Int32>(aLabelsDistance.getLength());
                    aTickmarkPropertiesList.push_back( m_aAxisProperties.makeTickmarkPropertiesForComplexCategories( nOffset + nCurrentLength, 0 ) );
                    nOffset += nCurrentLength;
                }
            }

            sal_Int32 nTickmarkPropertiesCount = aTickmarkPropertiesList.size();
            TickInfoArraysType::iterator aDepthIter             = aComplexTickInfos.begin();
            const TickInfoArraysType::const_iterator aDepthEnd  = aComplexTickInfos.end();
            for( sal_Int32 nDepth=0; aDepthIter != aDepthEnd && nDepth < nTickmarkPropertiesCount; ++aDepthIter, nDepth++ )
            {
                if(nDepth==0 && !m_aAxisProperties.m_nMajorTickmarks)
                    continue;
                createTickMarkLineShapes( *aDepthIter, aTickmarkPropertiesList[nDepth], *pTickFactory2D, true /*bOnlyAtLabels*/ );
            }
        }
        //create normal ticks for major and minor intervals
        {
            TickInfoArraysType aUnshiftedTickInfos;
            if( m_aScale.m_bShiftedCategoryPosition )// if m_bShiftedCategoryPosition==true the tickmarks in m_aAllTickInfos are shifted
            {
                pTickFactory2D->getAllTicks( aUnshiftedTickInfos );
                pTickFactory2D->updateScreenValues( aUnshiftedTickInfos );
                hideIdenticalScreenValues( aUnshiftedTickInfos );
            }
            TickInfoArraysType& rAllTickInfos = m_aScale.m_bShiftedCategoryPosition ? aUnshiftedTickInfos : m_aAllTickInfos;

            if (rAllTickInfos.empty())
                return;

            sal_Int32 nDepth = 0;
            sal_Int32 nTickmarkPropertiesCount = m_aAxisProperties.m_aTickmarkPropertiesList.size();
            for( auto& rTickInfos : rAllTickInfos )
            {
                if (nDepth == nTickmarkPropertiesCount)
                    break;

                createTickMarkLineShapes( rTickInfos, m_aAxisProperties.m_aTickmarkPropertiesList[nDepth], *pTickFactory2D, false /*bOnlyAtLabels*/ );
                nDepth++;
            }
        }
        //create axis main lines
        //it serves also as the handle shape for the axis selection
        {
            drawing::PointSequenceSequence aPoints(1);
            apTickFactory2D->createPointSequenceForAxisMainLine( aPoints );
            rtl::Reference<SvxShapePolyPolygon> xShape = ShapeFactory::createLine2D(
                    m_xGroupShape_Shapes, aPoints
                    , &m_aAxisProperties.m_aLineProperties );
            //because of this name this line will be used for marking the axis
            ::chart::ShapeFactory::setShapeName( xShape, u"MarkHandles"_ustr );
        }
        //create an additional line at NULL
        if( !AxisHelper::isAxisPositioningEnabled() )
        {
            double fExtraLineCrossesOtherAxis = getExtraLineIntersectionValue();
            if (!std::isnan(fExtraLineCrossesOtherAxis))
            {
                B2DVector aStart, aEnd;
                AxisLabelAlignment aLabelAlign = m_aAxisProperties.maLabelAlignment;
                get2DAxisMainLine(aStart, aEnd, aLabelAlign, fExtraLineCrossesOtherAxis);
                m_aAxisProperties.maLabelAlignment = aLabelAlign;
                drawing::PointSequenceSequence aPoints{{
                        {static_cast<sal_Int32>(aStart.getX()), static_cast<sal_Int32>(aStart.getY())},
                        {static_cast<sal_Int32>(aEnd.getX()), static_cast<sal_Int32>(aEnd.getY())} }};
                ShapeFactory::createLine2D(
                        m_xGroupShape_Shapes, aPoints, &m_aAxisProperties.m_aLineProperties );
            }
        }
    }

    createLabels();
}

void VCartesianAxis::createDataTableView(std::vector<std::unique_ptr<VSeriesPlotter>>& rSeriesPlotterList,
                                         rtl::Reference<SvNumberFormatsSupplierObj> const& xNumberFormatsSupplier,
                                         rtl::Reference<::chart::ChartModel> const& xChartDoc,
                                         css::uno::Reference<css::uno::XComponentContext> const& rComponentContext)
{
    if (!m_aAxisProperties.m_bDisplayDataTable)
        return;

    m_pDataTableView.reset(new DataTableView(xChartDoc, m_aAxisProperties.m_xDataTableModel, rComponentContext, m_aAxisProperties.m_bDataTableAlignAxisValuesWithColumns));
    m_pDataTableView->initializeValues(rSeriesPlotterList);
    m_xNumberFormatsSupplier = xNumberFormatsSupplier;
}


} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

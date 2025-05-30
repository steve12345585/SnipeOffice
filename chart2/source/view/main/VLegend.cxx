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

#include "VLegend.hxx"
#include "VButton.hxx"
#include <Legend.hxx>
#include <PropertyMapper.hxx>
#include <ChartModel.hxx>
#include <ObjectIdentifier.hxx>
#include <FormattedString.hxx>
#include <RelativePositionHelper.hxx>
#include <ShapeFactory.hxx>
#include <RelativeSizeHelper.hxx>
#include <LegendEntryProvider.hxx>
#include <chartview/DrawModelWrapper.hxx>
#include <com/sun/star/text/WritingMode2.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/TextHorizontalAdjust.hpp>
#include <com/sun/star/drawing/LineJoint.hpp>
#include <com/sun/star/chart/ChartLegendExpansion.hpp>
#include <com/sun/star/chart2/LegendPosition.hpp>
#include <com/sun/star/chart2/RelativePosition.hpp>
#include <com/sun/star/chart2/RelativeSize.hpp>
#include <com/sun/star/chart2/XFormattedString2.hpp>
#include <com/sun/star/chart2/data/XPivotTableDataProvider.hpp>
#include <com/sun/star/chart2/data/PivotTableFieldEntry.hpp>
#include <rtl/math.hxx>
#include <svl/ctloptions.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/UnitConversion.hxx>

#include <utility>
#include <vector>
#include <algorithm>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;

namespace chart
{

namespace
{

typedef std::pair< ::chart::tNameSequence, ::chart::tAnySequence > tPropertyValues;

double lcl_CalcViewFontSize(
    const Reference< beans::XPropertySet > & xProp,
    const awt::Size & rReferenceSize )
{
    double fResult = 10.0;

    float fFontHeight( 0.0 );
    if( xProp.is() && ( xProp->getPropertyValue( u"CharHeight"_ustr) >>= fFontHeight ))
    {
        fResult = fFontHeight;
        try
        {
            awt::Size aPropRefSize;
            if( (xProp->getPropertyValue( u"ReferencePageSize"_ustr) >>= aPropRefSize) &&
                (aPropRefSize.Height > 0))
            {
                fResult = ::chart::RelativeSizeHelper::calculate( fFontHeight, aPropRefSize, rReferenceSize );
            }
        }
        catch( const uno::Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }

    return convertPointToMm100(fResult);
}

void lcl_getProperties(
    const Reference< beans::XPropertySet > & xLegendProp,
    tPropertyValues & rOutLineFillProperties,
    tPropertyValues & rOutTextProperties,
    const awt::Size & rReferenceSize )
{
    // Get Line- and FillProperties from model legend
    if( !xLegendProp.is())
        return;

    // set rOutLineFillProperties
    ::chart::tPropertyNameValueMap aLineFillValueMap;
    ::chart::PropertyMapper::getValueMap( aLineFillValueMap, ::chart::PropertyMapper::getPropertyNameMapForFillAndLineProperties(), xLegendProp );

    aLineFillValueMap[ u"LineJoint"_ustr ] <<= drawing::LineJoint_ROUND;

    ::chart::PropertyMapper::getMultiPropertyListsFromValueMap(
        rOutLineFillProperties.first, rOutLineFillProperties.second, aLineFillValueMap );

    // set rOutTextProperties
    ::chart::tPropertyNameValueMap aTextValueMap;
    ::chart::PropertyMapper::getValueMap( aTextValueMap, ::chart::PropertyMapper::getPropertyNameMapForCharacterProperties(), xLegendProp );

    aTextValueMap[ u"TextAutoGrowHeight"_ustr ] <<= true;
    aTextValueMap[ u"TextAutoGrowWidth"_ustr ] <<= true;
    aTextValueMap[ u"TextHorizontalAdjust"_ustr ] <<= drawing::TextHorizontalAdjust_LEFT;
    aTextValueMap[ u"TextMaximumFrameWidth"_ustr ] <<= rReferenceSize.Width; //needs to be overwritten by actual available space in the legend

    // recalculate font size
    awt::Size aPropRefSize;
    float fFontHeight( 0.0 );
    if( (xLegendProp->getPropertyValue( u"ReferencePageSize"_ustr) >>= aPropRefSize) &&
        (aPropRefSize.Height > 0) &&
        (aTextValueMap[ u"CharHeight"_ustr ] >>= fFontHeight) )
    {
        aTextValueMap[ u"CharHeight"_ustr ] <<=
            static_cast< float >(
                ::chart::RelativeSizeHelper::calculate( fFontHeight, aPropRefSize, rReferenceSize ));

        if( aTextValueMap[ u"CharHeightAsian"_ustr ] >>= fFontHeight )
        {
            aTextValueMap[ u"CharHeightAsian"_ustr ] <<=
                static_cast< float >(
                    ::chart::RelativeSizeHelper::calculate( fFontHeight, aPropRefSize, rReferenceSize ));
        }
        if( aTextValueMap[ u"CharHeightComplex"_ustr ] >>= fFontHeight )
        {
            aTextValueMap[ u"CharHeightComplex"_ustr ] <<=
                static_cast< float >(
                    ::chart::RelativeSizeHelper::calculate( fFontHeight, aPropRefSize, rReferenceSize ));
        }
    }

    ::chart::PropertyMapper::getMultiPropertyListsFromValueMap(
        rOutTextProperties.first, rOutTextProperties.second, aTextValueMap );
}

awt::Size lcl_createTextShapes(
    const std::vector<ViewLegendEntry> & rEntries,
    const rtl::Reference<SvxShapeGroupAnyD> & xTarget,
    std::vector< rtl::Reference<SvxShapeText> > & rOutTextShapes,
    const tPropertyValues & rTextProperties )
{
    awt::Size aResult;

    for (ViewLegendEntry const & rEntry : rEntries)
    {
        try
        {
            OUString aLabelString;
            if (rEntry.xLabel)
            {
                // tdf#150034 limit legend label text
                if (rEntry.xLabel->getString().getLength() > 520)
                {
                    sal_Int32 nIndex = rEntry.xLabel->getString().indexOf(' ', 500);
                    rEntry.xLabel->setString(
                        rEntry.xLabel->getString().copy(0, nIndex > 500 ? nIndex : 500));
                }

                aLabelString += rEntry.xLabel->getString();
                // workaround for Issue #i67540#
                if( aLabelString.isEmpty())
                    aLabelString = " ";
            }

            rtl::Reference<SvxShapeText> xEntry =
                ShapeFactory::createText( xTarget, aLabelString,
                        rTextProperties.first, rTextProperties.second, uno::Any() );

            // adapt max-extent
            awt::Size aCurrSize( xEntry->getSize());
            aResult.Width  = std::max( aResult.Width,  aCurrSize.Width  );
            aResult.Height = std::max( aResult.Height, aCurrSize.Height );

            rOutTextShapes.push_back( xEntry );
        }
        catch( const uno::Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }

    return aResult;
}

void lcl_collectColumnWidths( std::vector< sal_Int32 >& rColumnWidths, const sal_Int32 nNumberOfRows, const sal_Int32 nNumberOfColumns,
                              const std::vector< rtl::Reference<SvxShapeText> >& rTextShapes, sal_Int32 nSymbolPlusDistanceWidth )
{
    rColumnWidths.clear();
    sal_Int32 nNumberOfEntries = rTextShapes.size();
    for (sal_Int32 nRow = 0; nRow < nNumberOfRows; ++nRow )
    {
        for (sal_Int32 nColumn = 0; nColumn < nNumberOfColumns; ++nColumn )
        {
            sal_Int32 nEntry = nColumn + nRow * nNumberOfColumns;
            if( nEntry < nNumberOfEntries )
            {
                awt::Size aTextSize( rTextShapes[ nEntry ]->getSize() );
                sal_Int32 nWidth = nSymbolPlusDistanceWidth + aTextSize.Width;
                if( nRow==0 )
                    rColumnWidths.push_back( nWidth );
                else
                    rColumnWidths[nColumn] = std::max( nWidth, rColumnWidths[nColumn] );
            }
        }
    }
}

void lcl_collectRowHeighs( std::vector< sal_Int32 >& rRowHeights, const sal_Int32 nNumberOfRows, const sal_Int32 nNumberOfColumns,
                           const std::vector< rtl::Reference<SvxShapeText> >& rTextShapes )
{
    // calculate maximum height for each row
    // and collect column widths
    rRowHeights.clear();
    sal_Int32 nNumberOfEntries = rTextShapes.size();
    for (sal_Int32 nRow = 0; nRow < nNumberOfRows; ++nRow)
    {
        sal_Int32 nCurrentRowHeight = 0;
        for (sal_Int32 nColumn = 0; nColumn < nNumberOfColumns; ++nColumn)
        {
            sal_Int32 nEntry = nColumn + nRow * nNumberOfColumns;
            if( nEntry < nNumberOfEntries )
            {
                awt::Size aTextSize( rTextShapes[ nEntry ]->getSize() );
                nCurrentRowHeight = std::max( nCurrentRowHeight, aTextSize.Height );
            }
        }
        rRowHeights.push_back( nCurrentRowHeight );
    }
}

sal_Int32 lcl_getTextLineHeight( const std::vector< sal_Int32 >& aRowHeights, const sal_Int32 nNumberOfRows, double fViewFontSize )
{
    const sal_Int32 nFontHeight = static_cast< sal_Int32 >( fViewFontSize );
    if (!nFontHeight)
        return 0;
    sal_Int32 nTextLineHeight = nFontHeight;
    for (sal_Int32 nRow = 0; nRow < nNumberOfRows; ++nRow)
    {
        sal_Int32 nFullTextHeight = aRowHeights[nRow];
        if( ( nFullTextHeight / nFontHeight ) <= 1 )
        {
            nTextLineHeight = nFullTextHeight;//found an entry with one line-> have real text height
            break;
        }
    }
    return nTextLineHeight;
}

//returns resulting legend size
awt::Size lcl_placeLegendEntries(
    std::vector<ViewLegendEntry> & rEntries,
    css::chart::ChartLegendExpansion eExpansion,
    bool bSymbolsLeftSide,
    double fViewFontSize,
    const awt::Size& rMaxSymbolExtent,
    tPropertyValues & rTextProperties,
    const rtl::Reference<SvxShapeGroupAnyD> & xTarget,
    const awt::Size& rRemainingSpace,
    sal_Int32 nYStartPosition,
    const awt::Size& rPageSize,
    bool bIsPivotChart,
    awt::Size& rDefaultLegendSize)
{
    bool bIsCustomSize = (eExpansion == css::chart::ChartLegendExpansion_CUSTOM);
    awt::Size aResultingLegendSize(0,0);
    // For Pivot charts set the *minimum* legend size as a function of page size.
    if ( bIsPivotChart )
        aResultingLegendSize = awt::Size((rPageSize.Width * 13) / 80, (rPageSize.Height * 31) / 90);
    if( bIsCustomSize )
        aResultingLegendSize = awt::Size(rRemainingSpace.Width, rRemainingSpace.Height + nYStartPosition);

    // #i109336# Improve auto positioning in chart
    sal_Int32 nXPadding = static_cast< sal_Int32 >( std::max( 100.0, fViewFontSize * 0.33 ) );
    sal_Int32 nXOffset  = static_cast< sal_Int32 >( std::max( 100.0, fViewFontSize * 0.66 ) );
    sal_Int32 nYPadding = static_cast< sal_Int32 >( std::max( 100.0, fViewFontSize * 0.2 ) );
    sal_Int32 nYOffset  = static_cast< sal_Int32 >( std::max( 100.0, fViewFontSize * 0.2 ) );

    const sal_Int32 nSymbolToTextDistance = static_cast< sal_Int32 >( std::max( 100.0, fViewFontSize * 0.22 ) );//minimum 1mm
    const sal_Int32 nSymbolPlusDistanceWidth = rMaxSymbolExtent.Width + nSymbolToTextDistance;
    sal_Int32 nMaxTextWidth = rRemainingSpace.Width - nSymbolPlusDistanceWidth;
    uno::Any* pFrameWidthAny = PropertyMapper::getValuePointer( rTextProperties.second, rTextProperties.first, u"TextMaximumFrameWidth");
    if(pFrameWidthAny)
    {
        if( eExpansion == css::chart::ChartLegendExpansion_HIGH )
        {
            // limit the width of texts to 30% of the total available width
            // #i109336# Improve auto positioning in chart
            nMaxTextWidth = rRemainingSpace.Width * 3 / 10;
        }
        *pFrameWidthAny <<= nMaxTextWidth;
    }

    std::vector< rtl::Reference<SvxShapeText> > aTextShapes;
    awt::Size aMaxEntryExtent = lcl_createTextShapes( rEntries, xTarget, aTextShapes, rTextProperties );
    OSL_ASSERT( aTextShapes.size() == rEntries.size());

    sal_Int32 nMaxEntryWidth = nXOffset + nSymbolPlusDistanceWidth + aMaxEntryExtent.Width;
    sal_Int32 nMaxEntryHeight = nYOffset + aMaxEntryExtent.Height;
    sal_Int32 nNumberOfEntries = rEntries.size();

    rDefaultLegendSize.Width = nMaxEntryWidth;
    rDefaultLegendSize.Height = nMaxEntryHeight + nYPadding;

    sal_Int32 nNumberOfColumns = 0, nNumberOfRows = 0;
    std::vector< sal_Int32 > aColumnWidths;
    std::vector< sal_Int32 > aRowHeights;

    sal_Int32 nTextLineHeight = static_cast< sal_Int32 >( fViewFontSize );

    // determine layout depending on LegendExpansion
    if( eExpansion == css::chart::ChartLegendExpansion_CUSTOM )
    {
        sal_Int32 nCurrentRow=0;
        sal_Int32 nCurrentColumn=-1;
        sal_Int32 nMaxColumnCount=-1;
        for( sal_Int32 nN=0; nN<static_cast<sal_Int32>(aTextShapes.size()); nN++ )
        {
            const rtl::Reference<SvxShapeText>& xShape( aTextShapes[nN] );
            if( !xShape.is() )
                continue;
            awt::Size aSize( xShape->getSize() );
            sal_Int32 nNewWidth = aSize.Width + nSymbolPlusDistanceWidth;
            sal_Int32 nCurrentColumnCount = aColumnWidths.size();

            //are we allowed to add a new column?
            if( nMaxColumnCount==-1 || (nCurrentColumn+1) < nMaxColumnCount )
            {
                //try add a new column
                nCurrentColumn++;
                if( nCurrentColumn < nCurrentColumnCount )
                {
                    //check whether the current column width is sufficient for the new entry
                    if( aColumnWidths[nCurrentColumn]>=nNewWidth )
                    {
                        //all good proceed with next entry
                        continue;
                    }

                    aColumnWidths[nCurrentColumn] = std::max( nNewWidth, aColumnWidths[nCurrentColumn] );
                } else
                    aColumnWidths.push_back(nNewWidth);

                //do the columns still fit into the given size?
                nCurrentColumnCount = aColumnWidths.size();//update count
                sal_Int32 nSumWidth = 0;
                for (sal_Int32 nColumn = 0; nColumn < nCurrentColumnCount; nColumn++)
                    nSumWidth += aColumnWidths[nColumn];

                if( nSumWidth <= rRemainingSpace.Width || nCurrentColumnCount==1 )
                {
                    //all good proceed with next entry
                    continue;
                }
                else
                {
                    //not enough space for the current amount of columns
                    //try again with less columns
                    nMaxColumnCount = nCurrentColumnCount-1;
                    nN=-1;
                    nCurrentRow=0;
                    nCurrentColumn=-1;
                    aColumnWidths.clear();
                }
            }
            else
            {
                //add a new row and try the same entry again
                nCurrentRow++;
                nCurrentColumn=-1;
                nN--;
            }
        }
        nNumberOfColumns = aColumnWidths.size();
        nNumberOfRows = nCurrentRow+1;

        //check if there is not enough space so that some entries must be removed
        lcl_collectRowHeighs( aRowHeights, nNumberOfRows, nNumberOfColumns, aTextShapes );
        nTextLineHeight = lcl_getTextLineHeight( aRowHeights, nNumberOfRows, fViewFontSize );
        sal_Int32 nSumHeight = 0;
        for (sal_Int32 nRow=0; nRow < nNumberOfRows; nRow++)
            nSumHeight += aRowHeights[nRow];
        sal_Int32 nRemainingSpace = rRemainingSpace.Height - nSumHeight;

        if( nRemainingSpace < -100 ) // 1mm tolerance for OOXML interop tdf#90404
        {
            //remove entries that are too big
            for (sal_Int32 nRow = nNumberOfRows; nRow--; )
            {
                for (sal_Int32 nColumn = nNumberOfColumns; nColumn--; )
                {
                    sal_Int32 nEntry = nColumn + nRow * nNumberOfColumns;
                    if( nEntry < static_cast<sal_Int32>(aTextShapes.size()) )
                    {
                        DrawModelWrapper::removeShape( aTextShapes[nEntry] );
                        aTextShapes.pop_back();
                    }
                    if( nEntry < nNumberOfEntries && ( nEntry != 0 || nNumberOfColumns != 1 ) )
                    {
                        DrawModelWrapper::removeShape( rEntries[ nEntry ].xSymbol );
                        rEntries.pop_back();
                        nNumberOfEntries--;
                    }
                }
                if (nRow == 0 && nNumberOfColumns == 1)
                {
                    try
                    {
                        OUString aLabelString = rEntries[0].xLabel->getString();
                        static constexpr OUString sDots = u"..."_ustr;
                        for (sal_Int32 nNewLen = aLabelString.getLength() - sDots.getLength(); nNewLen > 0; )
                        {
                            OUString aNewLabel = aLabelString.subView(0, nNewLen) + sDots;
                            rtl::Reference<SvxShapeText> xEntry = ShapeFactory::createText(
                                xTarget, aNewLabel, rTextProperties.first, rTextProperties.second, uno::Any());
                            nSumHeight = xEntry->getSize().Height;
                            nRemainingSpace = rRemainingSpace.Height - nSumHeight;
                            if (nRemainingSpace >= 0)
                            {
                                sal_Int32 nWidth = xEntry->getSize().Width + nSymbolPlusDistanceWidth;
                                if (rRemainingSpace.Width - nWidth >= 0)
                                {
                                    aTextShapes.push_back(xEntry);
                                    rEntries[0].xLabel->setString(aNewLabel);
                                    aRowHeights[0] = nSumHeight;
                                    aColumnWidths[0] = nWidth;
                                    break;
                                }
                            }
                            DrawModelWrapper::removeShape(xEntry);
                            // The intention here is to make pathological cases with extremely large labels
                            // converge a little faster
                            if (nNewLen > 10 && std::abs(nRemainingSpace) > nSumHeight / 10)
                                nNewLen -= nNewLen / 10;
                            else
                                --nNewLen;
                        }
                        if (aTextShapes.empty())
                        {
                            DrawModelWrapper::removeShape(rEntries[0].xSymbol);
                            rEntries.pop_back();
                            nNumberOfEntries--;
                            aRowHeights.pop_back();
                        }
                    }
                    catch (const uno::Exception&)
                    {
                        DBG_UNHANDLED_EXCEPTION("chart2");
                    }
                }
                else
                {
                    nSumHeight -= aRowHeights[nRow];
                    aRowHeights.pop_back();
                    nRemainingSpace = rRemainingSpace.Height - nSumHeight;
                    if (nRemainingSpace >= 0)
                        break;
                }
            }
            nNumberOfRows = static_cast<sal_Int32>(aRowHeights.size());
        }
        if( nRemainingSpace >= -100 ) // 1mm tolerance for OOXML interop tdf#90404
        {
            sal_Int32 nNormalSpacingHeight = 2*nYPadding+(nNumberOfRows-1)*nYOffset;
            if( nRemainingSpace < nNormalSpacingHeight )
            {
                //reduce spacing between the entries
                nYPadding = nYOffset = nRemainingSpace/(nNumberOfRows+1);
            }
            else
            {
                //we have some space left that should be spread equally between all rows
                sal_Int32 nRemainingSingleSpace = (nRemainingSpace-nNormalSpacingHeight)/(nNumberOfRows+1);
                nYPadding += nRemainingSingleSpace;
                nYOffset += nRemainingSingleSpace;
            }
        }

        //check spacing between columns
        sal_Int32 nSumWidth = 0;
        for (sal_Int32 nColumn = 0; nColumn < nNumberOfColumns; nColumn++)
            nSumWidth += aColumnWidths[nColumn];
        nRemainingSpace = rRemainingSpace.Width - nSumWidth;
        if( nRemainingSpace>=0 )
        {
            sal_Int32 nNormalSpacingWidth = 2*nXPadding+(nNumberOfColumns-1)*nXOffset;
            if( nRemainingSpace < nNormalSpacingWidth )
            {
                //reduce spacing between the entries
                nXPadding = nXOffset = nRemainingSpace/(nNumberOfColumns+1);
            }
            else
            {
                //we have some space left that should be spread equally between all columns
                sal_Int32 nRemainingSingleSpace = (nRemainingSpace-nNormalSpacingWidth)/(nNumberOfColumns+1);
                nXPadding += nRemainingSingleSpace;
                nXOffset += nRemainingSingleSpace;
            }
        }
    }
    else if( eExpansion == css::chart::ChartLegendExpansion_HIGH )
    {
        sal_Int32 nMaxNumberOfRows = nMaxEntryHeight
            ? (rRemainingSpace.Height - 2*nYPadding ) / nMaxEntryHeight
            : 0;

        nNumberOfColumns = nMaxNumberOfRows
            ? static_cast< sal_Int32 >(
                ceil( static_cast< double >( nNumberOfEntries ) /
                      static_cast< double >( nMaxNumberOfRows ) ))
            : 0;
        nNumberOfRows =  nNumberOfColumns
            ? static_cast< sal_Int32 >(
                ceil( static_cast< double >( nNumberOfEntries ) /
                      static_cast< double >( nNumberOfColumns ) ))
            : 0;
    }
    else if( eExpansion == css::chart::ChartLegendExpansion_WIDE )
    {
        sal_Int32 nMaxNumberOfColumns = nMaxEntryWidth
            ? (rRemainingSpace.Width - 2*nXPadding ) / nMaxEntryWidth
            : 0;

        nNumberOfRows = nMaxNumberOfColumns
            ? static_cast< sal_Int32 >(
                ceil( static_cast< double >( nNumberOfEntries ) /
                      static_cast< double >( nMaxNumberOfColumns ) ))
            : 0;
        nNumberOfColumns = nNumberOfRows
            ? static_cast< sal_Int32 >(
                ceil( static_cast< double >( nNumberOfEntries ) /
                      static_cast< double >( nNumberOfRows ) ))
            : 0;
    }
    else // css::chart::ChartLegendExpansion_BALANCED
    {
        double fAspect = nMaxEntryHeight
            ? static_cast< double >( nMaxEntryWidth ) / static_cast< double >( nMaxEntryHeight )
            : 0.0;

        nNumberOfRows = static_cast< sal_Int32 >(
            ceil( sqrt( static_cast< double >( nNumberOfEntries ) * fAspect )));
        nNumberOfColumns = nNumberOfRows
            ? static_cast< sal_Int32 >(
                ceil( static_cast< double >( nNumberOfEntries ) /
                      static_cast< double >( nNumberOfRows ) ))
            : 0;
    }

    if(nNumberOfRows<=0)
        return aResultingLegendSize;

    if( eExpansion != css::chart::ChartLegendExpansion_CUSTOM )
    {
        lcl_collectColumnWidths( aColumnWidths, nNumberOfRows, nNumberOfColumns, aTextShapes, nSymbolPlusDistanceWidth );
        lcl_collectRowHeighs( aRowHeights, nNumberOfRows, nNumberOfColumns, aTextShapes );
        nTextLineHeight = lcl_getTextLineHeight( aRowHeights, nNumberOfRows, fViewFontSize );
    }

    sal_Int32 nCurrentXPos = bSymbolsLeftSide ? nXPadding : -nXPadding;

    // place entries into column and rows
    sal_Int32 nMaxYPos = 0;

    for (sal_Int32 nColumn = 0; nColumn < nNumberOfColumns; ++nColumn)
    {
        sal_Int32 nCurrentYPos = nYPadding + nYStartPosition;
        for (sal_Int32 nRow = 0; nRow < nNumberOfRows; ++nRow)
        {
            sal_Int32 nEntry = nColumn + nRow * nNumberOfColumns;
            if( nEntry >= nNumberOfEntries )
                break;

            // text shape
            const rtl::Reference<SvxShapeText>& xTextShape( aTextShapes[nEntry] );
            if( xTextShape.is() )
            {
                awt::Size aTextSize( xTextShape->getSize() );
                sal_Int32 nTextXPos = nCurrentXPos + nSymbolPlusDistanceWidth;
                if( !bSymbolsLeftSide )
                    nTextXPos = nCurrentXPos - nSymbolPlusDistanceWidth - aTextSize.Width;
                xTextShape->setPosition( awt::Point( nTextXPos, nCurrentYPos ));
            }

            // symbol
            rtl::Reference<SvxShapeGroup> & xSymbol( rEntries[ nEntry ].xSymbol );
            if( xSymbol.is() )
            {
                awt::Size aSymbolSize( rMaxSymbolExtent );
                sal_Int32 nSymbolXPos = nCurrentXPos;
                if( !bSymbolsLeftSide )
                    nSymbolXPos = nCurrentXPos - rMaxSymbolExtent.Width;
                sal_Int32 nSymbolYPos = nCurrentYPos + ( ( nTextLineHeight - aSymbolSize.Height ) / 2 );
                xSymbol->setPosition( awt::Point( nSymbolXPos, nSymbolYPos ) );
            }

            nCurrentYPos += aRowHeights[ nRow ];
            if( nRow+1 < nNumberOfRows )
                nCurrentYPos += nYOffset;
            nMaxYPos = std::max( nMaxYPos, nCurrentYPos );
        }
        if( bSymbolsLeftSide )
        {
            nCurrentXPos += aColumnWidths[nColumn];
            if( nColumn+1 < nNumberOfColumns )
                nCurrentXPos += nXOffset;
        }
        else
        {
            nCurrentXPos -= aColumnWidths[nColumn];
            if( nColumn+1 < nNumberOfColumns )
                nCurrentXPos -= nXOffset;
        }
    }

    if( !bIsCustomSize )
    {
        if( bSymbolsLeftSide )
            aResultingLegendSize.Width  = std::max( aResultingLegendSize.Width, nCurrentXPos + nXPadding );
        else
        {
            sal_Int32 nLegendWidth = -(nCurrentXPos-nXPadding);
            aResultingLegendSize.Width  = std::max( aResultingLegendSize.Width, nLegendWidth );
        }
        aResultingLegendSize.Height = std::max( aResultingLegendSize.Height, nMaxYPos + nYPadding );
    }

    if( !bSymbolsLeftSide )
    {
        sal_Int32 nLegendWidth = aResultingLegendSize.Width;
        awt::Point aPos(0,0);
        for( sal_Int32 nEntry=0; nEntry<nNumberOfEntries; nEntry++ )
        {
            rtl::Reference<SvxShapeGroup> & xSymbol( rEntries[ nEntry ].xSymbol );
            aPos = xSymbol->getPosition();
            aPos.X += nLegendWidth;
            xSymbol->setPosition( aPos );
            rtl::Reference<SvxShapeText> & xText( aTextShapes[ nEntry ] );
            aPos = xText->getPosition();
            aPos.X += nLegendWidth;
            xText->setPosition( aPos );
        }
    }

    return aResultingLegendSize;
}

// #i109336# Improve auto positioning in chart
sal_Int32 lcl_getLegendLeftRightMargin()
{
    return 210;  // 1/100 mm
}

// #i109336# Improve auto positioning in chart
sal_Int32 lcl_getLegendTopBottomMargin()
{
    return 185;  // 1/100 mm
}

chart2::RelativePosition lcl_getDefaultPosition( LegendPosition ePos, const awt::Rectangle& rOutAvailableSpace, const awt::Size & rPageSize )
{
    chart2::RelativePosition aResult;

    switch( ePos )
    {
        case LegendPosition_LINE_START:
            {
                // #i109336# Improve auto positioning in chart
                const double fDefaultDistance = static_cast< double >( lcl_getLegendLeftRightMargin() ) /
                    static_cast< double >( rPageSize.Width );
                aResult = chart2::RelativePosition(
                    fDefaultDistance, 0.5, drawing::Alignment_LEFT );
            }
            break;
        case LegendPosition_LINE_END:
            {
                // #i109336# Improve auto positioning in chart
                const double fDefaultDistance = static_cast< double >( lcl_getLegendLeftRightMargin() ) /
                    static_cast< double >( rPageSize.Width );
                aResult = chart2::RelativePosition(
                    1.0 - fDefaultDistance, 0.5, drawing::Alignment_RIGHT );
            }
            break;
        case LegendPosition_PAGE_START:
            {
                // #i109336# Improve auto positioning in chart
                const double fDefaultDistance = static_cast< double >( lcl_getLegendTopBottomMargin() ) /
                    static_cast< double >( rPageSize.Height );
                double fDistance = (static_cast<double>(rOutAvailableSpace.Y)/static_cast<double>(rPageSize.Height)) + fDefaultDistance;
                aResult = chart2::RelativePosition(
                    0.5, fDistance, drawing::Alignment_TOP );
            }
            break;
        case LegendPosition_PAGE_END:
            {
                // #i109336# Improve auto positioning in chart
                const double fDefaultDistance = static_cast< double >( lcl_getLegendTopBottomMargin() ) /
                    static_cast< double >( rPageSize.Height );

                double fDistance = double(rPageSize.Height - (rOutAvailableSpace.Y + rOutAvailableSpace.Height));
                fDistance += fDefaultDistance;
                fDistance /= double(rPageSize.Height);

                aResult = chart2::RelativePosition(
                    0.5, 1.0 - fDistance, drawing::Alignment_BOTTOM );
            }
            break;
        case LegendPosition::LegendPosition_MAKE_FIXED_SIZE:
        default:
            // nothing to be set
            break;
    }

    return aResult;
}

/**  @return
         a point relative to the upper left corner that can be used for
         XShape::setPosition()
*/
awt::Point lcl_calculatePositionAndRemainingSpace(
    awt::Rectangle & rRemainingSpace,
    const awt::Size & rPageSize,
    const chart2::RelativePosition& rRelPos,
    LegendPosition ePos,
    const awt::Size& aLegendSize,
    bool bOverlay )
{
    // calculate position
    awt::Point aResult(
        static_cast< sal_Int32 >( rRelPos.Primary * rPageSize.Width ),
        static_cast< sal_Int32 >( rRelPos.Secondary * rPageSize.Height ));

    aResult = RelativePositionHelper::getUpperLeftCornerOfAnchoredObject(
        aResult, aLegendSize, rRelPos.Anchor );

    // adapt rRemainingSpace if LegendPosition is not CUSTOM
    // #i109336# Improve auto positioning in chart
    sal_Int32 nXDistance = lcl_getLegendLeftRightMargin();
    sal_Int32 nYDistance = lcl_getLegendTopBottomMargin();
    if (!bOverlay) switch( ePos )
    {
        case LegendPosition_LINE_START:
        {
            sal_Int32 nExtent = aLegendSize.Width;
            rRemainingSpace.Width -= ( nExtent + nXDistance );
            rRemainingSpace.X += ( nExtent + nXDistance );
        }
        break;
        case LegendPosition_LINE_END:
        {
            rRemainingSpace.Width -= ( aLegendSize.Width + nXDistance );
        }
        break;
        case LegendPosition_PAGE_START:
        {
            sal_Int32 nExtent = aLegendSize.Height;
            rRemainingSpace.Height -= ( nExtent + nYDistance );
            rRemainingSpace.Y += ( nExtent + nYDistance );
        }
        break;
        case LegendPosition_PAGE_END:
        {
            rRemainingSpace.Height -= ( aLegendSize.Height + nYDistance );
        }
        break;

        default:
            // nothing
            break;
    }

    // adjust the legend position. Esp. for old files that had slightly smaller legends
    const sal_Int32 nEdgeDistance( 30 );
    if( aResult.X + aLegendSize.Width > rPageSize.Width )
    {
        sal_Int32 nNewX( (rPageSize.Width - aLegendSize.Width) - nEdgeDistance );
        if( nNewX > rPageSize.Width / 4 )
            aResult.X = nNewX;
    }
    if( aResult.Y + aLegendSize.Height > rPageSize.Height )
    {
        sal_Int32 nNewY( (rPageSize.Height - aLegendSize.Height) - nEdgeDistance );
        if( nNewY > rPageSize.Height / 4 )
            aResult.Y = nNewY;
    }

    return aResult;
}

bool lcl_shouldSymbolsBePlacedOnTheLeftSide( const Reference< beans::XPropertySet >& xLegendProp, sal_Int16 nDefaultWritingMode )
{
    bool bSymbolsLeftSide = true;
    try
    {
        if( SvtCTLOptions::IsCTLFontEnabled() )
        {
            if(xLegendProp.is())
            {
                sal_Int16 nWritingMode=-1;
                if( xLegendProp->getPropertyValue( u"WritingMode"_ustr ) >>= nWritingMode )
                {
                    if( nWritingMode == text::WritingMode2::PAGE )
                        nWritingMode = nDefaultWritingMode;
                    if( nWritingMode == text::WritingMode2::RL_TB )
                        bSymbolsLeftSide=false;
                }
            }
        }
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
    return bSymbolsLeftSide;
}

std::vector<std::shared_ptr<VButton>> lcl_createButtons(
                       rtl::Reference<SvxShapeGroupAnyD> const & xLegendContainer,
                       ChartModel& rModel, bool bPlaceButtonsVertically, tools::Long & nUsedHeight)
{
    std::vector<std::shared_ptr<VButton>> aButtons;

    uno::Reference<chart2::data::XPivotTableDataProvider> xPivotTableDataProvider(rModel.getDataProvider(), uno::UNO_QUERY);
    if (!xPivotTableDataProvider.is())
        return aButtons;

    if (!xPivotTableDataProvider->getColumnFields().hasElements())
        return aButtons;

    awt::Size aSize(2000, 700);
    int x = 100;
    int y = 100;

    const css::uno::Sequence<chart2::data::PivotTableFieldEntry> aPivotFieldEntries = xPivotTableDataProvider->getColumnFields();
    for (chart2::data::PivotTableFieldEntry const & sColumnFieldEntry : aPivotFieldEntries)
    {
        auto pButton = std::make_shared<VButton>();
        aButtons.push_back(pButton);
        pButton->init(xLegendContainer);
        awt::Point aNewPosition(x, y);
        pButton->setLabel(sColumnFieldEntry.Name);
        pButton->setCID("FieldButton.Column." + OUString::number(sColumnFieldEntry.DimensionIndex));
        pButton->setPosition(aNewPosition);
        pButton->setSize(aSize);
        if (sColumnFieldEntry.Name == "Data")
        {
            pButton->showArrow(false);
            pButton->setBGColor(Color(0x00F6F6F6));
        }
        if (sColumnFieldEntry.HasHiddenMembers)
            pButton->setArrowColor(Color(0x0000FF));

        if (bPlaceButtonsVertically)
            y += aSize.Height + 100;
        else
            x += aSize.Width + 100;
    }
    if (bPlaceButtonsVertically)
        nUsedHeight += y + 100;
    else
        nUsedHeight += aSize.Height + 100;

    return aButtons;
}

} // anonymous namespace

VLegend::VLegend(
    rtl::Reference< Legend > xLegend,
    const Reference< uno::XComponentContext > & xContext,
    std::vector< LegendEntryProvider* >&& rLegendEntryProviderList,
    rtl::Reference<SvxShapeGroupAnyD> xTargetPage,
    ChartModel& rModel )
        : m_xTarget(std::move(xTargetPage))
        , m_xLegend(std::move(xLegend))
        , mrModel(rModel)
        , m_xContext(xContext)
        , m_aLegendEntryProviderList(std::move(rLegendEntryProviderList))
        , m_nDefaultWritingMode(text::WritingMode2::LR_TB)
{
}

void VLegend::setDefaultWritingMode( sal_Int16 nDefaultWritingMode )
{
    m_nDefaultWritingMode = nDefaultWritingMode;
}

bool VLegend::isVisible( const rtl::Reference< Legend > & xLegend )
{
    if( ! xLegend.is())
        return false;

    bool bShow = false;
    try
    {
        xLegend->getPropertyValue( u"Show"_ustr) >>= bShow;
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }

    return bShow;
}

void VLegend::createShapes(
    const awt::Size & rAvailableSpace,
    const awt::Size & rPageSize,
    awt::Size & rDefaultLegendSize )
{
    if(! (m_xLegend.is() && m_xTarget.is()))
        return;

    try
    {
        //create shape and add to page
        OUString aLegendParticle( ObjectIdentifier::createParticleForLegend( &mrModel ) );
        m_xShape = ShapeFactory::createGroup2D( m_xTarget,
                    ObjectIdentifier::createClassifiedIdentifierForParticle( aLegendParticle ) );

        // create and insert sub-shapes
        rtl::Reference<SvxShapeGroupAnyD> xLegendContainer = m_xShape;
        if( xLegendContainer.is() )
        {
            // for quickly setting properties
            tPropertyValues aLineFillProperties;
            tPropertyValues aTextProperties;

            css::chart::ChartLegendExpansion eExpansion = css::chart::ChartLegendExpansion_HIGH;
            awt::Size aLegendSize( rAvailableSpace );

            bool bCustom = false;
            LegendPosition eLegendPosition = LegendPosition_LINE_END;
            // get Expansion property
            m_xLegend->getPropertyValue(u"Expansion"_ustr) >>= eExpansion;
            if( eExpansion == css::chart::ChartLegendExpansion_CUSTOM )
            {
                RelativeSize aRelativeSize;
                if (m_xLegend->getPropertyValue(u"RelativeSize"_ustr) >>= aRelativeSize)
                {
                    aLegendSize.Width = static_cast<sal_Int32>(::rtl::math::approxCeil( aRelativeSize.Primary * rPageSize.Width ));
                    aLegendSize.Height = static_cast<sal_Int32>(::rtl::math::approxCeil( aRelativeSize.Secondary * rPageSize.Height ));
                    bCustom = true;
                }
                else
                {
                    eExpansion = css::chart::ChartLegendExpansion_HIGH;
                }
            }
            m_xLegend->getPropertyValue(u"AnchorPosition"_ustr) >>= eLegendPosition;
            lcl_getProperties( m_xLegend, aLineFillProperties, aTextProperties, rPageSize );

            // create entries
            double fViewFontSize = lcl_CalcViewFontSize( m_xLegend, rPageSize );//todo
            // #i109336# Improve auto positioning in chart
            sal_Int32 nSymbolHeight = static_cast< sal_Int32 >( fViewFontSize * 0.6  );
            sal_Int32 nSymbolWidth = nSymbolHeight;

            for (LegendEntryProvider* pLegendEntryProvider : m_aLegendEntryProviderList)
            {
                if (pLegendEntryProvider)
                {
                    awt::Size aCurrentRatio = pLegendEntryProvider->getPreferredLegendKeyAspectRatio();
                    sal_Int32 nCurrentWidth = aCurrentRatio.Width;
                    if( aCurrentRatio.Height > 0 )
                    {
                        nCurrentWidth = nSymbolHeight* aCurrentRatio.Width/aCurrentRatio.Height;
                    }
                    nSymbolWidth = std::max( nSymbolWidth, nCurrentWidth );
                }
            }
            awt::Size aMaxSymbolExtent( nSymbolWidth, nSymbolHeight );

            std::vector<ViewLegendEntry> aViewEntries;
            for(LegendEntryProvider* pLegendEntryProvider : m_aLegendEntryProviderList)
            {
                if (pLegendEntryProvider)
                {
                    std::vector<ViewLegendEntry> aNewEntries = pLegendEntryProvider->createLegendEntries(
                                                                    aMaxSymbolExtent, eLegendPosition, m_xLegend,
                                                                    xLegendContainer, m_xContext, mrModel);
                    aViewEntries.insert( aViewEntries.end(), aNewEntries.begin(), aNewEntries.end() );
                }
            }

            bool bSymbolsLeftSide = lcl_shouldSymbolsBePlacedOnTheLeftSide( m_xLegend, m_nDefaultWritingMode );

            uno::Reference<chart2::data::XPivotTableDataProvider> xPivotTableDataProvider( mrModel.getDataProvider(), uno::UNO_QUERY );
            bool bIsPivotChart = xPivotTableDataProvider.is();

            if ( !aViewEntries.empty() || bIsPivotChart )
            {
                // create buttons
                tools::Long nUsedButtonHeight = 0;
                bool bPlaceButtonsVertically = (eLegendPosition != LegendPosition_PAGE_START &&
                                                eLegendPosition != LegendPosition_PAGE_END &&
                                                eExpansion != css::chart::ChartLegendExpansion_WIDE);

                std::vector<std::shared_ptr<VButton>> aButtons = lcl_createButtons(xLegendContainer, mrModel, bPlaceButtonsVertically, nUsedButtonHeight);

                // A custom size includes the size we used for buttons already, so we need to
                // subtract that from the size that is available for the legend
                if (bCustom)
                    aLegendSize.Height -= nUsedButtonHeight;

                // place the legend entries
                aLegendSize = lcl_placeLegendEntries(aViewEntries, eExpansion, bSymbolsLeftSide, fViewFontSize,
                                                     aMaxSymbolExtent, aTextProperties, xLegendContainer,
                                                     aLegendSize, nUsedButtonHeight, rPageSize, bIsPivotChart, rDefaultLegendSize);

                uno::Reference<beans::XPropertySet> xModelPage(mrModel.getPageBackground());

                for (std::shared_ptr<VButton> const & pButton : aButtons)
                {
                    // adjust the width of the buttons if we place them vertically
                    if (bPlaceButtonsVertically)
                        pButton->setSize({aLegendSize.Width - 200, pButton->getSize().Height});

                    // create the buttons
                    pButton->createShapes(xModelPage);
                }

                rtl::Reference<SvxShapeRect> xBorder = ShapeFactory::createRectangle(
                    xLegendContainer, aLegendSize, awt::Point(0, 0), aLineFillProperties.first,
                    aLineFillProperties.second, ShapeFactory::StackPosition::Bottom);

                //because of this name this border will be used for marking the legend
                ShapeFactory::setShapeName(xBorder, u"MarkHandles"_ustr);
            }
        }
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2" );
    }
}

void VLegend::changePosition(
    awt::Rectangle & rOutAvailableSpace,
    const awt::Size & rPageSize,
    const css::awt::Size & rDefaultLegendSize )
{
    if(! m_xShape.is())
        return;

    try
    {
        // determine position and alignment depending on default position
        awt::Size aLegendSize = m_xShape->getSize();
        chart2::RelativePosition aRelativePosition;

        bool bDefaultLegendSize = rDefaultLegendSize.Width != 0 || rDefaultLegendSize.Height != 0;
        bool bAutoPosition =
            ! (m_xLegend->getPropertyValue( u"RelativePosition"_ustr) >>= aRelativePosition);

        LegendPosition ePos = LegendPosition_LINE_END;
        m_xLegend->getPropertyValue( u"AnchorPosition"_ustr) >>= ePos;

        bool bOverlay = false;
        m_xLegend->getPropertyValue(u"Overlay"_ustr) >>= bOverlay;
        //calculate position
        if( bAutoPosition )
        {
            // auto position: relative to remaining space
            aRelativePosition = lcl_getDefaultPosition( ePos, rOutAvailableSpace, rPageSize );
            awt::Point aPos = lcl_calculatePositionAndRemainingSpace(
                rOutAvailableSpace, rPageSize, aRelativePosition, ePos, aLegendSize, bOverlay );
            m_xShape->setPosition( aPos );
        }
        else
        {
            // manual position: relative to whole page
            awt::Rectangle aAvailableSpace( 0, 0, rPageSize.Width, rPageSize.Height );
            awt::Point aPos = lcl_calculatePositionAndRemainingSpace(
                aAvailableSpace, rPageSize, aRelativePosition, ePos, bDefaultLegendSize ? rDefaultLegendSize : aLegendSize, bOverlay );
            m_xShape->setPosition( aPos );

            if (!bOverlay)
            {
                // calculate remaining space as if having autoposition:
                aRelativePosition = lcl_getDefaultPosition( ePos, rOutAvailableSpace, rPageSize );
                lcl_calculatePositionAndRemainingSpace(
                    rOutAvailableSpace, rPageSize, aRelativePosition, ePos, bDefaultLegendSize ? rDefaultLegendSize : aLegendSize, bOverlay );
            }
        }
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2" );
    }
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

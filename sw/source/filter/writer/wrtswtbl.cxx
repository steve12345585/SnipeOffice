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

#include <memory>
#include <hintids.hxx>
#include <editeng/borderline.hxx>
#include <editeng/boxitem.hxx>
#include <editeng/brushitem.hxx>
#include <tools/fract.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <wrtswtbl.hxx>
#include <swtable.hxx>
#include <frmfmt.hxx>
#include <fmtfsize.hxx>
#include <fmtornt.hxx>
#include <htmltbl.hxx>

using ::editeng::SvxBorderLine;
using namespace ::com::sun::star;

sal_Int16 SwWriteTableCell::GetVertOri() const
{
    sal_Int16 eCellVertOri = text::VertOrientation::TOP;
    if( m_pBox->GetSttNd() )
    {
        const SfxItemSet& rItemSet = m_pBox->GetFrameFormat()->GetAttrSet();
        if( const SwFormatVertOrient *pItem = rItemSet.GetItemIfSet( RES_VERT_ORIENT, false ) )
        {
            sal_Int16 eBoxVertOri = pItem->GetVertOrient();
            if( text::VertOrientation::CENTER==eBoxVertOri || text::VertOrientation::BOTTOM==eBoxVertOri)
                eCellVertOri = eBoxVertOri;
        }
    }

    return eCellVertOri;
}

SwWriteTableRow::SwWriteTableRow( tools::Long nPosition, bool bUseLayoutHeights )
    : m_pBackground(nullptr), m_nPos(nPosition), mbUseLayoutHeights(bUseLayoutHeights),
    m_bTopBorder(true), m_bBottomBorder(true)
{
}

SwWriteTableCell *SwWriteTableRow::AddCell( const SwTableBox *pBox,
                                sal_uInt16 nRow, sal_uInt16 nCol,
                                sal_uInt16 nRowSpan, sal_uInt16 nColSpan,
                                tools::Long nHeight,
                                const SvxBrushItem *pBackgroundBrush )
{
    SwWriteTableCell *pCell =
        new SwWriteTableCell( pBox, nRow, nCol, nRowSpan, nColSpan,
                                nHeight, pBackgroundBrush );
    m_Cells.push_back(std::unique_ptr<SwWriteTableCell>(pCell));

    return pCell;
}

SwWriteTableCol::SwWriteTableCol(sal_uInt32 nPosition)
    : m_nPos(nPosition), m_nWidthOpt(0), m_bRelWidthOpt(false),
    m_bLeftBorder(true), m_bRightBorder(true)
{
}

sal_uInt32 SwWriteTable::GetBoxWidth( const SwTableBox *pBox )
{
    const SwFrameFormat *pFormat = pBox->GetFrameFormat();
    const SwFormatFrameSize& aFrameSize=
        pFormat->GetFormatAttr( RES_FRM_SIZE );

    return sal::static_int_cast<sal_uInt32>(aFrameSize.GetSize().Width());
}

tools::Long SwWriteTable::GetLineHeight( const SwTableLine *pLine )
{
#ifdef DBG_UTIL
    bool bOldGetLineHeightCalled = m_bGetLineHeightCalled;
    m_bGetLineHeightCalled = true;
#endif

    tools::Long nHeight = 0;
    if( m_bUseLayoutHeights )
    {
        // At first we try to get the height of the layout.
        bool bLayoutAvailable = false;
        nHeight = pLine->GetTableLineHeight(bLayoutAvailable);
        if( nHeight > 0 )
            return nHeight;

        // If no layout is found, we assume that the heights are fixed.
        // #i60390# - in some cases we still want to continue
        // to use the layout heights even if one of the rows has a height of 0
        // ('hidden' rows)
        m_bUseLayoutHeights = bLayoutAvailable;

#ifdef DBG_UTIL
        SAL_WARN_IF( !bLayoutAvailable && bOldGetLineHeightCalled, "sw", "Layout invalid?" );
#endif
    }

    const SwTableBoxes& rBoxes = pLine->GetTabBoxes();
    for( auto pBox : rBoxes )
    {
        if( pBox->GetSttNd() )
        {
            if( nHeight < ROW_DFLT_HEIGHT )
                nHeight = ROW_DFLT_HEIGHT;
        }
        else
        {
            tools::Long nTmp = 0;
            const SwTableLines &rLines = pBox->GetTabLines();
            for( size_t nLine=0; nLine<rLines.size(); nLine++ )
            {
                nTmp += GetLineHeight( rLines[nLine] );
            }
            if( nHeight < nTmp )
                nHeight = nTmp;
        }
    }

    return nHeight;
}

tools::Long SwWriteTable::GetLineHeight( const SwTableBox *pBox )
{
    const SwTableLine *pLine = pBox->GetUpper();

    if( !pLine )
        return 0;

    const SwFrameFormat *pLineFrameFormat = pLine->GetFrameFormat();
    const SfxItemSet& rItemSet = pLineFrameFormat->GetAttrSet();

    tools::Long nHeight = 0;
    if( const SwFormatFrameSize* pItem = rItemSet.GetItemIfSet( RES_FRM_SIZE ) )
        nHeight = pItem->GetHeight();

    return nHeight;
}

const SvxBrushItem *SwWriteTable::GetLineBrush( const SwTableBox *pBox,
                                                  SwWriteTableRow *pRow )
{
    const SwTableLine *pLine = pBox->GetUpper();

    while( pLine )
    {
        const SwFrameFormat *pLineFrameFormat = pLine->GetFrameFormat();
        const SfxItemSet& rItemSet = pLineFrameFormat->GetAttrSet();

        if( const SvxBrushItem* pItem = rItemSet.GetItemIfSet( RES_BACKGROUND, false ) )
        {
            if( !pLine->GetUpper() )
            {
                if( !pRow->GetBackground() )
                    pRow->SetBackground( pItem );
                pItem = nullptr;
            }

            return pItem;
        }

        pBox = pLine->GetUpper();
        pLine = pBox ? pBox->GetUpper() : nullptr;
    }

    return nullptr;
}

void SwWriteTable::MergeBorders( const SvxBorderLine* pBorderLine,
                                   bool bTable )
{
    if( Color(ColorTransparency, 0xffffffff) == m_nBorderColor )
    {
        if( !pBorderLine->GetColor().IsRGBEqual( COL_GRAY ) )
            m_nBorderColor = pBorderLine->GetColor();
    }

    if( !m_bCollectBorderWidth )
        return;

    const sal_uInt16 nOutWidth = pBorderLine->GetOutWidth();
    if( bTable )
    {
        if( nOutWidth && (!m_nBorder || nOutWidth < m_nBorder) )
            m_nBorder = nOutWidth;
    }
    else
    {
        if( nOutWidth && (!m_nInnerBorder || nOutWidth < m_nInnerBorder) )
            m_nInnerBorder = nOutWidth;
    }

    const sal_uInt16 nDist = pBorderLine->GetInWidth() ? pBorderLine->GetDistance()
                                                : 0;
    if( nDist && (!m_nCellSpacing || nDist < m_nCellSpacing) )
        m_nCellSpacing = nDist;
}

sal_uInt16 SwWriteTable::MergeBoxBorders( const SwTableBox *pBox,
                                        size_t const nRow, size_t const nCol,
                                        sal_uInt16 nRowSpan, sal_uInt16 nColSpan,
                                        sal_uInt16& rTopBorder,
                                        sal_uInt16 &rBottomBorder )
{
    sal_uInt16 nBorderMask = 0;

    const SwFrameFormat *pFrameFormat = pBox->GetFrameFormat();
    const SvxBoxItem& rBoxItem = pFrameFormat->GetFormatAttr( RES_BOX );

    if( rBoxItem.GetTop() )
    {
        nBorderMask |= 1;
        MergeBorders( rBoxItem.GetTop(), nRow==0 );
        rTopBorder = rBoxItem.GetTop()->GetOutWidth();
    }

    if( rBoxItem.GetLeft() )
    {
        nBorderMask |= 4;
        MergeBorders( rBoxItem.GetLeft(), nCol==0 );
    }

    if( rBoxItem.GetBottom() )
    {
        nBorderMask |= 2;
        MergeBorders( rBoxItem.GetBottom(), nRow+nRowSpan==m_aRows.size() );
        rBottomBorder = rBoxItem.GetBottom()->GetOutWidth();
    }

    if( rBoxItem.GetRight() )
    {
        nBorderMask |= 8;
        MergeBorders( rBoxItem.GetRight(), nCol+nColSpan==m_aCols.size() );
    }

    // If any distance is set, the smallest one is used. This holds for
    // the four distance of a box as well as for the distances of different
    // boxes.
    if( m_bCollectBorderWidth )
    {
        sal_uInt16 nDist = rBoxItem.GetDistance( SvxBoxItemLine::TOP );
        if( nDist && (!m_nCellPadding || nDist < m_nCellPadding) )
            m_nCellPadding = nDist;
        nDist = rBoxItem.GetDistance( SvxBoxItemLine::BOTTOM );
        if( nDist && (!m_nCellPadding || nDist < m_nCellPadding) )
            m_nCellPadding = nDist;
        nDist = rBoxItem.GetDistance( SvxBoxItemLine::LEFT );
        if( nDist && (!m_nCellPadding || nDist < m_nCellPadding) )
            m_nCellPadding = nDist;
        nDist = rBoxItem.GetDistance( SvxBoxItemLine::RIGHT );
        if( nDist && (!m_nCellPadding || nDist < m_nCellPadding) )
            m_nCellPadding = nDist;
    }

    return nBorderMask;
}

sal_uInt32  SwWriteTable::GetRawWidth( sal_uInt16 nCol, sal_uInt16 nColSpan ) const
{
    sal_uInt32 nWidth = m_aCols[nCol+nColSpan-1]->GetPos();
    if( nCol > 0 )
        nWidth = nWidth - m_aCols[nCol-1]->GetPos();

    return nWidth;
}

sal_uInt16 SwWriteTable::GetLeftSpace( sal_uInt16 nCol ) const
{
    sal_uInt16 nSpace = m_nCellPadding + m_nCellSpacing;

    // Additional subtract the line thickness in the first column.
    if( nCol==0 )
    {
        nSpace = nSpace + m_nLeftSub;

        const SwWriteTableCol *pCol = m_aCols[nCol].get();
        if( pCol->HasLeftBorder() )
            nSpace = nSpace + m_nBorder;
    }

    return nSpace;
}

sal_uInt16
SwWriteTable::GetRightSpace(size_t const nCol, sal_uInt16 nColSpan) const
{
    sal_uInt16 nSpace = m_nCellPadding;

    // Additional subtract in the last column CELLSPACING and
    // line thickness once again.
    if( nCol+nColSpan==m_aCols.size() )
    {
        nSpace += (m_nCellSpacing + m_nRightSub);

        const SwWriteTableCol *pCol = m_aCols[nCol+nColSpan-1].get();
        if( pCol->HasRightBorder() )
            nSpace = nSpace + m_nBorder;
    }

    return nSpace;
}

sal_uInt16 SwWriteTable::GetAbsWidth( sal_uInt16 nCol, sal_uInt16 nColSpan ) const
{
    sal_uInt32 nWidth = GetRawWidth( nCol, nColSpan );
    if( m_nBaseWidth != m_nTabWidth )
    {
        nWidth *= m_nTabWidth;
        nWidth /= m_nBaseWidth;
    }

    nWidth -= GetLeftSpace( nCol ) + GetRightSpace( nCol, nColSpan );

    OSL_ENSURE( nWidth > 0, "Column Width <= 0. OK?" );
    return nWidth > 0 ? o3tl::narrowing<sal_uInt16>(nWidth) : 0;
}

sal_uInt16 SwWriteTable::GetRelWidth( sal_uInt16 nCol, sal_uInt16 nColSpan ) const
{
    tools::Long nWidth = GetRawWidth( nCol, nColSpan );

    return o3tl::narrowing<sal_uInt16>(static_cast<tools::Long>(Fraction( nWidth*256 + GetBaseWidth()/2,
                                   GetBaseWidth() )));
}

sal_uInt16 SwWriteTable::GetPercentWidth( sal_uInt16 nCol, sal_uInt16 nColSpan ) const
{
    tools::Long nWidth = GetRawWidth( nCol, nColSpan );

    // Looks funny, but is nothing more than
    // [(100 * nWidth) + .5] without rounding errors
    return o3tl::narrowing<sal_uInt16>(static_cast<tools::Long>(Fraction( nWidth*100 + GetBaseWidth()/2,
                                   GetBaseWidth() )));
}

tools::Long SwWriteTable::GetAbsHeight(tools::Long nRawHeight, size_t const nRow,
                                   sal_uInt16 nRowSpan ) const
{
    nRawHeight -= (2*m_nCellPadding + m_nCellSpacing);

    // Additional subtract in the first column CELLSPACING and
    // line thickness once again.
    const SwWriteTableRow *pRow = nullptr;
    if( nRow==0 )
    {
        nRawHeight -= m_nCellSpacing;
        pRow = m_aRows[nRow].get();
        if( pRow->HasTopBorder() )
            nRawHeight -= m_nBorder;
    }

    // Subtract the line thickness in the last column
    if( nRow+nRowSpan==m_aRows.size() )
    {
        if( !pRow || nRowSpan > 1 )
            pRow = m_aRows[nRow+nRowSpan-1].get();
        if( pRow->HasBottomBorder() )
            nRawHeight -= m_nBorder;
    }

    OSL_ENSURE( nRawHeight > 0, "Row Height <= 0. OK?" );
    return std::max<tools::Long>(nRawHeight, 0);
}

bool SwWriteTable::ShouldExpandSub(const SwTableBox *pBox, bool /*bExpandedBefore*/,
    sal_uInt16 nDepth) const
{
    return !pBox->GetSttNd() && nDepth > 0;
}

// FIXME: the degree of coupling between this method and
// FillTableRowsCols which is called immediately afterwards
// is -extremely- unpleasant and potentially problematic.

void SwWriteTable::CollectTableRowsCols( tools::Long nStartRPos,
                                           sal_uInt32 nStartCPos,
                                           tools::Long nParentLineHeight,
                                           sal_uInt32 nParentLineWidth,
                                           const SwTableLines& rLines,
                                           sal_uInt16 nDepth )
{
    bool bSubExpanded = false;
    const SwTableLines::size_type nLines = rLines.size();

#if OSL_DEBUG_LEVEL > 0
    sal_uInt32 nEndCPos = 0;
#endif

    tools::Long nRPos = nStartRPos;
    for( SwTableLines::size_type nLine = 0; nLine < nLines; ++nLine )
    {
        /*const*/ SwTableLine *pLine = rLines[nLine];

        tools::Long nOldRPos = nRPos;

        if( nLine < nLines-1 || nParentLineHeight==0  )
        {
            tools::Long nLineHeight = GetLineHeight( pLine );
            nRPos += nLineHeight;
            if( nParentLineHeight && nStartRPos + nParentLineHeight <= nRPos )
            {
                /* If you have corrupt line height information, e.g. breaking rows in complex table
                layout, you may run into this robust code.
                It's not allowed that subrows leaves their parentrow. If this would happen the line
                height of subrow is reduced to a part of the remaining height */
                OSL_FAIL( "Corrupt line height I" );
                nRPos -= nLineHeight;
                nLineHeight = nStartRPos + nParentLineHeight - nRPos; // remaining parent height
                nLineHeight /= nLines - nLine; // divided through the number of remaining sub rows
                nRPos += nLineHeight;
            }
            std::unique_ptr<SwWriteTableRow> pRow(new SwWriteTableRow( nRPos, m_bUseLayoutHeights));
            m_aRows.insert( std::move(pRow) );
        }
        else
        {
#if OSL_DEBUG_LEVEL > 0
            tools::Long nCheckPos = nRPos + GetLineHeight( pLine );
#endif
            nRPos = nStartRPos + nParentLineHeight;
#if OSL_DEBUG_LEVEL > 0
            SwWriteTableRow aSrchRow( nRPos, m_bUseLayoutHeights );
            OSL_ENSURE( std::find_if(m_aRows.begin(), m_aRows.end(),
                            [&](std::unique_ptr<SwWriteTableRow> const & p)
                            { return *p == aSrchRow; }) != m_aRows.end(), "Parent-Row not found" );
            SwWriteTableRow aRowCheckPos(nCheckPos,m_bUseLayoutHeights);
            SwWriteTableRow aRowRPos(nRPos,m_bUseLayoutHeights);
            OSL_ENSURE( !m_bUseLayoutHeights ||
                    aRowCheckPos == aRowRPos,
                    "Height of the rows does not correspond with the parent" );
#endif
        }

        // If necessary insert a column for all boxes of the row
        const SwTableBoxes& rBoxes = pLine->GetTabBoxes();
        const SwTableBoxes::size_type nBoxes = rBoxes.size();

        sal_uInt32 nCPos = nStartCPos;
        for( SwTableBoxes::size_type nBox=0; nBox<nBoxes; ++nBox )
        {
            const SwTableBox *pBox = rBoxes[nBox];

            sal_uInt32 nOldCPos = nCPos;

            if( nBox < nBoxes-1 || (nParentLineWidth==0 && nLine==0)  )
            {
                nCPos = nCPos + GetBoxWidth( pBox );
                std::unique_ptr<SwWriteTableCol> pCol(new SwWriteTableCol( nCPos ));

                m_aCols.insert( std::move(pCol) );

                if( nBox==nBoxes-1 )
                {
                    OSL_ENSURE( nLine==0 && nParentLineWidth==0,
                            "Now the parent width will be flattened!" );
                    nParentLineWidth = nCPos-nStartCPos;
                }
            }
            else
            {
#if OSL_DEBUG_LEVEL > 0
                sal_uInt32 nCheckPos = nCPos + GetBoxWidth( pBox );
                if( !nEndCPos )
                {
                    nEndCPos = nCheckPos;
                }
                else
                {
                    OSL_ENSURE( SwWriteTableCol(nCheckPos) ==
                                                SwWriteTableCol(nEndCPos),
                    "Cell includes rows of different widths" );
                }
#endif
                nCPos = nStartCPos + nParentLineWidth;

#if OSL_DEBUG_LEVEL > 0
                SwWriteTableCol aSrchCol( nCPos );
                OSL_ENSURE( m_aCols.find( &aSrchCol ) != m_aCols.end(),
                        "Parent-Cell not found" );
                OSL_ENSURE( SwWriteTableCol(nCheckPos) ==
                                            SwWriteTableCol(nCPos),
                        "Width of the cells does not correspond with the parent" );
#endif
            }

            if( ShouldExpandSub( pBox, bSubExpanded, nDepth ) )
            {
                CollectTableRowsCols( nOldRPos, nOldCPos,
                                        nRPos - nOldRPos,
                                        nCPos - nOldCPos,
                                        pBox->GetTabLines(),
                                        nDepth-1 );
                bSubExpanded = true;
            }
        }
    }
}

void SwWriteTable::FillTableRowsCols( tools::Long nStartRPos, sal_uInt16 nStartRow,
                                        sal_uInt32 nStartCPos, sal_uInt16 nStartCol,
                                        tools::Long nParentLineHeight,
                                        sal_uInt32 nParentLineWidth,
                                        const SwTableLines& rLines,
                                        const SvxBrushItem* pParentBrush,
                                        sal_uInt16 nDepth,
                                        sal_uInt16 nNumOfHeaderRows )
{
    const SwTableLines::size_type nLines = rLines.size();
    bool bSubExpanded = false;

    // Specifying the border
    tools::Long nRPos = nStartRPos;
    sal_uInt16 nRow = nStartRow;

    for( SwTableLines::size_type nLine = 0; nLine < nLines; ++nLine )
    {
        const SwTableLine *pLine = rLines[nLine];

        // Determine the position of the last covered row
        tools::Long nOldRPos = nRPos;
        if( nLine < nLines-1 || nParentLineHeight==0 )
        {
            tools::Long nLineHeight = GetLineHeight( pLine );
            nRPos += nLineHeight;
            if( nParentLineHeight && nStartRPos + nParentLineHeight <= nRPos )
            {
                /* See comment in CollectTableRowCols */
                OSL_FAIL( "Corrupt line height II" );
                nRPos -= nLineHeight;
                nLineHeight = nStartRPos + nParentLineHeight - nRPos; // remaining parent height
                nLineHeight /= nLines - nLine; // divided through the number of remaining sub rows
                nRPos += nLineHeight;
            }
        }
        else
            nRPos = nStartRPos + nParentLineHeight;

        // And their index
        sal_uInt16 nOldRow = nRow;
        SwWriteTableRow aSrchRow( nRPos,m_bUseLayoutHeights );
        SwWriteTableRows::const_iterator it2 = std::find_if(m_aRows.begin(), m_aRows.end(),
                                                 [&](std::unique_ptr<SwWriteTableRow> const &p)
                                                 { return *p == aSrchRow; });

        // coupled methods out of sync ...
        assert( it2 != m_aRows.end() );
        nRow = it2 - m_aRows.begin();

        OSL_ENSURE( nOldRow <= nRow, "Don't look back!" );
        if( nOldRow > nRow )
        {
            nOldRow = nRow;
            if( nOldRow )
                --nOldRow;
        }

        SwWriteTableRow *pRow = m_aRows[nOldRow].get();
        SwWriteTableRow *pEndRow = m_aRows[nRow].get();
        if( nLine+1==nNumOfHeaderRows && nParentLineHeight==0 )
            m_nHeadEndRow = nRow;

        const SwTableBoxes& rBoxes = pLine->GetTabBoxes();

        const SwFrameFormat *pLineFrameFormat = pLine->GetFrameFormat();
        const SfxItemSet& rItemSet = pLineFrameFormat->GetAttrSet();

        tools::Long nHeight = 0;
        if( const SwFormatFrameSize* pFrameSizeItem = rItemSet.GetItemIfSet( RES_FRM_SIZE ))
            nHeight = pFrameSizeItem->GetHeight();

        const SvxBrushItem *pBrushItem, *pLineBrush = pParentBrush;
        if( const SvxBrushItem* pTmpBrush = rItemSet.GetItemIfSet( RES_BACKGROUND, false ) )
        {
            pLineBrush = pTmpBrush;

            // If the row spans the entire table, we can
            // print out the background to the row. Otherwise
            // we have to print out into the cell.
            bool bOutAtRow = !nParentLineWidth;
            if( !bOutAtRow && nStartCPos==0 )
            {
                SwWriteTableCol aCol( nParentLineWidth );
                bOutAtRow = m_aCols.find( &aCol ) == (m_aCols.end() - 1);
            }
            if( bOutAtRow )
            {
                pRow->SetBackground( pLineBrush );
                pBrushItem = nullptr;
            }
            else
                pBrushItem = pLineBrush;
        }
        else
        {
            pRow->SetBackground( pLineBrush );
            pBrushItem = nullptr;
        }

        const SwTableBoxes::size_type nBoxes = rBoxes.size();
        sal_uInt32 nCPos = nStartCPos;
        sal_uInt16 nCol = nStartCol;

        for( SwTableBoxes::size_type nBox=0; nBox<nBoxes; ++nBox )
        {
            const SwTableBox *pBox = rBoxes[nBox];

            // Determine the position of the last covered column
            sal_uInt32 nOldCPos = nCPos;
            if( nBox < nBoxes-1 || (nParentLineWidth==0 && nLine==0) )
            {
                nCPos = nCPos + GetBoxWidth( pBox );
                if( nBox==nBoxes-1 )
                    nParentLineWidth = nCPos - nStartCPos;
            }
            else
                nCPos = nStartCPos + nParentLineWidth;

            // And their index
            sal_uInt16 nOldCol = nCol;
            SwWriteTableCol aSrchCol( nCPos );
            SwWriteTableCols::const_iterator it = m_aCols.find( &aSrchCol );
            OSL_ENSURE( it != m_aCols.end(), "missing column" );
            if(it != m_aCols.end())
            {
                // if find fails for some nCPos value then it used to set nCol value with size of aCols.
                nCol = it - m_aCols.begin();
            }

            if( !ShouldExpandSub( pBox, bSubExpanded, nDepth ) )
            {
                sal_uInt16 nRowSpan = nRow - nOldRow + 1;

                // The new table model may have true row span attributes
                const sal_Int32 nAttrRowSpan = pBox->getRowSpan();
                if ( 1 < nAttrRowSpan )
                    nRowSpan = o3tl::narrowing<sal_uInt16>(nAttrRowSpan);
                else if ( nAttrRowSpan < 1 )
                    nRowSpan = 0;

                SAL_WARN_IF(nCol < nOldCol, "sw.filter", "unexpected " << nCol << " < " << nOldCol);
                sal_uInt16 nColSpan = nCol >= nOldCol ? nCol - nOldCol + 1 : 1;
                pRow->AddCell( pBox, nOldRow, nOldCol,
                               nRowSpan, nColSpan, nHeight,
                               pBrushItem );
                nHeight = 0; // The height requires only to be written once

                if( pBox->GetSttNd() )
                {
                    sal_uInt16 nTopBorder = USHRT_MAX, nBottomBorder = USHRT_MAX;
                    sal_uInt16 nBorderMask = MergeBoxBorders(pBox, nOldRow, nOldCol,
                        nRowSpan, nColSpan, nTopBorder, nBottomBorder);

                    // #i30094# add a sanity check here to ensure that
                    // we don't access an invalid aCols[] as &nCol
                    // above can be changed.
                    if (!(nBorderMask & 4) && nOldCol < m_aCols.size())
                    {
                        SwWriteTableCol *pCol = m_aCols[nOldCol].get();
                        OSL_ENSURE(pCol, "No TableCol found, panic!");
                        if (pCol)
                            pCol->m_bLeftBorder = false;
                    }

                    if (!(nBorderMask & 8))
                    {
                        SwWriteTableCol *pCol = m_aCols[nCol].get();
                        OSL_ENSURE(pCol, "No TableCol found, panic!");
                        if (pCol)
                            pCol->m_bRightBorder = false;
                    }

                    if (!(nBorderMask & 1))
                        pRow->SetTopBorder(false);

                    if (!(nBorderMask & 2))
                        pEndRow->SetBottomBorder(false);
                }
            }
            else
            {
                FillTableRowsCols( nOldRPos, nOldRow, nOldCPos, nOldCol,
                                    nRPos-nOldRPos, nCPos-nOldCPos,
                                    pBox->GetTabLines(),
                                    pLineBrush, nDepth-1,
                                    nNumOfHeaderRows );
                bSubExpanded = true;
            }

            nCol++; // The next cell begins in the next column
        }

        nRow++;
    }
}

SwWriteTable::SwWriteTable(const SwTable* pTable, const SwTableLines& rLines, tools::Long nWidth,
    sal_uInt32 nBWidth, bool bRel, sal_uInt16 nMaxDepth, sal_uInt16 nLSub, sal_uInt16 nRSub, sal_uInt32 nNumOfRowsToRepeat)
    : m_pTable(pTable), m_nBorderColor(ColorTransparency, sal_uInt32(-1)), m_nCellSpacing(0), m_nCellPadding(0), m_nBorder(0),
    m_nInnerBorder(0), m_nBaseWidth(nBWidth), m_nHeadEndRow(USHRT_MAX),
     m_nLeftSub(nLSub), m_nRightSub(nRSub), m_nTabWidth(nWidth), m_bRelWidths(bRel),
    m_bUseLayoutHeights(true),
#ifdef DBG_UTIL
    m_bGetLineHeightCalled(false),
#endif
    m_bColTags(true), m_bLayoutExport(false),
    m_bCollectBorderWidth(true)
{
    sal_uInt32 nParentWidth = m_nBaseWidth + m_nLeftSub + m_nRightSub;

    // First the table structure set. Behind the table is in each
    // case the end of a column
    std::unique_ptr<SwWriteTableCol> pCol(new SwWriteTableCol( nParentWidth ));
    m_aCols.insert( std::move(pCol) );
    m_bUseLayoutHeights = true;
    CollectTableRowsCols( 0, 0, 0, nParentWidth, rLines, nMaxDepth - 1 );

    // FIXME: awfully GetLineHeight writes to this in its first call
    // and proceeds to return a rather odd number fdo#62336, we have to
    // behave identically since the code in FillTableRowsCols duplicates
    // and is highly coupled to CollectTableRowsCols - sadly.
    m_bUseLayoutHeights = true;
    // And now fill with life
    FillTableRowsCols( 0, 0, 0, 0, 0, nParentWidth, rLines, nullptr, nMaxDepth - 1, static_cast< sal_uInt16 >(nNumOfRowsToRepeat) );

    // Adjust some Twip values to pixel boundaries
    if( !m_nBorder )
        m_nBorder = m_nInnerBorder;
}

SwWriteTable::SwWriteTable(const SwTable* pTable, const SwHTMLTableLayout *pLayoutInfo)
    : m_pTable(pTable), m_nBorderColor(ColorTransparency, sal_uInt32(-1)), m_nCellSpacing(0), m_nCellPadding(0), m_nBorder(0),
    m_nInnerBorder(0), m_nBaseWidth(pLayoutInfo->GetWidthOption()), m_nHeadEndRow(0),
    m_nLeftSub(0), m_nRightSub(0), m_nTabWidth(pLayoutInfo->GetWidthOption()),
    m_bRelWidths(pLayoutInfo->HasPercentWidthOption()), m_bUseLayoutHeights(false),
#ifdef DBG_UTIL
    m_bGetLineHeightCalled(false),
#endif
    m_bColTags(pLayoutInfo->HasColTags()), m_bLayoutExport(true),
    m_bCollectBorderWidth(pLayoutInfo->HaveBordersChanged())
{
    if( !m_bCollectBorderWidth )
    {
        m_nBorder = pLayoutInfo->GetBorder();
        m_nCellPadding = pLayoutInfo->GetCellPadding();
        m_nCellSpacing = pLayoutInfo->GetCellSpacing();
    }

    const sal_uInt16 nCols = pLayoutInfo->GetColCount();
    const sal_uInt16 nRows = pLayoutInfo->GetRowCount();

    // First set the table structure.
    for( sal_uInt16 nCol=0; nCol<nCols; ++nCol )
    {
        std::unique_ptr<SwWriteTableCol> pCol(
            new SwWriteTableCol( (nCol+1)*COL_DFLT_WIDTH ));

        if( m_bColTags )
        {
            const SwHTMLTableLayoutColumn *pLayoutCol =
                pLayoutInfo->GetColumn( nCol );
            pCol->SetWidthOpt( pLayoutCol->GetWidthOption(),
                               pLayoutCol->IsRelWidthOption() );
        }

        m_aCols.insert( std::move(pCol) );
    }

    for( sal_uInt16 nRow=0; nRow<nRows; ++nRow )
    {
        std::unique_ptr<SwWriteTableRow> pRow(
            new SwWriteTableRow( (nRow+1)*ROW_DFLT_HEIGHT, m_bUseLayoutHeights ));
        m_aRows.insert( std::move(pRow) );
    }

    // And now fill with life
    for( sal_uInt16 nRow=0; nRow<nRows; ++nRow )
    {
        SwWriteTableRow *pRow = m_aRows[nRow].get();

        bool bHeightExported = false;
        for( sal_uInt16 nCol=0; nCol<nCols; nCol++ )
        {
            const SwHTMLTableLayoutCell *pLayoutCell =
                pLayoutInfo->GetCell( nRow, nCol );

            const SwHTMLTableLayoutCnts *pLayoutCnts =
                pLayoutCell->GetContents().get();

            // The cell begins actually a row above or further forward?
            if( ( nRow>0 && pLayoutCnts == pLayoutInfo->GetCell(nRow-1,nCol)
                                                      ->GetContents().get() ) ||
                ( nCol>0 && pLayoutCnts == pLayoutInfo->GetCell(nRow,nCol-1)
                                                      ->GetContents().get() ) )
            {
                continue;
            }

            const sal_uInt16 nRowSpan = pLayoutCell->GetRowSpan();
            const sal_uInt16 nColSpan = pLayoutCell->GetColSpan();
            const SwTableBox *pBox = pLayoutCnts->GetTableBox();
            OSL_ENSURE( pBox,
                    "Table in Table can not be exported over layout" );

            tools::Long nHeight = bHeightExported ? 0 : GetLineHeight( pBox );
            const SvxBrushItem *pBrushItem = GetLineBrush( pBox, pRow );

            SwWriteTableCell *pCell =
                pRow->AddCell( pBox, nRow, nCol, nRowSpan, nColSpan,
                               nHeight, pBrushItem );
            pCell->SetWidthOpt( pLayoutCell->GetWidthOption(),
                                pLayoutCell->IsPercentWidthOption() );

            sal_uInt16 nTopBorder = USHRT_MAX, nBottomBorder = USHRT_MAX;
            sal_uInt16 nBorderMask =
            MergeBoxBorders( pBox, nRow, nCol, nRowSpan, nColSpan,
                                nTopBorder, nBottomBorder );

            SwWriteTableCol *pCol = m_aCols[nCol].get();
            if( !(nBorderMask & 4) )
                pCol->m_bLeftBorder = false;

            pCol = m_aCols[nCol+nColSpan-1].get();
            if( !(nBorderMask & 8) )
                pCol->m_bRightBorder = false;

            if( !(nBorderMask & 1) )
                pRow->SetTopBorder(false);

            SwWriteTableRow *pEndRow = m_aRows[nRow+nRowSpan-1].get();
            if( !(nBorderMask & 2) )
                pEndRow->SetBottomBorder(false);

            // The height requires only to be written once
            if( nHeight )
                bHeightExported = true;
        }
    }

    // Adjust some Twip values to pixel boundaries
    if( m_bCollectBorderWidth && !m_nBorder )
        m_nBorder = m_nInnerBorder;
}

SwWriteTable::~SwWriteTable()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

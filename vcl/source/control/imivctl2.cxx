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

#include "imivctl.hxx"
#include <sal/log.hxx>

IcnCursor_Impl::IcnCursor_Impl( SvxIconChoiceCtrl_Impl* pOwner )
{
    pView       = pOwner;
    pCurEntry   = nullptr;
    nDeltaWidth = 0;
    nDeltaHeight= 0;
    nCols       = 0;
    nRows       = 0;
}

IcnCursor_Impl::~IcnCursor_Impl()
{
}

sal_uInt16 IcnCursor_Impl::GetSortListPos( SvxIconChoiceCtrlEntryPtrVec& rList, tools::Long nValue,
    bool bVertical )
{
    sal_uInt16 nCount = rList.size();
    if( !nCount )
        return 0;

    sal_uInt16 nCurPos = 0;
    tools::Long nPrevValue = LONG_MIN;
    while( nCount )
    {
        const tools::Rectangle& rRect = pView->GetEntryBoundRect( rList[nCurPos] );
        tools::Long nCurValue;
        if( bVertical )
            nCurValue = rRect.Top();
        else
            nCurValue = rRect.Left();
        if( nValue >= nPrevValue && nValue <= nCurValue )
            return nCurPos;
        nPrevValue = nCurValue;
        nCount--;
        nCurPos++;
    }
    return rList.size();
}

void IcnCursor_Impl::ImplCreate()
{
    pView->CheckBoundingRects();
    DBG_ASSERT(xColumns==nullptr&&xRows==nullptr,"ImplCreate: Not cleared");

    SetDeltas();

    xColumns.reset(new IconChoiceMap);
    xRows.reset(new IconChoiceMap);

    size_t nCount = pView->maEntries.size();
    for( size_t nCur = 0; nCur < nCount; nCur++ )
    {
        SvxIconChoiceCtrlEntry* pEntry = pView->maEntries[ nCur ].get();
        // const Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
        tools::Rectangle rRect( pView->CalcBmpRect( pEntry ) );
        short nY = static_cast<short>( ((rRect.Top()+rRect.Bottom())/2) / nDeltaHeight );
        short nX = static_cast<short>( ((rRect.Left()+rRect.Right())/2) / nDeltaWidth );

        // capture rounding errors
        if( nY >= nRows )
            nY = sal::static_int_cast< short >(nRows - 1);
        if( nX >= nCols )
            nX = sal::static_int_cast< short >(nCols - 1);

        SvxIconChoiceCtrlEntryPtrVec& rColEntry = (*xColumns)[nX];
        sal_uInt16 nIns = GetSortListPos( rColEntry, rRect.Top(), true );
        rColEntry.insert( rColEntry.begin() + nIns, pEntry );

        SvxIconChoiceCtrlEntryPtrVec& rRowEntry = (*xRows)[nY];
        nIns = GetSortListPos( rRowEntry, rRect.Left(), false );
        rRowEntry.insert( rRowEntry.begin() + nIns, pEntry );

        pEntry->nX = nX;
        pEntry->nY = nY;
    }
}


void IcnCursor_Impl::Clear()
{
    if( xColumns )
    {
        xColumns.reset();
        xRows.reset();
        pCurEntry = nullptr;
        nDeltaWidth = 0;
        nDeltaHeight = 0;
    }
}

SvxIconChoiceCtrlEntry* IcnCursor_Impl::SearchCol(sal_uInt16 nCol, sal_uInt16 nTop, sal_uInt16 nBottom,
    bool bDown, bool bSimple )
{
    DBG_ASSERT(pCurEntry, "SearchCol: No reference entry");
    IconChoiceMap::iterator mapIt = xColumns->find( nCol );
    if ( mapIt == xColumns->end() )
        return nullptr;
    SvxIconChoiceCtrlEntryPtrVec const & rList = mapIt->second;
    const sal_uInt16 nCount = rList.size();
    if( !nCount )
        return nullptr;

    const tools::Rectangle& rRefRect = pView->GetEntryBoundRect(pCurEntry);

    if( bSimple )
    {
        SvxIconChoiceCtrlEntryPtrVec::const_iterator it = std::find( rList.begin(), rList.end(), pCurEntry );

        assert(it != rList.end()); //Entry not in Col-List
        if (it == rList.end())
            return nullptr;

        if( bDown )
        {
            while( ++it != rList.end() )
            {
                SvxIconChoiceCtrlEntry* pEntry = *it;
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                if( rRect.Top() > rRefRect.Top() )
                    return pEntry;
            }
            return nullptr;
        }
        else
        {
            SvxIconChoiceCtrlEntryPtrVec::const_reverse_iterator it2(it);
            while (it2 != rList.rend())
            {
                SvxIconChoiceCtrlEntry* pEntry = *it2;
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                if( rRect.Top() < rRefRect.Top() )
                    return pEntry;
                ++it2;
            }
            return nullptr;
        }
    }

    if( nTop > nBottom )
        std::swap(nTop, nBottom);

    tools::Long nMinDistance = LONG_MAX;
    SvxIconChoiceCtrlEntry* pResult = nullptr;
    for( sal_uInt16 nCur = 0; nCur < nCount; nCur++ )
    {
        SvxIconChoiceCtrlEntry* pEntry = rList[ nCur ];
        if( pEntry != pCurEntry )
        {
            sal_uInt16 nY = pEntry->nY;
            if( nY >= nTop && nY <= nBottom )
            {
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                tools::Long nDistance = rRect.Top() - rRefRect.Top();
                if( nDistance < 0 )
                    nDistance *= -1;
                if( nDistance && nDistance < nMinDistance )
                {
                    nMinDistance = nDistance;
                    pResult = pEntry;
                }
            }
        }
    }
    return pResult;
}

SvxIconChoiceCtrlEntry* IcnCursor_Impl::SearchRow(sal_uInt16 nRow, sal_uInt16 nLeft, sal_uInt16 nRight,
    bool bRight, bool bSimple )
{
    DBG_ASSERT(pCurEntry,"SearchRow: No reference entry");
    IconChoiceMap::iterator mapIt = xRows->find( nRow );
    if ( mapIt == xRows->end() )
        return nullptr;
    SvxIconChoiceCtrlEntryPtrVec const & rList = mapIt->second;
    const sal_uInt16 nCount = rList.size();
    if( !nCount )
        return nullptr;

    const tools::Rectangle& rRefRect = pView->GetEntryBoundRect(pCurEntry);

    if( bSimple )
    {
        SvxIconChoiceCtrlEntryPtrVec::const_iterator it = std::find( rList.begin(), rList.end(), pCurEntry );

        assert(it != rList.end()); //Entry not in Row-List
        if (it == rList.end())
            return nullptr;

        if( bRight )
        {
            while( ++it != rList.end() )
            {
                SvxIconChoiceCtrlEntry* pEntry = *it;
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                if( rRect.Left() > rRefRect.Left() )
                    return pEntry;
            }
            return nullptr;
        }
        else
        {
            SvxIconChoiceCtrlEntryPtrVec::const_reverse_iterator it2(it);
            while (it2 != rList.rend())
            {
                SvxIconChoiceCtrlEntry* pEntry = *it2;
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                if( rRect.Left() < rRefRect.Left() )
                    return pEntry;
                ++it2;
            }
            return nullptr;
        }

    }
    if( nRight < nLeft )
        std::swap(nRight, nLeft);

    tools::Long nMinDistance = LONG_MAX;
    SvxIconChoiceCtrlEntry* pResult = nullptr;
    for( sal_uInt16 nCur = 0; nCur < nCount; nCur++ )
    {
        SvxIconChoiceCtrlEntry* pEntry = rList[ nCur ];
        if( pEntry != pCurEntry )
        {
            sal_uInt16 nX = pEntry->nX;
            if( nX >= nLeft && nX <= nRight )
            {
                const tools::Rectangle& rRect = pView->GetEntryBoundRect( pEntry );
                tools::Long nDistance = rRect.Left() - rRefRect.Left();
                if( nDistance < 0 )
                    nDistance *= -1;
                if( nDistance && nDistance < nMinDistance )
                {
                    nMinDistance = nDistance;
                    pResult = pEntry;
                }
            }
        }
    }
    return pResult;
}


/*
    Searches, starting from the passed value, the next entry to the left/to the
    right. Example for bRight = sal_True:

                  c
                b c
              a b c
            S 1 1 1      ====> search direction
              a b c
                b c
                  c

    S : starting position
    1 : first searched rectangle
    a,b,c : 2nd, 3rd, 4th searched rectangle
*/

SvxIconChoiceCtrlEntry* IcnCursor_Impl::GoLeftRight( SvxIconChoiceCtrlEntry* pCtrlEntry, bool bRight )
{
    SvxIconChoiceCtrlEntry* pResult;
    pCurEntry = pCtrlEntry;
    Create();
    sal_uInt16 nY = pCtrlEntry->nY;
    sal_uInt16 nX = pCtrlEntry->nX;
    DBG_ASSERT(nY< nRows,"GoLeftRight:Bad column");
    DBG_ASSERT(nX< nCols,"GoLeftRight:Bad row");
    // neighbor in same row?
    if( bRight )
        pResult = SearchRow(
            nY, nX, sal::static_int_cast< sal_uInt16 >(nCols-1), true, true );
    else
        pResult = SearchRow( nY, 0, nX, false, true );
    if( pResult )
        return pResult;

    tools::Long nCurCol = nX;

    tools::Long nColOffs, nLastCol;
    if( bRight )
    {
        nColOffs = 1;
        nLastCol = nCols;
    }
    else
    {
        nColOffs = -1;
        nLastCol = -1;   // 0-1
    }

    sal_uInt16 nRowMin = nY;
    sal_uInt16 nRowMax = nY;
    do
    {
        SvxIconChoiceCtrlEntry* pEntry = SearchCol(static_cast<sal_uInt16>(nCurCol), nRowMin, nRowMax, true, false);
        if( pEntry )
            return pEntry;
        if( nRowMin )
            nRowMin--;
        if( nRowMax < (nRows-1))
            nRowMax++;
        nCurCol += nColOffs;
    } while( nCurCol != nLastCol );
    return nullptr;
}

SvxIconChoiceCtrlEntry* IcnCursor_Impl::GoPageUpDown( const SvxIconChoiceCtrlEntry* pStart, bool bDown)
{
    const tools::Long nPos = static_cast<tools::Long>(pView->GetEntryListPos( pStart ));
    tools::Long nEntriesInView = pView->aOutputSize.Height() / pView->nGridDY;
    nEntriesInView *=
        ((pView->aOutputSize.Width()+(pView->nGridDX/2)) / pView->nGridDX );
    tools::Long nNewPos = nPos;
    if( bDown )
    {
        nNewPos += nEntriesInView;
        if( nNewPos >= static_cast<tools::Long>(pView->maEntries.size()) )
            nNewPos = pView->maEntries.size() - 1;
    }
    else
    {
        nNewPos -= nEntriesInView;
        if( nNewPos < 0 )
            nNewPos = 0;
    }
    if( nPos != nNewPos )
        return pView->maEntries[ static_cast<size_t>(nNewPos) ].get();
    return nullptr;
}

SvxIconChoiceCtrlEntry* IcnCursor_Impl::GoUpDown( const SvxIconChoiceCtrlEntry* pCtrlEntry, bool bDown)
{
    sal_uLong nPos = pView->GetEntryListPos( pCtrlEntry );
    if( bDown && nPos < (pView->maEntries.size() - 1) )
        return pView->maEntries[ nPos + 1 ].get();
    else if( !bDown && nPos > 0 )
        return pView->maEntries[ nPos - 1 ].get();
    return nullptr;
}

void IcnCursor_Impl::SetDeltas()
{
    const Size& rSize = pView->aVirtOutputSize;
    nCols = rSize.Width() / pView->nGridDX;
    if( !nCols )
        nCols = 1;
    nRows = rSize.Height() / pView->nGridDY;
    if( (nRows * pView->nGridDY) < rSize.Height() )
        nRows++;
    if( !nRows )
        nRows = 1;

    nDeltaWidth = static_cast<short>(rSize.Width() / nCols);
    nDeltaHeight = static_cast<short>(rSize.Height() / nRows);
    if( !nDeltaHeight )
    {
        nDeltaHeight = 1;
        SAL_INFO("vcl", "SetDeltas:Bad height");
    }
    if( !nDeltaWidth )
    {
        nDeltaWidth = 1;
        SAL_INFO("vcl", "SetDeltas:Bad width");
    }
}

IcnGridMap_Impl::IcnGridMap_Impl(SvxIconChoiceCtrl_Impl* pView)
 : _pView(pView), _nGridCols(0), _nGridRows(0)
{
}

IcnGridMap_Impl::~IcnGridMap_Impl()
{
}

void IcnGridMap_Impl::Expand()
{
    if( !_pGridMap )
        Create_Impl();
    else
    {
        sal_uInt16 nNewGridRows = _nGridRows;
        sal_uInt16 nNewGridCols = _nGridCols;
        nNewGridCols += 50;

        size_t nNewCellCount = static_cast<size_t>(nNewGridRows) * nNewGridCols;
        bool* pNewGridMap = new bool[nNewCellCount];
        size_t nOldCellCount = static_cast<size_t>(_nGridRows) * _nGridCols;
        memcpy(pNewGridMap, _pGridMap.get(), nOldCellCount * sizeof(bool));
        memset(pNewGridMap + nOldCellCount, 0, (nNewCellCount-nOldCellCount) * sizeof(bool));
        _pGridMap.reset( pNewGridMap );
        _nGridRows = nNewGridRows;
        _nGridCols = nNewGridCols;
    }
}

void IcnGridMap_Impl::Create_Impl()
{
    DBG_ASSERT(!_pGridMap,"Unnecessary call to IcnGridMap_Impl::Create_Impl()");
    if( _pGridMap )
        return;
    GetMinMapSize( _nGridCols, _nGridRows );
    _nGridCols += 50;

    size_t nCellCount = static_cast<size_t>(_nGridRows) * _nGridCols;
    _pGridMap.reset( new bool[nCellCount] );
    memset(_pGridMap.get(), 0, nCellCount * sizeof(bool));

    const size_t nCount = _pView->maEntries.size();
    for( size_t nCur=0; nCur < nCount; nCur++ )
        OccupyGrids( _pView->maEntries[ nCur ].get() );
}

void IcnGridMap_Impl::GetMinMapSize( sal_uInt16& rDX, sal_uInt16& rDY ) const
{
    // The view grows in horizontal direction. Its max. height is _pView->nMaxVirtHeight
    tools::Long nY = _pView->nMaxVirtHeight;
    if( !nY )
        nY = _pView->pView->GetOutputSizePixel().Height();
    if( !(_pView->nFlags & IconChoiceFlags::Arranging) )
        nY -= _pView->nHorSBarHeight;

    tools::Long nX = _pView->aVirtOutputSize.Width();

    if( !nX )
        nX = DEFAULT_MAX_VIRT_WIDTH;
    if( !nY )
        nY = DEFAULT_MAX_VIRT_HEIGHT;

    tools::Long nDX = nX / _pView->nGridDX;
    tools::Long nDY = nY / _pView->nGridDY;

    if( !nDX )
        nDX++;
    if( !nDY )
        nDY++;

    rDX = static_cast<sal_uInt16>(nDX);
    rDY = static_cast<sal_uInt16>(nDY);
}

GridId IcnGridMap_Impl::GetGrid( sal_uInt16 nGridX, sal_uInt16 nGridY )
{
    Create();
    return nGridY + ( static_cast<GridId>(nGridX) * _nGridRows );
}

GridId IcnGridMap_Impl::GetGrid( const Point& rDocPos )
{
    Create();

    tools::Long nX = rDocPos.X();
    tools::Long nY = rDocPos.Y();
    nX -= LROFFS_WINBORDER;
    nY -= TBOFFS_WINBORDER;
    nX /= _pView->nGridDX;
    nY /= _pView->nGridDY;
    if( nX >= _nGridCols )
    {
        nX = _nGridCols - 1;
    }
    if( nY >= _nGridRows )
    {
        nY = _nGridRows - 1;
    }
    GridId nId = GetGrid( static_cast<sal_uInt16>(nX), static_cast<sal_uInt16>(nY) );
    DBG_ASSERT(nId <o3tl::make_unsigned(_nGridCols*_nGridRows),"GetGrid failed");
    return nId;
}

tools::Rectangle IcnGridMap_Impl::GetGridRect( GridId nId )
{
    Create();
    sal_uInt16 nGridX, nGridY;
    GetGridCoord( nId, nGridX, nGridY );
    const tools::Long nLeft = nGridX * _pView->nGridDX+ LROFFS_WINBORDER;
    const tools::Long nTop = nGridY * _pView->nGridDY + TBOFFS_WINBORDER;
    return tools::Rectangle(
        nLeft, nTop,
        nLeft + _pView->nGridDX,
        nTop + _pView->nGridDY );
}

GridId IcnGridMap_Impl::GetUnoccupiedGrid()
{
    Create();
    sal_uLong nStart = 0;
    bool bExpanded = false;

    while( true )
    {
        const sal_uLong nCount = static_cast<sal_uInt16>(_nGridCols * _nGridRows);
        for( sal_uLong nCur = nStart; nCur < nCount; nCur++ )
        {
            if( !_pGridMap[ nCur ] )
            {
                _pGridMap[ nCur ] = true;
                return static_cast<GridId>(nCur);
            }
        }
        DBG_ASSERT(!bExpanded,"ExpandGrid failed");
        if( bExpanded )
            return 0; // prevent never ending loop
        bExpanded = true;
        Expand();
        nStart = nCount;
    }
}

// An entry only means that there's a GridRect lying under its center. This
// variant is much faster than allocating via the bounding rectangle but can
// lead to small overlaps.
void IcnGridMap_Impl::OccupyGrids( const SvxIconChoiceCtrlEntry* pEntry )
{
    if( !_pGridMap || !SvxIconChoiceCtrl_Impl::IsBoundingRectValid( pEntry->aRect ))
        return;
    OccupyGrid( GetGrid( pEntry->aRect.Center()) );
}

void IcnGridMap_Impl::Clear()
{
    if( _pGridMap )
    {
        _pGridMap.reset();
        _nGridRows = 0;
        _nGridCols = 0;
        _aLastOccupiedGrid.SetEmpty();
    }
}

sal_uLong IcnGridMap_Impl::GetGridCount( const Size& rSizePixel, sal_uInt16 nDX, sal_uInt16 nDY)
{
    tools::Long ndx = (rSizePixel.Width() - LROFFS_WINBORDER) / nDX;
    if( ndx < 0 ) ndx *= -1;
    tools::Long ndy = (rSizePixel.Height() - TBOFFS_WINBORDER) / nDY;
    if( ndy < 0 ) ndy *= -1;
    return static_cast<sal_uLong>(ndx * ndy);
}

void IcnGridMap_Impl::OutputSizeChanged()
{
    if( !_pGridMap )
        return;

    sal_uInt16 nCols, nRows;
    GetMinMapSize( nCols, nRows );
    if( nRows != _nGridRows )
        Clear();
    else if( nCols >= _nGridCols )
        Expand();
}

// the gridmap should contain the data in a continuous region, to make it possible
// to copy the whole block if the gridmap needs to be expanded.
void IcnGridMap_Impl::GetGridCoord( GridId nId, sal_uInt16& rGridX, sal_uInt16& rGridY )
{
    rGridX = static_cast<sal_uInt16>(nId / _nGridRows);
    rGridY = static_cast<sal_uInt16>(nId % _nGridRows);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

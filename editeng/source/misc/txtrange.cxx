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


#include <editeng/txtrange.hxx>
#include <math.h>
#include <tools/poly.hxx>
#include <tools/debug.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

#include <vector>

TextRanger::TextRanger( const basegfx::B2DPolyPolygon& rPolyPolygon,
                        const basegfx::B2DPolyPolygon* pLinePolyPolygon,
                        sal_uInt16 nCacheSz, sal_uInt16 nLft, sal_uInt16 nRght,
                        bool bSimpl, bool bInnr, bool bVert ) :
    maPolyPolygon( rPolyPolygon.count() ),
    nCacheSize( nCacheSz ),
    nRight( nRght ),
    nLeft( nLft ),
    nUpper( 0 ),
    nLower( 0 ),
    nPointCount( 0 ),
    bSimple( bSimpl ),
    bInner( bInnr ),
    bVertical( bVert )
{
    sal_uInt32 nCount(rPolyPolygon.count());

    for(sal_uInt32 i(0); i < nCount; i++)
    {
        const basegfx::B2DPolygon aCandidate(rPolyPolygon.getB2DPolygon(i).getDefaultAdaptiveSubdivision());
        nPointCount += aCandidate.count();
        maPolyPolygon.Insert( tools::Polygon(aCandidate), static_cast<sal_uInt16>(i) );
    }

    if( pLinePolyPolygon )
    {
        nCount = pLinePolyPolygon->count();
        mpLinePolyPolygon = tools::PolyPolygon(nCount);

        for(sal_uInt32 i(0); i < nCount; i++)
        {
            const basegfx::B2DPolygon aCandidate(pLinePolyPolygon->getB2DPolygon(i).getDefaultAdaptiveSubdivision());
            nPointCount += aCandidate.count();
            mpLinePolyPolygon->Insert( tools::Polygon(aCandidate), static_cast<sal_uInt16>(i) );
        }
    }
    else
        mpLinePolyPolygon.reset();
}


TextRanger::~TextRanger()
{
    mRangeCache.clear();
}

/* TextRanger::SetVertical(..)
   If there's is a change in the writing direction,
   the cache has to be cleared.
*/
void TextRanger::SetVertical( bool bNew )
{
    if( IsVertical() != bNew )
    {
        bVertical = bNew;
        mRangeCache.clear();
    }
}

namespace {

//! SvxBoundArgs is used to perform temporary calculations on a range array.
//! Temporary instances are created in TextRanger::GetTextRanges()
class SvxBoundArgs
{
    std::vector<bool> aBoolArr;
    std::deque<tools::Long>* pLongArr;
    TextRanger *pTextRanger;
    tools::Long nMin;
    tools::Long nMax;
    tools::Long nTop;
    tools::Long nBottom;
    tools::Long nUpDiff;
    tools::Long nLowDiff;
    tools::Long nUpper;
    tools::Long nLower;
    tools::Long nStart;
    tools::Long nEnd;
    sal_uInt16 nCut;
    sal_uInt16 nLast;
    sal_uInt16 nNext;
    sal_uInt8 nAct;
    sal_uInt8 nFirst;
    bool bClosed : 1;
    bool bInner : 1;
    bool bMultiple : 1;
    bool bConcat : 1;
    bool bRotate : 1;
    void NoteRange( bool bToggle );
    tools::Long Cut( tools::Long nY, const Point& rPt1, const Point& rPt2 );
    void Add();
    void NoteFarPoint_( tools::Long nPx, tools::Long nPyDiff, tools::Long nDiff );
    void NoteFarPoint( tools::Long nPx, tools::Long nPyDiff, tools::Long nDiff )
        { if( nDiff ) NoteFarPoint_( nPx, nPyDiff, nDiff ); }
    tools::Long CalcMax( const Point& rPt1, const Point& rPt2, tools::Long nRange, tools::Long nFar );
    void CheckCut( const Point& rLst, const Point& rNxt );
    tools::Long A( const Point& rP ) const { return bRotate ? rP.Y() : rP.X(); }
    tools::Long B( const Point& rP ) const { return bRotate ? rP.X() : rP.Y(); }
public:
    SvxBoundArgs( TextRanger* pRanger, std::deque<tools::Long>* pLong, const Range& rRange );
    void NotePoint( const tools::Long nA ) { NoteMargin( nA - nStart, nA + nEnd ); }
    void NoteMargin( const tools::Long nL, const tools::Long nR )
        { if( nMin > nL ) nMin = nL; if( nMax < nR ) nMax = nR; }
    sal_uInt16 Area( const Point& rPt );
    void NoteUpLow( tools::Long nA, const sal_uInt8 nArea );
    void Calc( const tools::PolyPolygon& rPoly );
    void Concat( const tools::PolyPolygon* pPoly );
    // inlines
    void NoteLast() { if( bMultiple ) NoteRange( nAct == nFirst ); }
    void SetConcat( const bool bNew ){ bConcat = bNew; }
    bool IsConcat() const { return bConcat; }
};

}

SvxBoundArgs::SvxBoundArgs( TextRanger* pRanger, std::deque<tools::Long>* pLong,
    const Range& rRange )
    : pLongArr(pLong)
    , pTextRanger(pRanger)
    , nMin(0)
    , nMax(0)
    , nTop(rRange.Min())
    , nBottom(rRange.Max())
    , nCut(0)
    , nLast(0)
    , nNext(0)
    , nAct(0)
    , nFirst(0)
    , bClosed(false)
    , bInner(pRanger->IsInner())
    , bMultiple(bInner || !pRanger->IsSimple())
    , bConcat(false)
    , bRotate(pRanger->IsVertical())
{
    if( bRotate )
    {
        nStart = pRanger->GetUpper();
        nEnd = pRanger->GetLower();
        nLowDiff = pRanger->GetLeft();
        nUpDiff = pRanger->GetRight();
    }
    else
    {
        nStart = pRanger->GetLeft();
        nEnd = pRanger->GetRight();
        nLowDiff = pRanger->GetUpper();
        nUpDiff = pRanger->GetLower();
    }
    nUpper = nTop - nUpDiff;
    nLower = nBottom + nLowDiff;
    pLongArr->clear();
}

tools::Long SvxBoundArgs::CalcMax( const Point& rPt1, const Point& rPt2,
    tools::Long nRange, tools::Long nFarRange )
{
    double nDa = Cut( nRange, rPt1, rPt2 ) - Cut( nFarRange, rPt1, rPt2 );
    double nB;
    if( nDa < 0 )
    {
        nDa = -nDa;
        nB = nEnd;
    }
    else
        nB = nStart;

    nB = std::hypot(nB, nDa);

    if (nB == 0) // avoid div / 0
        return 0;

    nB = nRange + nDa * ( nFarRange - nRange ) / nB;

    bool bNote;
    if( nB < B(rPt2) )
        bNote = nB > B(rPt1);
    else
        bNote = nB < B(rPt1);
    if( bNote )
        return( tools::Long( nB ) );
    return 0;
}

void SvxBoundArgs::CheckCut( const Point& rLst, const Point& rNxt )
{
    if( nCut & 1 )
        NotePoint( Cut( nBottom, rLst, rNxt ) );
    if( nCut & 2 )
        NotePoint( Cut( nTop, rLst, rNxt ) );
    if( rLst.X() == rNxt.X() || rLst.Y() == rNxt.Y() )
        return;

    tools::Long nYps;
    if( nLowDiff && ( ( nCut & 1 ) || nLast == 1 || nNext == 1 ) )
    {
        nYps = CalcMax( rLst, rNxt, nBottom, nLower );
        if( nYps )
            NoteFarPoint_( Cut( nYps, rLst, rNxt ), nLower-nYps, nLowDiff );
    }
    if( nUpDiff && ( ( nCut & 2 ) || nLast == 2 || nNext == 2 ) )
    {
        nYps = CalcMax( rLst, rNxt, nTop, nUpper );
        if( nYps )
            NoteFarPoint_( Cut( nYps, rLst, rNxt ), nYps-nUpper, nUpDiff );
    }
}

void SvxBoundArgs::NoteFarPoint_( tools::Long nPa, tools::Long nPbDiff, tools::Long nDiff )
{
    tools::Long nTmpA;
    double nQuot = 2 * nDiff - nPbDiff;
    nQuot *= nPbDiff;
    nQuot = sqrt( nQuot );
    nQuot /= nDiff;
    nTmpA = nPa - tools::Long( nStart * nQuot );
    nPbDiff = nPa + tools::Long( nEnd * nQuot );
    NoteMargin( nTmpA, nPbDiff );
}

void SvxBoundArgs::NoteRange( bool bToggle )
{
    DBG_ASSERT( nMax >= nMin || bInner, "NoteRange: Min > Max?");
    if( nMax < nMin )
        return;
    if( !bClosed )
        bToggle = false;
    sal_uInt16 nIdx = 0;
    sal_uInt16 nCount = pLongArr->size();
    DBG_ASSERT( nCount == 2 * aBoolArr.size(), "NoteRange: Incompatible Sizes" );
    while( nIdx < nCount && (*pLongArr)[ nIdx ] < nMin )
        ++nIdx;
    bool bOdd = (nIdx % 2) != 0;
    // No overlap with existing intervals?
    if( nIdx == nCount || ( !bOdd && nMax < (*pLongArr)[ nIdx ] ) )
    {   // Then a new one is inserted ...
        pLongArr->insert( pLongArr->begin() + nIdx, nMin );
        pLongArr->insert( pLongArr->begin() + nIdx + 1, nMax );
        aBoolArr.insert( aBoolArr.begin() + (nIdx/2), bToggle );
    }
    else
    {   // expand an existing interval ...
        sal_uInt16 nMaxIdx = nIdx;
        // If we end up on a left interval boundary, it must be reduced to nMin.
        if( bOdd )
            --nIdx;
        else
            (*pLongArr)[ nIdx ] = nMin;
        while( nMaxIdx < nCount && (*pLongArr)[ nMaxIdx ] < nMax )
            ++nMaxIdx;
        DBG_ASSERT( nMaxIdx > nIdx || nMin == nMax, "NoteRange: Funny Situation." );
        if( nMaxIdx )
            --nMaxIdx;
        if( nMaxIdx < nIdx )
            nMaxIdx = nIdx;
        // If we end up on a right interval boundary, it must be raised to nMax.
        if( nMaxIdx % 2 )
            (*pLongArr)[ nMaxIdx-- ] = nMax;
        // Possible merge of intervals.
        sal_uInt16 nDiff = nMaxIdx - nIdx;
        nMaxIdx = nIdx / 2; // From here on is nMaxIdx the Index in BoolArray.
        if( nDiff )
        {
            pLongArr->erase( pLongArr->begin() + nIdx + 1, pLongArr->begin() + nIdx + 1 + nDiff );
            nDiff /= 2;
            sal_uInt16 nStop = nMaxIdx + nDiff;
            for( sal_uInt16 i = nMaxIdx; i < nStop; ++i )
                bToggle ^= aBoolArr[ i ];
            aBoolArr.erase( aBoolArr.begin() + nMaxIdx, aBoolArr.begin() + (nMaxIdx + nDiff) );
        }
        DBG_ASSERT( nMaxIdx < aBoolArr.size(), "NoteRange: Too much deleted" );
        aBoolArr[ nMaxIdx ] = aBoolArr[ nMaxIdx ] != bToggle;
    }
}

void SvxBoundArgs::Calc( const tools::PolyPolygon& rPoly )
{
    sal_uInt16 nCount;
    nAct = 0;
    for( auto const& rPol : rPoly )
    {
        nCount = rPol.GetSize();
        if( nCount )
        {
            const Point& rNull = rPol[ 0 ];
            bClosed = IsConcat() || ( rNull == rPol[ nCount - 1 ] );
            nLast = Area( rNull );
            if( nLast & 12 )
            {
                nFirst = 3;
                if( bMultiple )
                    nAct = 0;
            }
            else
            {
                // The first point of the polygon is within the line.
                if( nLast )
                {
                    if( bMultiple || !nAct )
                    {
                        nMin = USHRT_MAX;
                        nMax = 0;
                    }
                    if( nLast & 1 )
                        NoteFarPoint( A(rNull), nLower - B(rNull), nLowDiff );
                    else
                        NoteFarPoint( A(rNull), B(rNull) - nUpper, nUpDiff );
                }
                else
                {
                    if( bMultiple || !nAct )
                    {
                        nMin = A(rNull);
                        nMax = nMin + nEnd;
                        nMin -= nStart;
                    }
                    else
                        NotePoint( A(rNull) );
                }
                nFirst = 0; // leaving the line in which direction?
                nAct = 3;   // we are within the line at the moment.
            }
            if( nCount > 1 )
            {
                sal_uInt16 nIdx = 1;
                while( true )
                {
                    const Point& rLast = rPol[ nIdx - 1 ];
                    if( nIdx == nCount )
                        nIdx = 0;
                    const Point& rNext = rPol[ nIdx ];
                    nNext = Area( rNext );
                    nCut = nNext ^ nLast;
                    sal_uInt16 nOldAct = nAct;
                    if( nAct )
                        CheckCut( rLast, rNext );
                    if( nCut & 4 )
                    {
                        NoteUpLow( Cut( nLower, rLast, rNext ), 2 );
                        if( nAct && nAct != nOldAct )
                        {
                            nOldAct = nAct;
                            CheckCut( rLast, rNext );
                        }
                    }
                    if( nCut & 8 )
                    {
                        NoteUpLow( Cut( nUpper, rLast, rNext ), 1 );
                        if( nAct && nAct != nOldAct )
                            CheckCut( rLast, rNext );
                    }
                    if( !nIdx )
                    {
                        if( !( nNext & 12 ) )
                            NoteLast();
                        break;
                    }
                    if( !( nNext & 12 ) )
                    {
                        if( !nNext )
                            NotePoint( A(rNext) );
                        else if( nNext & 1 )
                            NoteFarPoint( A(rNext), nLower-B(rNext), nLowDiff );
                        else
                            NoteFarPoint( A(rNext), B(rNext)-nUpper, nUpDiff );
                    }
                    nLast = nNext;
                    if( ++nIdx == nCount && !bClosed )
                    {
                        if( !( nNext & 12 ) )
                            NoteLast();
                        break;
                    }
                }
            }
            if( bMultiple && IsConcat() )
            {
                Add();
                nAct = 0;
            }
        }
    }
    if( !bMultiple )
    {
        DBG_ASSERT( pLongArr->empty(), "I said: Simple!" );
        if( nAct )
        {
            if( bInner )
            {
                tools::Long nTmpMin = nMin + 2 * nStart;
                tools::Long nTmpMax = nMax - 2 * nEnd;
                if( nTmpMin <= nTmpMax )
                {
                    pLongArr->push_front(nTmpMax);
                    pLongArr->push_front(nTmpMin);
                }
            }
            else
            {
                pLongArr->push_front(nMax);
                pLongArr->push_front(nMin);
            }
        }
    }
    else if( !IsConcat() )
        Add();
}

void SvxBoundArgs::Add()
{
    size_t nCount = aBoolArr.size();
    if( nCount && ( !bInner || !pTextRanger->IsSimple() ) )
    {
        bool bDelete = aBoolArr.front();
        if( bInner )
            bDelete = !bDelete;
        sal_uInt16 nLongIdx = 1;
        for( size_t nBoolIdx = 1; nBoolIdx < nCount; ++nBoolIdx )
        {
            if( bDelete )
            {
                sal_uInt16 next = 2;
                while( nBoolIdx < nCount && !aBoolArr[ nBoolIdx++ ] &&
                       (!bInner || nBoolIdx < nCount ) )
                    next += 2;
                pLongArr->erase( pLongArr->begin() + nLongIdx, pLongArr->begin() + nLongIdx + next );
                next /= 2;
                nBoolIdx = nBoolIdx - next;
                nCount = nCount - next;
                aBoolArr.erase( aBoolArr.begin() + nBoolIdx, aBoolArr.begin() + (nBoolIdx + next) );
                if (nBoolIdx > 0)
                    aBoolArr[ nBoolIdx - 1 ] = false;
#if OSL_DEBUG_LEVEL > 1
                else
                    ++next;
#endif
            }
            bDelete = nBoolIdx < nCount && aBoolArr[ nBoolIdx ];
            nLongIdx += 2;
            DBG_ASSERT( nLongIdx == 2*nBoolIdx+1, "BoundArgs: Array-Idx Confusion" );
            DBG_ASSERT( aBoolArr.size()*2 == pLongArr->size(),
                        "BoundArgs: Array-Count: Confusion" );
        }
    }
    if( pLongArr->empty() )
        return;

    if( !bInner )
        return;

    pLongArr->pop_front();
    pLongArr->pop_back();

    // Here the line is held inside a large rectangle for "simple"
    // contour wrap. Currently (April 1999) the EditEngine evaluates
    // only the first rectangle. If it one day is able to output a line
    // in several parts, it may be advisable to delete the following lines.
    if( pTextRanger->IsSimple() && pLongArr->size() > 2 )
        pLongArr->erase( pLongArr->begin() + 1, pLongArr->end() - 1 );
}

void SvxBoundArgs::Concat( const tools::PolyPolygon* pPoly )
{
    SetConcat( true );
    DBG_ASSERT( pPoly, "Nothing to do?" );
    std::deque<tools::Long>* pOld = pLongArr;
    pLongArr = new std::deque<tools::Long>;
    aBoolArr.clear();
    bInner = false;
    Calc( *pPoly ); // Note that this updates pLongArr, which is why we swapped it out earlier.
    std::deque<tools::Long>::size_type nCount = pLongArr->size();
    std::deque<tools::Long>::size_type nIdx = 0;
    std::deque<tools::Long>::size_type i = 0;
    bool bSubtract = pTextRanger->IsInner();
    while( i < nCount )
    {
        std::deque<tools::Long>::size_type nOldCount = pOld->size();
        if( nIdx == nOldCount )
        {   // Reached the end of the old Array...
            if( !bSubtract )
                pOld->insert( pOld->begin() + nIdx, pLongArr->begin() + i, pLongArr->end() );
            break;
        }
        tools::Long nLeft = (*pLongArr)[ i++ ];
        tools::Long nRight = (*pLongArr)[ i++ ];
        std::deque<tools::Long>::size_type nLeftPos = nIdx + 1;
        while( nLeftPos < nOldCount && nLeft > (*pOld)[ nLeftPos ] )
            nLeftPos += 2;
        if( nLeftPos >= nOldCount )
        {   // The current interval belongs to the end of the old array ...
            if( !bSubtract )
                pOld->insert( pOld->begin() + nOldCount, pLongArr->begin() + i - 2, pLongArr->end() );
            break;
        }
        std::deque<tools::Long>::size_type nRightPos = nLeftPos - 1;
        while( nRightPos < nOldCount && nRight >= (*pOld)[ nRightPos ] )
            nRightPos += 2;
        if( nRightPos < nLeftPos )
        {   // The current interval belongs between two old intervals
            if( !bSubtract )
                pOld->insert( pOld->begin() + nRightPos, pLongArr->begin() + i - 2, pLongArr->begin() + i );
        }
        else if( bSubtract ) // Subtract, if necessary separate
        {
            const tools::Long nOld = (*pOld)[nLeftPos - 1];
            if (nLeft > nOld)
            {   // Now we split the left part...
                if( nLeft - 1 > nOld )
                {
                    pOld->insert( pOld->begin() + nLeftPos - 1, nOld );
                    pOld->insert( pOld->begin() + nLeftPos, nLeft - 1 );
                    nLeftPos += 2;
                    nRightPos += 2;
                }
            }
            if( nRightPos - nLeftPos > 1 )
                pOld->erase( pOld->begin() + nLeftPos, pOld->begin() + nRightPos - 1 );
            if (++nRight >= (*pOld)[nLeftPos])
                pOld->erase( pOld->begin() + nLeftPos - 1, pOld->begin() + nLeftPos + 1 );
            else
                (*pOld)[ nLeftPos - 1 ] = nRight;
        }
        else // Merge
        {
            if( nLeft < (*pOld)[ nLeftPos - 1 ] )
                (*pOld)[ nLeftPos - 1 ] = nLeft;
            if( nRight > (*pOld)[ nRightPos - 1 ] )
                (*pOld)[ nRightPos - 1 ] = nRight;
            if( nRightPos - nLeftPos > 1 )
                pOld->erase( pOld->begin() + nLeftPos, pOld->begin() + nRightPos - 1 );

        }
        nIdx = nLeftPos - 1;
    }
    delete pLongArr;
}

/*************************************************************************
 * SvxBoundArgs::Area returns the area in which the point is located.
 * 0 = within the line
 * 1 = below, but within the upper edge
 * 2 = above, but within the lower edge
 * 5 = below the upper edge
 *10 = above the lower edge
 *************************************************************************/

sal_uInt16 SvxBoundArgs::Area( const Point& rPt )
{
    tools::Long nB = B( rPt );
    if( nB >= nBottom )
    {
        if( nB >= nLower )
            return 5;
        return 1;
    }
    if( nB <= nTop )
    {
        if( nB <= nUpper )
            return 10;
        return 2;
    }
    return 0;
}

/*************************************************************************
 * lcl_Cut calculates the X-Coordinate of the distance (Pt1-Pt2) at the
 * Y-Coordinate nY.
 * It is assumed that the one of the points are located above and the other
 * one below the Y-Coordinate.
 *************************************************************************/

tools::Long SvxBoundArgs::Cut( tools::Long nB, const Point& rPt1, const Point& rPt2 )
{
    if( pTextRanger->IsVertical() )
    {
        double nQuot = nB - rPt1.X();
        nQuot /= ( rPt2.X() - rPt1.X() );
        nQuot *= ( rPt2.Y() - rPt1.Y() );
        return tools::Long( rPt1.Y() + nQuot );
    }
    double nQuot = nB - rPt1.Y();
    nQuot /= ( rPt2.Y() - rPt1.Y() );
    nQuot *= ( rPt2.X() - rPt1.X() );
    return tools::Long( rPt1.X() + nQuot );
}

void SvxBoundArgs::NoteUpLow( tools::Long nA, const sal_uInt8 nArea )
{
    if( nAct )
    {
        NoteMargin( nA, nA );
        if( bMultiple )
        {
            NoteRange( nArea != nAct );
            nAct = 0;
        }
        if( !nFirst )
            nFirst = nArea;
    }
    else
    {
        nAct = nArea;
        nMin = nA;
        nMax = nA;
    }
}

std::deque<tools::Long>* TextRanger::GetTextRanges( const Range& rRange )
{
    DBG_ASSERT( rRange.Min() || rRange.Max(), "Zero-Range not allowed, Bye Bye" );
    //Can we find the result we need in the cache?
    for (auto & elem : mRangeCache)
    {
        if (elem.range == rRange)
            return &(elem.results);
    }
    //Calculate a new result
    RangeCacheItem rngCache(rRange);
    SvxBoundArgs aArg( this, &(rngCache.results), rRange );
    aArg.Calc( maPolyPolygon );
    if( mpLinePolyPolygon )
        aArg.Concat( &*mpLinePolyPolygon );
    //Add new result to the cache
    mRangeCache.push_back(std::move(rngCache));
    if (mRangeCache.size() > nCacheSize)
        mRangeCache.pop_front();
    return &(mRangeCache.back().results);
}

const tools::Rectangle& TextRanger::GetBoundRect_() const
{
    DBG_ASSERT( !mxBound, "Don't call twice." );
    mxBound = maPolyPolygon.GetBoundRect();
    return *mxBound;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

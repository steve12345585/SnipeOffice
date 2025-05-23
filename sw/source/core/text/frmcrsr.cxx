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

#include <ndtxt.hxx>
#include <pam.hxx>
#include <frmatr.hxx>
#include <frmtool.hxx>
#include <viewopt.hxx>
#include <paratr.hxx>
#include <rootfrm.hxx>
#include <pagefrm.hxx>
#include <colfrm.hxx>
#include <swtypes.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/tstpitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/lspcitem.hxx>
#include "pormulti.hxx"
#include <doc.hxx>
#include <IDocumentDeviceAccess.hxx>
#include <sortedobjs.hxx>

#include <unicode/ubidi.h>

#include <txtfrm.hxx>
#include "inftxt.hxx"
#include "itrtxt.hxx"
#include <crstate.hxx>
#include <viewsh.hxx>
#include <swfntcch.hxx>
#include <flyfrm.hxx>

#define MIN_OFFSET_STEP 10

using namespace ::com::sun::star;

/*
 * - SurvivalKit: For how long do we get past the last char of the line.
 * - RightMargin abstains from adjusting position with -1
 * - GetCharRect returns a GetEndCharRect for CursorMoveState::RightMargin
 * - GetEndCharRect sets bRightMargin to true
 * - SwTextCursor::bRightMargin is set to false by CharCursorToLine
 */

namespace
{

SwTextFrame *GetAdjFrameAtPos( SwTextFrame *pFrame, const SwPosition &rPos,
                          const bool bRightMargin, const bool bNoScroll = true )
{
    // RightMargin in the last master line
    TextFrameIndex const nOffset = pFrame->MapModelToViewPos(rPos);
    SwTextFrame *pFrameAtPos = pFrame;
    if( !bNoScroll || pFrame->GetFollow() )
    {
        pFrameAtPos = pFrame->GetFrameAtPos( rPos );
        if (nOffset < pFrameAtPos->GetOffset() &&
            !pFrameAtPos->IsFollow() )
        {
            assert(pFrameAtPos->MapModelToViewPos(rPos) == nOffset);
            TextFrameIndex nNew(nOffset);
            if (nNew < TextFrameIndex(MIN_OFFSET_STEP))
                nNew = TextFrameIndex(0);
            else
                nNew -= TextFrameIndex(MIN_OFFSET_STEP);
            sw_ChangeOffset( pFrameAtPos, nNew );
        }
    }
    while( pFrame != pFrameAtPos )
    {
        pFrame = pFrameAtPos;
        pFrame->GetFormatted();
        pFrameAtPos = pFrame->GetFrameAtPos( rPos );
    }

    if( nOffset && bRightMargin )
    {
        while (pFrameAtPos &&
               pFrameAtPos->MapViewToModelPos(pFrameAtPos->GetOffset()) == rPos &&
               pFrameAtPos->IsFollow() )
        {
            pFrameAtPos->GetFormatted();
            pFrameAtPos = pFrameAtPos->FindMaster();
        }
        OSL_ENSURE( pFrameAtPos, "+GetCharRect: no frame with my rightmargin" );
    }
    return pFrameAtPos ? pFrameAtPos : pFrame;
}

}

bool sw_ChangeOffset(SwTextFrame* pFrame, TextFrameIndex nNew)
{
    // Do not scroll in areas and outside of flies
    OSL_ENSURE( !pFrame->IsFollow(), "Illegal Scrolling by Follow!" );
    if( pFrame->GetOffset() != nNew && !pFrame->IsInSct() )
    {
        SwFlyFrame *pFly = pFrame->FindFlyFrame();
        // Attention: if e.g. in a column frame the size is still invalid
        // we must not scroll around just like that
        if ( ( pFly && pFly->isFrameAreaDefinitionValid() &&
             !pFly->GetNextLink() && !pFly->GetPrevLink() ) ||
             ( !pFly && pFrame->IsInTab() ) )
        {
            SwViewShell* pVsh = pFrame->getRootFrame()->GetCurrShell();
            if( pVsh )
            {
                if( pVsh->GetRingContainer().size() > 1 ||
                    ( pFrame->GetDrawObjs() && pFrame->GetDrawObjs()->size() ) )
                {
                    if( !pFrame->GetOffset() )
                        return false;
                    nNew = TextFrameIndex(0);
                }
                pFrame->SetOffset( nNew );
                pFrame->SetPara( nullptr );
                pFrame->GetFormatted();
                if( pFrame->getFrameArea().HasArea() )
                    pFrame->getRootFrame()->GetCurrShell()->InvalidateWindows( pFrame->getFrameArea() );
                return true;
            }
        }
    }
    return false;
}

SwTextFrame& SwTextFrame::GetFrameAtOfst(TextFrameIndex const nWhere)
{
    SwTextFrame* pRet = this;
    while( pRet->HasFollow() && nWhere >= pRet->GetFollow()->GetOffset() )
        pRet = pRet->GetFollow();
    return *pRet;
}

SwTextFrame *SwTextFrame::GetFrameAtPos( const SwPosition &rPos )
{
    TextFrameIndex const nPos(MapModelToViewPos(rPos));
    SwTextFrame *pFoll = this;
    while( pFoll->GetFollow() )
    {
        if (nPos > pFoll->GetFollow()->GetOffset())
            pFoll = pFoll->GetFollow();
        else
        {
            if (nPos == pFoll->GetFollow()->GetOffset()
                 && !SwTextCursor::IsRightMargin() )
                 pFoll = pFoll->GetFollow();
            else
                break;
        }
    }
    return pFoll;
}

/*
 * GetCharRect() returns the char's char line described by aPos.
 * GetModelPositionForViewPoint() does the reverse: It goes from a document coordinate to
 * a Pam.
 * Both are virtual in the frame base class and thus are redefined here.
 */

bool SwTextFrame::GetCharRect( SwRect& rOrig, const SwPosition &rPos,
                            SwCursorMoveState *pCMS, bool bAllowFarAway ) const
{
    OSL_ENSURE( ! IsVertical() || ! IsSwapped(),"SwTextFrame::GetCharRect with swapped frame" );

    if (IsLocked())
        return false;

    // Find the right frame first. We need to keep in mind that:
    // - the cached information could be invalid  (GetPara() == 0)
    // - we could have a Follow
    // - the Follow chain grows dynamically; the one we end up in
    //   needs to be formatted

    // Optimisation: reading ahead saves us a GetAdjFrameAtPos
    const bool bRightMargin = pCMS && ( CursorMoveState::RightMargin == pCMS->m_eState );
    const bool bNoScroll = pCMS && pCMS->m_bNoScroll;
    SwTextFrame *pFrame = GetAdjFrameAtPos( const_cast<SwTextFrame*>(this), rPos, bRightMargin,
                                     bNoScroll );
    pFrame->GetFormatted();

    const SwFrame* pTmpFrame = pFrame->GetUpper();
    if (pTmpFrame->getFrameArea().Top() == FAR_AWAY && !bAllowFarAway)
        return false;

    SwRectFnSet aRectFnSet(pFrame);
    const SwTwips nUpperMaxY = aRectFnSet.GetPrtBottom(*pTmpFrame);
    const SwTwips nFrameMaxY = aRectFnSet.GetPrtBottom(*pFrame);

    // nMaxY is an absolute value
    SwTwips nMaxY = aRectFnSet.IsVert() ?
                    ( aRectFnSet.IsVertL2R() ? std::min( nFrameMaxY, nUpperMaxY ) : std::max( nFrameMaxY, nUpperMaxY ) ) :
                    std::min( nFrameMaxY, nUpperMaxY );

    bool bRet = false;

    if ( pFrame->IsEmpty() || ! aRectFnSet.GetHeight(pFrame->getFramePrintArea()) )
    {
        Point aPnt1 = pFrame->getFrameArea().Pos() + pFrame->getFramePrintArea().Pos();
        SwTextNode const*const pTextNd(GetTextNodeForParaProps());
        short nFirstOffset;
        pTextNd->GetFirstLineOfsWithNum(nFirstOffset, {});

        Point aPnt2;
        if ( aRectFnSet.IsVert() )
        {
            if( nFirstOffset > 0 )
                aPnt1.AdjustY(nFirstOffset );
            if ( aPnt1.X() < nMaxY && !aRectFnSet.IsVertL2R() )
                aPnt1.setX( nMaxY );
            aPnt2.setX( aPnt1.X() + pFrame->getFramePrintArea().Width() );
            aPnt2.setY( aPnt1.Y() );
            if( aPnt2.X() < nMaxY )
                aPnt2.setX( nMaxY );
        }
        else
        {
            if( nFirstOffset > 0 )
                aPnt1.AdjustX(nFirstOffset );

            if( aPnt1.Y() > nMaxY )
                aPnt1.setY( nMaxY );
            aPnt2.setX( aPnt1.X() );
            aPnt2.setY( aPnt1.Y() + pFrame->getFramePrintArea().Height() );
            if( aPnt2.Y() > nMaxY )
                aPnt2.setY( nMaxY );
        }

        rOrig = SwRect( aPnt1, aPnt2 );

        if ( pCMS )
        {
            pCMS->m_aRealHeight.setX( 0 );
            pCMS->m_aRealHeight.setY( aRectFnSet.IsVert() ? -rOrig.Width() : rOrig.Height() );
        }

        if ( pFrame->IsRightToLeft() )
            pFrame->SwitchLTRtoRTL( rOrig );

        bRet = true;
    }
    else
    {
        if( !pFrame->HasPara() )
            return false;

        SwFrameSwapper aSwapper( pFrame, true );
        if ( aRectFnSet.IsVert() )
            nMaxY = pFrame->SwitchVerticalToHorizontal( nMaxY );

        bool bGoOn = true;
        TextFrameIndex const nOffset = MapModelToViewPos(rPos);
        assert(nOffset != TextFrameIndex(COMPLETE_STRING)); // not going to end well
        TextFrameIndex nNextOfst;

        do
        {
            {
                SwTextSizeInfo aInf( pFrame );
                SwTextCursor  aLine( pFrame, &aInf );
                nNextOfst = aLine.GetEnd();
                // See comment in AdjustFrame
                // Include the line's last char?
                if (bRightMargin)
                    aLine.GetEndCharRect( &rOrig, nOffset, pCMS, nMaxY );
                else
                    aLine.GetCharRect( &rOrig, nOffset, pCMS, nMaxY );
                bRet = true;
            }

            if ( pFrame->IsRightToLeft() )
                pFrame->SwitchLTRtoRTL( rOrig );

            if ( aRectFnSet.IsVert() )
                pFrame->SwitchHorizontalToVertical( rOrig );

            if( pFrame->IsUndersized() && pCMS && !pFrame->GetNext() &&
                aRectFnSet.GetBottom(rOrig) == nUpperMaxY &&
                pFrame->GetOffset() < nOffset &&
                !pFrame->IsFollow() && !bNoScroll &&
                TextFrameIndex(pFrame->GetText().getLength()) != nNextOfst)
            {
                bGoOn = sw_ChangeOffset( pFrame, nNextOfst );
            }
            else
                bGoOn = false;
        } while ( bGoOn );

        if ( pCMS )
        {
            if ( pFrame->IsRightToLeft() )
            {
                if( pCMS->m_b2Lines && pCMS->m_p2Lines)
                {
                    pFrame->SwitchLTRtoRTL( pCMS->m_p2Lines->aLine );
                    pFrame->SwitchLTRtoRTL( pCMS->m_p2Lines->aPortion );
                }
            }

            if ( aRectFnSet.IsVert() )
            {
                if ( pCMS->m_bRealHeight )
                {
                    pCMS->m_aRealHeight.setY( -pCMS->m_aRealHeight.Y() );
                    if ( pCMS->m_aRealHeight.Y() < 0 )
                    {
                        // writing direction is from top to bottom
                        pCMS->m_aRealHeight.setX(  rOrig.Width() -
                                                   pCMS->m_aRealHeight.X() +
                                                   pCMS->m_aRealHeight.Y() );
                    }
                }
                if( pCMS->m_b2Lines && pCMS->m_p2Lines)
                {
                    pFrame->SwitchHorizontalToVertical( pCMS->m_p2Lines->aLine );
                    pFrame->SwitchHorizontalToVertical( pCMS->m_p2Lines->aPortion );
                }
            }

        }
    }
    if( bRet )
    {
        SwPageFrame *pPage = pFrame->FindPageFrame();
        assert(pPage && "Text escaped from page?");
        const SwTwips nOrigTop = aRectFnSet.GetTop(rOrig);
        const SwTwips nPageTop = aRectFnSet.GetTop(pPage->getFrameArea());
        const SwTwips nPageBott = aRectFnSet.GetBottom(pPage->getFrameArea());

        // We have the following situation: if the frame is in an invalid
        // sectionframe, it's possible that the frame is outside the page.
        // If we restrict the cursor position to the page area, we enforce
        // the formatting of the page, of the section frame and the frame itself.
        if( aRectFnSet.YDiff( nPageTop, nOrigTop ) > 0 )
            aRectFnSet.SetTop( rOrig, nPageTop );

        if ( aRectFnSet.YDiff( nOrigTop, nPageBott ) > 0 )
            aRectFnSet.SetTop( rOrig, nPageBott );
    }

    return bRet;
}

/*
 * GetAutoPos() looks up the char's char line which is described by rPos
 * and is used by the auto-positioned frame.
 */

bool SwTextFrame::GetAutoPos( SwRect& rOrig, const SwPosition &rPos ) const
{
    if( IsHiddenNow() )
        return false;

    TextFrameIndex const nOffset = MapModelToViewPos(rPos);
    SwTextFrame* pFrame = &(const_cast<SwTextFrame*>(this)->GetFrameAtOfst( nOffset ));

    pFrame->GetFormatted();
    const SwFrame* pTmpFrame = pFrame->GetUpper();

    SwRectFnSet aRectFnSet(pTmpFrame);
    SwTwips nUpperMaxY = aRectFnSet.GetPrtBottom(*pTmpFrame);

    // nMaxY is in absolute value
    SwTwips nMaxY;
    if ( aRectFnSet.IsVert() )
    {
        if ( aRectFnSet.IsVertL2R() )
            nMaxY = std::min( SwTwips(aRectFnSet.GetPrtBottom(*pFrame)), nUpperMaxY );
        else
            nMaxY = std::max( SwTwips(aRectFnSet.GetPrtBottom(*pFrame)), nUpperMaxY );
    }
    else
        nMaxY = std::min( SwTwips(aRectFnSet.GetPrtBottom(*pFrame)), nUpperMaxY );
    if ( pFrame->IsEmpty() || ! aRectFnSet.GetHeight(pFrame->getFramePrintArea()) )
    {
        Point aPnt1 = pFrame->getFrameArea().Pos() + pFrame->getFramePrintArea().Pos();
        Point aPnt2;
        if ( aRectFnSet.IsVert() )
        {
            if ( aPnt1.X() < nMaxY && !aRectFnSet.IsVertL2R() )
                aPnt1.setX( nMaxY );

            aPnt2.setX( aPnt1.X() + pFrame->getFramePrintArea().Width() );
            aPnt2.setY( aPnt1.Y() );
            if( aPnt2.X() < nMaxY )
                aPnt2.setX( nMaxY );
        }
        else
        {
            if( aPnt1.Y() > nMaxY )
                aPnt1.setY( nMaxY );
            aPnt2.setX( aPnt1.X() );
            aPnt2.setY( aPnt1.Y() + pFrame->getFramePrintArea().Height() );
            if( aPnt2.Y() > nMaxY )
                aPnt2.setY( nMaxY );
        }
        rOrig = SwRect( aPnt1, aPnt2 );
        return true;
    }
    else
    {
        if( !pFrame->HasPara() )
            return false;

        SwFrameSwapper aSwapper( pFrame, true );
        if ( aRectFnSet.IsVert() )
            nMaxY = pFrame->SwitchVerticalToHorizontal( nMaxY );

        SwTextSizeInfo aInf( pFrame );
        SwTextCursor aLine( pFrame, &aInf );
        SwCursorMoveState aTmpState( CursorMoveState::SetOnlyText );
        aTmpState.m_bRealHeight = true;
        aLine.GetCharRect( &rOrig, nOffset, &aTmpState, nMaxY );
        if( aTmpState.m_aRealHeight.X() >= 0 )
        {
            rOrig.Pos().AdjustY(aTmpState.m_aRealHeight.X() );
            rOrig.Height( aTmpState.m_aRealHeight.Y() );
        }

        if ( pFrame->IsRightToLeft() )
            pFrame->SwitchLTRtoRTL( rOrig );

        if ( aRectFnSet.IsVert() )
            pFrame->SwitchHorizontalToVertical( rOrig );

        return true;
    }
}

/** determine top of line for given position in the text frame

    - Top of first paragraph line is the top of the printing area of the text frame
    - If a proportional line spacing is applied use top of anchor character as
      top of the line.
*/
bool SwTextFrame::GetTopOfLine( SwTwips& _onTopOfLine,
                             const SwPosition& _rPos ) const
{
    bool bRet = true;

    // get position offset
    TextFrameIndex const nOffset = MapModelToViewPos(_rPos);

    if (TextFrameIndex(GetText().getLength()) < nOffset)
    {
        bRet = false;
    }
    else
    {
        SwRectFnSet aRectFnSet(this);
        if ( IsEmpty() || !aRectFnSet.GetHeight(getFramePrintArea()) )
        {
            // consider upper space amount considered
            // for previous frame and the page grid.
            _onTopOfLine = aRectFnSet.GetPrtTop(*this);
        }
        else
        {
            // determine formatted text frame that contains the requested position
            SwTextFrame* pFrame = &(const_cast<SwTextFrame*>(this)->GetFrameAtOfst( nOffset ));
            pFrame->GetFormatted();
            aRectFnSet.Refresh(pFrame);
            // If proportional line spacing is applied
            // to the text frame, the top of the anchor character is also the
            // top of the line.
            // Otherwise the line layout determines the top of the line
            const SvxLineSpacingItem& rSpace = GetAttrSet()->GetLineSpacing();
            if ( rSpace.GetInterLineSpaceRule() == SvxInterLineSpaceRule::Prop )
            {
                SwRect aCharRect;
                if ( GetAutoPos( aCharRect, _rPos ) )
                {
                    _onTopOfLine = aRectFnSet.GetTop(aCharRect);
                }
                else
                {
                    bRet = false;
                }
            }
            else
            {
                // assure that text frame is in a horizontal layout
                SwFrameSwapper aSwapper( pFrame, true );
                // determine text line that contains the requested position
                SwTextSizeInfo aInf( pFrame );
                SwTextCursor aLine( pFrame, &aInf );
                aLine.CharCursorToLine( nOffset );
                // determine top of line
                _onTopOfLine = aLine.Y();
                if ( aRectFnSet.IsVert() )
                {
                    _onTopOfLine = pFrame->SwitchHorizontalToVertical( _onTopOfLine );
                }
            }
        }
    }

    return bRet;
}

// Minimum distance of non-empty lines is a little less than 2 cm
#define FILL_MIN_DIST 1100

struct SwFillData
{
    SwRect aFrame;
    const SwCursorMoveState *pCMS;
    SwPosition* pPos;
    const Point& rPoint;
    SwTwips nLineWidth;
    bool bFirstLine : 1;
    bool bInner     : 1;
    bool bColumn    : 1;
    bool bEmpty     : 1;
    SwFillData( const SwCursorMoveState *pC, SwPosition* pP, const SwRect& rR,
        const Point& rPt ) : aFrame( rR ), pCMS( pC ), pPos( pP ), rPoint( rPt ),
        nLineWidth( 0 ), bFirstLine( true ), bInner( false ), bColumn( false ),
        bEmpty( true ){}
    SwFillMode Mode() const { return pCMS->m_pFill->eMode; }
    tools::Long X() const { return rPoint.X(); }
    tools::Long Y() const { return rPoint.Y(); }
    tools::Long Left() const { return aFrame.Left(); }
    tools::Long Right() const { return aFrame.Right(); }
    tools::Long Bottom() const { return aFrame.Bottom(); }
    SwFillCursorPos &Fill() const { return *pCMS->m_pFill; }
    void SetTab( sal_uInt16 nNew ) { pCMS->m_pFill->nTabCnt = nNew; }
    void SetSpace( sal_uInt16 nNew ) { pCMS->m_pFill->nSpaceCnt = nNew; }
    void SetSpaceOnly( sal_uInt16 nNew ) { pCMS->m_pFill->nSpaceOnlyCnt = nNew; }
    void SetOrient( const sal_Int16 eNew ){ pCMS->m_pFill->eOrient = eNew; }
};

bool SwTextFrame::GetModelPositionForViewPoint_(SwPosition* pPos, const Point& rPoint,
                    const bool bChgFrame, SwCursorMoveState* pCMS ) const
{
    // GetModelPositionForViewPoint_ is called by GetModelPositionForViewPoint and GetKeyCursorOfst.
    // Never just a return false.

    if( IsLocked() || IsHiddenNow() )
        return false;

    const_cast<SwTextFrame*>(this)->GetFormatted();

    Point aOldPoint( rPoint );

    if ( IsVertical() )
    {
        SwitchVerticalToHorizontal( const_cast<Point&>(rPoint) );
        const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();
    }

    if ( IsRightToLeft() )
        SwitchRTLtoLTR( const_cast<Point&>(rPoint) );

    std::unique_ptr<SwFillData> pFillData;
    if ( pCMS && pCMS->m_pFill )
        pFillData.reset(new SwFillData( pCMS, pPos, getFrameArea(), rPoint ));

    if ( IsEmpty() )
    {
        *pPos = MapViewToModelPos(TextFrameIndex(0));
        if( pCMS && pCMS->m_bFieldInfo )
        {
            SwTwips nDiff = rPoint.X() - getFrameArea().Left() - getFramePrintArea().Left();
            if( nDiff > 50 || nDiff < 0 )
                pCMS->m_bPosCorr = true;
        }
    }
    else
    {
        SwTextSizeInfo aInf( const_cast<SwTextFrame*>(this) );
        SwTextCursor  aLine( const_cast<SwTextFrame*>(this), &aInf );

        // See comment in AdjustFrame()
        SwTwips nMaxY = getFrameArea().Top() + getFramePrintArea().Top() + getFramePrintArea().Height();
        aLine.TwipsToLine( rPoint.Y() );
        while( aLine.Y() + aLine.GetLineHeight() > nMaxY )
        {
            if( !aLine.Prev() )
                break;
        }

        if( aLine.GetDropLines() >= aLine.GetLineNr() && 1 != aLine.GetLineNr()
            && rPoint.X() < aLine.FirstLeft() + aLine.GetDropLeft() )
            while( aLine.GetLineNr() > 1 )
                aLine.Prev();

        TextFrameIndex nOffset = aLine.GetModelPositionForViewPoint(pPos, rPoint, bChgFrame, pCMS);

        if( pCMS && pCMS->m_eState == CursorMoveState::NONE && aLine.GetEnd() == nOffset )
            pCMS->m_eState = CursorMoveState::RightMargin;

    // pPos is a pure IN parameter and must not be evaluated.
    // pIter->GetModelPositionForViewPoint returns from a nesting with COMPLETE_STRING.
    // If SwTextIter::GetModelPositionForViewPoint calls GetModelPositionForViewPoint further by itself
    // nNode changes the position.
    // In such cases, pPos must not be calculated.
        if (TextFrameIndex(COMPLETE_STRING) != nOffset)
        {
            *pPos = MapViewToModelPos(nOffset);
            if( pFillData )
            {
                if (TextFrameIndex(GetText().getLength()) > nOffset ||
                    rPoint.Y() < getFrameArea().Top() )
                    pFillData->bInner = true;
                pFillData->bFirstLine = aLine.GetLineNr() < 2;
                if (GetText().getLength())
                {
                    pFillData->bEmpty = false;
                    pFillData->nLineWidth = aLine.GetCurr()->Width();
                }
            }
        }
    }
    bool bChgFillData = false;
    if( pFillData && FindPageFrame()->getFrameArea().Contains( aOldPoint ) )
    {
        FillCursorPos( *pFillData );
        bChgFillData = true;
    }

    if ( IsVertical() )
    {
        if ( bChgFillData )
            SwitchHorizontalToVertical( pFillData->Fill().aCursor.Pos() );
        const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();
    }

    if ( IsRightToLeft() && bChgFillData )
    {
            SwitchLTRtoRTL( pFillData->Fill().aCursor.Pos() );
            const sal_Int16 eOrient = pFillData->pCMS->m_pFill->eOrient;

            if ( text::HoriOrientation::LEFT == eOrient )
                pFillData->SetOrient( text::HoriOrientation::RIGHT );
            else if ( text::HoriOrientation::RIGHT == eOrient )
                pFillData->SetOrient( text::HoriOrientation::LEFT );
    }

    const_cast<Point&>(rPoint) = aOldPoint;

    return true;
}

bool SwTextFrame::GetModelPositionForViewPoint(SwPosition* pPos, Point& rPoint,
                               SwCursorMoveState* pCMS, bool ) const
{
    const bool bChgFrame = !(pCMS && CursorMoveState::UpDown == pCMS->m_eState);
    return GetModelPositionForViewPoint_( pPos, rPoint, bChgFrame, pCMS );
}

/*
 * Layout-oriented cursor movement to the line start.
 */

bool SwTextFrame::LeftMargin(SwPaM *pPam) const
{
    assert(GetMergedPara() || &pPam->GetPointNode() == static_cast<SwContentNode const*>(GetDep()));

    SwTextFrame *pFrame = GetAdjFrameAtPos( const_cast<SwTextFrame*>(this), *pPam->GetPoint(),
                                     SwTextCursor::IsRightMargin() );
    pFrame->GetFormatted();
    TextFrameIndex nIndx;
    if ( pFrame->IsEmpty() )
        nIndx = TextFrameIndex(0);
    else
    {
        SwTextSizeInfo aInf( pFrame );
        SwTextCursor  aLine( pFrame, &aInf );

        aLine.CharCursorToLine(pFrame->MapModelToViewPos(*pPam->GetPoint()));
        nIndx = aLine.GetStart();
        if( pFrame->GetOffset() && !pFrame->IsFollow() && !aLine.GetPrev() )
        {
            sw_ChangeOffset(pFrame, TextFrameIndex(0));
            nIndx = TextFrameIndex(0);
        }
    }
    *pPam->GetPoint() = pFrame->MapViewToModelPos(nIndx);
    SwTextCursor::SetRightMargin( false );
    return true;
}

bool SwTextFrame::IsInHyphenatedWord(SwPaM *pPam, bool bSelection) const
{
    assert(GetMergedPara() || &pPam->GetPointNode() == static_cast<SwContentNode const*>(GetDep()));

    SwTextFrame *pFrame = GetAdjFrameAtPos( const_cast<SwTextFrame*>(this), *pPam->GetPoint(),
                                     SwTextCursor::IsRightMargin() );
    pFrame->GetFormatted();
    if (!IsEmpty())
    {
        SwTextSizeInfo aInf( pFrame );
        SwTextCursor  aLine( pFrame, &aInf );
        TextFrameIndex const nCursorPos(MapModelToViewPos(*pPam->GetPoint()));
        aLine.CharCursorToLine(nCursorPos);
        if ( aLine.GetCurr()->IsEndHyph() )
        {
           TextFrameIndex nPos(aLine.GetStart() + aLine.GetCurr()->GetLen());
           while( nPos > nCursorPos && ' ' != aInf.GetText()[sal_Int32(nPos) - 1] )
               --nPos;
           if ( nPos == nCursorPos && ( bSelection ||
                // without selection, the cursor must be inside the word, not before that
                // to apply the character formatting, as usual
                ( nPos > aLine.GetStart() && ' ' != aInf.GetText()[sal_Int32(nPos) - 1] ) ) )
                return true;
        }
        // the hyphenated word starts in the previous line
        if ( aLine.GetStart() > TextFrameIndex(0) )
        {
            TextFrameIndex nPos(aLine.GetStart());
            aLine.CharCursorToLine(nPos - TextFrameIndex(1));
            if ( aLine.GetCurr()->IsEndHyph() )
            {
                while( nPos < nCursorPos && ' ' != aInf.GetText()[sal_Int32(nPos)] )
                    ++nPos;
                if ( nPos == nCursorPos &&
                     ( bSelection || ' ' != aInf.GetText()[sal_Int32(nPos)] ) )
                     return true;
            }
        }
    }
    return false;
}

/*
 * To the line end: That's the position before the last char of the line.
 * Exception: In the last line, it should be able to place the cursor after
 * the last char in order to append text.
 */

bool SwTextFrame::RightMargin(SwPaM *pPam, bool bAPI) const
{
    assert(GetMergedPara() || &pPam->GetPointNode() == static_cast<SwContentNode const*>(GetDep()));

    SwTextFrame *pFrame = GetAdjFrameAtPos( const_cast<SwTextFrame*>(this), *pPam->GetPoint(),
                                     SwTextCursor::IsRightMargin() );
    pFrame->GetFormatted();
    TextFrameIndex nRightMargin(0);
    if (!IsEmpty())
    {
        SwTextSizeInfo aInf( pFrame );
        SwTextCursor  aLine( pFrame, &aInf );

        aLine.CharCursorToLine(MapModelToViewPos(*pPam->GetPoint()));
        nRightMargin = aLine.GetStart() + aLine.GetCurr()->GetLen();

        // We skip hard line breaks
        if( aLine.GetCurr()->GetLen() &&
            CH_BREAK == aInf.GetText()[sal_Int32(nRightMargin) - 1])
            --nRightMargin;
        else if( !bAPI && (aLine.GetNext() || pFrame->GetFollow()) )
        {
            while( nRightMargin > aLine.GetStart() &&
                ' ' == aInf.GetText()[sal_Int32(nRightMargin) - 1])
                --nRightMargin;
        }
    }
    *pPam->GetPoint() = pFrame->MapViewToModelPos(nRightMargin);
    SwTextCursor::SetRightMargin( !bAPI );
    return true;
}

// The following two methods try to put the Cursor into the next/successive
// line. If we do not have a preceding/successive line we forward the call
// to the base class.
// The Cursor's horizontal justification is done afterwards by the CursorShell.

namespace {

class SwSetToRightMargin
{
    bool m_bRight;

public:
    SwSetToRightMargin()
        : m_bRight(false)
    {
    }
    ~SwSetToRightMargin() { SwTextCursor::SetRightMargin(m_bRight); }
    void SetRight(const bool bNew) { m_bRight = bNew; }
};

}

bool SwTextFrame::UnitUp_( SwPaM *pPam, const SwTwips nOffset,
                        bool bSetInReadOnly ) const
{
    // Set the RightMargin if needed
    SwSetToRightMargin aSet;

    if( IsInTab() &&
        pPam->GetPointNode().StartOfSectionNode() !=
        pPam->GetMarkNode().StartOfSectionNode() )
    {
        // If the PaM is located within different boxes, we have a table selection,
        // which is handled by the base class.
        return SwContentFrame::UnitUp( pPam, nOffset, bSetInReadOnly );
    }

    const_cast<SwTextFrame*>(this)->GetFormatted();
    const TextFrameIndex nPos = MapModelToViewPos(*pPam->GetPoint());
    SwRect aCharBox;

    if( !IsEmpty() && !IsHiddenNow() )
    {
        TextFrameIndex nFormat(COMPLETE_STRING);
        do
        {
            if (nFormat != TextFrameIndex(COMPLETE_STRING) && !IsFollow())
                sw_ChangeOffset( const_cast<SwTextFrame*>(this), nFormat );

            SwTextSizeInfo aInf( const_cast<SwTextFrame*>(this) );
            SwTextCursor  aLine( const_cast<SwTextFrame*>(this), &aInf );

            // Optimize away flys with no flow and IsDummy()
            if( nPos )
                aLine.CharCursorToLine( nPos );
            else
                aLine.Top();

            const SwLineLayout *pPrevLine = aLine.GetPrevLine();
            const TextFrameIndex nStart = aLine.GetStart();
            aLine.GetCharRect( &aCharBox, nPos );

            bool bSecondOfDouble = ( aInf.IsMulti() && ! aInf.IsFirstMulti() );
            bool bPrevLine = ( pPrevLine && pPrevLine != aLine.GetCurr() );

            if( !pPrevLine && !bSecondOfDouble && GetOffset() && !IsFollow() )
            {
                nFormat = GetOffset();
                TextFrameIndex nDiff = aLine.GetLength();
                if( !nDiff )
                    nDiff = TextFrameIndex(MIN_OFFSET_STEP);
                if( nFormat > nDiff )
                    nFormat = nFormat - nDiff;
                else
                    nFormat = TextFrameIndex(0);
                continue;
            }

            // We select the target line for the cursor, in case we are in a
            // double line portion, prev line = curr line
            if( bPrevLine && !bSecondOfDouble )
            {
                aLine.PrevLine();
                while ( aLine.GetStart() == nStart &&
                        nullptr != ( pPrevLine = aLine.GetPrevLine() ) &&
                        pPrevLine != aLine.GetCurr() )
                    aLine.PrevLine();
            }

            if ( bPrevLine || bSecondOfDouble )
            {
                aCharBox.Width( aCharBox.SSize().Width() / 2 );
                aCharBox.Pos().setX( aCharBox.Pos().X() - 150 );

                // See comment in SwTextFrame::GetModelPositionForViewPoint()
#if OSL_DEBUG_LEVEL > 0
                const SwNodeOffset nOldNode = pPam->GetPoint()->GetNodeIndex();
#endif
                // The node should not be changed
                TextFrameIndex nTmpOfst = aLine.GetModelPositionForViewPoint(pPam->GetPoint(),
                                                         aCharBox.Pos(), false );
#if OSL_DEBUG_LEVEL > 0
                OSL_ENSURE( nOldNode == pPam->GetPoint()->GetNodeIndex(),
                        "SwTextFrame::UnitUp: illegal node change" );
#endif

                // We make sure that we move up.
                if( nTmpOfst >= nStart && nStart && !bSecondOfDouble )
                {
                    nTmpOfst = nStart;
                    aSet.SetRight( true );
                }
                *pPam->GetPoint() = MapViewToModelPos(nTmpOfst);
                return true;
            }

            if ( IsFollow() )
            {
                aLine.GetCharRect( &aCharBox, nPos );
                aCharBox.Width( aCharBox.SSize().Width() / 2 );
            }
            break;
        } while ( true );
    }
    /* If 'this' is a follow and a prev failed, we need to go to the
     * last line of the master, which is us.
     * Or: If we are a follow with follow, we need to get the master.
     */
    if ( IsFollow() )
    {
        const SwTextFrame *pTmpPrev = FindMaster();
        TextFrameIndex nOffs = GetOffset();
        if( pTmpPrev )
        {
            SwViewShell *pSh = getRootFrame()->GetCurrShell();
            const bool bProtectedAllowed = pSh && pSh->GetViewOptions()->IsCursorInProtectedArea();
            const SwTextFrame *pPrevPrev = pTmpPrev;
            // We skip protected frames and frames without content here
            while( pPrevPrev && ( pPrevPrev->GetOffset() == nOffs ||
                   ( !bProtectedAllowed && pPrevPrev->IsProtected() ) ) )
            {
                pTmpPrev = pPrevPrev;
                nOffs = pTmpPrev->GetOffset();
                if ( pPrevPrev->IsFollow() )
                    pPrevPrev = pTmpPrev->FindMaster();
                else
                    pPrevPrev = nullptr;
            }
            if ( !pPrevPrev )
                return pTmpPrev->SwContentFrame::UnitUp( pPam, nOffset, bSetInReadOnly );
            aCharBox.Pos().setY( pPrevPrev->getFrameArea().Bottom() - 1 );
            return pPrevPrev->GetKeyCursorOfst( pPam->GetPoint(), aCharBox.Pos() );
        }
    }
    return SwContentFrame::UnitUp( pPam, nOffset, bSetInReadOnly );
}

// Used for Bidi. nPos is the logical position in the string, bLeft indicates
// if left arrow or right arrow was pressed. The return values are:
// nPos: the new visual position
// bLeft: whether the break iterator has to add or subtract from the
//        current position
static void lcl_VisualMoveRecursion(const SwLineLayout& rCurrLine, TextFrameIndex nIdx,
                              TextFrameIndex & nPos, bool& bRight,
                              sal_uInt8& nCursorLevel, sal_uInt8 nDefaultDir )
{
    const SwLinePortion* pPor = rCurrLine.GetFirstPortion();
    const SwLinePortion* pLast = nullptr;

    // What's the current portion?
    while ( pPor && nIdx + pPor->GetLen() <= nPos )
    {
        nIdx = nIdx + pPor->GetLen();
        pLast = pPor;
        pPor = pPor->GetNextPortion();
    }

    if ( bRight )
    {
        bool bRecurse = pPor && pPor->IsMultiPortion() &&
                           static_cast<const SwMultiPortion*>(pPor)->IsBidi();

        // 1. special case: at beginning of bidi portion
        if ( bRecurse && nIdx == nPos )
        {
            nPos = nPos + pPor->GetLen();

            // leave bidi portion
            if ( nCursorLevel != nDefaultDir )
            {
                bRecurse = false;
            }
            else
                // special case:
                // buffer: abcXYZ123 in LTR paragraph
                // view:   abc123ZYX
                // cursor is between c and X in the buffer and cursor level = 0
                nCursorLevel++;
        }

        // 2. special case: at beginning of portion after bidi portion
        else if ( pLast && pLast->IsMultiPortion() &&
                 static_cast<const SwMultiPortion*>(pLast)->IsBidi() && nIdx == nPos )
        {
            // enter bidi portion
            if ( nCursorLevel != nDefaultDir )
            {
                bRecurse = true;
                nIdx = nIdx - pLast->GetLen();
                pPor = pLast;
            }
        }

        // Recursion
        if ( bRecurse )
        {
            const SwLineLayout& rLine = static_cast<const SwMultiPortion*>(pPor)->GetRoot();
            TextFrameIndex nTmpPos = nPos - nIdx;
            bool bTmpForward = ! bRight;
            sal_uInt8 nTmpCursorLevel = nCursorLevel;
            lcl_VisualMoveRecursion(rLine, TextFrameIndex(0), nTmpPos, bTmpForward,
                                     nTmpCursorLevel, nDefaultDir + 1 );

            nPos = nTmpPos + nIdx;
            bRight = bTmpForward;
            nCursorLevel = nTmpCursorLevel;
        }

        // go forward
        else
        {
            bRight = true;
            nCursorLevel = nDefaultDir;
        }

    }
    else
    {
        bool bRecurse = pPor && pPor->IsMultiPortion() && static_cast<const SwMultiPortion*>(pPor)->IsBidi();

        // 1. special case: at beginning of bidi portion
        if ( bRecurse && nIdx == nPos )
        {
            // leave bidi portion
            if ( nCursorLevel == nDefaultDir )
            {
                bRecurse = false;
            }
        }

        // 2. special case: at beginning of portion after bidi portion
        else if ( pLast && pLast->IsMultiPortion() &&
                 static_cast<const SwMultiPortion*>(pLast)->IsBidi() && nIdx == nPos )
        {
            nPos = nPos - pLast->GetLen();

            // enter bidi portion
            if ( nCursorLevel % 2 == nDefaultDir % 2 )
            {
                bRecurse = true;
                nIdx = nIdx - pLast->GetLen();
                pPor = pLast;

                // special case:
                // buffer: abcXYZ123 in LTR paragraph
                // view:   abc123ZYX
                // cursor is behind 3 in the buffer and cursor level = 2
                if ( nDefaultDir + 2 == nCursorLevel )
                    nPos = nPos + pLast->GetLen();
            }
        }

        // go forward
        if ( bRecurse )
        {
            const SwLineLayout& rLine = static_cast<const SwMultiPortion*>(pPor)->GetRoot();
            TextFrameIndex nTmpPos = nPos - nIdx;
            bool bTmpForward = ! bRight;
            sal_uInt8 nTmpCursorLevel = nCursorLevel;
            lcl_VisualMoveRecursion(rLine, TextFrameIndex(0), nTmpPos, bTmpForward,
                                     nTmpCursorLevel, nDefaultDir + 1 );

            // special case:
            // buffer: abcXYZ123 in LTR paragraph
            // view:   abc123ZYX
            // cursor is between Z and 1 in the buffer and cursor level = 2
            if ( nTmpPos == pPor->GetLen() && nTmpCursorLevel == nDefaultDir + 1 )
            {
                nTmpPos = nTmpPos - pPor->GetLen();
                nTmpCursorLevel = nDefaultDir;
                bTmpForward = ! bTmpForward;
            }

            nPos = nTmpPos + nIdx;
            bRight = bTmpForward;
            nCursorLevel = nTmpCursorLevel;
        }

        // go backward
        else
        {
            bRight = false;
            nCursorLevel = nDefaultDir;
        }
    }
}

void SwTextFrame::PrepareVisualMove(TextFrameIndex & nPos, sal_uInt8& nCursorLevel,
                                  bool& bForward, bool bInsertCursor )
{
    if( IsEmpty() || IsHiddenNow() )
        return;

    GetFormatted();

    SwTextSizeInfo aInf(this);
    SwTextCursor  aLine(this, &aInf);

    if( nPos )
        aLine.CharCursorToLine( nPos );
    else
        aLine.Top();

    const SwLineLayout* pLine = aLine.GetCurr();
    const TextFrameIndex nStt = aLine.GetStart();
    const TextFrameIndex nLen = pLine->GetLen();

    // We have to distinguish between an insert and overwrite cursor:
    // The insert cursor position depends on the cursor level:
    // buffer:  abcXYZdef in LTR paragraph
    // display: abcZYXdef
    // If cursor is between c and X in the buffer and cursor level is 0,
    // the cursor blinks between c and Z and -> sets the cursor between Z and Y.
    // If the cursor level is 1, the cursor blinks between X and d and
    // -> sets the cursor between d and e.
    // The overwrite cursor simply travels to the next visual character.
    if ( bInsertCursor )
    {
        lcl_VisualMoveRecursion( *pLine, nStt, nPos, bForward,
                                 nCursorLevel, IsRightToLeft() ? 1 : 0 );
        return;
    }

    const sal_uInt8 nDefaultDir = static_cast<sal_uInt8>(IsRightToLeft() ? UBIDI_RTL : UBIDI_LTR);
    const bool bVisualRight = ( nDefaultDir == UBIDI_LTR && bForward ) ||
                                  ( nDefaultDir == UBIDI_RTL && ! bForward );

    // Bidi functions from icu 2.0

    const sal_Unicode* pLineString = GetText().getStr();

    UErrorCode nError = U_ZERO_ERROR;
    UBiDi* pBidi = ubidi_openSized( sal_Int32(nLen), 0, &nError );
    ubidi_setPara( pBidi, reinterpret_cast<const UChar *>(pLineString),
                    sal_Int32(nLen), nDefaultDir, nullptr, &nError );

    TextFrameIndex nTmpPos(0);
    bool bOutOfBounds = false;

    if ( nPos < nStt + nLen )
    {
        nTmpPos = TextFrameIndex(ubidi_getVisualIndex( pBidi, sal_Int32(nPos), &nError ));

        // visual indices are always LTR aligned
        if ( bVisualRight )
        {
            if (nTmpPos + TextFrameIndex(1) < nStt + nLen)
                ++nTmpPos;
            else
            {
                nPos = nDefaultDir == UBIDI_RTL ? TextFrameIndex(0) : nStt + nLen;
                bOutOfBounds = true;
            }
        }
        else
        {
            if ( nTmpPos )
                --nTmpPos;
            else
            {
                nPos = nDefaultDir == UBIDI_RTL ? nStt + nLen : TextFrameIndex(0);
                bOutOfBounds = true;
            }
        }
    }
    else
    {
        nTmpPos = nDefaultDir == UBIDI_LTR ? nPos - TextFrameIndex(1) : TextFrameIndex(0);
    }

    if ( ! bOutOfBounds )
    {
        nPos = TextFrameIndex(ubidi_getLogicalIndex( pBidi, sal_Int32(nTmpPos), &nError ));

        if ( bForward )
        {
            if ( nPos )
                --nPos;
            else
            {
                ++nPos;
                bForward = ! bForward;
            }
        }
        else
            ++nPos;
    }

    ubidi_close( pBidi );
}

bool SwTextFrame::UnitDown_(SwPaM *pPam, const SwTwips nOffset,
                         bool bSetInReadOnly ) const
{

    if ( IsInTab() &&
        pPam->GetPointNode().StartOfSectionNode() !=
        pPam->GetMarkNode().StartOfSectionNode() )
    {
        // If the PaM is located within different boxes, we have a table selection,
        // which is handled by the base class.
        return SwContentFrame::UnitDown( pPam, nOffset, bSetInReadOnly );
    }
    const_cast<SwTextFrame*>(this)->GetFormatted();
    const TextFrameIndex nPos = MapModelToViewPos(*pPam->GetPoint());
    SwRect aCharBox;
    const SwContentFrame *pTmpFollow = nullptr;

    if ( IsVertical() )
        const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();

    if ( !IsEmpty() && !IsHiddenNow() )
    {
        TextFrameIndex nFormat(COMPLETE_STRING);
        do
        {
            if (nFormat != TextFrameIndex(COMPLETE_STRING) && !IsFollow() &&
                !sw_ChangeOffset( const_cast<SwTextFrame*>(this), nFormat ) )
                break;

            SwTextSizeInfo aInf( const_cast<SwTextFrame*>(this) );
            SwTextCursor  aLine( const_cast<SwTextFrame*>(this), &aInf );
            nFormat = aLine.GetEnd();

            aLine.CharCursorToLine( nPos );

            const SwLineLayout* pNextLine = aLine.GetNextLine();
            const TextFrameIndex nStart = aLine.GetStart();
            aLine.GetCharRect( &aCharBox, nPos );

            bool bFirstOfDouble = ( aInf.IsMulti() && aInf.IsFirstMulti() );

            if( pNextLine || bFirstOfDouble )
            {
                aCharBox.Width( aCharBox.SSize().Width() / 2 );
#if OSL_DEBUG_LEVEL > 0
                // See comment in SwTextFrame::GetModelPositionForViewPoint()
                const SwNodeOffset nOldNode = pPam->GetPoint()->GetNodeIndex();
#endif
                if ( pNextLine && ! bFirstOfDouble )
                    aLine.NextLine();

                TextFrameIndex nTmpOfst = aLine.GetModelPositionForViewPoint( pPam->GetPoint(),
                                 aCharBox.Pos(), false );
#if OSL_DEBUG_LEVEL > 0
                OSL_ENSURE( nOldNode == pPam->GetPoint()->GetNodeIndex(),
                    "SwTextFrame::UnitDown: illegal node change" );
#endif

                // We make sure that we move down.
                if( nTmpOfst <= nStart && ! bFirstOfDouble )
                    nTmpOfst = nStart + TextFrameIndex(1);
                *pPam->GetPoint() = MapViewToModelPos(nTmpOfst);

                if ( IsVertical() )
                    const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();

                return true;
            }
            pTmpFollow = GetFollow();
            if( nullptr != pTmpFollow )
            {   // Skip protected follows
                const SwContentFrame* pTmp = pTmpFollow;
                SwViewShell *pSh = getRootFrame()->GetCurrShell();
                if( !pSh || !pSh->GetViewOptions()->IsCursorInProtectedArea() )
                {
                    while( pTmpFollow && pTmpFollow->IsProtected() )
                    {
                        pTmp = pTmpFollow;
                        pTmpFollow = pTmpFollow->GetFollow();
                    }
                }
                if( !pTmpFollow ) // Only protected ones left
                {
                    if ( IsVertical() )
                        const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();
                    return pTmp->SwContentFrame::UnitDown( pPam, nOffset, bSetInReadOnly );
                }

                aLine.GetCharRect( &aCharBox, nPos );
                aCharBox.Width( aCharBox.SSize().Width() / 2 );
            }
            else if( !IsFollow() )
            {
                TextFrameIndex nTmpLen(aInf.GetText().getLength());
                if( aLine.GetEnd() < nTmpLen )
                {
                    if( nFormat <= GetOffset() )
                    {
                        nFormat = std::min(GetOffset() + TextFrameIndex(MIN_OFFSET_STEP),
                                       nTmpLen );
                        if( nFormat <= GetOffset() )
                            break;
                    }
                    continue;
                }
            }
            break;
        } while( true );
    }
    else
        pTmpFollow = GetFollow();

    if ( IsVertical() )
        const_cast<SwTextFrame*>(this)->SwapWidthAndHeight();

    // We take a shortcut for follows
    if( pTmpFollow )
    {
        aCharBox.Pos().setY( pTmpFollow->getFrameArea().Top() + 1 );
        return static_cast<const SwTextFrame*>(pTmpFollow)->GetKeyCursorOfst( pPam->GetPoint(),
                                                     aCharBox.Pos() );
    }
    return SwContentFrame::UnitDown( pPam, nOffset, bSetInReadOnly );
}

bool SwTextFrame::UnitUp(SwPaM *pPam, const SwTwips nOffset,
                      bool bSetInReadOnly ) const
{
    /* We call ContentNode::GertFrame() in CursorSh::Up().
     * This _always returns the master.
     * In order to not mess with cursor travelling, we correct here
     * in SwTextFrame.
     * We calculate UnitUp for pFrame. pFrame is either a master (= this) or a
     * follow (!= this).
     */
    const SwTextFrame *pFrame = GetAdjFrameAtPos( const_cast<SwTextFrame*>(this), *(pPam->GetPoint()),
                                           SwTextCursor::IsRightMargin() );
    const bool bRet = pFrame->UnitUp_( pPam, nOffset, bSetInReadOnly );

    // No SwTextCursor::SetRightMargin( false );
    // Instead we have a SwSetToRightMargin in UnitUp_
    return bRet;
}

bool SwTextFrame::UnitDown(SwPaM *pPam, const SwTwips nOffset,
                        bool bSetInReadOnly ) const
{
    const SwTextFrame *pFrame = GetAdjFrameAtPos(const_cast<SwTextFrame*>(this), *(pPam->GetPoint()),
                                           SwTextCursor::IsRightMargin() );
    const bool bRet = pFrame->UnitDown_( pPam, nOffset, bSetInReadOnly );
    SwTextCursor::SetRightMargin( false );
    return bRet;
}

void SwTextFrame::FillCursorPos( SwFillData& rFill ) const
{
    if( !rFill.bColumn && GetUpper()->IsColBodyFrame() ) // ColumnFrames now with BodyFrame
    {
        const SwColumnFrame* pTmp =
            static_cast<const SwColumnFrame*>(GetUpper()->GetUpper()->GetUpper()->Lower()); // The 1st column
        // The first SwFrame in BodyFrame of the first column
        const SwFrame* pFrame = static_cast<const SwLayoutFrame*>(pTmp->Lower())->Lower();
        sal_uInt16 nNextCol = 0;
        // In which column do we end up in?
        while( rFill.X() > pTmp->getFrameArea().Right() && pTmp->GetNext() )
        {
            pTmp = static_cast<const SwColumnFrame*>(pTmp->GetNext());
            if( static_cast<const SwLayoutFrame*>(pTmp->Lower())->Lower() ) // ColumnFrames now with BodyFrame
            {
                pFrame = static_cast<const SwLayoutFrame*>(pTmp->Lower())->Lower();
                nNextCol = 0;
            }
            else
                ++nNextCol; // Empty columns require column breaks
        }
        if( pTmp != GetUpper()->GetUpper() ) // Did we end up in another column?
        {
            if( !pFrame )
                return;
            if( nNextCol )
            {
                while( pFrame->GetNext() )
                    pFrame = pFrame->GetNext();
            }
            else
            {
                while( pFrame->GetNext() && pFrame->getFrameArea().Bottom() < rFill.Y() )
                    pFrame = pFrame->GetNext();
            }
            // No filling, if the last frame in the targeted column does
            // not contain a paragraph, but e.g. a table
            if( pFrame->IsTextFrame() )
            {
                rFill.Fill().nColumnCnt = nNextCol;
                rFill.bColumn = true;
                if( rFill.pPos )
                {
                    SwTextFrame const*const pTextFrame(static_cast<const SwTextFrame*>(pFrame));
                    *rFill.pPos = pTextFrame->MapViewToModelPos(
                            TextFrameIndex(pTextFrame->GetText().getLength()));
                }
                if( nNextCol )
                {
                    rFill.aFrame = pTmp->getFramePrintArea();
                    rFill.aFrame += pTmp->getFrameArea().Pos();
                }
                else
                    rFill.aFrame = pFrame->getFrameArea();
                static_cast<const SwTextFrame*>(pFrame)->FillCursorPos( rFill );
            }
            return;
        }
    }
    std::unique_ptr<SwFont> pFnt;
    SwTextFormatColl* pColl = GetTextNodeForParaProps()->GetTextColl();
    SwTwips nFirst = GetTextNodeForParaProps()->GetSwAttrSet().GetULSpace().GetLower();
    SwTwips nDiff = rFill.Y() - getFrameArea().Bottom();
    if( nDiff < nFirst )
        nDiff = -1;
    else
        pColl = &pColl->GetNextTextFormatColl();
    SwAttrSet aSet(const_cast<SwDoc&>(GetDoc()).GetAttrPool(), aTextFormatCollSetRange );
    const SwAttrSet* pSet = &pColl->GetAttrSet();
    SwViewShell *pSh = getRootFrame()->GetCurrShell();
    if (GetTextNodeForParaProps()->HasSwAttrSet())
    {
        // sw_redlinehide: pSet is mostly used for para props, but there are
        // accesses to char props via pFnt - why does it use only the node's
        // props for this, and not hints?
        aSet.Put( *GetTextNodeForParaProps()->GetpSwAttrSet() );
        aSet.SetParent( pSet );
        pSet = &aSet;
        pFnt.reset(new SwFont( pSet, &GetDoc().getIDocumentSettingAccess() ));
    }
    else
    {
        SwFontAccess aFontAccess( pColl, pSh );
        pFnt.reset(new SwFont( aFontAccess.Get()->GetFont() ));
        pFnt->CheckFontCacheId( pSh, pFnt->GetActual() );
    }
    OutputDevice* pOut = pSh->GetOut();
    if( !pSh->GetViewOptions()->getBrowseMode() || pSh->GetViewOptions()->IsPrtFormat() )
        pOut = GetDoc().getIDocumentDeviceAccess().getReferenceDevice( true );

    pFnt->SetFntChg( true );
    pFnt->ChgPhysFnt( pSh, *pOut );

    SwTwips nLineHeight = pFnt->GetHeight( pSh, *pOut );

    bool bFill = false;
    if( nLineHeight )
    {
        bFill = true;
        const SvxULSpaceItem &rUL = pSet->GetULSpace();
        SwTwips nDist = std::max( rUL.GetLower(), rUL.GetUpper() );
        if( rFill.Fill().nColumnCnt )
        {
            rFill.aFrame.Height( nLineHeight );
            nDiff = rFill.Y() - rFill.Bottom();
            nFirst = 0;
        }
        else if( nDist < nFirst )
            nFirst = nFirst - nDist;
        else
            nFirst = 0;
        nDist = std::max( nDist, SwTwips(GetLineSpace()) );
        nDist += nLineHeight;
        nDiff -= nFirst;

        if( nDiff > 0 )
        {
            nDiff /= nDist;
            rFill.Fill().nParaCnt = o3tl::narrowing<sal_uInt16>(nDiff + 1);
            rFill.nLineWidth = 0;
            rFill.bInner = false;
            rFill.bEmpty = true;
            rFill.SetOrient( text::HoriOrientation::LEFT );
        }
        else
            nDiff = -1;
        if( rFill.bInner )
            bFill = false;
        else
        {
            const SvxTabStopItem &rRuler = pSet->GetTabStops();
            const SvxFirstLineIndentItem& rFirstLine(pSet->GetFirstLineIndent());
            const SvxTextLeftMarginItem& rTextLeftMargin(pSet->GetTextLeftMargin());
            const SvxRightMarginItem& rRightMargin(pSet->GetRightMargin());

            SwRect &rRect = rFill.Fill().aCursor;
            rRect.Top( rFill.Bottom() + (nDiff+1) * nDist - nLineHeight );
            if( nFirst && nDiff > -1 )
                rRect.Top( rRect.Top() + nFirst );
            rRect.Height( nLineHeight );

            SwTwips nLeft = rFill.Left() + rTextLeftMargin.ResolveLeft(rFirstLine, /*metrics*/ {})
                            + GetTextNodeForParaProps()->GetLeftMarginWithNum();
            SwTwips nRight = rFill.Right() - rRightMargin.ResolveRight({});
            SwTwips nCenter = ( nLeft + nRight ) / 2;
            rRect.Left( nLeft );
            if( SwFillMode::Margin == rFill.Mode() )
            {
                if( rFill.bEmpty )
                {
                    rFill.SetOrient( text::HoriOrientation::LEFT );
                    if( rFill.X() < nCenter )
                    {
                        if( rFill.X() > ( nLeft + 2 * nCenter ) / 3 )
                        {
                            rFill.SetOrient( text::HoriOrientation::CENTER );
                            rRect.Left( nCenter );
                        }
                    }
                    else if( rFill.X() > ( nRight + 2 * nCenter ) / 3 )
                    {
                        rFill.SetOrient( text::HoriOrientation::RIGHT );
                        rRect.Left( nRight );
                    }
                    else
                    {
                        rFill.SetOrient( text::HoriOrientation::CENTER );
                        rRect.Left( nCenter );
                    }
                }
                else
                    bFill = false;
            }
            else
            {
                SwTwips nSpace = 0;
                if( SwFillMode::Tab != rFill.Mode() )
                {
                    SwDrawTextInfo aDrawInf( pSh, *pOut, u"  "_ustr, 0, 2 );
                    nSpace = pFnt->GetTextSize_( aDrawInf ).Width()/2;
                }
                if( rFill.X() >= nRight )
                {
                    if( SwFillMode::Indent != rFill.Mode() && ( rFill.bEmpty ||
                        rFill.X() > rFill.nLineWidth + FILL_MIN_DIST ) )
                    {
                        rFill.SetOrient( text::HoriOrientation::RIGHT );
                        rRect.Left( nRight );
                    }
                    else
                        bFill = false;
                }
                else if( SwFillMode::Indent == rFill.Mode() )
                {
                    SwTwips nIndent = rFill.X();
                    if( !rFill.bEmpty || nIndent > nRight )
                        bFill = false;
                    else
                    {
                        nIndent -= rFill.Left();
                        if( nIndent >= 0 && nSpace )
                        {
                            nIndent /= nSpace;
                            nIndent *= nSpace;
                            rFill.SetTab( sal_uInt16( nIndent ) );
                            rRect.Left( nIndent + rFill.Left() );
                        }
                        else
                            bFill = false;
                    }
                }
                else if( rFill.X() > nLeft )
                {
                    SwTwips nTextLeft = rFill.Left() + rTextLeftMargin.ResolveTextLeft({})
                                        + GetTextNodeForParaProps()->GetLeftMarginWithNum(true);
                    rFill.nLineWidth += rFill.bFirstLine ? nLeft : nTextLeft;
                    SwTwips nLeftTab;
                    SwTwips nRightTab = nLeft;
                    sal_uInt16 nSpaceCnt = 0;
                    sal_uInt16 nSpaceOnlyCnt = 0;
                    sal_uInt16 nIdx = 0;
                    int nTabCnt = 0;
                    do
                    {
                        nLeftTab = nRightTab;
                        if( nIdx < rRuler.Count() )
                        {
                            const SvxTabStop &rTabStop = rRuler.operator[](nIdx);
                            nRightTab = nTextLeft + rTabStop.GetTabPos();
                            if( nLeftTab < nTextLeft && nRightTab > nTextLeft )
                                nRightTab = nTextLeft;
                            else
                                ++nIdx;
                            if( nRightTab > rFill.nLineWidth )
                                ++nTabCnt;
                        }
                        else
                        {
                            const SvxTabStopItem& rTab =
                                pSet->GetPool()->GetUserOrPoolDefaultItem( RES_PARATR_TABSTOP );
                            const SwTwips nDefTabDist = rTab[0].GetTabPos();
                            nRightTab = nLeftTab - nTextLeft;
                            nRightTab /= nDefTabDist;
                            nRightTab = nRightTab * nDefTabDist + nTextLeft;
                            while ( nRightTab <= nLeftTab )
                                nRightTab += nDefTabDist;
                            if( nRightTab > rFill.nLineWidth )
                                ++nTabCnt;
                            while ( nRightTab < rFill.X() )
                            {
                                nRightTab += nDefTabDist;
                                if( nRightTab > rFill.nLineWidth )
                                    ++nTabCnt;
                            }
                            if( nLeftTab < nRightTab - nDefTabDist )
                                nLeftTab = nRightTab - nDefTabDist;
                        }
                        if( nRightTab > nRight )
                            nRightTab = nRight;
                    }
                    while( rFill.X() > nRightTab );
                    --nTabCnt;
                    if( SwFillMode::TabSpace == rFill.Mode() )
                    {
                        if( nSpace > 0 )
                        {
                            if( !nTabCnt )
                                nLeftTab = rFill.nLineWidth;
                            while( nLeftTab < rFill.X() )
                            {
                                nLeftTab += nSpace;
                                ++nSpaceCnt;
                            }
                            if( nSpaceCnt )
                            {
                                nLeftTab -= nSpace;
                                --nSpaceCnt;
                            }
                            if( rFill.X() - nLeftTab > nRightTab - rFill.X() )
                            {
                                nSpaceCnt = 0;
                                ++nTabCnt;
                                rRect.Left( nRightTab );
                            }
                            else
                            {
                                if( rFill.X() - nLeftTab > nSpace/2 )
                                {
                                    ++nSpaceCnt;
                                    rRect.Left( nLeftTab + nSpace );
                                }
                                else
                                    rRect.Left( nLeftTab );
                            }
                        }
                        else if( rFill.X() - nLeftTab < nRightTab - rFill.X() )
                            rRect.Left( nLeftTab );
                        else
                        {
                            if( nRightTab >= nRight )
                            {
                                rFill.SetOrient( text::HoriOrientation::RIGHT );
                                rRect.Left( nRight );
                                nTabCnt = 0;
                                nSpaceCnt = 0;
                            }
                            else
                            {
                                rRect.Left( nRightTab );
                                ++nTabCnt;
                            }
                        }
                    }
                    else if( SwFillMode::Space == rFill.Mode() )
                    {
                        SwTwips nLeftSpace = nLeft;
                        while( nLeftSpace < rFill.X() )
                        {
                            nLeftSpace += nSpace;
                            ++nSpaceOnlyCnt;
                        }
                        rRect.Left( nLeftSpace );
                    }
                    else
                    {
                        if( rFill.X() - nLeftTab < nRightTab - rFill.X() )
                            rRect.Left( nLeftTab );
                        else
                        {
                            if( nRightTab >= nRight )
                            {
                                rFill.SetOrient( text::HoriOrientation::RIGHT );
                                rRect.Left( nRight );
                                nTabCnt = 0;
                                nSpaceCnt = 0;
                            }
                            else
                            {
                                rRect.Left( nRightTab );
                                ++nTabCnt;
                            }
                        }
                    }
                    rFill.SetTab( nTabCnt );
                    rFill.SetSpace( nSpaceCnt );
                    rFill.SetSpaceOnly( nSpaceOnlyCnt );
                    if( bFill )
                    {
                        if( std::abs( rFill.X() - nCenter ) <=
                            std::abs( rFill.X() - rRect.Left() ) )
                        {
                            rFill.SetOrient( text::HoriOrientation::CENTER );
                            rFill.SetTab( 0 );
                            rFill.SetSpace( 0 );
                            rFill.SetSpaceOnly( 0 );
                            rRect.Left( nCenter );
                        }
                        if( !rFill.bEmpty )
                            rFill.nLineWidth += FILL_MIN_DIST;
                        if( rRect.Left() < rFill.nLineWidth )
                            bFill = false;
                    }
                }
            }
            // Do we extend over the page's/column's/etc. lower edge?
            const SwFrame* pUp = GetUpper();
            if( pUp->IsInSct() )
            {
                if( pUp->IsSctFrame() )
                    pUp = pUp->GetUpper();
                else if( pUp->IsColBodyFrame() &&
                         pUp->GetUpper()->GetUpper()->IsSctFrame() )
                    pUp = pUp->GetUpper()->GetUpper()->GetUpper();
            }
            SwRectFnSet aRectFnSet(this);
            SwTwips nLimit = aRectFnSet.GetPrtBottom(*pUp);
            SwTwips nRectBottom = rRect.Bottom();
            if ( aRectFnSet.IsVert() )
                nRectBottom = SwitchHorizontalToVertical( nRectBottom );

            if( aRectFnSet.YDiff( nLimit, nRectBottom ) < 0 )
                bFill = false;
            else
                rRect.Width( 1 );
        }
    }
    const_cast<SwCursorMoveState*>(rFill.pCMS)->m_bFillRet = bFill;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

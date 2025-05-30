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

#include <hintids.hxx>
#include <viewopt.hxx>
#include <tools/multisel.hxx>
#include <editeng/udlnitem.hxx>
#include <pagefrm.hxx>
#include <tgrditem.hxx>

#include <EnhancedPDFExportHelper.hxx>
#include <IDocumentSettingAccess.hxx>

#include <viewsh.hxx>
#include "itrpaint.hxx"
#include <txtfrm.hxx>
#include <swfont.hxx>
#include "txtpaint.hxx"
#include "porfld.hxx"
#include "porfly.hxx"
#include "portab.hxx"
#include <txatbase.hxx>
#include <charfmt.hxx>
#include "redlnitr.hxx"
#include "porrst.hxx"
#include "pormulti.hxx"
#include <doc.hxx>
#include <fmturl.hxx>

// Returns, if we have an underline breaking situation
// Adding some more conditions here means you also have to change them
// in SwTextPainter::CheckSpecialUnderline
bool IsUnderlineBreak( const SwLinePortion& rPor, const SwFont& rFnt )
{
    return LINESTYLE_NONE == rFnt.GetUnderline() ||
           rPor.IsFlyPortion() || rPor.IsFlyCntPortion() ||
           rPor.IsBreakPortion() || rPor.IsMarginPortion() ||
           rPor.IsHolePortion() ||
          ( rPor.IsMultiPortion() && ! static_cast<const SwMultiPortion&>(rPor).IsBidi() ) ||
           rFnt.GetEscapement() < 0 || rFnt.IsWordLineMode() ||
           SvxCaseMap::SmallCaps == rFnt.GetCaseMap();
}

static Color GetUnderColor( const SwFont *pFont )
{
    return pFont->GetUnderColor() == COL_AUTO ?
        pFont->GetColor() : pFont->GetUnderColor();
}

void SwTextPainter::CtorInitTextPainter( SwTextFrame *pNewFrame, SwTextPaintInfo *pNewInf )
{
    CtorInitTextCursor( pNewFrame, pNewInf );
    m_pInf = pNewInf;
    SwFont *pMyFnt = GetFnt();
    GetInfo().SetFont( pMyFnt );
    m_bPaintDrop = false;
}

SwLinePortion *SwTextPainter::CalcPaintOfst(const SwRect &rPaint, bool& rbSkippedNumPortions)
{
    SwLinePortion *pPor = m_pCurr->GetFirstPortion();
    GetInfo().SetPaintOfst( 0 );
    SwTwips nPaintOfst = rPaint.Left();

    // nPaintOfst was exactly set to the end, therefore <=
    // nPaintOfst is document global, therefore add up nLeftMar
    // const sal_uInt16 nLeftMar = sal_uInt16(GetLeftMargin());
    // 8310: paint of LineBreaks in empty lines.
    if( nPaintOfst && m_pCurr->Width() )
    {
        SwLinePortion *pLast = nullptr;
        // 7529 and 4757: not <= nPaintOfst
        while( pPor && GetInfo().X() + pPor->Width() + (pPor->Height()/2)
                       < nPaintOfst )
        {
            if( pPor->InSpaceGrp() && GetInfo().GetSpaceAdd() )
            {
                tools::Long nTmp = GetInfo().X() +pPor->Width() +
                    pPor->CalcSpacing( GetInfo().GetSpaceAdd(), GetInfo() );
                if( nTmp + (pPor->Height()/2) >= nPaintOfst )
                    break;
                GetInfo().X( nTmp );
                GetInfo().SetIdx( GetInfo().GetIdx() + pPor->GetLen() );
            }
            else
                pPor->Move( GetInfo() );
            if (pPor->InNumberGrp()
                && !static_cast<SwNumberPortion const*>(pPor)->HasFollow())
            {
                rbSkippedNumPortions = true; // all numbering portions were skipped?
            }
            pLast = pPor;
            pPor = pPor->GetNextPortion();
        }

        // 7529: if PostIts return also pLast.
        if( pLast && !pLast->Width() && pLast->IsPostItsPortion() )
        {
            pPor = pLast;
            GetInfo().SetIdx( GetInfo().GetIdx() - pPor->GetLen() );
        }
    }
    return pPor;
}

// There are two possibilities to output transparent font:
// 1) DrawRect on the whole line and DrawText afterwards
//    (objectively fast, subjectively slow)
// 2) For every portion a DrawRect with subsequent DrawText is done
//    (objectively slow, subjectively fast)
// Since the user usually judges subjectively the second method is set as default.
void SwTextPainter::DrawTextLine( const SwRect &rPaint, SwSaveClip &rClip,
    const bool bUnderSized,
    ::std::optional<SwTaggedPDFHelper> & roTaggedLabel,
    ::std::optional<SwTaggedPDFHelper> & roTaggedParagraph,
    bool const isPDFTaggingEnabled)
{
    // maybe catch-up adjustment
    GetAdjusted();
    AddExtraBlankWidth();
    GetInfo().SetpSpaceAdd( m_pCurr->GetpLLSpaceAdd() );
    GetInfo().ResetSpaceIdx();
    GetInfo().SetKanaComp( m_pCurr->GetpKanaComp() );
    GetInfo().ResetKanaIdx();
    // The size of the frame
    GetInfo().SetIdx( GetStart() );
    GetInfo().SetPos( GetTopLeft() );

    const bool bDrawInWindow = GetInfo().OnWin();

    // 6882: blank lines can't be optimized by removing them if Formatting Marks are shown
    const bool bEndPor = GetInfo().GetOpt().IsParagraph() && GetInfo().GetText().isEmpty();

    bool bSkippedNumPortions(false);
    SwLinePortion *pPor = bEndPor ? m_pCurr->GetFirstPortion() : CalcPaintOfst(rPaint, bSkippedNumPortions);

    if (bSkippedNumPortions // ugly but hard to check earlier in PaintSwFrame:
        && !GetInfo().GetTextFrame()->GetTextNodeForParaProps()->IsOutline())
    {   // there is a num portion but it is outside of the frame area and not painted
        assert(!roTaggedLabel);
        assert(!roTaggedParagraph);
        Frame_Info aFrameInfo(*m_pFrame, false); // open LBody
        roTaggedParagraph.emplace(nullptr, &aFrameInfo, nullptr, *GetInfo().GetOut());
    }

    SwTaggedPDFHelper::EndCurrentLink(*GetInfo().GetOut());

    // Optimization!
    SwTwips nMaxRight = std::min<SwTwips>( rPaint.Right(), Right() );
    const SwTwips nTmpLeft = GetInfo().X();
    //compatibility settings: allow tabstop text to exceed right margin
    const auto& iDSA = GetInfo().GetTextFrame()->GetDoc().getIDocumentSettingAccess();
    const bool bTabOverMargin = iDSA.get(DocumentSettingId::TAB_OVER_MARGIN);
    const bool bTabOverSpacing = iDSA.get(DocumentSettingId::TAB_OVER_SPACING);
    if (bTabOverMargin || bTabOverSpacing)
    {
        SwLinePortion* pPorIter = pPor;
        while( pPorIter )
        {
            if( pPorIter->InTabGrp() )
            {
                const SwTabPortion* pTabPor = static_cast<SwTabPortion*>(pPorIter);
                const SwTwips nTabPos = nTmpLeft + pTabPor->GetTabPos();
                if( nMaxRight < nTabPos )
                {
                    nMaxRight = rPaint.Right();
                    break;
                }
            }
            pPorIter = pPorIter->GetNextPortion();
        }
    }
    if( !bEndPor && nTmpLeft >= nMaxRight )
        return;

    // DropCaps!
    // 7538: of course for the printer, too
    if( !m_bPaintDrop )
    {
        // 8084: Optimization, less painting
        // AMA: By 8084 7538 has been revived
        // bDrawInWindow removed, so that DropCaps also can be printed
        m_bPaintDrop = pPor == m_pCurr->GetFirstPortion()
                     && GetDropLines() >= GetLineNr();
    }

    SwTwips nTmpHeight, nTmpAscent;
    CalcAscentAndHeight( nTmpAscent, nTmpHeight );

    // bClip decides if there's a need to clip
    // The whole thing must be done before retouching

    bool bClip = ( bDrawInWindow || bUnderSized ) && !rClip.IsChg();
    if( bClip && pPor )
    {
        // If TopLeft or BottomLeft of the line are outside, the we must clip.
        // The check for Right() is done in the output loop ...

        if( GetInfo().GetPos().X() < rPaint.Left() ||
            GetInfo().GetPos().Y() < rPaint.Top() ||
            GetInfo().GetPos().Y() + nTmpHeight > rPaint.Top() + rPaint.Height() )
        {
            bClip = false;
            rClip.ChgClip(rPaint, m_pFrame, m_pCurr->GetExtraAscent(), m_pCurr->GetExtraDescent());
        }
#if OSL_DEBUG_LEVEL > 1
        static bool bClipAlways = false;
        if( bClip && bClipAlways )
        {   bClip = false;
            rClip.ChgClip( rPaint );
        }
#endif
    }

    // Alignment
    OutputDevice* pOut = GetInfo().GetOut();
    Point aPnt1( nTmpLeft, GetInfo().GetPos().Y() );
    if ( aPnt1.X() < rPaint.Left() )
        aPnt1.setX( rPaint.Left() );
    if ( aPnt1.Y() < rPaint.Top() )
        aPnt1.setY( rPaint.Top() );
    Point aPnt2( GetInfo().GetPos().X() + nMaxRight - GetInfo().X(),
                 GetInfo().GetPos().Y() + nTmpHeight );
    if ( aPnt2.X() > rPaint.Right() )
        aPnt2.setX( rPaint.Right() );
    if ( aPnt2.Y() > rPaint.Bottom() )
        aPnt2.setY( rPaint.Bottom() );

    const SwRect aLineRect( aPnt1, aPnt2 );

    if( m_pCurr->IsClipping() )
    {
        const SwTextFrame& rFrame = *GetInfo().GetTextFrame();
        // tdf#117448 at small fixed line height, enlarge clipping area in table cells
        // to show previously clipped text content on the area of paragraph margins
        if ( rFrame.IsInTab() )
            rClip.ChgClip(aLineRect, m_pFrame, rFrame.GetTopMargin(), rFrame.GetBottomMargin());
        else
            rClip.ChgClip( aLineRect, m_pFrame );
        bClip = false;
    }

    if( !pPor && !bEndPor )
        return;

    // Baseline output also if non-TextPortion (compare TabPor with Fill)
    // if no special vertical alignment is used,
    // we calculate Y value for the whole line
    SwTextGridItem const*const pGrid(GetGridItem(GetTextFrame()->FindPageFrame()));
    const bool bAdjustBaseLine =
        GetLineInfo().HasSpecialAlign( GetTextFrame()->IsVertical() ) ||
        ( nullptr != pGrid ) || m_pCurr->GetHangingBaseline();
    const SwTwips nLineBaseLine = GetInfo().GetPos().Y() + nTmpAscent;
    if ( ! bAdjustBaseLine )
        GetInfo().Y( nLineBaseLine );

    // 7529: Pre-paint post-its
    if( GetInfo().OnWin() && pPor && !pPor->Width() )
    {
        SeekAndChg( GetInfo() );

        if( bAdjustBaseLine )
        {
            const SwTwips nOldY = GetInfo().Y();

            GetInfo().Y( GetInfo().GetPos().Y() + AdjustBaseLine( *m_pCurr, nullptr,
                GetInfo().GetFont()->GetHeight( GetInfo().GetVsh(), *pOut ),
                GetInfo().GetFont()->GetAscent( GetInfo().GetVsh(), *pOut )
            ) );

            pPor->PrePaint( GetInfo(), pPor );
            GetInfo().Y( nOldY );
        }
        else
            pPor->PrePaint( GetInfo(), pPor );
    }

    // 7923: EndPortions output chars, too, that's why we change the font
    if( bEndPor )
        SeekStartAndChg( GetInfo() );

    const bool bRest = m_pCurr->IsRest();
    bool bFirst = true;

    SwArrowPortion *pArrow = nullptr;
    // Reference portion for the paragraph end portion
    SwLinePortion* pEndTempl = m_pCurr->GetFirstPortion();

    while( pPor )
    {
        bool bSeeked = true;
        GetInfo().SetLen( pPor->GetLen() );

        const SwTwips nOldY = GetInfo().Y();

        if ( bAdjustBaseLine )
        {
            GetInfo().Y( GetInfo().GetPos().Y() + AdjustBaseLine( *m_pCurr, pPor ) );

            // we store the last portion, because a possible paragraph
            // end character has the same font as this portion
            // (only in special vertical alignment case, otherwise the first
            // portion of the line is used)
            if ( pPor->Width() && pPor->InTextGrp() )
                pEndTempl = pPor;
        }

        // set redlining for line break symbol
        if ( pPor->IsBreakPortion() && GetInfo().GetOpt().IsParagraph() && GetRedln() )
        {
            SeekAndChg( GetInfo() );
            if ( m_pCurr->GetRedlineEndType() != RedlineType::None )
                static_cast<SwBreakPortion&>(*pPor).SetRedline( m_pCurr->GetRedlineEndType() );
        }

        // A special case are GluePortions which output blanks.

        // 6168: Avoid that the rest of a FieldPortion gets the attributes of the
        // next portion with SeekAndChgBefore():
        if( bRest && pPor->InFieldGrp() && !pPor->GetLen() )
            SeekAndChgBefore( GetInfo() );
        else if ( pPor->IsQuoVadisPortion() )
        {
            // A remark on QuoVadis/ErgoSum:
            // We use the Font set for the Paragraph for these portions.
            // Thus, we initialize:
            TextFrameIndex nOffset = GetInfo().GetIdx();
            SeekStartAndChg( GetInfo(), true );
            if( GetRedln() && m_pCurr->HasRedline() )
            {
                std::pair<SwTextNode const*, sal_Int32> const pos(
                        GetTextFrame()->MapViewToModel(nOffset));
                GetRedln()->Seek(*m_pFont, pos.first->GetIndex(), pos.second, 0);
            }
        }
        else if( pPor->InTextGrp() || pPor->InFieldGrp() || pPor->InTabGrp() )
            SeekAndChg( GetInfo() );
        else if ( !bFirst && pPor->IsBreakPortion() && GetInfo().GetOpt().IsParagraph() )
        {
            // Paragraph symbols should have the same font as the paragraph in front of them,
            // except for the case that there's redlining in the paragraph
            if( GetRedln() )
                SeekAndChg( GetInfo() );
            else
                SeekAndChgBefore( GetInfo() );
        }
        else
            bSeeked = false;

        // bRest = false;

        // If the end of the portion juts out, it is clipped.
        // A safety distance of half the height is added, so that
        // TTF-"f" isn't overlapping into the page margin.
        if( bClip &&
            GetInfo().X() + pPor->Width() + ( pPor->Height() / 2 ) > nMaxRight )
        {
            bClip = false;
            rClip.ChgClip(rPaint, m_pFrame, m_pCurr->GetExtraAscent(), m_pCurr->GetExtraDescent());
        }

        // Portions, which lay "below" the text like post-its
        SwLinePortion *pNext = pPor->GetNextPortion();
        if( GetInfo().OnWin() && pNext && !pNext->Width() )
        {
            // Fix 11289: Fields were omitted here because of Last!=Owner during
            // loading Brief.sdw. Now the fields are allowed again,
            // by bSeeked Last!=Owner is being avoided.
            if ( !bSeeked )
                SeekAndChg( GetInfo() );
            pNext->PrePaint( GetInfo(), pPor );
        }

        // We calculate a separate font for underlining.
        CheckSpecialUnderline( pPor, bAdjustBaseLine ? nOldY : 0 );
        SwUnderlineFont* pUnderLineFnt = GetInfo().GetUnderFnt();
        if ( pUnderLineFnt )
        {
            const Point aTmpPoint( GetInfo().X(),
                                   bAdjustBaseLine ?
                                   pUnderLineFnt->GetPos().Y() :
                                   nLineBaseLine );
            pUnderLineFnt->SetPos( aTmpPoint );
        }

        // in extended input mode we do not want a common underline font.
        SwUnderlineFont* pOldUnderLineFnt = nullptr;
        if ( GetRedln() && GetRedln()->ExtOn() )
        {
            pOldUnderLineFnt = GetInfo().GetUnderFnt();
            GetInfo().SetUnderFnt( nullptr );
        }

        // multiple numbering portions are possible :(
        if ((pPor->InNumberGrp() // also footnote label
                // weird special case, bullet with soft hyphen
             || (pPor->InHyphGrp() && pNext && pNext->InNumberGrp()))
            && !GetInfo().GetTextFrame()->GetTextNodeForParaProps()->IsOutline()
            && !roTaggedLabel) // note: CalcPaintOfst may skip some portions
        {
            assert(isPDFTaggingEnabled);
            Por_Info aPorInfo(*pPor, *this, 1); // open Lbl
            roTaggedLabel.emplace(nullptr, nullptr, &aPorInfo, *pOut);
        }

        {
            // #i16816# tagged pdf support
            Por_Info aPorInfo(*pPor, *this, 0);
            SwTaggedPDFHelper aTaggedPDFHelper( nullptr, nullptr, &aPorInfo, *pOut );

            if( pPor->IsMultiPortion() )
                PaintMultiPortion( rPaint, static_cast<SwMultiPortion&>(*pPor) );
            else
                pPor->Paint( GetInfo() );
        }

        // lazy open LBody and paragraph tag after num portions have been painted to Lbl
        if (pPor->InNumberGrp() // also footnote label
            // note: numbering portion may be split if it has multiple scripts
            && !static_cast<SwNumberPortion const*>(pPor)->HasFollow()) // so wait for the last one
        {
            if (!GetInfo().GetTextFrame()->GetTextNodeForParaProps()->IsOutline())
            {
                assert(roTaggedLabel);
                roTaggedLabel.reset(); // close Lbl
                assert(!roTaggedParagraph);
                Frame_Info aFrameInfo(*m_pFrame, false); // open LBody
                roTaggedParagraph.emplace(nullptr, &aFrameInfo, nullptr, *pOut);
            }
            else
            {
                assert(!roTaggedLabel);
            }
        }

        // reset underline font
        if ( pOldUnderLineFnt )
            GetInfo().SetUnderFnt( pOldUnderLineFnt );

        // reset (for special vertical alignment)
        GetInfo().Y( nOldY );

        if (GetFnt()->IsURL() && pPor->InTextGrp())
            GetInfo().NotifyURL(*pPor);
        else if (pPor->IsFlyCntPortion())
        {
            if (auto* pFlyContentPortion = dynamic_cast<sw::FlyContentPortion*>(pPor))
            {
                if (auto* pFlyFrame = pFlyContentPortion->GetFlyFrame())
                {
                    if (auto* pFormat = pFlyFrame->GetFormat())
                    {
                        auto& url = pFormat->GetURL();
                        if (!url.GetURL().isEmpty()) // TODO: url.GetMap() ?
                            GetInfo().NotifyURL(*pPor);
                    }
                }
            }
        }

        bFirst &= !pPor->GetLen();
        if( pNext || !pPor->IsMarginPortion() )
            pPor->Move( GetInfo() );
        if( pPor->IsArrowPortion() && GetInfo().OnWin() && !pArrow )
            pArrow = static_cast<SwArrowPortion*>(pPor);

        pPor = bDrawInWindow || GetInfo().X() <= nMaxRight ||
               // #i16816# tagged pdf support
               ( GetInfo().GetVsh() &&
                 GetInfo().GetVsh()->GetViewOptions()->IsPDFExport() &&
                 pNext && pNext->IsHolePortion() ) ?
               pNext :
               nullptr;
        if (!pPor && isPDFTaggingEnabled && (roTaggedLabel || !roTaggedParagraph))
        {   // check if the end of the list label is off-screen
            auto FindEndOfNumbering = [&](SwLinePortion const* pP) {
                while (pP)
                {
                    if (pP->InNumberGrp()
                        && !static_cast<SwNumberPortion const*>(pP)->HasFollow())
                    {
                        if (roTaggedLabel)
                        {
                            roTaggedLabel.reset();
                        } // else, if the numbering isn't visible at all, no Lbl
                        if (!GetInfo().GetTextFrame()->GetTextNodeForParaProps()->IsOutline())
                        {
                            Frame_Info aFrameInfo(*m_pFrame, false); // open LBody
                            roTaggedParagraph.emplace(nullptr, &aFrameInfo, nullptr, *GetInfo().GetOut());
                        }
                        return true;
                    }
                    pP = pP->GetNextPortion();
                }
                return false;
            };
            if (!FindEndOfNumbering(pNext)) // check rest of current line
            {
                // check lines that will be cut off
                if (rPaint.Bottom() < Y() + GetLineHeight())
                {
                    for (SwLineLayout const* pLine = GetNext(); pLine; pLine = pLine->GetNext())
                    {
                        if (FindEndOfNumbering(pLine->GetFirstPortion()))
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    // delete underline font
    delete GetInfo().GetUnderFnt();
    GetInfo().SetUnderFnt( nullptr );

    // paint remaining stuff, e.g. the line ending symbols, pilcrow (¶) and the line break
    if( bDrawInWindow )
    {
        // If special vertical alignment is enabled, GetInfo().Y() is the
        // top of the current line. Therefore is has to be adjusted for
        // the painting of the remaining stuff. We first store the old value.
        const SwTwips nOldY = GetInfo().Y();

        if( !GetNextLine() &&
            GetInfo().GetVsh() && !GetInfo().GetVsh()->IsPreview() &&
            GetInfo().GetOpt().IsParagraph() && !GetTextFrame()->GetFollow() &&
            GetInfo().GetIdx() >= TextFrameIndex(GetInfo().GetText().getLength()))
        {
            bool bHasRedlineEnd( GetRedln() && m_pCurr->HasRedlineEnd() );
            RedlineType eRedlineEnd = bHasRedlineEnd ? m_pCurr->GetRedlineEndType() : RedlineType::None;
            if( bHasRedlineEnd )
            {
                TextFrameIndex nOffset = GetInfo().GetIdx();
                SeekStartAndChg( GetInfo(), true );
                std::pair<SwTextNode const*, sal_Int32> const pos(
                        GetTextFrame()->MapViewToModel(nOffset));
                GetRedln()->Seek(*m_pFont, pos.first->GetIndex(), pos.second, 0);
            }
            const SwTmpEndPortion aEnd( *pEndTempl,
                           bHasRedlineEnd && eRedlineEnd != RedlineType::Delete ? m_pFont->GetUnderline() : LINESTYLE_NONE,
                           bHasRedlineEnd && eRedlineEnd == RedlineType::Delete ? m_pFont->GetStrikeout() : STRIKEOUT_NONE,
                           bHasRedlineEnd ? m_pFont->GetColor() : COL_AUTO );
            GetFnt()->ChgPhysFnt( GetInfo().GetVsh(), *pOut );

            if ( bAdjustBaseLine )
                GetInfo().Y( GetInfo().GetPos().Y()
                           + AdjustBaseLine( *m_pCurr, &aEnd ) );
            GetInfo().X( GetInfo().X() +
                // tdf#163042 In the case of shrunk lines with a single portion, adjust
                // the line width (if needed, i.e. if the shrunk line doesn't end in a space)
                // to show the terminating pilcrow at the correct position, and not before that
                ( ( !( pEndTempl->GetNextPortion() && pEndTempl->GetNextPortion()->IsHolePortion() ) &&
                    std::abs( m_pCurr->Width() - m_pCurr->GetFirstPortion()->Width() ) <= 1 && m_pCurr->ExtraShrunkWidth() > 0 )
                        ? m_pCurr->ExtraShrunkWidth() - m_pCurr->Width() : 0 ) +
                    ( GetCurr()->IsHanging() ? GetCurr()->GetHangingMargin() : 0 ) );
            aEnd.Paint( GetInfo() );
            GetInfo().Y( nOldY );
        }
        if( GetInfo().GetVsh() && !GetInfo().GetVsh()->IsPreview() )
        {
            const bool bNextUndersized =
                ( GetTextFrame()->GetNext() &&
                  0 == GetTextFrame()->GetNext()->getFramePrintArea().Height() &&
                  GetTextFrame()->GetNext()->IsTextFrame() &&
                  static_cast<SwTextFrame*>(GetTextFrame()->GetNext())->IsUndersized() ) ;

            if( bUnderSized || bNextUndersized )
            {
                if ( bAdjustBaseLine )
                    GetInfo().Y( GetInfo().GetPos().Y() + m_pCurr->GetAscent() );

                // Left arrow (text overflowing)
                if( pArrow )
                    GetInfo().DrawRedArrow( *pArrow );

                // GetInfo().Y() must be current baseline
                SwTwips nDiff = GetInfo().Y() + nTmpHeight - nTmpAscent - GetTextFrame()->getFrameArea().Bottom();
                if( ( nDiff > 0 &&
                      (GetEnd() < TextFrameIndex(GetInfo().GetText().getLength()) ||
                        ( nDiff > nTmpHeight/2 && GetPrevLine() ) ) ) ||
                    (nDiff >= 0 && bNextUndersized) )

                {
                    // Right arrow (text overflowing)
                    SwArrowPortion aArrow( GetInfo() );
                    GetInfo().DrawRedArrow( aArrow );
                }

                GetInfo().Y( nOldY );
            }
        }
    }

    if( m_pCurr->IsClipping() )
        rClip.ChgClip( rPaint, m_pFrame );
}

void SwTextPainter::CheckSpecialUnderline( const SwLinePortion* pPor,
                                          tools::Long nAdjustBaseLine )
{
    // Check if common underline should not be continued
    if ( IsUnderlineBreak( *pPor, *m_pFont ) )
    {
        // delete underline font
        delete GetInfo().GetUnderFnt();
        GetInfo().SetUnderFnt( nullptr );
        return;
    }
    // Reuse calculated underline font as much as possible.
    if (GetInfo().GetUnderFnt() &&
        GetInfo().GetIdx() + pPor->GetLen() <= GetInfo().GetUnderFnt()->GetEnd() + TextFrameIndex(1))
    {
        SwFont &rFont = GetInfo().GetUnderFnt()->GetFont();
        const Color aColor = GetUnderColor( GetInfo().GetFont() );
        if ( GetUnderColor( &rFont ) != aColor )
            rFont.SetColor( aColor );
        return;
    }

    // If current underline matches the common underline font, we continue
    // to use the common underline font.
    // Bug 120769:Color of underline display wrongly
    if ( GetInfo().GetUnderFnt() &&
        GetInfo().GetUnderFnt()->GetFont().GetUnderline() == GetFnt()->GetUnderline() &&
        GetInfo().GetFont() && GetInfo().GetFont()->GetUnderColor() != COL_AUTO )
        return;
    //Bug 120769(End)

    OSL_ENSURE( GetFnt() && LINESTYLE_NONE != GetFnt()->GetUnderline(),
            "CheckSpecialUnderline without underlined font" );
    MultiSelection aUnderMulti( Range( 0, GetInfo().GetText().getLength() ) );
    const SwFont* pParaFnt = GetAttrHandler().GetFont();
    if( pParaFnt && pParaFnt->GetUnderline() == GetFnt()->GetUnderline() )
        aUnderMulti.SelectAll();

    if (sw::MergedPara const*const pMerged = GetTextFrame()->GetMergedPara())
    {
        // first, add the paragraph properties to MultiSelection - if there are
        // Hints too, they will override the positions if they're added later
        sal_Int32 nTmp(0);
        for (auto const& e : pMerged->extents)
        {
            if (const SvxUnderlineItem* pItem = e.pNode->GetSwAttrSet().GetItemIfSet(
                        RES_CHRATR_UNDERLINE))
            {
                const bool bUnderSelect(m_pFont->GetUnderline() ==
                    pItem->GetLineStyle());
                aUnderMulti.Select(Range(nTmp, nTmp + e.nEnd - e.nStart - 1),
                        bUnderSelect);
            }
            nTmp += e.nEnd - e.nStart;
        }
    }

    SwTextNode const* pNode(nullptr);
    sw::MergedAttrIter iter(*GetTextFrame());
    for (SwTextAttr const* pTextAttr = iter.NextAttr(&pNode); pTextAttr;
         pTextAttr = iter.NextAttr(&pNode))
    {
        SvxUnderlineItem const*const pItem =
            CharFormat::GetItem(*pTextAttr, RES_CHRATR_UNDERLINE);

        if (pItem)
        {
            TextFrameIndex const nStart(
                GetTextFrame()->MapModelToView(pNode, pTextAttr->GetStart()));
            TextFrameIndex const nEnd(
                GetTextFrame()->MapModelToView(pNode, *pTextAttr->End()));
            if (nEnd > nStart)
            {
                const bool bUnderSelect = m_pFont->GetUnderline() == pItem->GetLineStyle();
                aUnderMulti.Select(Range(sal_Int32(nStart), sal_Int32(nEnd) - 1),
                        bUnderSelect);
            }
        }
    }

    const TextFrameIndex nIndx = GetInfo().GetIdx();
    TextFrameIndex nUnderEnd(0);
    const size_t nCnt = aUnderMulti.GetRangeCount();

    // find the underline range the current portion is contained in
    for( size_t i = 0; i < nCnt; ++i )
    {
        const Range& rRange = aUnderMulti.GetRange( i );
        if (nUnderEnd == TextFrameIndex(rRange.Min()))
            nUnderEnd = TextFrameIndex(rRange.Max());
        else if (nIndx >= TextFrameIndex(rRange.Min()))
        {
            nUnderEnd = TextFrameIndex(rRange.Max());
        }
        else
            break;
    }

    if ( GetEnd() && GetEnd() <= nUnderEnd )
        nUnderEnd = GetEnd() - TextFrameIndex(1);

    // calculate the new common underline font
    SwFont* pUnderlineFnt = nullptr;
    Point aCommonBaseLine;

    // check, if underlining is not isolated
    if (nIndx + GetInfo().GetLen() < nUnderEnd + TextFrameIndex(1))
    {
        // here starts the algorithm for calculating the underline font
        SwScriptInfo& rScriptInfo = GetInfo().GetParaPortion()->GetScriptInfo();
        SwAttrIter aIter(*GetInfo().GetTextFrame()->GetTextNodeFirst(),
                         rScriptInfo, GetTextFrame());

        TextFrameIndex nTmpIdx = nIndx;
        sal_uLong nSumWidth = 0;
        sal_uLong nSumHeight = 0;
        sal_uLong nBold = 0;
        sal_uInt16 nMaxBaseLineOfst = 0;
        int nNumberOfPortions = 0;

        while (nTmpIdx <= nUnderEnd && pPor)
        {
            if ( pPor->IsFlyPortion() || pPor->IsFlyCntPortion() ||
                pPor->IsBreakPortion() || pPor->IsMarginPortion() ||
                pPor->IsHolePortion() ||
                ( pPor->IsMultiPortion() && ! static_cast<const SwMultiPortion*>(pPor)->IsBidi() ) )
                break;

            aIter.Seek( nTmpIdx );
            if ( aIter.GetFnt()->GetEscapement() < 0 || m_pFont->IsWordLineMode() ||
                 SvxCaseMap::SmallCaps == m_pFont->GetCaseMap() )
                break;

            if ( !aIter.GetFnt()->GetEscapement() )
            {
                nSumWidth += pPor->Width();
                const sal_uLong nFontHeight = aIter.GetFnt()->GetHeight();

                // If we do not have a common baseline we take the baseline
                // and the font of the lowest portion.
                if ( nAdjustBaseLine )
                {
                    const sal_uInt16 nTmpBaseLineOfst = AdjustBaseLine( *m_pCurr, pPor );
                    if ( nMaxBaseLineOfst < nTmpBaseLineOfst )
                    {
                        nMaxBaseLineOfst = nTmpBaseLineOfst;
                        nSumHeight = nFontHeight;
                    }
                }
                // in horizontal layout we build a weighted sum of the heights
                else
                    nSumHeight += pPor->Width() * nFontHeight;

                if ( WEIGHT_NORMAL != aIter.GetFnt()->GetWeight() )
                    nBold += pPor->Width();
            }

            ++nNumberOfPortions;

            nTmpIdx += pPor->GetLen();
            pPor = pPor->GetNextPortion();
        }

        // resulting height
        if ( nNumberOfPortions > 1 && nSumWidth )
        {
            const sal_uLong nNewFontHeight = nAdjustBaseLine ?
                                         nSumHeight :
                                         nSumHeight / nSumWidth;

            pUnderlineFnt = new SwFont( *GetInfo().GetFont() );

            // font height
            const SwFontScript nActual = pUnderlineFnt->GetActual();
            pUnderlineFnt->SetSize( Size( pUnderlineFnt->GetSize( nActual ).Width(),
                                          nNewFontHeight ), nActual );

            // font weight
            if ( 2 * nBold > nSumWidth )
                pUnderlineFnt->SetWeight( WEIGHT_BOLD, nActual );
            else
                pUnderlineFnt->SetWeight( WEIGHT_NORMAL, nActual );

            // common base line
            aCommonBaseLine.setY( nAdjustBaseLine + nMaxBaseLineOfst );
        }
    }

    // an escaped redlined portion should also have a special underlining
    if( ! pUnderlineFnt && m_pFont->GetEscapement() > 0 && GetRedln() &&
        GetRedln()->ChkSpecialUnderline() )
        pUnderlineFnt = new SwFont( *m_pFont );

    delete GetInfo().GetUnderFnt();

    if ( pUnderlineFnt )
    {
        pUnderlineFnt->SetProportion( 100 );
        pUnderlineFnt->SetEscapement( 0 );
        pUnderlineFnt->SetStrikeout( STRIKEOUT_NONE );
        pUnderlineFnt->SetOverline( LINESTYLE_NONE );
        const Color aFillColor( COL_TRANSPARENT );
        pUnderlineFnt->SetFillColor( aFillColor );

        GetInfo().SetUnderFnt( new SwUnderlineFont( *pUnderlineFnt, nUnderEnd,
                                                     aCommonBaseLine ) );
    }
    else
        // I'm sorry, we do not have a special underlining font for you.
        GetInfo().SetUnderFnt( nullptr );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

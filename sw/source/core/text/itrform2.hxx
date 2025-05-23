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
#pragma once

#include "itrpaint.hxx"

class SwFlyCntPortion;
class SwDropPortion;
class SwFormatDrop;
class SwTextAttr;
class SwNumberPortion;
class SwErgoSumPortion;
class SwExpandPortion;
class SwMultiPortion;
class SwFootnotePortion;

class SwTextFormatter : public SwTextPainter
{
    const SwFormatDrop *m_pDropFormat;
    SwMultiPortion* m_pMulti; // during formatting a multi-portion
    sal_uInt8 m_nContentEndHyph;  // Counts consecutive hyphens at the line end
    sal_uInt8 m_nContentMidHyph;  // Counts consecutive hyphens before flies
    TextFrameIndex m_nLeftScanIdx; // for increasing performance during
    TextFrameIndex m_nRightScanIdx; // scanning for portion ends
    bool m_bOnceMore : 1; // Another round?
    bool m_bFlyInContentBase : 1; // Base reference that sets a character-bound frame
    bool m_bTruncLines : 1; // Flag for extending the repaint rect, if needed
    bool m_bUnclipped : 1; // Flag whether repaint is larger than the fixed line height
    std::unique_ptr<sw::MergedAttrIterByEnd> m_pByEndIter; // HACK for TryNewNoLengthPortion
    SwLinePortion* m_pFirstOfBorderMerge; // The first text portion of a joined border (during portion building)

    SwLinePortion *NewPortion(SwTextFormatInfo &rInf, ::std::optional<TextFrameIndex>);
    SwTextPortion  *NewTextPortion( SwTextFormatInfo &rInf );
    SwLinePortion *NewExtraPortion( SwTextFormatInfo &rInf );
    SwTabPortion *NewTabPortion( SwTextFormatInfo &rInf, bool bAuto ) const;
    SwNumberPortion *NewNumberPortion( SwTextFormatInfo &rInf ) const;
    SwDropPortion *NewDropPortion( SwTextFormatInfo &rInf );
    SwNumberPortion *NewFootnoteNumPortion( SwTextFormatInfo const &rInf ) const;
    SwErgoSumPortion *NewErgoSumPortion( SwTextFormatInfo const &rInf ) const;
    SwExpandPortion *NewFieldPortion( SwTextFormatInfo &rInf,
                                    const SwTextAttr *pHt ) const;
    SwFootnotePortion *NewFootnotePortion( SwTextFormatInfo &rInf, SwTextAttr *pHt );

    /**
        Sets a new portion for an object anchored as character
     */
    SwFlyCntPortion *NewFlyCntPortion( SwTextFormatInfo &rInf,
                                       SwTextAttr *pHt ) const;
    SwLinePortion *WhichFirstPortion( SwTextFormatInfo &rInf );
    SwTextPortion *WhichTextPor( SwTextFormatInfo &rInf ) const;
    SwExpandPortion * TryNewNoLengthPortion( SwTextFormatInfo const & rInfo );

    // The center piece of formatting
    void BuildPortions( SwTextFormatInfo &rInf );

    bool BuildMultiPortion( SwTextFormatInfo &rInf, SwMultiPortion& rMulti );

    /**
        Calculation of the emulated right side.

        Determines the next object, that reaches into the rest of the line and
        constructs the appropriate FlyPortion.
        SwTextFly::GetFrame(const SwRect&, bool) will be needed for this.

        The right edge can be shortened by flys
     */
    void CalcFlyWidth( SwTextFormatInfo &rInf );

    // Is overloaded by SwTextFormatter because of UpdatePos
    void CalcAdjustLine( SwLineLayout *pCurr );

    // considers line spacing attributes
    void CalcRealHeight( bool bNewLine = false );

    // Transfers the data to rInf
    void FeedInf( SwTextFormatInfo &rInf ) const;

    // Treats underflow situations
    SwLinePortion *Underflow( SwTextFormatInfo &rInf );

    // Calculates the ascent and the height from the fontmetric
    void CalcAscent( SwTextFormatInfo &rInf, SwLinePortion *pPor );

    // determines, if an optimized repaint rectangle is allowed
    bool AllowRepaintOpt() const;

    // Is called by FormatLine
    void FormatReset( SwTextFormatInfo &rInf );

    /**
        The position of the portions changes with the adjustment.

        This method updates the reference point of the anchored as character objects,
        for example after adjustment change (right alignment, justified, etc.)
        Mainly to correct the X position.
     */
    void UpdatePos(SwLineLayout *pCurr, Point aStart, TextFrameIndex nStartIdx,
            bool bAlways = false ) const;

    /**
        Set all anchored as character objects to the passed BaseLine
        (in Y direction).
     */
    void AlignFlyInCntBase( tools::Long nBaseLine ) const;

    /**
        This is called after the real height of the line has been calculated
        Therefore it is possible, that more flys from below intersect with the
        line, or that flys from above do not intersect with the line anymore.
        We check this and return true, meaning that the line has to be
        formatted again.
     */
    bool ChkFlyUnderflow( SwTextFormatInfo &rInf ) const;

    // Insert portion
    void InsertPortion( SwTextFormatInfo &rInf, SwLinePortion *pPor );

    // Guess height for the DropPortion
    void GuessDropHeight( const sal_uInt16 nLines );

public:
    // Calculate the height for the DropPortion
    void CalcDropHeight( const sal_uInt16 nLines );

    // Calculates the paragraphs bottom, takes anchored objects within it into
    // account which have a wrap setting of "wrap at 1st paragraph"
    SwTwips CalcBottomLine() const;

    // Takes character-bound objects into account when calculating the
    // repaint rect in lines with fixed line height
    void CalcUnclipped( SwTwips& rTop, SwTwips& rBottom );

    // Amongst others for DropCaps
    bool CalcOnceMore();

    void CtorInitTextFormatter( SwTextFrame *pFrame, SwTextFormatInfo *pInf );
    SwTextFormatter(SwTextFrame *pTextFrame, SwTextFormatInfo *pTextFormatInf)
        : SwTextPainter(pTextFrame->GetTextNodeFirst())
        , m_bUnclipped(false)
    {
        CtorInitTextFormatter( pTextFrame, pTextFormatInf );
    }
    virtual ~SwTextFormatter() override;

    TextFrameIndex FormatLine(TextFrameIndex nStart);

    void RecalcRealHeight();

    // We format a line for interactive hyphenation
    bool Hyphenate(SwInterHyphInfoTextFrame & rInf);

    // A special method for QuoVadis texts:
    // nErgo is the page number of the ErgoSum Footnote
    // At 0 it's still unclear
    TextFrameIndex FormatQuoVadis(TextFrameIndex nStart);

    // The emergency break: Cancel formatting, discard line
    bool IsStop() const { return GetInfo().IsStop(); }

    // The counterpart: Continue formatting at all costs
    bool IsNewLine() const { return GetInfo().IsNewLine(); }

    // FormatQuick(); Refresh formatting information
    bool IsQuick() const { return GetInfo().IsQuick(); }

    // Create a SwLineLayout if needed, which avoids Footnote/Fly to oscillate
    void MakeDummyLine();

    // SwTextIter functionality
    void Insert( SwLineLayout *pLine );

    // The remaining height to the page border
    SwTwips GetFrameRstHeight() const;

    // How wide would you be without any bounds (Flys etc.)?
    SwTwips CalcFitToContent_( );

    SwLinePortion* MakeRestPortion(const SwLineLayout* pLine, TextFrameIndex nPos);

    const SwFormatDrop *GetDropFormat() const { return m_pDropFormat; }
    void ClearDropFormat() { m_pDropFormat = nullptr; }

    SwMultiPortion *GetMulti() const { return m_pMulti; }

    bool IsOnceMore() const { return m_bOnceMore; }
    void SetOnceMore( bool bNew ) { m_bOnceMore = bNew; }

    bool HasTruncLines() const { return m_bTruncLines; }
    void SetTruncLines( bool bNew ) { m_bTruncLines = bNew; }

    bool IsUnclipped() const { return m_bUnclipped; }
    void SetUnclipped( bool bNew ) { m_bUnclipped = bNew; }

    bool IsFlyInCntBase() const { return m_bFlyInContentBase; }
    void SetFlyInCntBase( bool bNew = true ) { m_bFlyInContentBase = bNew; }

    SwTextFormatInfo &GetInfo()
        { return static_cast<SwTextFormatInfo&>(SwTextIter::GetInfo()); }
    const SwTextFormatInfo &GetInfo() const
        { return static_cast<const SwTextFormatInfo&>(SwTextIter::GetInfo()); }

    void InitCntHyph() { CntHyphens( m_nContentEndHyph, m_nContentMidHyph ); }
    const sal_uInt8 &CntEndHyph() const { return m_nContentEndHyph; }
    const sal_uInt8 &CntMidHyph() const { return m_nContentMidHyph; }
    sal_uInt8 &CntEndHyph() { return m_nContentEndHyph; }
    sal_uInt8 &CntMidHyph() { return m_nContentMidHyph; }

    /**
     * Merge border of the drop portion with modifying the font of
     * the portions' part. Removing left or right border.
     * @param   rPortion    drop portion for merge
    **/
    static void MergeCharacterBorder( SwDropPortion const & rPortion );

    /**
     * Merge border of the line portion with setting the portion's
     * m_bJoinBorderWidthNext and m_bJoinBorderWidthPrev members and
     * changing the size (width, height and ascent) of the portion
     * to get a merged border.
     * @param   rPortion    portion for merge
     * @param   pPrev       portion immediately before rPortion
     * @param   rInf        contain information
    **/
    void MergeCharacterBorder( SwLinePortion& rPortion, SwLinePortion const *pPrev, SwTextFormatInfo& rInf );

    bool ClearIfIsFirstOfBorderMerge(SwLinePortion const *pPortion);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

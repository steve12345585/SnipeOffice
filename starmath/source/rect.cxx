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

#include <osl/diagnose.h>
#include <o3tl/sorted_vector.hxx>
#include <vcl/metric.hxx>
#include <vcl/svapp.hxx>
#include <vcl/virdev.hxx>
#include <sal/log.hxx>

#include <format.hxx>
#include <rect.hxx>
#include <types.hxx>
#include <smmod.hxx>

namespace {

bool SmGetGlyphBoundRect(const vcl::RenderContext &rDev,
                         const OUString &rText, tools::Rectangle &rRect)
    // basically the same as 'GetTextBoundRect' (in class 'OutputDevice')
    // but with a string as argument.
{
    // handle special case first
    if (rText.isEmpty())
    {
        rRect.SetEmpty();
        return true;
    }

    // get a device where 'OutputDevice::GetTextBoundRect' will be successful
    OutputDevice *pGlyphDev;
    if (rDev.GetOutDevType() != OUTDEV_PRINTER)
        pGlyphDev = const_cast<OutputDevice *>(&rDev);
    else
    {
        // since we format for the printer (where GetTextBoundRect will fail)
        // we need a virtual device here.
        pGlyphDev = &SmModule::get()->GetDefaultVirtualDev();
    }

    const FontMetric  aDevFM (rDev.GetFontMetric());

    pGlyphDev->Push(vcl::PushFlags::FONT | vcl::PushFlags::MAPMODE);
    vcl::Font aFnt(rDev.GetFont());
    aFnt.SetAlignment(ALIGN_TOP);

    // use scale factor when calling GetTextBoundRect to counter
    // negative effects from antialiasing which may otherwise result
    // in significant incorrect bounding rectangles for some characters.
    Size aFntSize = aFnt.GetFontSize();

    // Workaround to avoid HUGE font sizes and resulting problems
    tools::Long nScaleFactor = 1;
    while( aFntSize.Height() > 2000 * nScaleFactor )
        nScaleFactor *= 2;

    aFnt.SetFontSize( Size( aFntSize.Width() / nScaleFactor, aFntSize.Height() / nScaleFactor ) );
    pGlyphDev->SetFont(aFnt);

    tools::Long nTextWidth = rDev.GetTextWidth(rText);
    tools::Rectangle   aResult (Point(), Size(nTextWidth, rDev.GetTextHeight())),
                       aTmp;

    bool bSuccess = pGlyphDev->GetTextBoundRect(aTmp, rText);
    OSL_ENSURE( bSuccess, "GetTextBoundRect failed" );


    if (!aTmp.IsEmpty())
    {
        aResult = tools::Rectangle(aTmp.Left() * nScaleFactor, aTmp.Top() * nScaleFactor,
                            aTmp.Right() * nScaleFactor, aTmp.Bottom() * nScaleFactor);
        if (&rDev != pGlyphDev) /* only when rDev is a printer... */
        {
            tools::Long nGDTextWidth  = pGlyphDev->GetTextWidth(rText);
            if (nGDTextWidth != 0  &&
                nTextWidth != nGDTextWidth)
            {
                aResult.SetRight( aResult.Right() * nTextWidth );
                aResult.SetRight( aResult.Right() / ( nGDTextWidth * nScaleFactor) );
            }
        }
    }

    // move rectangle to match possibly different baselines
    // (because of different devices)
    tools::Long nDelta = aDevFM.GetAscent() - pGlyphDev->GetFontMetric().GetAscent() * nScaleFactor;
    aResult.Move(0, nDelta);

    pGlyphDev->Pop();

    rRect = aResult;
    return bSuccess;
}

bool SmIsMathAlpha(std::u16string_view aText)
    // true iff symbol (from StarMath Font) should be treated as letter
{
    // Set of symbols, which should be treated as letters in StarMath Font
    // (to get a normal (non-clipped) SmRect in contrast to the other operators
    // and symbols).
    static o3tl::sorted_vector<sal_Unicode> const aMathAlpha({
        MS_ALEPH,               MS_IM,                  MS_RE,
        MS_WP,                  u'\xE070',              MS_EMPTYSET,
        u'\x2113',              u'\xE0D6',              u'\x2107',
        u'\x2127',              u'\x210A',              MS_HBAR,
        MS_LAMBDABAR,           MS_SETN,                MS_SETZ,
        MS_SETQ,                MS_SETR,                MS_SETC,
        u'\x2373',              u'\xE0A5',              u'\x2112',
        u'\x2130',              u'\x2131'
    });

    if (aText.empty())
        return false;

    OSL_ENSURE(aText.size() == 1, "Sm : string must be exactly one character long");
    sal_Unicode cChar = aText[0];

    // is it a greek symbol?
    if (u'\xE0AC' <= cChar  &&  cChar <= u'\xE0D4')
        return true;
    // or, does it appear in 'aMathAlpha'?
    return aMathAlpha.find(cChar) != aMathAlpha.end();
}

}


SmRect::SmRect()
    // constructs empty rectangle at (0, 0) with width and height 0.
    : aTopLeft(0, 0)
    , aSize(0, 0)
    , nBaseline(0)
    , nAlignT(0)
    , nAlignM(0)
    , nAlignB(0)
    , nGlyphTop(0)
    , nGlyphBottom(0)
    , nItalicLeftSpace(0)
    , nItalicRightSpace(0)
    , nLoAttrFence(0)
    , nHiAttrFence(0)
    , nBorderWidth(0)
    , bHasBaseline(false)
    , bHasAlignInfo(false)
{
}


void SmRect::CopyAlignInfo(const SmRect &rRect)
{
    nBaseline     = rRect.nBaseline;
    bHasBaseline  = rRect.bHasBaseline;
    nAlignT       = rRect.nAlignT;
    nAlignM       = rRect.nAlignM;
    nAlignB       = rRect.nAlignB;
    bHasAlignInfo = rRect.bHasAlignInfo;
    nLoAttrFence  = rRect.nLoAttrFence;
    nHiAttrFence  = rRect.nHiAttrFence;
}


SmRect::SmRect(const OutputDevice &rDev, const SmFormat *pFormat,
               const OUString &rText, sal_uInt16 nBorder)
    // get rectangle fitting for drawing 'rText' on OutputDevice 'rDev'
    : aTopLeft(0, 0)
    , aSize(rDev.GetTextWidth(rText), rDev.GetTextHeight())
{
    const FontMetric  aFM (rDev.GetFontMetric());
    bool              bIsMath  = aFM.GetFamilyName().equalsIgnoreAsciiCase( FONTNAME_MATH );
    bool              bAllowSmaller = bIsMath && !SmIsMathAlpha(rText);
    const tools::Long        nFontHeight = rDev.GetFont().GetFontSize().Height();

    nBorderWidth  = nBorder;
    bHasAlignInfo = true;
    bHasBaseline  = true;
    nBaseline     = aFM.GetAscent();
    nAlignT       = nBaseline - nFontHeight * 750 / 1000;
    nAlignM       = nBaseline - nFontHeight * 121 / 422;
        // that's where the horizontal bars of '+', '-', ... are
        // (1/3 of ascent over baseline)
        // (121 = 1/3 of 12pt ascent, 422 = 12pt fontheight)
    nAlignB       = nBaseline;

    // workaround for printer fonts with very small (possible 0 or even
    // negative(!)) leading
    if (aFM.GetInternalLeading() < 5  &&  rDev.GetOutDevType() == OUTDEV_PRINTER)
    {
        OutputDevice    *pWindow = Application::GetDefaultDevice();

        pWindow->Push(vcl::PushFlags::MAPMODE | vcl::PushFlags::FONT);

        pWindow->SetMapMode(rDev.GetMapMode());
        pWindow->SetFont(rDev.GetFontMetric());

        tools::Long  nDelta = pWindow->GetFontMetric().GetInternalLeading();
        if (nDelta == 0)
        {   // this value approx. fits a Leading of 80 at a
            // Fontheight of 422 (12pt)
            nDelta = nFontHeight * 8 / 43;
        }
        SetTop(GetTop() - nDelta);

        pWindow->Pop();
    }

    // get GlyphBoundRect
    tools::Rectangle  aGlyphRect;
    bool bSuccess = SmGetGlyphBoundRect(rDev, rText, aGlyphRect);
    if (!bSuccess)
        SAL_WARN("starmath", "Ooops... (Font missing?)");

    nItalicLeftSpace  = GetLeft() - aGlyphRect.Left() + nBorderWidth;
    nItalicRightSpace = aGlyphRect.Right() - GetRight() + nBorderWidth;
    if (nItalicLeftSpace  < 0  &&  !bAllowSmaller)
        nItalicLeftSpace  = 0;
    if (nItalicRightSpace < 0  &&  !bAllowSmaller)
        nItalicRightSpace = 0;

    tools::Long  nDist = 0;
    if (pFormat)
        nDist = (rDev.GetFont().GetFontSize().Height()
                * pFormat->GetDistance(DIS_ORNAMENTSIZE)) / 100;

    nHiAttrFence = aGlyphRect.Top() - 1 - nBorderWidth - nDist;
    nLoAttrFence = SmFromTo(GetAlignB(), GetBottom(), 0.0);

    nGlyphTop    = aGlyphRect.Top() - nBorderWidth;
    nGlyphBottom = aGlyphRect.Bottom() + nBorderWidth;

    if (bAllowSmaller)
    {
        // for symbols and operators from the StarMath Font
        // we adjust upper and lower margin of the symbol
        SetTop(nGlyphTop);
        SetBottom(nGlyphBottom);
    }

    if (nHiAttrFence < GetTop())
        nHiAttrFence = GetTop();

    if (nLoAttrFence > GetBottom())
        nLoAttrFence = GetBottom();

    OSL_ENSURE(rText.isEmpty() || !IsEmpty(),
               "Sm: empty rectangle created");
}


SmRect::SmRect(tools::Long nWidth, tools::Long nHeight)
    // this constructor should never be used for anything textlike because
    // it will not provide useful values for baseline, AlignT and AlignB!
    // It's purpose is to get a 'SmRect' for the horizontal line in fractions
    // as used in 'SmBinVerNode'.
    : aTopLeft(0, 0)
    , aSize(nWidth, nHeight)
    , nBaseline(0)
    , nItalicLeftSpace(0)
    , nItalicRightSpace(0)
    , nBorderWidth(0)
    , bHasBaseline(false)
    , bHasAlignInfo(true)
{
    nAlignT = nGlyphTop    = nHiAttrFence = GetTop();
    nAlignB = nGlyphBottom = nLoAttrFence = GetBottom();
    nAlignM = (nAlignT + nAlignB) / 2;        // this is the default
}


void SmRect::SetLeft(tools::Long nLeft)
{
    if (nLeft <= GetRight())
    {   aSize.setWidth( GetRight() - nLeft + 1 );
        aTopLeft.setX( nLeft );
    }
}


void SmRect::SetRight(tools::Long nRight)
{
    if (nRight >= GetLeft())
        aSize.setWidth( nRight - GetLeft() + 1 );
}


void SmRect::SetBottom(tools::Long nBottom)
{
    if (nBottom >= GetTop())
        aSize.setHeight( nBottom - GetTop() + 1 );
}


void SmRect::SetTop(tools::Long nTop)
{
    if (nTop <= GetBottom())
    {   aSize.setHeight( GetBottom() - nTop + 1 );
        aTopLeft.setY( nTop );
    }
}


void SmRect::Move(const Point &rPosition)
    // move rectangle by position 'rPosition'.
{
    aTopLeft  += rPosition;

    tools::Long  nDelta = rPosition.Y();
    nBaseline += nDelta;
    nAlignT   += nDelta;
    nAlignM   += nDelta;
    nAlignB   += nDelta;
    nGlyphTop    += nDelta;
    nGlyphBottom += nDelta;
    nHiAttrFence += nDelta;
    nLoAttrFence += nDelta;
}


Point SmRect::AlignTo(const SmRect &rRect, RectPos ePos,
                            RectHorAlign eHor, RectVerAlign eVer) const
{   Point  aPos (GetTopLeft());
        // will become the topleft point of the new rectangle position

    // set horizontal or vertical new rectangle position depending on ePos
    switch (ePos)
    {   case RectPos::Left:
            aPos.setX( rRect.GetItalicLeft() - GetItalicRightSpace()
                       - GetWidth() );
            break;
        case RectPos::Right:
            aPos.setX( rRect.GetItalicRight() + 1 + GetItalicLeftSpace() );
            break;
        case RectPos::Top:
            aPos.setY( rRect.GetTop() - GetHeight() );
            break;
        case RectPos::Bottom:
            aPos.setY( rRect.GetBottom() + 1 );
            break;
        case RectPos::Attribute:
            aPos.setX( rRect.GetItalicCenterX() - GetItalicWidth() / 2
                       + GetItalicLeftSpace() );
            break;
        default:
            assert(false);
    }

    // check if horizontal position is already set
    if (ePos == RectPos::Left || ePos == RectPos::Right || ePos == RectPos::Attribute)
        // correct error in current vertical position
        switch (eVer)
        {   case RectVerAlign::Top :
                aPos.AdjustY(rRect.GetAlignT() - GetAlignT() );
                break;
            case RectVerAlign::Mid :
                aPos.AdjustY(rRect.GetAlignM() - GetAlignM() );
                break;
            case RectVerAlign::Baseline :
                // align baselines if possible else align mid's
                if (HasBaseline() && rRect.HasBaseline())
                    aPos.AdjustY(rRect.GetBaseline() - GetBaseline() );
                else
                    aPos.AdjustY(rRect.GetAlignM() - GetAlignM() );
                break;
            case RectVerAlign::Bottom :
                aPos.AdjustY(rRect.GetAlignB() - GetAlignB() );
                break;
            case RectVerAlign::CenterY :
                aPos.AdjustY(rRect.GetCenterY() - GetCenterY() );
                break;
            case RectVerAlign::AttributeHi:
                aPos.AdjustY(rRect.GetHiAttrFence() - GetBottom() );
                break;
            case RectVerAlign::AttributeMid :
                aPos.AdjustY(SmFromTo(rRect.GetAlignB(), rRect.GetAlignT(), 0.4)
                            - GetCenterY() );
                break;
            case RectVerAlign::AttributeLo :
                aPos.AdjustY(rRect.GetLoAttrFence() - GetTop() );
                break;
        default :
                assert(false);
        }

    // check if vertical position is already set
    if (ePos == RectPos::Top || ePos == RectPos::Bottom)
        // correct error in current horizontal position
        switch (eHor)
        {   case RectHorAlign::Left:
                aPos.AdjustX(rRect.GetItalicLeft() - GetItalicLeft() );
                break;
            case RectHorAlign::Center:
                aPos.AdjustX(rRect.GetItalicCenterX() - GetItalicCenterX() );
                break;
            case RectHorAlign::Right:
                aPos.AdjustX(rRect.GetItalicRight() - GetItalicRight() );
                break;
            default:
                assert(false);
        }

    return aPos;
}


void SmRect::Union(const SmRect &rRect)
    // rectangle union of current one with 'rRect'. The result is to be the
    // smallest rectangles that covers the space of both rectangles.
    // (empty rectangles cover no space)
    //! Italic correction is NOT taken into account here!
{
    if (rRect.IsEmpty())
        return;

    tools::Long  nL  = rRect.GetLeft(),
          nR  = rRect.GetRight(),
          nT  = rRect.GetTop(),
          nB  = rRect.GetBottom(),
          nGT = rRect.nGlyphTop,
          nGB = rRect.nGlyphBottom;
    if (!IsEmpty())
    {   tools::Long  nTmp;

        if ((nTmp = GetLeft()) < nL)
            nL = nTmp;
        if ((nTmp = GetRight()) > nR)
            nR = nTmp;
        if ((nTmp = GetTop()) < nT)
            nT = nTmp;
        if ((nTmp = GetBottom()) > nB)
            nB = nTmp;
        if ((nTmp = nGlyphTop) < nGT)
            nGT = nTmp;
        if ((nTmp = nGlyphBottom) > nGB)
            nGB = nTmp;
    }

    SetLeft(nL);
    SetRight(nR);
    SetTop(nT);
    SetBottom(nB);
    nGlyphTop    = nGT;
    nGlyphBottom = nGB;
}


SmRect & SmRect::ExtendBy(const SmRect &rRect, RectCopyMBL eCopyMode)
    // let current rectangle be the union of itself and 'rRect'
    // (the smallest rectangle surrounding both). Also adapt values for
    // 'AlignT', 'AlignM', 'AlignB', baseline and italic-spaces.
    // The baseline is set according to 'eCopyMode'.
    // If one of the rectangles has no relevant info the other one is copied.
{
    // get some values used for (italic) spaces adaptation
    // ! (need to be done before changing current SmRect) !
    tools::Long  nL = std::min(GetItalicLeft(),  rRect.GetItalicLeft()),
          nR = std::max(GetItalicRight(), rRect.GetItalicRight());

    Union(rRect);

    SetItalicSpaces(GetLeft() - nL, nR - GetRight());

    if (!HasAlignInfo())
        CopyAlignInfo(rRect);
    else if (rRect.HasAlignInfo())
    {
        assert(HasAlignInfo());
        nAlignT = std::min(GetAlignT(), rRect.GetAlignT());
        nAlignB = std::max(GetAlignB(), rRect.GetAlignB());
        nHiAttrFence = std::min(GetHiAttrFence(), rRect.GetHiAttrFence());
        nLoAttrFence = std::max(GetLoAttrFence(), rRect.GetLoAttrFence());

        switch (eCopyMode)
        {   case RectCopyMBL::This:
                // already done
                break;
            case RectCopyMBL::Arg:
                CopyMBL(rRect);
                break;
            case RectCopyMBL::None:
                bHasBaseline = false;
                nAlignM = (nAlignT + nAlignB) / 2;
                break;
            case RectCopyMBL::Xor:
                if (!HasBaseline())
                    CopyMBL(rRect);
                break;
            default :
                assert(false);
        }
    }

    return *this;
}


void SmRect::ExtendBy(const SmRect &rRect, RectCopyMBL eCopyMode,
                          tools::Long nNewAlignM)
    // as 'ExtendBy' but sets AlignM value to 'nNewAlignM'.
    // (this version will be used in 'SmBinVerNode' to provide means to
    // align eg "{a over b} over c" correctly where AlignM should not
    // be (AlignT + AlignB) / 2)
{
    OSL_ENSURE(HasAlignInfo(), "Sm: no align info");

    ExtendBy(rRect, eCopyMode);
    nAlignM = nNewAlignM;
}


SmRect & SmRect::ExtendBy(const SmRect &rRect, RectCopyMBL eCopyMode,
                          bool bKeepVerAlignParams)
    // as 'ExtendBy' but keeps original values for AlignT, -M and -B and
    // baseline.
    // (this is used in 'SmSupSubNode' where the sub-/supscripts shouldn't
    // be allowed to modify these values.)
{
    tools::Long  nOldAlignT   = GetAlignT(),
          nOldAlignM   = GetAlignM(),
          nOldAlignB   = GetAlignB(),
          nOldBaseline = nBaseline;     //! depends not on 'HasBaseline'
    bool  bOldHasAlignInfo = HasAlignInfo();

    ExtendBy(rRect, eCopyMode);

    if (bKeepVerAlignParams)
    {   nAlignT   = nOldAlignT;
        nAlignM   = nOldAlignM;
        nAlignB   = nOldAlignB;
        nBaseline = nOldBaseline;
        bHasAlignInfo = bOldHasAlignInfo;
    }

    return *this;
}


tools::Long SmRect::OrientedDist(const Point &rPoint) const
    // return oriented distance of rPoint to the current rectangle,
    // especially the return value is <= 0 iff the point is inside the
    // rectangle.
    // For simplicity the maximum-norm is used.
{
    bool  bIsInside = IsInsideItalicRect(rPoint);

    // build reference point to define the distance
    Point  aRef;
    if (bIsInside)
    {   Point  aIC (GetItalicCenterX(), GetCenterY());

        aRef.setX( rPoint.X() >= aIC.X() ? GetItalicRight() : GetItalicLeft() );
        aRef.setY( rPoint.Y() >= aIC.Y() ? GetBottom() : GetTop() );
    }
    else
    {
        // x-coordinate
        if (rPoint.X() > GetItalicRight())
            aRef.setX( GetItalicRight() );
        else if (rPoint.X() < GetItalicLeft())
            aRef.setX( GetItalicLeft() );
        else
            aRef.setX( rPoint.X() );
        // y-coordinate
        if (rPoint.Y() > GetBottom())
            aRef.setY( GetBottom() );
        else if (rPoint.Y() < GetTop())
            aRef.setY( GetTop() );
        else
            aRef.setY( rPoint.Y() );
    }

    // build distance vector
    Point  aDist (aRef - rPoint);

    tools::Long nAbsX = std::abs(aDist.X()),
         nAbsY = std::abs(aDist.Y());

    return bIsInside ? - std::min(nAbsX, nAbsY) : std::max (nAbsX, nAbsY);
}


bool SmRect::IsInsideRect(const Point &rPoint) const
{
    return     rPoint.Y() >= GetTop()
           &&  rPoint.Y() <= GetBottom()
           &&  rPoint.X() >= GetLeft()
           &&  rPoint.X() <= GetRight();
}


bool SmRect::IsInsideItalicRect(const Point &rPoint) const
{
    return     rPoint.Y() >= GetTop()
           &&  rPoint.Y() <= GetBottom()
           &&  rPoint.X() >= GetItalicLeft()
           &&  rPoint.X() <= GetItalicRight();
}

SmRect SmRect::AsGlyphRect() const
{
    SmRect aRect (*this);
    aRect.SetTop(nGlyphTop);
    aRect.SetBottom(nGlyphBottom);
    return aRect;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <format.hxx>


SmFormat::SmFormat()
:   aBaseSize(0, o3tl::convert(12, o3tl::Length::pt, SmO3tlLengthUnit()))
{
    eHorAlign       = SmHorAlign::Center;
    nGreekCharStyle = 0;
    bIsTextmode     = bIsRightToLeft = bScaleNormalBrackets = false;

    vSize[SIZ_TEXT]     = 100;
    vSize[SIZ_INDEX]    = 60;
    vSize[SIZ_FUNCTION] =
    vSize[SIZ_OPERATOR] = 100;
    vSize[SIZ_LIMITS]   = 60;

    vDist[DIS_HORIZONTAL]           = 10;
    vDist[DIS_VERTICAL]             = 5;
    vDist[DIS_ROOT]                 = 0;
    vDist[DIS_SUPERSCRIPT]          =
    vDist[DIS_SUBSCRIPT]            = 20;
    vDist[DIS_NUMERATOR]            =
    vDist[DIS_DENOMINATOR]          = 0;
    vDist[DIS_FRACTION]             = 10;
    vDist[DIS_STROKEWIDTH]          = 5;
    vDist[DIS_UPPERLIMIT]           =
    vDist[DIS_LOWERLIMIT]           = 0;
    vDist[DIS_BRACKETSIZE]          =
    vDist[DIS_BRACKETSPACE]         = 5;
    vDist[DIS_MATRIXROW]            = 3;
    vDist[DIS_MATRIXCOL]            = 30;
    vDist[DIS_ORNAMENTSIZE]         =
    vDist[DIS_ORNAMENTSPACE]        = 0;
    vDist[DIS_OPERATORSIZE]         = 50;
    vDist[DIS_OPERATORSPACE]        = 20;
    vDist[DIS_LEFTSPACE]            =
    vDist[DIS_RIGHTSPACE]           = 0;
    vDist[DIS_TOPSPACE]             =
    vDist[DIS_BOTTOMSPACE]          =
    vDist[DIS_NORMALBRACKETSIZE]    = 0;

    vFont[FNT_VARIABLE] =
    vFont[FNT_FUNCTION] =
    vFont[FNT_NUMBER]   =
    vFont[FNT_TEXT]     =
    vFont[FNT_SERIF]    = SmFace(FNTNAME_TIMES, aBaseSize);
    vFont[FNT_SANS]     = SmFace(FNTNAME_HELV,  aBaseSize);
    vFont[FNT_FIXED]    = SmFace(FNTNAME_COUR,  aBaseSize);
    vFont[FNT_MATH]     = SmFace(FNTNAME_MATH,  aBaseSize);

    vFont[FNT_MATH].SetCharSet( RTL_TEXTENCODING_UNICODE );

    vFont[FNT_VARIABLE].SetItalic(ITALIC_NORMAL);
    vFont[FNT_FUNCTION].SetItalic(ITALIC_NONE);
    vFont[FNT_NUMBER]  .SetItalic(ITALIC_NONE);
    vFont[FNT_TEXT]    .SetItalic(ITALIC_NONE);
    vFont[FNT_SERIF]   .SetItalic(ITALIC_NONE);
    vFont[FNT_SANS]    .SetItalic(ITALIC_NONE);
    vFont[FNT_FIXED]   .SetItalic(ITALIC_NONE);

    for ( sal_uInt16 i = FNT_BEGIN;  i <= FNT_END;  i++ )
    {
        SmFace &rFace = vFont[i];
        rFace.SetTransparent( true );
        rFace.SetAlignment( ALIGN_BASELINE );
        rFace.SetColor( COL_AUTO );
        bDefaultFont[i] = false;
    }
}


void SmFormat::SetFont(sal_uInt16 nIdent, const SmFace &rFont, bool bDefault )
{
    vFont[nIdent] = rFont;
    vFont[nIdent].SetTransparent( true );
    vFont[nIdent].SetAlignment( ALIGN_BASELINE );

    bDefaultFont[nIdent] = bDefault;
}

SmFormat & SmFormat::operator = (const SmFormat &rFormat)
{
    SetBaseSize(rFormat.GetBaseSize());
    SetHorAlign(rFormat.GetHorAlign());
    SetTextmode(rFormat.IsTextmode());
    SetRightToLeft(rFormat.IsRightToLeft());
    SetGreekCharStyle(rFormat.GetGreekCharStyle());
    SetScaleNormalBrackets(rFormat.IsScaleNormalBrackets());

    sal_uInt16  i;
    for (i = FNT_BEGIN;  i <= FNT_END;  i++)
    {
        SetFont(i, rFormat.GetFont(i));
        SetDefaultFont(i, rFormat.IsDefaultFont(i));
    }
    for (i = SIZ_BEGIN;  i <= SIZ_END;  i++)
        SetRelSize(i, rFormat.GetRelSize(i));
    for (i = DIS_BEGIN;  i <= DIS_END;  i++)
        SetDistance(i, rFormat.GetDistance(i));

    return *this;
}


bool SmFormat::operator == (const SmFormat &rFormat) const
{
    bool bRes = aBaseSize == rFormat.aBaseSize  &&
                eHorAlign == rFormat.eHorAlign  &&
                nGreekCharStyle == rFormat.nGreekCharStyle &&
                bIsTextmode == rFormat.bIsTextmode  &&
                bIsRightToLeft == rFormat.bIsRightToLeft &&
                bScaleNormalBrackets  == rFormat.bScaleNormalBrackets;

    sal_uInt16 i;
    for (i = 0;  i <= SIZ_END && bRes;  ++i)
    {
        if (vSize[i] != rFormat.vSize[i])
            bRes = false;
    }
    for (i = 0;  i <= DIS_END && bRes;  ++i)
    {
        if (vDist[i] != rFormat.vDist[i])
            bRes = false;
    }
    for (i = 0;  i <= FNT_END && bRes;  ++i)
    {
        if (vFont[i] != rFormat.vFont[i]  ||
            bDefaultFont[i] != rFormat.bDefaultFont[i])
            bRes = false;
    }

    return bRes;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

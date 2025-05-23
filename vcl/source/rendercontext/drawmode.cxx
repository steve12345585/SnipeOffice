/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <vcl/bitmap/BitmapMonochromeFilter.hxx>
#include <vcl/bitmap/BitmapTypes.hxx>
#include <vcl/rendercontext/DrawModeFlags.hxx>
#include <vcl/settings.hxx>
#include <vcl/virdev.hxx>

#include <drawmode.hxx>

namespace vcl::drawmode
{
Color GetLineColor(Color const& rColor, DrawModeFlags nDrawMode,
                   StyleSettings const& rStyleSettings)
{
    Color aColor(rColor);

    if (nDrawMode
        & (DrawModeFlags::BlackLine | DrawModeFlags::WhiteLine | DrawModeFlags::GrayLine
           | DrawModeFlags::SettingsLine))
    {
        if (!aColor.IsTransparent())
        {
            if (nDrawMode & DrawModeFlags::BlackLine)
            {
                aColor = COL_BLACK;
            }
            else if (nDrawMode & DrawModeFlags::WhiteLine)
            {
                aColor = COL_WHITE;
            }
            else if (nDrawMode & DrawModeFlags::GrayLine)
            {
                const sal_uInt8 cLum = aColor.GetLuminance();
                aColor = Color(cLum, cLum, cLum);
            }
            else if (nDrawMode & DrawModeFlags::SettingsLine)
            {
                if (nDrawMode & DrawModeFlags::SettingsForSelection)
                    aColor = rStyleSettings.GetHighlightColor();
                else
                    aColor = rStyleSettings.GetWindowTextColor();
            }
        }
    }

    return aColor;
}

Color GetFillColor(Color const& rColor, DrawModeFlags nDrawMode,
                   StyleSettings const& rStyleSettings)
{
    Color aColor(rColor);

    if (nDrawMode
        & (DrawModeFlags::BlackFill | DrawModeFlags::WhiteFill | DrawModeFlags::GrayFill
           | DrawModeFlags::NoFill | DrawModeFlags::SettingsFill))
    {
        if (!aColor.IsTransparent())
        {
            if (nDrawMode & DrawModeFlags::BlackFill)
            {
                aColor = COL_BLACK;
            }
            else if (nDrawMode & DrawModeFlags::WhiteFill)
            {
                aColor = COL_WHITE;
            }
            else if (nDrawMode & DrawModeFlags::GrayFill)
            {
                const sal_uInt8 cLum = aColor.GetLuminance();
                aColor = Color(cLum, cLum, cLum);
            }
            else if (nDrawMode & DrawModeFlags::NoFill)
            {
                aColor = COL_TRANSPARENT;
            }
            else if (nDrawMode & DrawModeFlags::SettingsFill)
            {
                if (nDrawMode & DrawModeFlags::SettingsForSelection)
                    aColor = rStyleSettings.GetHighlightColor();
                else
                    aColor = rStyleSettings.GetWindowColor();
            }
        }
    }

    return aColor;
}

Color GetHatchColor(Color const& rColor, DrawModeFlags nDrawMode,
                    StyleSettings const& rStyleSettings)
{
    Color aColor(rColor);

    if (nDrawMode & DrawModeFlags::BlackLine)
    {
        aColor = COL_BLACK;
    }
    else if (nDrawMode & DrawModeFlags::WhiteLine)
    {
        aColor = COL_WHITE;
    }
    else if (nDrawMode & DrawModeFlags::GrayLine)
    {
        const sal_uInt8 cLum = aColor.GetLuminance();
        aColor = Color(cLum, cLum, cLum);
    }
    else if (nDrawMode & DrawModeFlags::SettingsLine)
    {
        if (nDrawMode & DrawModeFlags::SettingsForSelection)
            aColor = rStyleSettings.GetHighlightColor();
        else
            aColor = rStyleSettings.GetWindowTextColor();
    }

    return aColor;
}

Color GetTextColor(Color const& rColor, DrawModeFlags nDrawMode,
                   StyleSettings const& rStyleSettings)
{
    Color aColor(rColor);

    if (nDrawMode
        & (DrawModeFlags::BlackText | DrawModeFlags::WhiteText | DrawModeFlags::GrayText
           | DrawModeFlags::SettingsText))
    {
        if (nDrawMode & DrawModeFlags::BlackText)
        {
            aColor = COL_BLACK;
        }
        else if (nDrawMode & DrawModeFlags::WhiteText)
        {
            aColor = COL_WHITE;
        }
        else if (nDrawMode & DrawModeFlags::GrayText)
        {
            const sal_uInt8 cLum = aColor.GetLuminance();
            aColor = Color(cLum, cLum, cLum);
        }
        else if (nDrawMode & DrawModeFlags::SettingsText)
        {
            if (nDrawMode & DrawModeFlags::SettingsForSelection)
                aColor = rStyleSettings.GetHighlightTextColor();
            else
                aColor = rStyleSettings.GetWindowTextColor();
        }
    }

    return aColor;
}

vcl::Font GetFont(vcl::Font const& rFont, DrawModeFlags nDrawMode,
                  StyleSettings const& rStyleSettings)
{
    vcl::Font aFont(rFont);

    if (nDrawMode
        & (DrawModeFlags::BlackText | DrawModeFlags::WhiteText | DrawModeFlags::GrayText
           | DrawModeFlags::SettingsText | DrawModeFlags::BlackFill | DrawModeFlags::WhiteFill
           | DrawModeFlags::GrayFill | DrawModeFlags::NoFill | DrawModeFlags::SettingsFill))
    {
        Color aTextColor(aFont.GetColor());

        if (nDrawMode & DrawModeFlags::BlackText)
        {
            aTextColor = COL_BLACK;
        }
        else if (nDrawMode & DrawModeFlags::WhiteText)
        {
            aTextColor = COL_WHITE;
        }
        else if (nDrawMode & DrawModeFlags::GrayText)
        {
            const sal_uInt8 cLum = aTextColor.GetLuminance();
            aTextColor = Color(cLum, cLum, cLum);
        }
        else if (nDrawMode & DrawModeFlags::SettingsText)
        {
            if (nDrawMode & DrawModeFlags::SettingsForSelection)
                aTextColor = rStyleSettings.GetHighlightTextColor();
            else
                aTextColor = rStyleSettings.GetWindowTextColor();
        }

        aFont.SetColor(aTextColor);

        if (!aFont.IsTransparent())
        {
            Color aTextFillColor(aFont.GetFillColor());

            if (nDrawMode & DrawModeFlags::BlackFill)
            {
                aTextFillColor = COL_BLACK;
            }
            else if (nDrawMode & DrawModeFlags::WhiteFill)
            {
                aTextFillColor = COL_WHITE;
            }
            else if (nDrawMode & DrawModeFlags::GrayFill)
            {
                const sal_uInt8 cLum = aTextFillColor.GetLuminance();
                aTextFillColor = Color(cLum, cLum, cLum);
            }
            else if (nDrawMode & DrawModeFlags::SettingsFill)
            {
                if (nDrawMode & DrawModeFlags::SettingsForSelection)
                    aTextFillColor = rStyleSettings.GetHighlightColor();
                else
                    aTextFillColor = rStyleSettings.GetWindowColor();
            }
            else if (nDrawMode & DrawModeFlags::NoFill)
            {
                aTextFillColor = COL_TRANSPARENT;
            }

            aFont.SetFillColor(aTextFillColor);
        }
    }

    return aFont;
}

BitmapEx GetBitmapEx(BitmapEx const& rBitmapEx, DrawModeFlags nDrawMode)
{
    BitmapEx aBmpEx(rBitmapEx);

    if (nDrawMode & (DrawModeFlags::BlackBitmap | DrawModeFlags::WhiteBitmap))
    {
        Bitmap aColorBmp(aBmpEx.GetSizePixel(), vcl::PixelFormat::N8_BPP,
                         &Bitmap::GetGreyPalette(256));
        sal_uInt8 cCmpVal;

        if (nDrawMode & DrawModeFlags::BlackBitmap)
            cCmpVal = 0;
        else
            cCmpVal = 255;

        aColorBmp.Erase(Color(cCmpVal, cCmpVal, cCmpVal));

        if (aBmpEx.IsAlpha())
        {
            // Create one-bit mask out of alpha channel, by thresholding it at alpha=0.5. As
            // DRAWMODE_BLACK/WHITEBITMAP requires monochrome output, having alpha-induced
            // grey levels is not acceptable
            BitmapEx aMaskEx(aBmpEx.GetAlphaMask().GetBitmap());
            aMaskEx.Invert(); // convert to transparency
            BitmapFilter::Filter(aMaskEx, BitmapMonochromeFilter(129));
            aMaskEx.Invert(); // convert to alpha
            aBmpEx = BitmapEx(aColorBmp, aMaskEx.GetBitmap());
        }
        else
        {
            aBmpEx = BitmapEx(aColorBmp);
        }
    }

    if (nDrawMode & DrawModeFlags::GrayBitmap && !aBmpEx.IsEmpty())
        aBmpEx.Convert(BmpConversion::N8BitGreys);

    return aBmpEx;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

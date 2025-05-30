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

#include <sal/config.h>

#include <rtl/ustring.hxx>
#include <tools/color.hxx>
#include <tools/fontenum.hxx>
#include <tools/gen.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <vcl/fntstyle.hxx>
#include <vcl/font.hxx>

/* The following class is extraordinarily similar to FontAttributes. */

class ImplFont
{
public:
    explicit            ImplFont();
    explicit            ImplFont( const ImplFont& );

    // device independent font functions
    const OUString&     GetFamilyName() const                           { return maFamilyName; }
    FontFamily          GetFamilyType()                                 { if(meFamily==FAMILY_DONTKNOW)  AskConfig(); return meFamily; }
    const OUString&     GetStyleName() const                            { return maStyleName; }

    FontWeight          GetWeight()                                     { if(meWeight==WEIGHT_DONTKNOW)  AskConfig(); return meWeight; }
    FontItalic          GetItalic()                                     { if(meItalic==ITALIC_DONTKNOW)  AskConfig(); return meItalic; }
    FontPitch           GetPitch()                                      { if(mePitch==PITCH_DONTKNOW)    AskConfig(); return mePitch; }
    FontWidth           GetWidthType()                                  { if(meWidthType==WIDTH_DONTKNOW) AskConfig(); return meWidthType; }
    TextAlign           GetAlignment() const                            { return meAlign; }
    rtl_TextEncoding    GetCharSet() const                              { return meCharSet; }
    const Size&         GetFontSize() const                      { return maAverageFontSize; }

    void                SetFamilyName( const OUString& sFamilyName )    { maFamilyName = sFamilyName; }
    void                SetStyleName( const OUString& sStyleName )      { maStyleName = sStyleName; }
    void                SetFamilyType( const FontFamily eFontFamily )   { meFamily = eFontFamily; }

    void                SetPitch( const FontPitch ePitch )              { mePitch = ePitch; }
    void                SetItalic( const FontItalic eItalic )           { meItalic = eItalic; }
    void                SetWeight( const FontWeight eWeight )           { meWeight = eWeight; }
    void                SetWidthType( const FontWidth eWidthType )      { meWidthType = eWidthType; }
    void                SetAlignment( const TextAlign eAlignment )      { meAlign = eAlignment; }
    void                SetCharSet( const rtl_TextEncoding eCharSet )   { meCharSet = eCharSet; }
    void                SetFontSize( const Size& rSize )
    {
        if(rSize.Height() != maAverageFontSize.Height())
        {
            // reset evtl. buffered calculated AverageFontSize, it depends
            // on Font::Height
            mnCalculatedAverageFontWidth = 0;
        }
        maAverageFontSize = rSize;
    }

    // straight properties, no getting them from AskConfig()
    FontFamily          GetFamilyTypeNoAsk() const                      { return meFamily; }
    FontWeight          GetWeightNoAsk() const                          { return meWeight; }
    FontItalic          GetItalicNoAsk() const                          { return meItalic; }
    FontPitch           GetPitchNoAsk() const                           { return mePitch; }
    FontWidth           GetWidthTypeNoAsk() const                       { return meWidthType; }

    // device dependent functions
    int                 GetQuality() const                              { return mnQuality; }

    void                SetQuality( int nQuality )                      { mnQuality = nQuality; }
    void                IncreaseQualityBy( int nQualityAmount )         { mnQuality += nQualityAmount; }
    void                DecreaseQualityBy( int nQualityAmount )         { mnQuality -= nQualityAmount; }

    tools::Long         GetCalculatedAverageFontWidth() const           { return mnCalculatedAverageFontWidth; }
    void                SetCalculatedAverageFontWidth(tools::Long nNew) { mnCalculatedAverageFontWidth = nNew; }

    bool                operator==( const ImplFont& ) const;
    bool                EqualIgnoreColor( const ImplFont& ) const;

    size_t              GetHashValue() const;
    size_t              GetHashValueIgnoreColor() const;

private:
    friend class vcl::Font;
    friend SvStream&    ReadImplFont( SvStream& rIStm, ImplFont&, tools::Long& );
    friend SvStream&    WriteImplFont( SvStream& rOStm, const ImplFont&, tools::Long );

    void                AskConfig();

    // Device independent variables
    OUString            maFamilyName;
    OUString            maStyleName;
    FontWeight          meWeight;
    FontFamily          meFamily;
    FontPitch           mePitch;
    FontWidth           meWidthType;
    FontItalic          meItalic;
    TextAlign           meAlign;
    FontLineStyle       meUnderline;
    FontLineStyle       meOverline;
    FontStrikeout       meStrikeout;
    FontRelief          meRelief;
    FontEmphasisMark    meEmphasisMark;
    FontKerning         meKerning;
    short               mnSpacing;
    Size                maAverageFontSize;
    rtl_TextEncoding    meCharSet;

    LanguageTag         maLanguageTag;
    LanguageTag         maCJKLanguageTag;

    // Flags - device independent
    bool                mbOutline:1,
                        mbConfigLookup:1,   // config lookup should only be done once
                        mbShadow:1,
                        mbVertical:1,
                        mbTransparent:1;    // compatibility, now on output device

    // deprecated variables - device independent
    Color               maColor;            // compatibility, now on output device
    Color               maFillColor;        // compatibility, now on output device

    // Device dependent variables
    bool                mbWordLine:1;

    // TODO: metric data, should be migrated to ImplFontMetric
    Degree10            mnOrientation;

    int                 mnQuality;

    tools::Long         mnCalculatedAverageFontWidth;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

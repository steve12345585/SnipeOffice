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

#ifndef INCLUDED_VCL_FONT_HXX
#define INCLUDED_VCL_FONT_HXX

#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <vcl/dllapi.h>
#include <tools/color.hxx>
#include <tools/fontenum.hxx>
#include <tools/long.hxx>
#include <tools/degree.hxx>
#include <i18nlangtag/lang.h>
#include <vcl/fntstyle.hxx>
#include <o3tl/cow_wrapper.hxx>

class Size;
class LanguageTag;
class SvStream;

class ImplFont;
class FontAttributes;
namespace vcl { class Font; }
// need to first declare these outside the vcl namespace, or the friend declarations won't work right
VCL_DLLPUBLIC SvStream&  ReadFont( SvStream& rIStm, vcl::Font& );
VCL_DLLPUBLIC SvStream&  WriteFont( SvStream& rOStm, const vcl::Font& );

namespace vcl {

class SAL_WARN_UNUSED VCL_DLLPUBLIC Font
{
public:
    explicit            Font();
                        Font( const Font& ); // TODO make me explicit
                        Font( Font&& ) noexcept;
    explicit            Font( const OUString& rFamilyName, const Size& );
    explicit            Font( const OUString& rFamilyName, const OUString& rStyleName, const Size& );
    explicit            Font( FontFamily eFamily, const Size& );
    virtual             ~Font();

    const OUString&     GetFamilyName() const;
    FontFamily          GetFamilyTypeMaybeAskConfig();
    FontFamily          GetFamilyType() const;
    const OUString&     GetStyleName() const;

    FontWeight          GetWeightMaybeAskConfig();
    FontWeight          GetWeight() const;
    FontItalic          GetItalicMaybeAskConfig();
    FontItalic          GetItalic() const;
    FontPitch           GetPitchMaybeAskConfig();
    FontPitch           GetPitch() const;
    FontWidth           GetWidthTypeMaybeAskConfig();
    FontWidth           GetWidthType() const;
    TextAlign           GetAlignment() const;
    rtl_TextEncoding    GetCharSet() const;
    FontEmphasisMark    GetEmphasisMarkStyle() const;

    void                SetFamilyName( const OUString& rFamilyName );
    void                SetStyleName( const OUString& rStyleName );
    void                SetFamily( FontFamily );

    void                SetPitch( FontPitch ePitch );
    void                SetItalic( FontItalic );
    void                SetWeight( FontWeight );
    void                SetWidthType( FontWidth );
    void                SetAlignment( TextAlign );
    void                SetCharSet( rtl_TextEncoding );

    // Device dependent functions
    int                 GetQuality() const;

    void                SetQuality(int);
    void                IncreaseQualityBy(int);
    void                DecreaseQualityBy(int);

    // setting the color on the font is obsolete, the only remaining
    // valid use is for keeping backward compatibility with old MetaFiles
    const Color&        GetColor() const;
    const Color&        GetFillColor() const;

    bool                IsTransparent() const;

    void                SetColor( const Color& );
    void                SetFillColor( const Color& );

    void                SetTransparent( bool bTransparent );

    void                SetFontSize( const Size& );
    const Size&         GetFontSize() const;
    void                SetFontHeight( tools::Long nHeight );
    tools::Long                GetFontHeight() const;
    void                SetAverageFontWidth( tools::Long nWidth );
    tools::Long                GetAverageFontWidth() const;
    SAL_DLLPRIVATE const Size& GetAverageFontSize() const;
    SAL_DLLPRIVATE const FontFamily& GetFontFamily() const;

    // tdf#127471 for corrections on EMF/WMF we need the AvgFontWidth in Windows-specific notation
    tools::Long         GetOrCalculateAverageFontWidth() const;

    // Prefer LanguageTag over LanguageType
    SAL_DLLPRIVATE void SetLanguageTag( const LanguageTag & );
    SAL_DLLPRIVATE const LanguageTag& GetLanguageTag() const;
    SAL_DLLPRIVATE void SetCJKContextLanguageTag( const LanguageTag& );
    SAL_DLLPRIVATE const LanguageTag& GetCJKContextLanguageTag() const;
    void                SetLanguage( LanguageType );
    LanguageType        GetLanguage() const;
    void                SetCJKContextLanguage( LanguageType );
    LanguageType        GetCJKContextLanguage() const;

    void                SetOrientation( Degree10 nLineOrientation );
    Degree10            GetOrientation() const;
    void                SetVertical( bool bVertical );
    bool                IsVertical() const;
    void                SetKerning( FontKerning nKerning );
    FontKerning         GetKerning() const;
    bool                IsKerning() const;
    void                SetFixKerning(const short nSpacing);
    short               GetFixKerning() const;
    bool                IsFixKerning() const;

    void                SetOutline( bool bOutline );
    bool                IsOutline() const;
    void                SetShadow( bool bShadow );
    bool                IsShadow() const;
    void                SetRelief( FontRelief );
    FontRelief          GetRelief() const;
    void                SetUnderline( FontLineStyle );
    FontLineStyle       GetUnderline() const;
    void                SetOverline( FontLineStyle );
    FontLineStyle       GetOverline() const;
    void                SetStrikeout( FontStrikeout );
    FontStrikeout       GetStrikeout() const;
    void                SetEmphasisMark( FontEmphasisMark );
    FontEmphasisMark    GetEmphasisMark() const;
    void                SetWordLineMode( bool bWordLine );
    bool                IsWordLineMode() const;

    void                Merge( const Font& rFont );
    SAL_DLLPRIVATE void GetFontAttributes( FontAttributes& rAttrs ) const;

    Font&               operator=( const Font& );
    Font&               operator=( Font&& ) noexcept;
    bool                operator==( const Font& ) const;
    bool                operator!=( const Font& rFont ) const
                            { return !(Font::operator==( rFont )); }
    bool                IsSameInstance( const Font& ) const;
    SAL_DLLPRIVATE bool EqualIgnoreColor( const Font& ) const;

    // Compute value usable as hash.
    SAL_DLLPRIVATE size_t GetHashValueIgnoreColor() const;

    friend VCL_DLLPUBLIC SvStream&  ::ReadFont( SvStream& rIStm, vcl::Font& );
    friend VCL_DLLPUBLIC SvStream&  ::WriteFont( SvStream& rOStm, const vcl::Font& );

    static Font identifyFont( const void* pBuffer, sal_uInt32 nLen );

    typedef o3tl::cow_wrapper< ImplFont > ImplType;

    inline bool IsUnderlineAbove() const;

private:
    ImplType mpImplFont;
};

inline bool Font::IsUnderlineAbove() const
{
    if (!IsVertical())
        return false;
    // the underline is right for Japanese only
    return (LANGUAGE_JAPANESE == GetLanguage()) ||
           (LANGUAGE_JAPANESE == GetCJKContextLanguage());
}

}

#endif  // _VCL_FONT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#ifndef INCLUDED_TOOLS_COLOR_HXX
#define INCLUDED_TOOLS_COLOR_HXX

#include <sal/types.h>
#include <tools/toolsdllapi.h>
#include <com/sun/star/uno/Any.hxx>
#include <config_global.h>
#include <basegfx/color/bcolor.hxx>
#include <osl/endian.h>

namespace color
{

constexpr sal_uInt32 extractRGB(sal_uInt32 nColorNumber)
{
    return nColorNumber & 0x00FFFFFF;
}

constexpr sal_uInt8 ColorChannelMerge(sal_uInt8 nDst, sal_uInt8 nSrc, sal_uInt8 nSrcTrans)
{
    return sal_uInt8(((sal_Int32(nDst) - nSrc) * nSrcTrans + ((nSrc << 8) | nDst)) >> 8);
}

}

/** used to deliberately select the right constructor */
enum ColorTransparencyTag { ColorTransparency = 0 };
enum ColorAlphaTag { ColorAlpha = 0 };

// Color

class SAL_WARN_UNUSED TOOLS_DLLPUBLIC Color
{
    union
    {
        sal_uInt32 mValue;
        struct
        {
#ifdef OSL_BIGENDIAN
                sal_uInt8 T;
                sal_uInt8 R;
                sal_uInt8 G;
                sal_uInt8 B;
#else
                sal_uInt8 B;
                sal_uInt8 G;
                sal_uInt8 R;
                sal_uInt8 T;
#endif
        };
    };

public:
    constexpr Color()
        : mValue(0) // black
    {}

#if HAVE_CPP_CONSTEVAL
    consteval
#else
    constexpr
#endif
    Color(const sal_uInt32 nColor)
        : mValue(nColor)
    {
        assert(nColor <= 0xffffff && "don't pass transparency to this constructor, use the Color(ColorTransparencyTag,...) or Color(ColorAlphaTag,...) constructor to make it explicit");
    }

    constexpr Color(enum ColorTransparencyTag, sal_uInt32 nColor)
        : mValue(nColor)
    {
    }

    constexpr Color(enum ColorAlphaTag, sal_uInt32 nColor)
        : mValue((nColor & 0xffffff) | ((255 - (nColor >> 24)) << 24))
    {
    }

    constexpr Color(enum ColorTransparencyTag, sal_uInt8 nTransparency, sal_uInt8 nRed, sal_uInt8 nGreen, sal_uInt8 nBlue)
        : mValue(sal_uInt32(nBlue) | (sal_uInt32(nGreen) << 8) | (sal_uInt32(nRed) << 16) | (sal_uInt32(nTransparency) << 24))
    {}

    constexpr Color(enum ColorAlphaTag, sal_uInt8 nAlpha, sal_uInt8 nRed, sal_uInt8 nGreen, sal_uInt8 nBlue)
        : Color(ColorTransparency, 255 - nAlpha, nRed, nGreen, nBlue)
    {}

    constexpr Color(sal_uInt8 nRed, sal_uInt8 nGreen, sal_uInt8 nBlue)
        : Color(ColorTransparency, 0, nRed, nGreen, nBlue)
    {}

    // constructor to create a tools-Color from ::basegfx::BColor
    explicit Color(const basegfx::BColor& rBColor)
        : Color(ColorTransparency, 0,
                sal_uInt8(std::lround(rBColor.getRed() * 255.0)),
                sal_uInt8(std::lround(rBColor.getGreen() * 255.0)),
                sal_uInt8(std::lround(rBColor.getBlue() * 255.0)))
    {}

    /** Casts the color to corresponding uInt32.
      * Primarily used when passing Color objects to UNO API
      * @return corresponding sal_uInt32
      */
    constexpr explicit operator sal_uInt32() const
    {
        return mValue;
    }

    /** Casts the color to corresponding iInt32.
      * If there is no transparency, will be positive.
      * @return corresponding sal_Int32
      */
    constexpr explicit operator sal_Int32() const
    {
        return sal_Int32(mValue);
    }

    /* Basic RGBA operations */

    /** Gets the red value.
      * @return R
      */
    sal_uInt8 GetRed() const
    {
        return R;
    }

    /** Gets the green value.
      * @return G
      */
    sal_uInt8 GetGreen() const
    {
        return G;
    }

    /** Gets the blue value.
      * @return B
      */
    sal_uInt8 GetBlue() const
    {
        return B;
    }

    /** Gets the alpha value.
      * @return A
      */
    sal_uInt8 GetAlpha() const
    {
        return 255 - T;
    }

    /** Is the color transparent?
     */
    bool IsTransparent() const
    {
        return GetAlpha() != 255;
    }

    /** Is the color fully transparent i.e. 100% transparency ?
     */
    bool IsFullyTransparent() const
    {
        return GetAlpha() == 0;
    }

    /** Sets the red value.
      * @param nRed
      */
    void SetRed(sal_uInt8 nRed)
    {
        R = nRed;
    }

    /** Sets the green value.
      * @param nGreen
      */
    void SetGreen(sal_uInt8 nGreen)
    {
        G = nGreen;
    }

    /** Sets the blue value.
      * @param nBlue
      */
    void SetBlue(sal_uInt8 nBlue)
    {
        B = nBlue;
    }

    /** Sets the alpha value.
      * @param nAlpha
      */
    void SetAlpha(sal_uInt8 nAlpha)
    {
        T = 255 - nAlpha;
    }

    /** Returns the same color but ignoring the transparency value.
      * @return RGB version
      */
    Color GetRGBColor() const
    {
        return { GetRed(), GetGreen(), GetBlue() };
    }

    /* Comparison and operators */

    /** Check if the color RGB value is equal than rColor.
      * @param rColor
      * @return is equal
      */
    bool IsRGBEqual( const Color& rColor ) const
    {
        return ( mValue & 0x00FFFFFF ) == ( rColor.mValue & 0x00FFFFFF );
    }

    /** Check if the color value is lower than aCompareColor.
      * @param aCompareColor
      * @return is lower
      */
    bool operator<(const Color& aCompareColor) const
    {
        return mValue < aCompareColor.mValue;
    }

    /** Check if the color value is greater than aCompareColor.
      * @param aCompareColor
      * @return is greater
      */
    bool operator>(const Color& aCompareColor) const
    {
        return mValue > aCompareColor.mValue;
    }

    /** Check if the color value is equal than rColor.
      * @param rColor
      * @return is equal
      */
    bool operator==(const Color& rColor) const
    {
        return mValue == rColor.mValue;
    }

    /** Gets the color error compared to another.
      * It describes how different they are.
      * It takes the abs of differences in parameters.
      * @param rCompareColor
      * @return error
      */
    sal_uInt16 GetColorError(const Color& rCompareColor) const
    {
    return static_cast<sal_uInt16>(
        abs(static_cast<int>(GetBlue()) - rCompareColor.GetBlue()) +
        abs(static_cast<int>(GetGreen()) - rCompareColor.GetGreen()) +
        abs(static_cast<int>(GetRed()) - rCompareColor.GetRed()));
    }

    /* Light and contrast */

    /** Gets the color luminance. It means perceived brightness.
      * @return luminance
      */
    sal_uInt8 GetLuminance() const
    {
        return sal_uInt8((GetBlue() * 29UL + GetGreen() * 151UL + GetRed() * 76UL) >> 8);
    }

    /** Gets the color luminance following WCAG 2.1.
      * @return luminance
      */
    sal_uInt8 GetWCAGLuminance() const;

    /** Increases the color luminance by cLumInc.
      * @param cLumInc
      */
    void IncreaseLuminance(sal_uInt8 cLumInc);

    /** Decreases the color luminance by cLumDec.
      * @param cLumDec
      */
    void DecreaseLuminance(sal_uInt8 cLumDec);

    /** Decreases color contrast with white by cContDec.
      * @param cContDec
      */
    void DecreaseContrast(sal_uInt8 cContDec);

    /** Comparison with luminance thresholds.
      * @return is dark
      */
    bool IsDark() const;

    /** Comparison with luminance thresholds.
      * @return is dark
      */
    bool IsBright() const;

    /* Color filters */

    /**
     * Apply tint or shade to a color.
     *
     * The input value is the percentage (in 100th of percent) of how much the
     * color changes towards the black (shade) or white (tint). If the value
     * is positive, the color is tinted, if the value is negative, the color is
     * shaded.
     **/
    void ApplyTintOrShade(sal_Int16 n100thPercent);

    /**
     * Apply luminance offset and/or modulation.
     *
     * The input values are in percentages (in 100th percents). 100% modulation and 0% offset
     * results in no change.
     */
    void ApplyLumModOff(sal_Int16 nMod, sal_Int16 nOff);

    /** Inverts color. 1 and 0 are switched.
      * Note that the result will be the complementary color.
      * For example, if you have red, you will get cyan: FF0000 -> 00FFFF.
      */
    void Invert()
    {
        SetRed(~GetRed());
        SetGreen(~GetGreen());
        SetBlue(~GetBlue());
    }

    /** Merges color with rMergeColor.
      * Allows to get resulting color when superposing another.
      * @param rMergeColor
      * @param cTransparency
      */
    void Merge(const Color& rMergeColor, sal_uInt8 cTransparency)
    {
        SetRed(color::ColorChannelMerge(GetRed(), rMergeColor.GetRed(), cTransparency));
        SetGreen(color::ColorChannelMerge(GetGreen(), rMergeColor.GetGreen(), cTransparency));
        SetBlue(color::ColorChannelMerge(GetBlue(), rMergeColor.GetBlue(), cTransparency));
    }

    /* Change of format */

    /** Color space conversion tools
      * The range for h/s/b is:
      *   - Hue: 0-360 degree
      *   - Saturation: 0-100%
      *   - Brightness: 0-100%
      * @param nHue
      * @param nSaturation
      * @param nBrightness
      * @return rgb color
      */
    static Color HSBtoRGB(sal_uInt16 nHue, sal_uInt16 nSaturation, sal_uInt16 nBrightness);

    /** Converts a string into a color. Supports:
      * #RRGGBB
      * #rrggbb
      * #RGB
      * #rgb
      * RRGGBB
      * rrggbb
      * RGB
      * rgb
      * If fails returns Color().
      */
    static Color STRtoRGB(std::u16string_view colorname);

    /** Color space conversion tools
      * @param nHue
      * @param nSaturation
      * @param nBrightness
      */
    void RGBtoHSB(sal_uInt16& nHue, sal_uInt16& nSaturation, sal_uInt16& nBrightness) const;

    /* Return color as RGB hex string: rrggbb
     * for example "00ff00" for green color
     * @return hex string
     */
    OUString AsRGBHexString() const;

    /* Return color as RGB hex string: RRGGBB
     * for example "00FF00" for green color
     * @return hex string
     */
    OUString AsRGBHEXString() const;

    /* get ::basegfx::BColor from this color
     * @return basegfx color
     */
    basegfx::BColor getBColor() const
    {
        basegfx::BColor aColor(GetRed() / 255.0, GetGreen() / 255.0, GetBlue() / 255.0);
        if (mValue == Color(ColorTransparency, 0xFF, 0xFF, 0xFF, 0xFF).mValue)
            aColor.setAutomatic(true);
        return aColor;
    }
};

// to reduce the noise when moving these into and out of Any
inline bool operator >>=( const css::uno::Any & rAny, Color & value )
{
  sal_Int32 nTmp = {}; // spurious -Werror=maybe-uninitialized
  if (!(rAny >>= nTmp))
      return false;
  value = Color(ColorTransparency, nTmp);
  return true;
}

inline void operator <<=( css::uno::Any & rAny, Color value )
{
    rAny <<= sal_Int32(value);
}

namespace com::sun::star::uno {
    template<> inline Any::Any(Color const & value): Any(sal_Int32(value)) {}
}

// Test compile time conversion of Color to sal_uInt32

static_assert (sal_uInt32(Color(ColorTransparency, 0x00, 0x12, 0x34, 0x56)) == 0x00123456);
static_assert (sal_uInt32(Color(0x12, 0x34, 0x56)) == 0x00123456);

// Color types

inline constexpr ::Color COL_TRANSPARENT             ( ColorTransparency, 0xFF, 0xFF, 0xFF, 0xFF );
inline constexpr ::Color COL_AUTO                    ( ColorTransparency, 0xFF, 0xFF, 0xFF, 0xFF );
// These are used when drawing to the separate alpha channel we use in vcl
inline constexpr ::Color COL_ALPHA_TRANSPARENT       ( 0x00, 0x00, 0x00 );
inline constexpr ::Color COL_ALPHA_OPAQUE            ( 0xff, 0xff, 0xff );

inline constexpr ::Color COL_BLACK                   ( 0x00, 0x00, 0x00 );
inline constexpr ::Color COL_BLUE                    ( 0x00, 0x00, 0x80 );
inline constexpr ::Color COL_GREEN                   ( 0x00, 0x80, 0x00 );
inline constexpr ::Color COL_CYAN                    ( 0x00, 0x80, 0x80 );
inline constexpr ::Color COL_RED                     ( 0x80, 0x00, 0x00 );
inline constexpr ::Color COL_MAGENTA                 ( 0x80, 0x00, 0x80 );
inline constexpr ::Color COL_BROWN                   ( 0x80, 0x80, 0x00 );
inline constexpr ::Color COL_GRAY                    ( 0x80, 0x80, 0x80 );
inline constexpr ::Color COL_GRAY3                   ( 0xCC, 0xCC, 0xCC );
inline constexpr ::Color COL_GRAY7                   ( 0x66, 0x66, 0x66 );
inline constexpr ::Color COL_LIGHTGRAY               ( 0xC0, 0xC0, 0xC0 );
inline constexpr ::Color COL_LIGHTBLUE               ( 0x00, 0x00, 0xFF );
inline constexpr ::Color COL_LIGHTGREEN              ( 0x00, 0xFF, 0x00 );
inline constexpr ::Color COL_LIGHTCYAN               ( 0x00, 0xFF, 0xFF );
inline constexpr ::Color COL_LIGHTRED                ( 0xFF, 0x00, 0x00 );
inline constexpr ::Color COL_LIGHTMAGENTA            ( 0xFF, 0x00, 0xFF );
inline constexpr ::Color COL_LIGHTGRAYBLUE           ( 0xE0, 0xE0, 0xFF );
inline constexpr ::Color COL_YELLOW                  ( 0xFF, 0xFF, 0x00 );
inline constexpr ::Color COL_WHITE                   ( 0xFF, 0xFF, 0xFF );
inline constexpr ::Color COL_AUTHOR_TABLE_INS        ( 0xE1, 0xF2, 0xFA );
inline constexpr ::Color COL_AUTHOR_TABLE_DEL        ( 0xFC, 0xE6, 0xF4 );

template<typename charT, typename traits>
inline std::basic_ostream<charT, traits>& operator <<(std::basic_ostream<charT, traits>& rStream, const Color& rColor)
{
    std::ios_base::fmtflags nOrigFlags = rStream.flags();
    rStream << "rgba[" << std::hex << std::setfill ('0')
            << std::setw(2) << static_cast<int>(rColor.GetRed())
            << std::setw(2) << static_cast<int>(rColor.GetGreen())
            << std::setw(2) << static_cast<int>(rColor.GetBlue())
            << std::setw(2) << static_cast<int>(rColor.GetAlpha()) << "]";
    rStream.setf(nOrigFlags);
    return rStream;
}

namespace color
{
TOOLS_DLLPUBLIC bool createFromString(OString const& rString, Color& rColor);
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

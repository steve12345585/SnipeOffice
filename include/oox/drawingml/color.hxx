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

#ifndef INCLUDED_OOX_DRAWINGML_COLOR_HXX
#define INCLUDED_OOX_DRAWINGML_COLOR_HXX

#include <vector>

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <oox/helper/helper.hxx>
#include <oox/dllapi.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <tools/color.hxx>
#include <docmodel/theme/ThemeColorType.hxx>
#include <docmodel/color/ComplexColor.hxx>

namespace oox { class GraphicHelper; }

namespace oox::drawingml
{

model::ThemeColorType schemeTokenToThemeColorType(sal_uInt32 nToken);
model::ThemeColorType schemeNameToThemeColorType(OUString const& rSchemeName);

class OOX_DLLPUBLIC Color
{
public:
    Color();

    /** Returns the RGB value for the passed DrawingML color token, or nDefaultRgb on error. */
    static ::Color      getDmlPresetColor( sal_Int32 nToken, ::Color nDefaultRgb );
    /** Returns the RGB value for the passed VML color token, or nDefaultRgb on error. */
    static ::Color      getVmlPresetColor( sal_Int32 nToken, ::Color nDefaultRgb );
    /** Returns the RGB value for the passed VML color token, or nDefaultRgb on error. */
    static ::Color      getHighlightColor(sal_Int32 nToken, ::Color nDefaultRgb);

    /** Sets the color to unused state. */
    void                setUnused();
    /** Sets an RGB value (hexadecimal RRGGBB) from the a:srgbClr element. */
    void                setSrgbClr( sal_Int32 nRgb );
    void                setSrgbClr( ::Color nRgb );
    /** Sets the percentual RGB values from the a:scrgbClr element. */
    void                setScrgbClr( sal_Int32 nR, sal_Int32 nG, sal_Int32 nB );
    /** Sets the HSL values from the a:hslClr element. */
    void                setHslClr( sal_Int32 nHue, sal_Int32 nSat, sal_Int32 nLum );
    /** Sets a predefined color from the a:prstClr element. */
    void                setPrstClr( sal_Int32 nToken );
    /** Sets a predefined color from the w:highlight element. */
    void                setHighlight(sal_Int32 nToken);
    /** Sets a scheme color from the a:schemeClr element. */
    void                setSchemeClr( sal_Int32 nToken );
    /** Sets the scheme name from the a:schemeClr element for interoperability purposes */
    void                setSchemeName( const OUString& sSchemeName ) { msSchemeName = sSchemeName; }
    /** Sets a system color from the a:sysClr element. */
    void                setSysClr( sal_Int32 nToken, sal_Int32 nLastRgb );
    /** Sets a palette color index. */
    void                setPaletteClr( sal_Int32 nPaletteIdx );

    /** Inserts the passed color transformation. */
    void                addTransformation( sal_Int32 nElement, sal_Int32 nValue = -1 );
    /** Inserts Chart specific color tint (-1.0...0.0 = shade, 0.0...1.0 = tint). */
    void                addChartTintTransformation( double fTint );
    /** Inserts Excel specific color tint (-1.0...0.0 = shade, 0.0...1.0 = tint). */
    void                addExcelTintTransformation( double fTint );
    /** Removes all color transformations. */
    void                clearTransformations();
    /** Removes transparence from the color. */
    void                clearTransparence();

    /** Overwrites this color with the passed color, if it is used. */
    void                assignIfUsed( const Color& rColor ) { if( rColor.isUsed() ) *this = rColor; }

    /** Returns true, if the color is initialized. */
    bool                isUsed() const { return meMode != ColorMode::Unused; }
    /** Returns true, if the color is a placeholder color in theme style lists. */
    bool                isPlaceHolder() const { return meMode == ColorMode::PlaceHolder; }
    /** Returns the final RGB color value.
        @param nPhClr  Actual color for the phClr placeholder color used in theme style lists. */
    ::Color             getColor( const GraphicHelper& rGraphicHelper, ::Color nPhClr = API_RGB_TRANSPARENT ) const;

    /** Returns true, if the color is transparent. */
    bool                hasTransparency() const;
    /** Returns the transparency of the color (0 = opaque, 100 = full transparent). */
    sal_Int16           getTransparency() const;

    /** Returns the scheme name from the a:schemeClr element for interoperability purposes */
    const OUString&     getSchemeColorName() const { return msSchemeName; }
    sal_Int16           getSchemeColorIndex() const;
    sal_Int16           getTintOrShade() const;
    sal_Int16           getLumMod() const;
    sal_Int16           getLumOff() const;

    model::ThemeColorType getThemeColorType() const
    {
        return meThemeColorType;
    }

    model::ComplexColor createComplexColor(const GraphicHelper& rGraphicHelper, sal_Int16 nPhClrTheme) const;

    /** Returns the unaltered list of transformations for interoperability purposes */
    const css::uno::Sequence< css::beans::PropertyValue >& getTransformations() const { return maInteropTransformations;}

    /** Translates between color transformation tokens and their names */
    static OUString     getColorTransformationName( sal_Int32 nElement );
    /** Translates between color transformation token names and the corresponding token */
    static sal_Int32    getColorTransformationToken( std::u16string_view sName );
    /** Translates between ColorMap token names and the corresponding token */
    static sal_Int32    getColorMapToken(std::u16string_view sName);

    /// Compares this color with rOther.
    bool equals(const Color& rOther, const GraphicHelper& rGraphicHelper, ::Color nPhClr) const;

    model::ComplexColor getComplexColor() const;

    bool operator==(const Color&) const;

private:
    /** Internal helper for getColor(). */
    void                setResolvedRgb( ::Color nRgb ) const;

    /** Converts the color components to RGB values. */
    void                toRgb() const;
    /** Converts the color components to CRGB values (gamma corrected percentage). */
    void                toCrgb() const;
    /** Converts the color components to HSL values. */
    void                toHsl() const;

private:
    enum class ColorMode
    {
        Unused,              /// Color is not used, or undefined.
        AbsoluteRgb,         /// Absolute RGB (r/g/b: 0...255).
        RelativeRgb,         /// Relative RGB (r/g/b: 0...100000).
        Hsl,                 /// HSL (hue: 0...21600000, sat/lum: 0...100000).
        Scheme,              /// Color from scheme.
        ApplicationPalette,  /// Color from application defined palette.
        SystemPalette,       /// Color from system palette.
        PlaceHolder,         /// Placeholder color in theme style lists.
        Finalized            /// Finalized RGB color.
    };

    struct Transformation
    {
        sal_Int32           mnToken;
        sal_Int32           mnValue;

        explicit            Transformation( sal_Int32 nToken, sal_Int32 nValue ) : mnToken( nToken ), mnValue( nValue ) {}

        bool operator==(const Transformation&) const noexcept = default;
    };

    mutable ColorMode   meMode;         /// Current color mode.
    mutable std::vector< Transformation >
                        maTransforms;  /// Color transformations.
    mutable sal_Int32   mnC1;           /// Red, red%, hue, scheme token, palette index, system token, or final RGB.
    mutable sal_Int32   mnC2;           /// Green, green%, saturation, or system default RGB.
    mutable sal_Int32   mnC3;           /// Blue, blue%, or luminance.
    sal_Int32           mnAlpha;        /// Alpha value (color opacity).
    OUString            msSchemeName;   /// Scheme name from the a:schemeClr element for interoperability purposes
    model::ThemeColorType meThemeColorType;
    css::uno::Sequence< css::beans::PropertyValue >
                        maInteropTransformations;   /// Unaltered list of transformations for interoperability purposes
};

} // namespace oox::drawingml

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

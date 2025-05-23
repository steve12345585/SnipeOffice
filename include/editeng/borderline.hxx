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

#ifndef INCLUDED_EDITENG_BORDERLINE_HXX
#define INCLUDED_EDITENG_BORDERLINE_HXX

#include <com/sun/star/table/BorderLineStyle.hpp>

#include <tools/color.hxx>
#include <tools/long.hxx>
#include <tools/mapunit.hxx>
#include <editeng/editengdllapi.h>
#include <svtools/borderline.hxx>
#include <docmodel/color/ComplexColor.hxx>
#include <o3tl/hash_combine.hxx>

class IntlWrapper;

// Line width defaults in twips
// Thin matches Excel's default values
// See tdf#48622 for the discussion leading to these defaults.
namespace SvxBorderLineWidth
{
constexpr inline sal_Int16 Hairline = 1; // 0.05pt
constexpr inline sal_Int16 VeryThin = 10; // 0.5pt
constexpr inline sal_Int16 Thin = 15; // 0.75pt
constexpr inline sal_Int16 Medium = 30; // 1.5pt
constexpr inline sal_Int16 Thick = 45; // 2.25pt
constexpr inline sal_Int16 ExtraThick = 90; // 4.5pt
};

// Abstracts over values from css::table::BorderLineStyle
enum class SvxBorderLineStyle : sal_Int16
{
    /** No border line
     */
    NONE = css::table::BorderLineStyle::NONE,

    /** Solid border line.
     */
    SOLID = css::table::BorderLineStyle::SOLID,

    /** Dotted border line.
     */
    DOTTED = css::table::BorderLineStyle::DOTTED,

    /** Dashed border line.
     */
    DASHED = css::table::BorderLineStyle::DASHED,

    /** Double border line. Widths of the lines and the gap are all equal,
        and vary equally with the total width.
     */
    DOUBLE = css::table::BorderLineStyle::DOUBLE,

    /** Double border line with a thin line outside and a thick line
        inside separated by a small gap.
     */
    THINTHICK_SMALLGAP = css::table::BorderLineStyle::THINTHICK_SMALLGAP,

    /** Double border line with a thin line outside and a thick line
        inside separated by a medium gap.
     */
    THINTHICK_MEDIUMGAP = css::table::BorderLineStyle::THINTHICK_MEDIUMGAP,

    /** Double border line with a thin line outside and a thick line
        inside separated by a large gap.
     */
    THINTHICK_LARGEGAP = css::table::BorderLineStyle::THINTHICK_LARGEGAP,

    /** Double border line with a thick line outside and a thin line
        inside separated by a small gap.
     */
    THICKTHIN_SMALLGAP = css::table::BorderLineStyle::THICKTHIN_SMALLGAP,

    /** Double border line with a thick line outside and a thin line
        inside separated by a medium gap.
     */
    THICKTHIN_MEDIUMGAP = css::table::BorderLineStyle::THICKTHIN_MEDIUMGAP,

    /** Double border line with a thick line outside and a thin line
        inside separated by a large gap.
     */
    THICKTHIN_LARGEGAP = css::table::BorderLineStyle::THICKTHIN_LARGEGAP,

    /** 3D embossed border line.
     */
    EMBOSSED = css::table::BorderLineStyle::EMBOSSED,

    /** 3D engraved border line.
     */
    ENGRAVED = css::table::BorderLineStyle::ENGRAVED,

    /** Outset border line.
     */
    OUTSET = css::table::BorderLineStyle::OUTSET,

    /** Inset border line.
     */
    INSET = css::table::BorderLineStyle::INSET,

    /** Finely dashed border line.
     */
    FINE_DASHED = css::table::BorderLineStyle::FINE_DASHED,

    /** Double border line consisting of two fixed thin lines separated by a
        variable gap.
     */
    DOUBLE_THIN = css::table::BorderLineStyle::DOUBLE_THIN,

    /** Line consisting of a repetition of one dash and one dot. */
    DASH_DOT = css::table::BorderLineStyle::DASH_DOT,

    /** Line consisting of a repetition of one dash and 2 dots. */
    DASH_DOT_DOT = css::table::BorderLineStyle::DASH_DOT_DOT,

    /** Maximum valid border line style value.
     */
    BORDER_LINE_STYLE_MAX = css::table::BorderLineStyle::BORDER_LINE_STYLE_MAX,
};

namespace editeng
{

// convert border style between Word formats and LO
SvxBorderLineStyle EDITENG_DLLPUBLIC ConvertBorderStyleFromWord(int);
/// convert border width in twips between Word formats and LO
double EDITENG_DLLPUBLIC ConvertBorderWidthToWord(SvxBorderLineStyle, double);
double EDITENG_DLLPUBLIC ConvertBorderWidthFromWord(SvxBorderLineStyle,
        double, int);

class EDITENG_DLLPUBLIC SvxBorderLine final
{
    tools::Long m_nWidth;
    tools::Long m_nMult;
    tools::Long m_nDiv;
    Color (*m_pColorOutFn)(Color);
    Color (*m_pColorInFn)(Color);
    Color (*m_pColorGapFn)(Color);
    BorderWidthImpl m_aWidthImpl;
    Color m_aColor;
    model::ComplexColor m_aComplexColor;
    SvxBorderLineStyle   m_nStyle;
    bool m_bMirrorWidths;
    bool m_bUseLeftTop;

public:
    SvxBorderLine( const Color *pCol = nullptr,
            tools::Long nWidth = 0,
            SvxBorderLineStyle nStyle = SvxBorderLineStyle::SOLID,
            Color (*pColorOutFn)( Color ) = &darkColor,
            Color (*pColorInFn)( Color ) = &darkColor );

    const Color & GetColor() const { return m_aColor; }
    void SetColor(const Color & rColor ) { m_aColor = rColor; }

    model::ComplexColor const& getComplexColor() const
    {
        auto pUnConst = const_cast<SvxBorderLine*>(this);
        pUnConst->m_aComplexColor.setFinalColor(GetColor());
        return m_aComplexColor;
    }

    void setComplexColor(model::ComplexColor const& rComplexColor)
    {
        m_aComplexColor = rComplexColor;
    }

    bool setComplexColorFromAny(css::uno::Any const& rValue);


    Color           GetColorOut( bool bLeftOrTop = true ) const;
    Color           GetColorIn( bool bLeftOrTop = true ) const;
    bool            HasGapColor() const { return m_pColorGapFn != nullptr; }
    Color           GetColorGap() const;

    void SetWidth( tools::Long nWidth );
    /** Guess the style and width from the three lines widths values.

        When the value of nStyle is SvxBorderLine::DOUBLE, the style set will be guessed
        using the three values to match the best possible style among the following:
            - SvxBorderLine::DOUBLE
            - SvxBorderLine::THINTHICK_SMALLGAP
            - SvxBorderLine::THINTHICK_MEDIUMGAP
            - SvxBorderLine::THINTHICK_LARGEGAP
            - SvxBorderLine::THICKTHIN_SMALLGAP
            - SvxBorderLine::THICKTHIN_MEDIUMGAP
            - SvxBorderLine::THICKTHIN_LARGEGAP

        If no styles matches the width, then the width is set to 0.

        There is one known case that could fit several styles: \a nIn = \a nDist = 0.75 pt,
        \a nOut = 1.5 pt. This case fits SvxBorderLine::THINTHICK_SMALLGAP and
        SvxBorderLine::THINTHICK_MEDIUMGAP with a 1.5 pt width and
        SvxBorderLine::THINTHICK_LARGEGAP with a 0.75 pt width. The same case happens
        also for thick-thin styles.

        \param nStyle the border style used to guess the width.
        \param nIn the width of the inner line in 1th pt
        \param nOut the width of the outer line in 1th pt
        \param nDist the width of the gap between the lines in 1th pt
     */
    void            GuessLinesWidths( SvxBorderLineStyle nStyle, sal_uInt16 nOut, sal_uInt16 nIn = 0, sal_uInt16 nDist = 0 );

    // TODO Hacky method to mirror lines in only a few cases
    void            SetMirrorWidths() { m_bMirrorWidths = true; }
    tools::Long            GetWidth( ) const { return m_nWidth; }
    sal_uInt16      GetOutWidth() const;
    sal_uInt16      GetInWidth() const;
    sal_uInt16      GetDistance() const;

    SvxBorderLineStyle  GetBorderLineStyle() const { return m_nStyle; }

    void            SetBorderLineStyle( SvxBorderLineStyle nNew );
    void            ScaleMetrics( tools::Long nMult, tools::Long nDiv );

    bool            operator==( const SvxBorderLine &rCmp ) const;

    OUString        GetValueString( MapUnit eSrcUnit, MapUnit eDestUnit,
                                    const IntlWrapper* pIntl,
                                    bool bMetricStr = false ) const;

    bool            HasPriority( const SvxBorderLine& rOtherLine ) const;

    bool isEmpty() const {
        return m_aWidthImpl.IsEmpty()
            || m_nStyle == SvxBorderLineStyle::NONE
            || m_nWidth == 0;
    }
    bool isDouble() const { return m_aWidthImpl.IsDouble(); }
    sal_uInt16 GetScaledWidth() const { return GetOutWidth() + GetInWidth() + GetDistance(); }

    static Color darkColor( Color aMain );
    static Color lightColor( Color aMain );

    static Color threeDLightColor( Color aMain );
    static Color threeDMediumColor( Color aMain );
    static Color threeDDarkColor( Color aMain );

    static BorderWidthImpl getWidthImpl( SvxBorderLineStyle nStyle );
};

EDITENG_DLLPUBLIC bool operator!=( const SvxBorderLine& rLeft, const SvxBorderLine& rRight );

} // namespace editeng

#endif

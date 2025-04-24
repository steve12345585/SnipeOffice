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

#include <drawinglayer/drawinglayerdllapi.h>

#include <basegfx/range/b2drange.hxx>
#include <drawinglayer/primitive2d/textenumsprimitive2d.hxx>
#include <vector>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <vcl/kernarray.hxx>
#include <vcl/svapp.hxx>
#include <tools/fontenum.hxx>
#include <span>

// predefines
class VirtualDevice;
class GDIMetaFile;
enum class DrawTextFlags;
class SalLayout;
namespace vcl
{
class Font;
}
namespace tools
{
class Rectangle;
}
namespace drawinglayer::attribute
{
class FontAttribute;
}
namespace com::sun::star::lang
{
struct Locale;
}
namespace vcl::text
{
enum class ComplexTextLayoutFlags : sal_uInt8;
}
namespace basegfx
{
class BColor;
}

// access to one global impTimedRefDev incarnation in namespace drawinglayer::primitive

namespace drawinglayer::primitive2d
{
/** TextLayouterDevice class

    This helper class exists to isolate all accesses to VCL
    text formatting/handling functionality for primitive implementations.
    When in the future FontHandling may move to an own library independent
    from VCL, primitives will be prepared.
 */
class DRAWINGLAYER_DLLPUBLIC TextLayouterDevice
{
    /// internally used VirtualDevice
    SolarMutexGuard maSolarGuard;
    VirtualDevice& mrDevice;
    double mnFontScalingFixX = 1.0;
    double mnFontScalingFixY = 1.0;

public:
    /// constructor/destructor
    TextLayouterDevice();
    ~TextLayouterDevice() COVERITY_NOEXCEPT_FALSE;

    /// tooling methods
    void setFont(const vcl::Font& rFont);
    void setFontAttribute(const attribute::FontAttribute& rFontAttribute, double fFontScaleX,
                          double fFontScaleY, const css::lang::Locale& rLocale);
    void setLayoutMode(vcl::text::ComplexTextLayoutFlags nTextLayoutMode);
    vcl::text::ComplexTextLayoutFlags getLayoutMode() const;
    void setTextColor(const basegfx::BColor& rColor);

    double getTextHeight() const;
    double getOverlineHeight() const;
    double getOverlineOffset() const;
    double getUnderlineHeight() const;
    double getUnderlineOffset() const;
    double getStrikeoutOffset() const;

    double getTextWidth(const OUString& rText, sal_uInt32 nIndex, sal_uInt32 nLength) const;

    void getTextOutlines(basegfx::B2DPolyPolygonVector&, const OUString& rText, sal_uInt32 nIndex,
                         sal_uInt32 nLength, const ::std::vector<double>& rDXArray,
                         const ::std::vector<sal_Bool>& rKashidaArray) const;

    basegfx::B2DRange getTextBoundRect(const OUString& rText, sal_uInt32 nIndex,
                                       sal_uInt32 nLength) const;

    double getFontAscent() const;
    double getFontDescent() const;

    void addTextRectActions(const tools::Rectangle& rRectangle, const OUString& rText,
                            DrawTextFlags nStyle, GDIMetaFile& rGDIMetaFile) const;

    ::std::vector<double> getTextArray(const OUString& rText, sal_uInt32 nIndex, sal_uInt32 nLength,
                                       bool bCaret = false) const;
    std::unique_ptr<SalLayout> getSalLayout(const OUString& rText, sal_uInt32 nIndex,
                                            sal_uInt32 nLength,
                                            const basegfx::B2DPoint& rStartPoint,
                                            const KernArray& rDXArray,
                                            std::span<const sal_Bool> pKashidaAry) const;
    void createEmphasisMarks(
        const SalLayout& rSalLayout, TextEmphasisMark aTextEmphasisMark, bool bAbove,
        const std::function<void(const basegfx::B2DPoint&, const basegfx::B2DPolyPolygon&, bool,
                                 const tools::Rectangle&, const tools::Rectangle&)>& rCallback)
        const;
};

// helper methods for vcl font handling

/** Create a VCL-Font based on the definitions in FontAttribute
            and the given FontScaling. The FontScaling defines the FontHeight
            (fFontScaleY) and the FontWidth (fFontScaleX). The combination of
            both defines FontStretching, where no stretching happens at
            fFontScaleY == fFontScaleX
         */
vcl::Font DRAWINGLAYER_DLLPUBLIC getVclFontFromFontAttribute(
    const attribute::FontAttribute& rFontAttribute, double fFontScaleX, double fFontScaleY,
    double fFontRotation, const css::lang::Locale& rLocale);

/** Generate FontAttribute DataSet derived from the given VCL-Font.
            The FontScaling with fFontScaleY, fFontScaleX relationship (see
            above) will be set in return parameter o_rSize to allow further
            processing
         */
attribute::FontAttribute DRAWINGLAYER_DLLPUBLIC getFontAttributeFromVclFont(
    basegfx::B2DVector& o_rSize, const vcl::Font& rFont, bool bRTL, bool bBiDiStrong);

} // end of namespace drawinglayer::primitive2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

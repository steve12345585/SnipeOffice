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

#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

#include <vcl/dllapi.h>
#include <vcl/vclenum.hxx> // for typedef sal_UCS4
#include <vcl/vcllayout.hxx>

#include "justificationdata.hxx"
#include "ImplLayoutRuns.hxx"
#include "impglyphitem.hxx"

#include <com/sun/star/i18n/XBreakIterator.hpp>

#include <hb.h>

#include <memory>
#include <span>
#include <vector>

#define MAX_FALLBACK 16

class GenericSalLayout;
class SalGraphics;
enum class SalLayoutFlags;

namespace vcl::font {
    class PhysicalFontFace;
}

namespace vcl::text {
    class TextLayoutCache;
}

class VCL_DLLPUBLIC MultiSalLayout final : public SalLayout
{
public:
    SAL_DLLPRIVATE void            DrawText(SalGraphics&) const override;
    SAL_DLLPRIVATE sal_Int32       GetTextBreak(double nMaxWidth, double nCharExtra, int nFactor) const override;
    SAL_DLLPRIVATE double          GetTextWidth() const final override;
    SAL_DLLPRIVATE double GetPartialTextWidth(sal_Int32 skipStart, sal_Int32 amt) const override;
    SAL_DLLPRIVATE double          FillDXArray(std::vector<double>* pDXArray, const OUString& rStr) const override;
    SAL_DLLPRIVATE double FillPartialDXArray(std::vector<double>* pDXArray, const OUString& rStr,
                                             sal_Int32 skipStart, sal_Int32 amt) const override;
    SAL_DLLPRIVATE void            GetCaretPositions(std::vector<double>& rCaretPositions, const OUString& rStr) const override;
    SAL_DLLPRIVATE bool            GetNextGlyph(const GlyphItem** pGlyph, basegfx::B2DPoint& rPos, int& nStart,
                                 const LogicalFontInstance** ppGlyphFont = nullptr) const override;
    SAL_DLLPRIVATE bool            GetOutline(basegfx::B2DPolyPolygonVector&) const override;
    SAL_DLLPRIVATE bool HasFontKashidaPositions() const override;
    SAL_DLLPRIVATE bool            IsKashidaPosValid(int nCharPos, int nNextCharPos) const override;
    SAL_DLLPRIVATE SalLayoutGlyphs GetGlyphs() const final override;

    // used only by OutputDevice::ImplLayout, TODO: make friend
    SAL_DLLPRIVATE explicit        MultiSalLayout( std::unique_ptr<SalLayout> pBaseLayout );
    SAL_DLLPRIVATE void            AddFallback(std::unique_ptr<SalLayout> pFallbackLayout, ImplLayoutRuns const &);
    // give up ownership of the initial pBaseLayout taken by the ctor
    SAL_DLLPRIVATE std::unique_ptr<SalLayout>  ReleaseBaseLayout();
    SAL_DLLPRIVATE bool            LayoutText(vcl::text::ImplLayoutArgs&, const SalLayoutGlyphsImpl*) override;
    SAL_DLLPRIVATE void            AdjustLayout(vcl::text::ImplLayoutArgs&) override;

    SAL_DLLPRIVATE void SetIncomplete(bool bIncomplete);

    SAL_DLLPRIVATE void ImplAdjustMultiLayout(vcl::text::ImplLayoutArgs& rArgs,
                                              vcl::text::ImplLayoutArgs& rMultiArgs,
                                              const JustificationData& rstJustification);

    SAL_DLLPRIVATE ImplLayoutRuns* GetFallbackRuns() { return maFallbackRuns; }

    SAL_DLLPRIVATE virtual         ~MultiSalLayout() override;

    virtual void drawSalLayout(void* /*pSurface*/, const basegfx::BColor& /*rTextColor*/, bool /*bAntiAliased*/) const override;

private:
                    MultiSalLayout( const MultiSalLayout& ) = delete;
                    MultiSalLayout& operator=( const MultiSalLayout& ) = delete;

    std::unique_ptr<GenericSalLayout> mpLayouts[ MAX_FALLBACK ];
    ImplLayoutRuns  maFallbackRuns[ MAX_FALLBACK ];
    int             mnLevel;
    bool            mbIncomplete;
};

class VCL_DLLPUBLIC GenericSalLayout : public SalLayout
{
    friend void MultiSalLayout::ImplAdjustMultiLayout(vcl::text::ImplLayoutArgs& rArgs,
                                                      vcl::text::ImplLayoutArgs& rMultiArgs,
                                                      const JustificationData& rstJustification);

public:
                    GenericSalLayout(LogicalFontInstance&);
                    ~GenericSalLayout() override;

    void            AdjustLayout(vcl::text::ImplLayoutArgs&) final override;
    bool            LayoutText(vcl::text::ImplLayoutArgs&, const SalLayoutGlyphsImpl*) final override;
    void            DrawText(SalGraphics&) const final override;
    SalLayoutGlyphs GetGlyphs() const final override;

    bool HasFontKashidaPositions() const final override;
    bool            IsKashidaPosValid(int nCharPos, int nNextCharPos) const final override;

    // used by upper layers
    double          GetTextWidth() const final override;
    double GetPartialTextWidth(sal_Int32 skipStart, sal_Int32 amt) const final override;
    double          FillDXArray(std::vector<double>* pDXArray, const OUString& rStr) const final override;
    double FillPartialDXArray(std::vector<double>* pDXArray, const OUString& rStr,
                              sal_Int32 skipStart, sal_Int32 amt) const final override;
    sal_Int32       GetTextBreak(double nMaxWidth, double nCharExtra, int nFactor) const final override;
    void            GetCaretPositions(std::vector<double>& rCaretPositions, const OUString& rStr) const override;

    // used by display layers
    LogicalFontInstance& GetFont() const
        { return *m_GlyphItems.GetFont(); }

    bool            GetNextGlyph(const GlyphItem** pGlyph, basegfx::B2DPoint& rPos, int& nStart,
                                 const LogicalFontInstance** ppGlyphFont = nullptr) const override;

    const SalLayoutGlyphsImpl& GlyphsImpl() const { return m_GlyphItems; }

    virtual void drawSalLayout(void* /*pSurface*/, const basegfx::BColor& /*rTextColor*/, bool /*bAntiAliased*/) const override;

private:
    // for glyph+font+script fallback
    SAL_DLLPRIVATE void MoveGlyph(int nStart, double nNewXPos);
    SAL_DLLPRIVATE void DropGlyph(int nStart);
    SAL_DLLPRIVATE void Simplify(bool bIsBase);

                    GenericSalLayout( const GenericSalLayout& ) = delete;
                    GenericSalLayout& operator=( const GenericSalLayout& ) = delete;

    SAL_DLLPRIVATE void ApplyJustificationData(const JustificationData& rstJustification);
    SAL_DLLPRIVATE void Justify(double nNewWidth);
    SAL_DLLPRIVATE void ApplyAsianKerning(std::u16string_view rStr);

    SAL_DLLPRIVATE void GetCharWidths(std::vector<double>& rCharWidths,
                                  const OUString& rStr) const;

    SAL_DLLPRIVATE void SetNeedFallback(vcl::text::ImplLayoutArgs& rArgs, sal_Int32 nCharPos,
                                        sal_Int32 nCharEnd, bool bRightToLeft);

    SAL_DLLPRIVATE bool HasVerticalAlternate(sal_UCS4 aChar, sal_UCS4 aNextChar);

    SAL_DLLPRIVATE void ParseFeatures(std::u16string_view name);

    css::uno::Reference<css::i18n::XBreakIterator> mxBreak;

    SalLayoutGlyphsImpl m_GlyphItems;

    OString         msLanguage;
    std::vector<hb_feature_t> maFeatures;

    hb_set_t*       mpVertGlyphs;
    const bool      mbFuzzing;
    bool m_bHasFontKashidaPositions = false;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include "svgpaint.hxx"
#include "svgnode.hxx"
#include "svgtools.hxx"
#include <tools/fontenum.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <drawinglayer/primitive2d/Primitive2DContainer.hxx>


// predefines

namespace svgio::svgreader {
    class SvgGradientNode;
    class SvgPatternNode;
    class SvgMarkerNode;
    class SvgClipPathNode;
    class SvgFilterNode;
    class SvgMaskNode;
}


namespace svgio::svgreader
    {
        enum class StrokeLinecap
        {
            notset,
            butt,
            round,
            square
        };

        enum class StrokeLinejoin
        {
            notset,
            miter,
            round,
            bevel
        };

        enum class FontSize
        {
            notset,
            xx_small,
            x_small,
            small,
            smaller,
            medium,
            large,
            larger,
            x_large,
            xx_large,
            initial
        };

        enum class FontStretch
        {
            notset,
            normal,
            wider,
            narrower,
            ultra_condensed,
            extra_condensed,
            condensed,
            semi_condensed,
            semi_expanded,
            expanded,
            extra_expanded,
            ultra_expanded
        };

        FontStretch getWider(FontStretch aSource);
        FontStretch getNarrower(FontStretch aSource);

        enum class FontStyle
        {
            notset,
            normal,
            italic,
            oblique
        };

        enum class FontWeight
        {
            notset,
            N100,
            N200,
            N300,
            N400, // same as normal
            N500,
            N600,
            N700, // same as bold
            N800,
            N900,
            bolder,
            lighter,
        };

        FontWeight getBolder(FontWeight aSource);
        FontWeight getLighter(FontWeight aSource);
        ::FontWeight getVclFontWeight(FontWeight aSource);

        enum class TextAlign
        {
            notset,
            left,
            right,
            center,
            justify
        };

        enum class TextDecoration
        {
            notset,
            none,
            underline,
            overline,
            line_through,
            blink
        };

        enum class TextAnchor
        {
            notset,
            start,
            middle,
            end
        };

        enum class FillRule
        {
            notset,
            nonzero,
            evenodd
        };

        enum class BaselineShift
        {
            Baseline,
            Sub,
            Super,
            Percentage,
            Length
        };

        enum class DominantBaseline
        {
            Auto,
            Middle,
            Hanging,
            Central
        };

        enum class Overflow
        {
            notset,
            hidden,
            visible
        };

        enum class Visibility
        {
            notset,
            visible,
            hidden,
            collapse,
            inherit
        };

        class SvgStyleAttributes
        {
        private:
            SvgNode&                    mrOwner;
            const SvgStyleAttributes*   mpCssStyle;
            SvgPaint                    maFill;
            SvgPaint                    maStroke;
            SvgPaint                    maStopColor;
            SvgNumber                   maStrokeWidth;
            SvgNumber                   maStopOpacity;
            SvgNumber                   maFillOpacity;
            SvgNumberVector             maStrokeDasharray;
            SvgNumber                   maStrokeDashOffset;
            StrokeLinecap               maStrokeLinecap;
            StrokeLinejoin              maStrokeLinejoin;
            SvgNumber                   maStrokeMiterLimit;
            SvgNumber                   maStrokeOpacity;
            SvgStringVector             maFontFamily;
            FontSize                    maFontSize;
            SvgNumber                   maFontSizeNumber;
            FontStretch                 maFontStretch;
            FontStyle                   maFontStyle;
            FontWeight                  maFontWeight;
            TextAlign                   maTextAlign;
            TextDecoration              maTextDecoration;
            TextAnchor                  maTextAnchor;
            SvgPaint                    maColor;
            SvgNumber                   maOpacity;
            Overflow                    maOverflow;
            Visibility                  maVisibility;
            OUString               maTitle;
            OUString               maDesc;

            /// link to content. If set, the node can be fetched on demand
            OUString               maClipPathXLink;
            OUString               maFilterXLink;
            OUString               maMaskXLink;

            /// link to markers. If set, the node can be fetched on demand
            OUString               maMarkerStartXLink;
            OUString               maMarkerMidXLink;
            OUString               maMarkerEndXLink;

            /// fill rule
            FillRule                    maFillRule;

            // ClipRule setting (only valid when mbIsClipPathContent == true, default is FillRule_nonzero)
            FillRule                    maClipRule;

            // BaselineShift: Type and number (in case of BaselineShift_Percentage or BaselineShift_Length)
            BaselineShift               maBaselineShift;
            SvgNumber                   maBaselineShiftNumber;

            DominantBaseline            maDominantBaseline;

            mutable std::vector<sal_uInt16> maResolvingParent;


            // #121221# Defines if evtl. an empty array *is* set
            bool                        mbStrokeDasharraySet : 1;

            // tdf#155651 Defines if 'context-fill' is used in fill
            bool                        mbUseFillFromContextFill : 1;

            // tdf#155651 Defines if 'context-stroke' is used in fill
            bool                        mbUseFillFromContextStroke : 1;

            // tdf#155651 Defines if 'context-fill' is used in stroke
            bool                        mbUseStrokeFromContextFill : 1;

            // tdf#155651 Defines if 'context-stroke' is used in stroke
            bool                        mbUseStrokeFromContextStroke : 1;

            // tdf#94765 Check id references in gradient/pattern getters
            OUString                    maNodeFillURL;
            OUString                    maNodeStrokeURL;

            /// internal helpers
            void add_fillGradient(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const SvgGradientNode& rFillGradient,
                const basegfx::B2DRange& rGeoRange) const;
            void add_fillPatternTransform(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const SvgPatternNode& rFillGradient,
                const basegfx::B2DRange& rGeoRange) const;
            void add_fillPattern(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const SvgPatternNode& rFillGradient,
                const basegfx::B2DRange& rGeoRange) const;
            void add_fill(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const basegfx::B2DRange& rGeoRange) const;
            void add_stroke(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const basegfx::B2DRange& rGeoRange) const;
            bool prepare_singleMarker(
                drawinglayer::primitive2d::Primitive2DContainer& rMarkerPrimitives,
                basegfx::B2DHomMatrix& rMarkerTransform,
                basegfx::B2DRange& rClipRange,
                const SvgMarkerNode& rMarker) const;
            void add_markers(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const basegfx::utils::PointIndexSet* pHelpPointIndices) const;


        public:
            /// local attribute scanner
            void parseStyleAttribute(SVGToken aSVGToken, const OUString& rContent);

            /// helper which does the necessary with a given path
            void add_text(
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                drawinglayer::primitive2d::Primitive2DContainer&& rSource) const;
            void add_path(
                const basegfx::B2DPolyPolygon& rPath,
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                const basegfx::utils::PointIndexSet* pHelpPointIndices) const;
            void add_postProcess(
                drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                drawinglayer::primitive2d::Primitive2DContainer&& rSource,
                const std::optional<basegfx::B2DHomMatrix>& pTransform) const;

            /// helper to set mpCssStyle temporarily for CSS style hierarchies
            void setCssStyle(const SvgStyleAttributes* pNew) { mpCssStyle = pNew; }
            const SvgStyleAttributes* getCssStyle() const { return mpCssStyle; }

            /// scan helpers
            void readCssStyle(std::u16string_view rCandidate);
            const SvgStyleAttributes* getCssStyleOrParentStyle() const;

            const SvgMarkerNode* getMarkerParentNode() const;

            SvgStyleAttributes(SvgNode& rOwner);
            ~SvgStyleAttributes();

            // Check if this attribute is part of a ClipPath.
            // If so, rough geometry will be created on decomposition by patching
            // values for fill, stroke, strokeWidth and others
            bool isClipPathContent() const;

            /// fill content
            bool isFillSet() const; // #i125258# ask if fill is a direct hard attribute (no hierarchy)
            const basegfx::BColor* getFill() const;
            void setFill(const SvgPaint& rFill) { maFill = rFill; }

            /// stroke content
            const basegfx::BColor* getStroke() const;

            /// context fill content
            const basegfx::BColor* getContextFill() const;

            /// context stroke content
            const basegfx::BColor* getContextStroke() const;

            /// stop color content
            const basegfx::BColor& getStopColor() const;

            /// stroke-width content
            SvgNumber getStrokeWidth() const;

            /// stop opacity content
            SvgNumber getStopOpacity() const;

            /// access to evtl. set fill gradient
            const SvgGradientNode* getSvgGradientNodeFill() const;

            /// access to evtl. set fill pattern
            const SvgPatternNode* getSvgPatternNodeFill() const;

            /// access to evtl. set stroke gradient
            const SvgGradientNode* getSvgGradientNodeStroke() const;

            /// access to evtl. set stroke pattern
            const SvgPatternNode* getSvgPatternNodeStroke() const;

            /// fill opacity content
            SvgNumber getFillOpacity() const;

            /// fill rule content
            FillRule getFillRule() const;

            /// clip rule content
            FillRule getClipRule() const;

            /// fill StrokeDasharray content
            const SvgNumberVector& getStrokeDasharray() const;

            /// StrokeDashOffset content
            SvgNumber getStrokeDashOffset() const;

            /// StrokeLinecap content
            StrokeLinecap getStrokeLinecap() const;
            void setStrokeLinecap(const StrokeLinecap aStrokeLinecap) { maStrokeLinecap = aStrokeLinecap; }

            /// StrokeLinejoin content
            StrokeLinejoin getStrokeLinejoin() const;
            void setStrokeLinejoin(const StrokeLinejoin aStrokeLinejoin) { maStrokeLinejoin = aStrokeLinejoin; }

            /// StrokeMiterLimit content
            SvgNumber getStrokeMiterLimit() const;

            /// StrokeOpacity content
            SvgNumber getStrokeOpacity() const;

            /// Font content
            const SvgStringVector& getFontFamily() const;

            /// FontSize content
            void setFontSize(const FontSize aFontSize) { maFontSize = aFontSize; }
            SvgNumber getFontSizeNumber() const;

            /// FontStretch content
            FontStretch getFontStretch() const;
            void setFontStretch(const FontStretch aFontStretch) { maFontStretch = aFontStretch; }

            /// FontStyle content
            FontStyle getFontStyle() const;
            void setFontStyle(const FontStyle aFontStyle) { maFontStyle = aFontStyle; }

            /// FontWeight content
            FontWeight getFontWeight() const;
            void setFontWeight(const FontWeight aFontWeight) { maFontWeight = aFontWeight; }

            /// TextAlign content
            TextAlign getTextAlign() const;
            void setTextAlign(const TextAlign aTextAlign) { maTextAlign = aTextAlign; }

            /// TextDecoration content
            const SvgStyleAttributes* getTextDecorationDefiningSvgStyleAttributes() const;
            TextDecoration getTextDecoration() const;
            void setTextDecoration(const TextDecoration aTextDecoration) { maTextDecoration = aTextDecoration; }

            /// TextAnchor content
            TextAnchor getTextAnchor() const;
            void setTextAnchor(const TextAnchor aTextAnchor) { maTextAnchor = aTextAnchor; }

            /// Color content
            const basegfx::BColor* getColor() const;

            /// Resolve current color (defaults to black if no color is specified)
            const basegfx::BColor* getCurrentColor() const;

            /// Opacity content
            SvgNumber getOpacity() const;
            void setOpacity(const SvgNumber& rOpacity) { maOpacity = rOpacity; }

            /// Overflow
            Overflow getOverflow() const;
            void setOverflow(const Overflow aOverflow) { maOverflow = aOverflow; }

            /// Visibility
            Visibility getVisibility() const;
            void setVisibility(const Visibility aVisibility) { maVisibility = aVisibility; }

            // Title content
            const OUString& getTitle() const { return maTitle; }

            // Desc content
            const OUString& getDesc() const { return maDesc; }

            // ClipPathXLink content
            OUString getClipPathXLink() const;
            const SvgClipPathNode* accessClipPathXLink() const;

            // FilterXLink content
            OUString getFilterXLink() const;
            const SvgFilterNode* accessFilterXLink() const;

            // MaskXLink content
            OUString getMaskXLink() const;
            const SvgMaskNode* accessMaskXLink() const;

            // MarkerStartXLink content
            OUString getMarkerStartXLink() const;
            const SvgMarkerNode* accessMarkerStartXLink() const;

            // MarkerMidXLink content
            OUString getMarkerMidXLink() const;
            const SvgMarkerNode* accessMarkerMidXLink() const;

            // MarkerEndXLink content
            OUString getMarkerEndXLink() const;
            const SvgMarkerNode* accessMarkerEndXLink() const;

            // BaselineShift
            void setBaselineShift(const BaselineShift aBaselineShift) { maBaselineShift = aBaselineShift; }
            BaselineShift getBaselineShift() const;
            SvgNumber getBaselineShiftNumber() const;

            // DominantBaseline
            void setDominantBaseline(const DominantBaseline aDominantBaseline) { maDominantBaseline = aDominantBaseline; }
            DominantBaseline getDominantBaseline() const;
        };

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

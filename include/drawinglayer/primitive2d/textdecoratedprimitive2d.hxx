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

#include <drawinglayer/primitive2d/textprimitive2d.hxx>
#include <drawinglayer/primitive2d/textenumsprimitive2d.hxx>

namespace basegfx::utils {
    class B2DHomMatrixBufferedOnDemandDecompose;
}

namespace drawinglayer::primitive2d
{
        /** TextDecoratedPortionPrimitive2D class

            This primitive expands the TextSimplePortionPrimitive2D by common
            decorations used in the office. It can be decomposed and will create
            a TextSimplePortionPrimitive2D and all the contained decorations (if used)
            as geometry.
         */
        class DRAWINGLAYER_DLLPUBLIC TextDecoratedPortionPrimitive2D final : public TextSimplePortionPrimitive2D
        {
        private:
            /// a sequence used for buffering broken up text for WordLineMode
            mutable Primitive2DContainer                maBufferedBrokenUpText;

            /// a sequence used for buffering getOrCreateDecorationGeometryContent
            mutable Primitive2DContainer                maBufferedDecorationGeometry;

            /// decoration definitions
            basegfx::BColor                             maOverlineColor;
            basegfx::BColor                             maTextlineColor;
            TextLine                                    meFontOverline;
            TextLine                                    meFontUnderline;
            TextStrikeout                               meTextStrikeout;
            TextEmphasisMark                            meTextEmphasisMark;
            TextRelief                                  meTextRelief;

            bool                                        mbUnderlineAbove : 1;
            bool                                        mbWordLineMode : 1;
            bool                                        mbEmphasisMarkAbove : 1;
            bool                                        mbEmphasisMarkBelow : 1;
            bool                                        mbShadow : 1;

            /// helper methods
            void impCreateGeometryContent(
                Primitive2DContainer& rTarget,
                basegfx::utils::B2DHomMatrixBufferedOnDemandDecompose const & rDecTrans,
                const OUString& rText,
                sal_Int32 nTextPosition,
                sal_Int32 nTextLength,
                const ::std::vector< double >& rDXArray,
                const ::std::vector< sal_Bool >& rKashidaArray,
                const attribute::FontAttribute& rFontAttribute) const;

            /// local decomposition.
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            TextDecoratedPortionPrimitive2D(
                /// TextSimplePortionPrimitive2D parameters
                const basegfx::B2DHomMatrix& rNewTransform,
                const OUString& rText,
                sal_Int32 nTextPosition,
                sal_Int32 nTextLength,
                std::vector< double >&& rDXArray,
                std::vector< sal_Bool >&& rKashidaArray,
                const attribute::FontAttribute& rFontAttribute,
                const css::lang::Locale& rLocale,
                const basegfx::BColor& rFontColor,
                const Color& rFillColor,

                /// local parameters
                const basegfx::BColor& rOverlineColor,
                const basegfx::BColor& rTextlineColor,
                TextLine eFontOverline = TEXT_LINE_NONE,
                TextLine eFontUnderline = TEXT_LINE_NONE,
                bool bUnderlineAbove = false,
                TextStrikeout eTextStrikeout = TEXT_STRIKEOUT_NONE,
                bool bWordLineMode = false,
                TextEmphasisMark eTextEmphasisMark = TEXT_FONT_EMPHASIS_MARK_NONE,
                bool bEmphasisMarkAbove = true,
                bool bEmphasisMarkBelow = false,
                TextRelief eTextRelief = TEXT_RELIEF_NONE,
                bool bShadow = false);

            /// data read access
            TextLine getFontOverline() const { return meFontOverline; }
            TextLine getFontUnderline() const { return meFontUnderline; }
            TextStrikeout getTextStrikeout() const { return meTextStrikeout; }
            TextEmphasisMark getTextEmphasisMark() const { return meTextEmphasisMark; }
            TextRelief getTextRelief() const { return meTextRelief; }
            const basegfx::BColor& getOverlineColor() const { return maOverlineColor; }
            const basegfx::BColor& getTextlineColor() const { return maTextlineColor; }
            bool getUnderlineAbove() const { return mbUnderlineAbove; }
            bool getWordLineMode() const { return mbWordLineMode; }
            bool getEmphasisMarkAbove() const { return mbEmphasisMarkAbove; }
            bool getEmphasisMarkBelow() const { return mbEmphasisMarkBelow; }
            bool getShadow() const { return mbShadow; }

            /// helper top create DecorationGeometry as Primitives
            const Primitive2DContainer& getOrCreateDecorationGeometryContent(
                basegfx::utils::B2DHomMatrixBufferedOnDemandDecompose const & rDecTrans,
                const OUString& rText,
                sal_Int32 nTextPosition,
                sal_Int32 nTextLength,
                const std::vector< double >& rDXArray) const;

            /// helper for break-up text if needed
            const Primitive2DContainer& getOrCreateBrokenUpText() const;

            /// helpers for determining various decoration states
            virtual bool hasTextRelief() const override;
            virtual bool hasShadow() const override;
            virtual bool hasTextDecoration() const override;

            /// compare operator
            virtual bool operator==( const BasePrimitive2D& rPrimitive ) const override;

            /// get range
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

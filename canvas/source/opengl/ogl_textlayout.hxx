/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <comphelper/compbase.hxx>

#include <com/sun/star/rendering/XTextLayout.hpp>

#include "ogl_canvasfont.hxx"


/* Definition of TextLayout class */

namespace oglcanvas
{
    typedef ::comphelper::WeakComponentImplHelper< css::rendering::XTextLayout > TextLayoutBaseT;

    class TextLayout : public TextLayoutBaseT
    {
    public:
        TextLayout( css::rendering::StringContext               aText,
                    sal_Int8                                    nDirection,
                    sal_Int64                                   nRandomSeed,
                    CanvasFont::ImplRef                         rFont );

        /// make noncopyable
        TextLayout(const TextLayout&) = delete;
        const TextLayout& operator=(const TextLayout&) = delete;

        /// Dispose all internal references
        virtual void disposing(std::unique_lock<std::mutex>& rGuard) override;

        // XTextLayout
        virtual css::uno::Sequence< css::uno::Reference< css::rendering::XPolyPolygon2D > > SAL_CALL queryTextShapes(  ) override;
        virtual css::uno::Sequence< css::geometry::RealRectangle2D > SAL_CALL queryInkMeasures(  ) override;
        virtual css::uno::Sequence< css::geometry::RealRectangle2D > SAL_CALL queryMeasures(  ) override;
        virtual css::uno::Sequence< double > SAL_CALL queryLogicalAdvancements(  ) override;
        virtual void SAL_CALL applyLogicalAdvancements( const css::uno::Sequence< double >& aAdvancements ) override;
        virtual css::uno::Sequence< sal_Bool > SAL_CALL queryKashidaPositions(  ) override;
        virtual void SAL_CALL applyKashidaPositions( const css::uno::Sequence< sal_Bool >& aPositions ) override;
        virtual css::geometry::RealRectangle2D SAL_CALL queryTextBounds(  ) override;
        virtual double SAL_CALL justify( double nSize ) override;
        virtual double SAL_CALL combinedJustify( const css::uno::Sequence< css::uno::Reference< css::rendering::XTextLayout > >& aNextLayouts, double nSize ) override;
        virtual css::rendering::TextHit SAL_CALL getTextHit( const css::geometry::RealPoint2D& aHitPoint ) override;
        virtual css::rendering::Caret SAL_CALL getCaret( sal_Int32 nInsertionIndex, sal_Bool bExcludeLigatures ) override;
        virtual sal_Int32 SAL_CALL getNextInsertionIndex( sal_Int32 nStartIndex, sal_Int32 nCaretAdvancement, sal_Bool bExcludeLigatures ) override;
        virtual css::uno::Reference< css::rendering::XPolyPolygon2D > SAL_CALL queryVisualHighlighting( sal_Int32 nStartIndex, sal_Int32 nEndIndex ) override;
        virtual css::uno::Reference< css::rendering::XPolyPolygon2D > SAL_CALL queryLogicalHighlighting( sal_Int32 nStartIndex, sal_Int32 nEndIndex ) override;
        virtual double SAL_CALL getBaselineOffset(  ) override;
        virtual sal_Int8 SAL_CALL getMainTextDirection(  ) override;
        virtual css::uno::Reference< css::rendering::XCanvasFont > SAL_CALL getFont(  ) override;
        virtual css::rendering::StringContext SAL_CALL getText(  ) override;

    private:
        css::rendering::StringContext              maText;
        css::uno::Sequence< double >               maLogicalAdvancements;
        css::uno::Sequence< sal_Bool >             maKashidaPositions;
        CanvasFont::ImplRef                        mpFont;
        sal_Int8                                   mnTextDirection;
    };

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

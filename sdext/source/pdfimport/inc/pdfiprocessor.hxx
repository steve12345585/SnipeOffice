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

#ifndef INCLUDED_SDEXT_SOURCE_PDFIMPORT_INC_PDFIPROCESSOR_HXX
#define INCLUDED_SDEXT_SOURCE_PDFIMPORT_INC_PDFIPROCESSOR_HXX

#include <com/sun/star/drawing/LineJoint.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/task/XStatusIndicator.hpp>
#include <com/sun/star/geometry/RealSize2D.hpp>
#include <com/sun/star/geometry/RealRectangle2D.hpp>
#include <com/sun/star/geometry/Matrix2D.hpp>

#include <basegfx/matrix/b2dhommatrix.hxx>

#include <rtl/ustring.hxx>

#include <memory>
#include <unordered_map>

#include "imagecontainer.hxx"
#include "contentsink.hxx"
#include "treevisitorfactory.hxx"
#include "genericelements.hxx"

namespace pdfi
{

    class  PDFIProcessor;
    struct Element;
    struct DocumentElement;
    struct PageElement;
    class  ElementFactory;
    class  XmlEmitter;
    class  CharGlyph;

    /** Main entry from the parser

        Creates the internal DOM tree from the render calls
     */
    class PDFIProcessor final : public ContentSink
    {
    public:
        css::uno::Reference<
            css::uno::XComponentContext >  m_xContext;
        basegfx::B2DHomMatrix prevTextMatrix;
        double prevCharWidth;

        explicit PDFIProcessor( const css::uno::Reference< css::task::XStatusIndicator >& xStat,
            css::uno::Reference< css::uno::XComponentContext > const & xContext) ;

        void emit( XmlEmitter&               rEmitter,
                   const TreeVisitorFactory& rVisitorFactory );

        sal_Int32 getGCId( const GraphicsContext& rGC );
        const GraphicsContext& getGraphicsContext( sal_Int32 nGCId ) const;
        GraphicsContext& getCurrentContext() { return m_aGCStack.back(); }
        const GraphicsContext& getCurrentContext() const { return m_aGCStack.back(); }
        const ImageContainer& getImages() const { return m_aImages; };

        const css::uno::Reference< css::task::XStatusIndicator >& getStatusIndicator() const
        { return m_xStatusIndicator; }

        const FontAttributes& getFont( sal_Int32 nFontId ) const;
        sal_Int32 getFontId( const FontAttributes& rAttr ) const;

        static void sortElements( Element* pElement );

        static OUString SubstituteBidiMirrored(std::u16string_view rString);

    private:
        void processGlyphLine();

        // ContentSink interface implementation

        virtual void setPageNum( sal_Int32 nNumPages ) override;
        virtual void startPage( const css::geometry::RealSize2D& rSize ) override;
        virtual void endPage() override;

        virtual void hyperLink( const css::geometry::RealRectangle2D& rBounds,
                                const OUString&                             rURI ) override;
        virtual void pushState() override;
        virtual void popState() override;
        virtual void setFlatness( double ) override;
        virtual void setTransformation( const css::geometry::AffineMatrix2D& rMatrix ) override;
        virtual void setLineDash( const css::uno::Sequence<double>& dashes,
                                  double                                         start ) override;
        virtual void setLineJoin(basegfx::B2DLineJoin) override;
        virtual void setLineCap(sal_Int8) override;
        virtual void setMiterLimit(double) override;
        virtual void setLineWidth(double) override;
        virtual void setFillColor( const css::rendering::ARGBColor& rColor ) override;
        virtual void setStrokeColor( const css::rendering::ARGBColor& rColor ) override;
        virtual void setFont( const FontAttributes& rFont ) override;
        virtual void setTextRenderMode( sal_Int32 ) override;

        virtual void strokePath( const css::uno::Reference<
                                       css::rendering::XPolyPolygon2D >& rPath ) override;
        virtual void fillPath( const css::uno::Reference<
                                     css::rendering::XPolyPolygon2D >& rPath ) override;
        virtual void eoFillPath( const css::uno::Reference<
                                       css::rendering::XPolyPolygon2D >& rPath ) override;

        virtual void intersectClip(const css::uno::Reference<
                                         css::rendering::XPolyPolygon2D >& rPath) override;
        virtual void intersectClipToStroke(const css::uno::Reference<
                                                 css::rendering::XPolyPolygon2D >& rPath) override;
        virtual void intersectEoClip(const css::uno::Reference<
                                           css::rendering::XPolyPolygon2D >& rPath) override;

        virtual void drawGlyphs( const OUString&                               rGlyphs,
                                 const css::geometry::RealRectangle2D& rRect,
                                 const css::geometry::Matrix2D&        rFontMatrix,
                                 double fontSize) override;
        virtual void endText() override;

        virtual void drawMask(const css::uno::Sequence<
                                    css::beans::PropertyValue>& xBitmap,
                              bool                                           bInvert ) override;
        /// Given image must already be color-mapped and normalized to sRGB.
        virtual void drawImage(const css::uno::Sequence<
                                     css::beans::PropertyValue>& xBitmap ) override;
        /** Given image must already be color-mapped and normalized to sRGB.

            maskColors must contain two sequences of color components
         */
        virtual void drawColorMaskedImage(const css::uno::Sequence<
                                                css::beans::PropertyValue>& xBitmap,
                                          const css::uno::Sequence<
                                                css::uno::Any>&             xMaskColors ) override;
        virtual void drawMaskedImage(const css::uno::Sequence<
                                           css::beans::PropertyValue>& xBitmap,
                                     const css::uno::Sequence<
                                           css::beans::PropertyValue>& xMask,
                                     bool                                             bInvertMask) override;
        virtual void drawAlphaMaskedImage(const css::uno::Sequence<
                                                css::beans::PropertyValue>& xImage,
                                          const css::uno::Sequence<
                                                css::beans::PropertyValue>& xMask) override;

        virtual void tilingPatternFill(int nX0, int nY0, int nX1, int nY1,
                                       double nxStep, double nyStep,
                                       int nPaintType,
                                       css::geometry::AffineMatrix2D& rMat,
                                       const css::uno::Sequence<css::beans::PropertyValue>& xTile) override;

        void startIndicator( const OUString& rText );
        void endIndicator();

        void setupImage(ImageId nImage);

        typedef std::unordered_map<sal_Int32,FontAttributes> IdToFontMap;
        typedef std::unordered_map<FontAttributes,sal_Int32,FontAttrHash> FontToIdMap;

        typedef std::unordered_map<sal_Int32,GraphicsContext> IdToGCMap;
        typedef std::unordered_map<GraphicsContext, sal_Int32, GraphicsContextHash> GCToIdMap;

        typedef std::vector<GraphicsContext> GraphicsContextStack;

        std::vector<CharGlyph>             m_GlyphsList;

        std::shared_ptr<DocumentElement> m_pDocument;
        PageElement*                       m_pCurPage;
        Element*                           m_pCurElement;
        sal_Int32                          m_nNextFontId;
        IdToFontMap                        m_aIdToFont;
        FontToIdMap                        m_aFontToId;

        GraphicsContextStack               m_aGCStack;
        sal_Int32                          m_nNextGCId;
        IdToGCMap                          m_aIdToGC;
        GCToIdMap                          m_aGCToId;

        ImageContainer                     m_aImages;

        sal_Int32                          m_nPages;
        sal_Int32                          m_nNextZOrder;
        css::uno::Reference< css::task::XStatusIndicator >
                                           m_xStatusIndicator;
    };
    class CharGlyph final
    {
        public:
            CharGlyph(Element* pCurElement, const GraphicsContext& rCurrentContext,
                double width, double prevSpaceWidth, const OUString& rGlyphs  )
               : m_pCurElement(pCurElement), m_rCurrentContext(rCurrentContext),
                 m_Width(width), m_PrevSpaceWidth(prevSpaceWidth), m_rGlyphs(rGlyphs) {};

            OUString& getGlyph(){ return m_rGlyphs; }
            double getWidth() const { return m_Width; }
            double getPrevSpaceWidth() const { return m_PrevSpaceWidth; }
            GraphicsContext&  getGC(){ return m_rCurrentContext; }
            Element*  getCurElement(){ return m_pCurElement; }

        private:
            Element*                    m_pCurElement ;
            GraphicsContext             m_rCurrentContext ;
            double                      m_Width ;
            double                      m_PrevSpaceWidth ;
            OUString                    m_rGlyphs ;
    };
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <memory>
#include <stack>
#include <cppuhelper/implbase.hxx>
#include <oox/drawingml/color.hxx>
#include <oox/drawingml/graphicshapecontext.hxx>
#include <oox/core/fragmenthandler2.hxx>
#include <oox/core/xmlfilterbase.hxx>
#include <rtl/ref.hxx>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/graphic/XGraphicMapper.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/frame/XModel.hpp>

namespace oox::vml { class DrawingFragment; }

namespace oox::shape {

class LockedCanvasContext;
class ShapeFilterBase;
class WordprocessingCanvasContext;
class WpgContext;
class WpsContext;

class ShapeFragmentHandler final : public core::FragmentHandler2
{
public:
    typedef rtl::Reference<ShapeFragmentHandler> Pointer_t;

    explicit ShapeFragmentHandler(core::XmlFilterBase& rFilter,
                                  const OUString& rFragmentPath )
    : FragmentHandler2(rFilter, rFragmentPath)
    {
    }
};

class OOX_DLLPUBLIC ShapeContextHandler final :
    public ::cppu::WeakImplHelper< css::xml::sax::XFastContextHandler >
{
public:
    explicit ShapeContextHandler(rtl::Reference<ShapeFilterBase> xFilterBase);

    virtual ~ShapeContextHandler() override;

    // css::xml::sax::XFastContextHandler:
    virtual void SAL_CALL startFastElement
    (::sal_Int32 Element,
     const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL startUnknownElement
    (const OUString & Namespace,
     const OUString & Name,
     const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL endFastElement(::sal_Int32 Element) override;

    virtual void SAL_CALL endUnknownElement
    (const OUString & Namespace,
     const OUString & Name) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL
    createFastChildContext
    (::sal_Int32 Element,
     const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL
    createUnknownChildContext
    (const OUString & Namespace,
     const OUString & Name,
     const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL characters(const OUString & aChars) override;

    css::uno::Reference< css::drawing::XShape > getShape();

    void setDrawPage(const css::uno::Reference< css::drawing::XDrawPage > & the_value);

    void setModel(const css::uno::Reference< css::frame::XModel > & the_value);

    void setRelationFragmentPath(const OUString & the_value);

    sal_Int32 getStartToken() const;
    void popStartToken();
    void pushStartToken( sal_Int32 _starttoken );

    void setPosition(const css::awt::Point& rPosition);
    void setSize(const css::awt::Size& rSize);

    const bool& getFullWPGSupport() { return m_bFullWPGSUpport; }
    void setFullWPGSupport(bool bUse) { m_bFullWPGSUpport = bUse; }

    bool isWordProcessingGroupShape() const { return mxWpgContext ? true : false; }
    bool isWordprocessingCanvas() const { return mxWordprocessingCanvasContext ? true : false; }

    void setDocumentProperties(const css::uno::Reference<css::document::XDocumentProperties>& xDocProps);
    void setMediaDescriptor(const css::uno::Sequence<css::beans::PropertyValue>& rMediaDescriptor);

    void setGraphicMapper(css::uno::Reference<css::graphic::XGraphicMapper> const & rGraphicMapper);

    void setTheme(const oox::drawingml::ThemePtr& pTheme) { mpThemePtr = pTheme; }
    const oox::drawingml::ThemePtr& getTheme() const { return mpThemePtr; }

private:
    ShapeContextHandler(ShapeContextHandler const &) = delete;
    void operator =(ShapeContextHandler const &) = delete;

    // Special stack which always has at least one element.
    // In case of group shapes with embedded content it will have more element than one.
    std::stack<sal_uInt32> mnStartTokenStack;

    css::awt::Point maPosition;
    css::awt::Size maSize;  // from cx and cy, in EMU
    bool m_bFullWPGSUpport; // Is this DrawingML shape supposed to be processed as WPG?

    drawingml::ShapePtr mpShape;
    std::shared_ptr< vml::Drawing > mpDrawing;

    typedef rtl::Reference<drawingml::GraphicShapeContext>
    GraphicShapeContextPtr;
    rtl::Reference<vml::DrawingFragment> mxDrawingFragmentHandler;
    css::uno::Reference<XFastContextHandler> mxGraphicShapeContext;
    rtl::Reference<drawingml::DiagramGraphicDataContext> mxDiagramShapeContext;
    rtl::Reference<LockedCanvasContext> mxLockedCanvasContext;
    rtl::Reference<WordprocessingCanvasContext> mxWordprocessingCanvasContext;
    rtl::Reference<WpsContext> mxWpsContext;
    css::uno::Reference<css::drawing::XShape> mxSavedShape;
    rtl::Reference<WpgContext> mxWpgContext;
    rtl::Reference<drawingml::ChartGraphicDataContext> mxChartShapeContext;
    css::uno::Reference<css::document::XDocumentProperties> mxDocumentProperties;
    css::uno::Sequence<css::beans::PropertyValue> maMediaDescriptor;

    ::rtl::Reference< ShapeFilterBase > mxShapeFilterBase;
    drawingml::ThemePtr mpThemePtr;
    css::uno::Reference<css::drawing::XDrawPage> mxDrawPage;
    OUString msRelationFragmentPath;

    css::uno::Reference<XFastContextHandler> const & getGraphicShapeContext(::sal_Int32 Element);
    css::uno::Reference<XFastContextHandler> getChartShapeContext(::sal_Int32 Element);
    css::uno::Reference<XFastContextHandler> getDrawingShapeContext();
    css::uno::Reference<XFastContextHandler> getDiagramShapeContext();
    css::uno::Reference<XFastContextHandler> getLockedCanvasContext(sal_Int32 nElement);
    css::uno::Reference<XFastContextHandler> getWordprocessingCanvasContext(sal_Int32 nElement);
    css::uno::Reference<XFastContextHandler> getWpsContext(sal_Int32 nStartElement, sal_Int32 nElement);
    css::uno::Reference<XFastContextHandler> getWpgContext(sal_Int32 nElement);
    css::uno::Reference<XFastContextHandler> getContextHandler(sal_Int32 nElement = 0);

    void applyFontRefColor(const oox::drawingml::ShapePtr& pShape,
                           const oox::drawingml::Color& rFontRefColor);
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

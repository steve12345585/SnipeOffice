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

#include "vclprocessor2d.hxx"
#include <vcl/outdev.hxx>

#include <memory>

namespace drawinglayer::primitive2d
{
class PolyPolygonColorPrimitive2D;
class PolygonHairlinePrimitive2D;
class PolygonStrokePrimitive2D;
class BitmapPrimitive2D;
class PolyPolygonGradientPrimitive2D;
class UnifiedTransparencePrimitive2D;
class ControlPrimitive2D;
class FillHatchPrimitive2D;
class BackgroundColorPrimitive2D;
class BorderLinePrimitive2D;
class FillGradientPrimitive2D;
class PatternFillPrimitive2D;
}

namespace drawinglayer::processor2d
{
/** VclPixelProcessor2D class

    This processor derived from VclProcessor2D is the base class for rendering
    all fed primitives to a VCL Window. It is the currently used renderer
    for all VCL editing output from the DrawingLayer.
 */
class VclPixelProcessor2D final : public VclProcessor2D
{
    AntialiasingFlags m_nOrigAntiAliasing;

    bool m_bRenderSimpleTextDirect;
    bool m_bRenderDecoratedTextDirect;

    /*  the local processor for BasePrimitive2D-Implementation based primitives,
        called from the common process()-implementation
     */
    virtual void processBasePrimitive2D(const primitive2d::BasePrimitive2D& rCandidate) override;

    // some helpers to try direct paints (shortcuts)
    void tryDrawPolyPolygonColorPrimitive2DDirect(
        const primitive2d::PolyPolygonColorPrimitive2D& rSource, double fTransparency);
    bool
    tryDrawPolygonHairlinePrimitive2DDirect(const primitive2d::PolygonHairlinePrimitive2D& rSource,
                                            double fTransparency);
    bool tryDrawPolygonStrokePrimitive2DDirect(const primitive2d::PolygonStrokePrimitive2D& rSource,
                                               double fTransparency);

    void processTextSimplePortionPrimitive2D(
        const primitive2d::TextSimplePortionPrimitive2D& rCandidate);
    void processTextDecoratedPortionPrimitive2D(
        const primitive2d::TextSimplePortionPrimitive2D& rCandidate);
    void processPolygonHairlinePrimitive2D(
        const primitive2d::PolygonHairlinePrimitive2D& rPolygonHairlinePrimitive2D);
    void processBitmapPrimitive2D(const primitive2d::BitmapPrimitive2D& rBitmapCandidate);
    void processPolyPolygonGradientPrimitive2D(
        const primitive2d::PolyPolygonGradientPrimitive2D& rPolygonCandidate);
    void processPolyPolygonColorPrimitive2D(
        const primitive2d::PolyPolygonColorPrimitive2D& rPolyPolygonColorPrimitive2D);
    void processUnifiedTransparencePrimitive2D(
        const primitive2d::UnifiedTransparencePrimitive2D& rUniTransparenceCandidate);
    void processControlPrimitive2D(const primitive2d::ControlPrimitive2D& rControlPrimitive);
    void processPolygonStrokePrimitive2D(
        const primitive2d::PolygonStrokePrimitive2D& rPolygonStrokePrimitive2D);
    void processFillHatchPrimitive2D(const primitive2d::FillHatchPrimitive2D& rFillHatchPrimitive);
    void
    processBackgroundColorPrimitive2D(const primitive2d::BackgroundColorPrimitive2D& rPrimitive);
    void
    processBorderLinePrimitive2D(const drawinglayer::primitive2d::BorderLinePrimitive2D& rBorder);
    void processInvertPrimitive2D(const primitive2d::BasePrimitive2D& rCandidate);
    void processMetaFilePrimitive2D(const primitive2d::BasePrimitive2D& rCandidate);
    void processFillGradientPrimitive2D(const primitive2d::FillGradientPrimitive2D& rPrimitive);
    void processPatternFillPrimitive2D(const primitive2d::PatternFillPrimitive2D& rPrimitive);

public:
    /// constructor/destructor
    VclPixelProcessor2D(const geometry::ViewInformation2D& rViewInformation, OutputDevice& rOutDev);
    virtual ~VclPixelProcessor2D() override;
};
} // end of namespace drawinglayer::processor2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

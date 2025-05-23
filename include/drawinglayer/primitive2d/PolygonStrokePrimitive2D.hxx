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

#include <drawinglayer/primitive2d/BufferedDecompositionPrimitive2D.hxx>
#include <drawinglayer/attribute/lineattribute.hxx>
#include <drawinglayer/attribute/strokeattribute.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>

namespace drawinglayer::primitive2d
{
/** PolygonStrokePrimitive2D class

    This primitive defines a line with line width, line join, line color
    and stroke attributes. It will be decomposed dependent on the definition
    to the needed primitives, e.g. filled PolyPolygons for fat lines.
 */
class DRAWINGLAYER_DLLPUBLIC PolygonStrokePrimitive2D : public BufferedDecompositionPrimitive2D
{
private:
    /// the line geometry
    basegfx::B2DPolygon maPolygon;

    /// the line attributes like width, join and color
    attribute::LineAttribute maLineAttribute;

    /// the line stroking (if used)
    attribute::StrokeAttribute maStrokeAttribute;

    /// the buffered result of PolygonStrokePrimitive2D::getB2DRange
    mutable basegfx::B2DRange maBufferedRange;

protected:
    /// local decomposition.
    virtual Primitive2DReference
    create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

public:
    /// constructor
    PolygonStrokePrimitive2D(basegfx::B2DPolygon aPolygon,
                             const attribute::LineAttribute& rLineAttribute,
                             attribute::StrokeAttribute aStrokeAttribute);

    /// constructor without stroking
    PolygonStrokePrimitive2D(basegfx::B2DPolygon aPolygon,
                             const attribute::LineAttribute& rLineAttribute);

    /// data read access
    const basegfx::B2DPolygon& getB2DPolygon() const { return maPolygon; }
    const attribute::LineAttribute& getLineAttribute() const { return maLineAttribute; }
    const attribute::StrokeAttribute& getStrokeAttribute() const { return maStrokeAttribute; }

    /// compare operator
    virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

    /// get range
    virtual basegfx::B2DRange
    getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

    /// provide unique ID
    virtual sal_uInt32 getPrimitive2DID() const override;
};

} // end of namespace primitive2d::drawinglayer

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

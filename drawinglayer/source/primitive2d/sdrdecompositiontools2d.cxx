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

#include <drawinglayer/primitive2d/sdrdecompositiontools2d.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>
#include <drawinglayer/primitive2d/PolyPolygonHairlinePrimitive2D.hxx>
#include <drawinglayer/primitive2d/hiddengeometryprimitive2d.hxx>


namespace drawinglayer::primitive2d
{
        Primitive2DReference createHiddenGeometryPrimitives2D(
            const basegfx::B2DHomMatrix& rMatrix)
        {
            const basegfx::B2DPolygon& aUnitOutline(basegfx::utils::createUnitPolygon());

            return createHiddenGeometryPrimitives2D(
                false/*bFilled*/,
                basegfx::B2DPolyPolygon(aUnitOutline),
                rMatrix);
        }

        Primitive2DReference createHiddenGeometryPrimitives2D(
            const basegfx::B2DPolyPolygon& rPolyPolygon)
        {
            return createHiddenGeometryPrimitives2D(
                false/*bFilled*/,
                rPolyPolygon,
                basegfx::B2DHomMatrix());
        }

        Primitive2DReference createHiddenGeometryPrimitives2D(
            bool bFilled,
            const basegfx::B2DRange& rRange)
        {
            return createHiddenGeometryPrimitives2D(
                bFilled,
                rRange,
                basegfx::B2DHomMatrix());
        }

        Primitive2DReference createHiddenGeometryPrimitives2D(
            bool bFilled,
            const basegfx::B2DRange& rRange,
            const basegfx::B2DHomMatrix& rMatrix)
        {
            const basegfx::B2DPolyPolygon aOutline(basegfx::utils::createPolygonFromRect(rRange));

            return createHiddenGeometryPrimitives2D(
                bFilled,
                aOutline,
                rMatrix);
        }

        Primitive2DReference createHiddenGeometryPrimitives2D(
            bool bFilled,
            const basegfx::B2DPolyPolygon& rPolyPolygon,
            const basegfx::B2DHomMatrix& rMatrix)
        {
            // create fill or line primitive
            Primitive2DReference xReference;
            basegfx::B2DPolyPolygon aScaledOutline(rPolyPolygon);
            aScaledOutline.transform(rMatrix);

            if(bFilled)
            {
                xReference = new PolyPolygonColorPrimitive2D(
                    std::move(aScaledOutline),
                    basegfx::BColor(0.0, 0.0, 0.0));
            }
            else
            {
                const basegfx::BColor aGrayTone(0xc0 / 255.0, 0xc0 / 255.0, 0xc0 / 255.0);

                xReference = new PolyPolygonHairlinePrimitive2D(
                    std::move(aScaledOutline),
                    aGrayTone);
            }

            // create HiddenGeometryPrimitive2D
            return new HiddenGeometryPrimitive2D(Primitive2DContainer { xReference });
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

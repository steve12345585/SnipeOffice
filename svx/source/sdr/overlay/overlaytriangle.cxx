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

#include <sdr/overlay/overlaytriangle.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>


namespace sdr::overlay
{
        drawinglayer::primitive2d::Primitive2DContainer OverlayTriangle::createOverlayObjectPrimitive2DSequence()
        {
            basegfx::B2DPolygon aPolygon;

            aPolygon.append(getBasePosition());
            aPolygon.append(maSecondPosition);
            aPolygon.append(maThirdPosition);
            aPolygon.setClosed(true);

            const drawinglayer::primitive2d::Primitive2DReference aReference(
                new drawinglayer::primitive2d::PolyPolygonColorPrimitive2D(
                    basegfx::B2DPolyPolygon(aPolygon),
                    getBaseColor().getBColor()));

            return drawinglayer::primitive2d::Primitive2DContainer { aReference };
        }

        OverlayTriangle::OverlayTriangle(
            const basegfx::B2DPoint& rBasePos,
            const basegfx::B2DPoint& rSecondPos,
            const basegfx::B2DPoint& rThirdPos,
            Color aTriangleColor)
        :   OverlayObjectWithBasePosition(rBasePos, aTriangleColor),
            maSecondPosition(rSecondPos),
            maThirdPosition(rThirdPos)
        {
        }

        OverlayTriangle::~OverlayTriangle()
        {
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

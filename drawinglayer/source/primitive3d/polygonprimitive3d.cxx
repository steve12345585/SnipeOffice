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

#include <drawinglayer/primitive3d/polygonprimitive3d.hxx>
#include <basegfx/polygon/b3dpolygontools.hxx>
#include <basegfx/polygon/b3dpolypolygon.hxx>
#include <primitive3d/polygontubeprimitive3d.hxx>
#include <drawinglayer/primitive3d/drawinglayer_primitivetypes3d.hxx>
#include <utility>


using namespace com::sun::star;


namespace drawinglayer::primitive3d
{
        PolygonHairlinePrimitive3D::PolygonHairlinePrimitive3D(
            basegfx::B3DPolygon aPolygon,
            const basegfx::BColor& rBColor)
        :   maPolygon(std::move(aPolygon)),
            maBColor(rBColor)
        {
        }

        bool PolygonHairlinePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(BasePrimitive3D::operator==(rPrimitive))
            {
                const PolygonHairlinePrimitive3D& rCompare = static_cast<const PolygonHairlinePrimitive3D&>(rPrimitive);

                return (getB3DPolygon() == rCompare.getB3DPolygon()
                    && getBColor() == rCompare.getBColor());
            }

            return false;
        }

        basegfx::B3DRange PolygonHairlinePrimitive3D::getB3DRange(const geometry::ViewInformation3D& /*rViewInformation*/) const
        {
            return basegfx::utils::getRange(getB3DPolygon());
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(PolygonHairlinePrimitive3D, PRIMITIVE3D_ID_POLYGONHAIRLINEPRIMITIVE3D)



        Primitive3DContainer PolygonStrokePrimitive3D::create3DDecomposition(const geometry::ViewInformation3D& /*rViewInformation*/) const
        {
            Primitive3DContainer aRetval;

            if(getB3DPolygon().count())
            {
                basegfx::B3DPolyPolygon aHairLinePolyPolygon;

                if(0.0 == getStrokeAttribute().getFullDotDashLen())
                {
                    aHairLinePolyPolygon = basegfx::B3DPolyPolygon(getB3DPolygon());
                }
                else
                {
                    // apply LineStyle
                    basegfx::utils::applyLineDashing(getB3DPolygon(), getStrokeAttribute().getDotDashArray(), &aHairLinePolyPolygon, getStrokeAttribute().getFullDotDashLen());
                }

                // prepare result
                aRetval.resize(aHairLinePolyPolygon.count());

                if(getLineAttribute().getWidth())
                {
                    // create fat line data
                    const double fRadius(getLineAttribute().getWidth() / 2.0);
                    const basegfx::B2DLineJoin aLineJoin(getLineAttribute().getLineJoin());
                    const css::drawing::LineCap aLineCap(getLineAttribute().getLineCap());

                    for(sal_uInt32 a(0); a < aHairLinePolyPolygon.count(); a++)
                    {
                        // create tube primitives
                        const Primitive3DReference xRef(
                            new PolygonTubePrimitive3D(
                                aHairLinePolyPolygon.getB3DPolygon(a),
                                getLineAttribute().getColor(),
                                fRadius,
                                aLineJoin,
                                aLineCap));
                        aRetval[a] = xRef;
                    }
                }
                else
                {
                    // create hair line data for all sub polygons
                    for(sal_uInt32 a(0); a < aHairLinePolyPolygon.count(); a++)
                    {
                        const basegfx::B3DPolygon& aCandidate = aHairLinePolyPolygon.getB3DPolygon(a);
                        const Primitive3DReference xRef(new PolygonHairlinePrimitive3D(aCandidate, getLineAttribute().getColor()));
                        aRetval[a] = xRef;
                    }
                }
            }

            return aRetval;
        }

        PolygonStrokePrimitive3D::PolygonStrokePrimitive3D(
            basegfx::B3DPolygon aPolygon,
            const attribute::LineAttribute& rLineAttribute,
            attribute::StrokeAttribute aStrokeAttribute)
        :   maPolygon(std::move(aPolygon)),
            maLineAttribute(rLineAttribute),
            maStrokeAttribute(std::move(aStrokeAttribute))
        {
        }

        bool PolygonStrokePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(BufferedDecompositionPrimitive3D::operator==(rPrimitive))
            {
                const PolygonStrokePrimitive3D& rCompare = static_cast<const PolygonStrokePrimitive3D&>(rPrimitive);

                return (getB3DPolygon() == rCompare.getB3DPolygon()
                    && getLineAttribute() == rCompare.getLineAttribute()
                    && getStrokeAttribute() == rCompare.getStrokeAttribute());
            }

            return false;
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(PolygonStrokePrimitive3D, PRIMITIVE3D_ID_POLYGONSTROKEPRIMITIVE3D)

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

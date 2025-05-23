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

#include <drawinglayer/primitive2d/mediaprimitive2d.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>
#include <utility>
#include <vcl/GraphicObject.hxx>
#include <drawinglayer/primitive2d/graphicprimitive2d.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/hiddengeometryprimitive2d.hxx>


namespace drawinglayer::primitive2d
{
        Primitive2DReference MediaPrimitive2D::create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const
        {
            Primitive2DContainer xRetval;
            xRetval.resize(1);

            // create background object
            basegfx::B2DPolygon aBackgroundPolygon(basegfx::utils::createUnitPolygon());
            aBackgroundPolygon.transform(getTransform());
            const Primitive2DReference xRefBackground(
                new PolyPolygonColorPrimitive2D(
                    basegfx::B2DPolyPolygon(aBackgroundPolygon),
                    getBackgroundColor()));
            xRetval[0] = xRefBackground;

            if(GraphicType::Bitmap == maSnapshot.GetType() || GraphicType::GdiMetafile == maSnapshot.GetType())
            {
                const GraphicObject aGraphicObject(maSnapshot);
                const GraphicAttr aGraphicAttr;
                xRetval.resize(2);
                xRetval[1] = new GraphicPrimitive2D(getTransform(), aGraphicObject, aGraphicAttr);
            }

            if(getDiscreteBorder())
            {
                const basegfx::B2DVector aDiscreteInLogic(rViewInformation.getInverseObjectToViewTransformation() *
                    basegfx::B2DVector(static_cast<double>(getDiscreteBorder()), static_cast<double>(getDiscreteBorder())));
                const double fDiscreteSize(aDiscreteInLogic.getX() + aDiscreteInLogic.getY());

                basegfx::B2DRange aSourceRange(0.0, 0.0, 1.0, 1.0);
                aSourceRange.transform(getTransform());

                basegfx::B2DRange aDestRange(aSourceRange);
                aDestRange.grow(-0.5 * fDiscreteSize);

                if(basegfx::fTools::equalZero(aDestRange.getWidth()) || basegfx::fTools::equalZero(aDestRange.getHeight()))
                {
                    // shrunk primitive has no content (zero size in X or Y), nothing to display. Still create
                    // invisible content for HitTest and BoundRect
                    const Primitive2DReference xHiddenLines(new HiddenGeometryPrimitive2D(std::move(xRetval)));

                    xRetval = Primitive2DContainer { xHiddenLines, };
                }
                else
                {
                    // create transformation matrix from original range to shrunk range
                    basegfx::B2DHomMatrix aTransform;
                    aTransform.translate(-aSourceRange.getMinX(), -aSourceRange.getMinY());
                    aTransform.scale(aDestRange.getWidth() / aSourceRange.getWidth(), aDestRange.getHeight() / aSourceRange.getHeight());
                    aTransform.translate(aDestRange.getMinX(), aDestRange.getMinY());

                    // add transform primitive
                    xRetval = Primitive2DContainer {
                        new TransformPrimitive2D(aTransform, std::move(xRetval)) // Scaled
                    };
                }
            }

            return new GroupPrimitive2D(std::move(xRetval));
        }

        MediaPrimitive2D::MediaPrimitive2D(
            basegfx::B2DHomMatrix aTransform,
            OUString aURL,
            const basegfx::BColor& rBackgroundColor,
            sal_uInt32 nDiscreteBorder,
            Graphic aSnapshot)
        :   maTransform(std::move(aTransform)),
            maURL(std::move(aURL)),
            maBackgroundColor(rBackgroundColor),
            mnDiscreteBorder(nDiscreteBorder),
            maSnapshot(std::move(aSnapshot))
        {
        }

        bool MediaPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(BufferedDecompositionPrimitive2D::operator==(rPrimitive))
            {
                const MediaPrimitive2D& rCompare = static_cast<const MediaPrimitive2D&>(rPrimitive);

                return (getTransform() == rCompare.getTransform()
                    && maURL == rCompare.maURL
                    && getBackgroundColor() == rCompare.getBackgroundColor()
                    && getDiscreteBorder() == rCompare.getDiscreteBorder()
                    && maSnapshot.IsNone() == rCompare.maSnapshot.IsNone());
            }

            return false;
        }

        basegfx::B2DRange MediaPrimitive2D::getB2DRange(const geometry::ViewInformation2D& rViewInformation) const
        {
            basegfx::B2DRange aRetval(0.0, 0.0, 1.0, 1.0);
            aRetval.transform(getTransform());

            if(getDiscreteBorder())
            {
                const basegfx::B2DVector aDiscreteInLogic(rViewInformation.getInverseObjectToViewTransformation() *
                    basegfx::B2DVector(static_cast<double>(getDiscreteBorder()), static_cast<double>(getDiscreteBorder())));
                const double fDiscreteSize(aDiscreteInLogic.getX() + aDiscreteInLogic.getY());

                aRetval.grow(-0.5 * fDiscreteSize);
            }

            return aRetval;
        }

        // provide unique ID
        sal_uInt32 MediaPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_MEDIAPRIMITIVE2D;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

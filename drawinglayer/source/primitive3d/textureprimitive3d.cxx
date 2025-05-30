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

#include <primitive3d/textureprimitive3d.hxx>
#include <drawinglayer/primitive3d/drawinglayer_primitivetypes3d.hxx>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/utils/gradienttools.hxx>
#include <utility>


using namespace com::sun::star;


namespace drawinglayer::primitive3d
{
        TexturePrimitive3D::TexturePrimitive3D(
            const Primitive3DContainer& rChildren,
            const basegfx::B2DVector& rTextureSize,
            bool bModulate, bool bFilter)
        :   GroupPrimitive3D(rChildren),
            maTextureSize(rTextureSize),
            mbModulate(bModulate),
            mbFilter(bFilter)
        {
        }

        bool TexturePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(GroupPrimitive3D::operator==(rPrimitive))
            {
                const TexturePrimitive3D& rCompare = static_cast<const TexturePrimitive3D&>(rPrimitive);

                return (getModulate() == rCompare.getModulate()
                    && getFilter() == rCompare.getFilter());
            }

            return false;
        }



        UnifiedTransparenceTexturePrimitive3D::UnifiedTransparenceTexturePrimitive3D(
            double fTransparence,
            const Primitive3DContainer& rChildren)
        :   TexturePrimitive3D(rChildren, basegfx::B2DVector(), false, false),
            mfTransparence(fTransparence)
        {
        }

        bool UnifiedTransparenceTexturePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(TexturePrimitive3D::operator==(rPrimitive))
            {
                const UnifiedTransparenceTexturePrimitive3D& rCompare = static_cast<const UnifiedTransparenceTexturePrimitive3D&>(rPrimitive);

                return (getTransparence() == rCompare.getTransparence());
            }

            return false;
        }

        basegfx::B3DRange UnifiedTransparenceTexturePrimitive3D::getB3DRange(const geometry::ViewInformation3D& rViewInformation) const
        {
            // do not use the fallback to decomposition here since for a correct BoundRect we also
            // need invisible (1.0 == getTransparence()) geometry; these would be deleted in the decomposition
            return getChildren().getB3DRange(rViewInformation);
        }

        Primitive3DContainer UnifiedTransparenceTexturePrimitive3D::get3DDecomposition(const geometry::ViewInformation3D& /*rViewInformation*/) const
        {
            if(0.0 == getTransparence())
            {
                // no transparence used, so just use content
                return getChildren();
            }
            else if(getTransparence() > 0.0 && getTransparence() < 1.0)
            {
                // create TransparenceTexturePrimitive3D with fixed transparence as replacement
                const basegfx::BColor aGray(getTransparence(), getTransparence(), getTransparence());

                // create ColorStops with StartColor == EndColor == aGray
                const basegfx::BColorStops aColorStops {
                    basegfx::BColorStop(0.0, aGray),
                    basegfx::BColorStop(1.0, aGray) };

                const attribute::FillGradientAttribute aFillGradient(css::awt::GradientStyle_LINEAR, 0.0, 0.0, 0.0, 0.0, aColorStops);
                const Primitive3DReference xRef(new TransparenceTexturePrimitive3D(aFillGradient, getChildren(), getTextureSize()));
                return { xRef };
            }
            else
            {
                // completely transparent or invalid definition, add nothing
                return Primitive3DContainer();
            }
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(UnifiedTransparenceTexturePrimitive3D, PRIMITIVE3D_ID_UNIFIEDTRANSPARENCETEXTUREPRIMITIVE3D)



        GradientTexturePrimitive3D::GradientTexturePrimitive3D(
            attribute::FillGradientAttribute aGradient,
            const Primitive3DContainer& rChildren,
            const basegfx::B2DVector& rTextureSize,
            bool bModulate,
            bool bFilter)
        :   TexturePrimitive3D(rChildren, rTextureSize, bModulate, bFilter),
            maGradient(std::move(aGradient))
        {
        }

        bool GradientTexturePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(TexturePrimitive3D::operator==(rPrimitive))
            {
                const GradientTexturePrimitive3D& rCompare = static_cast<const GradientTexturePrimitive3D&>(rPrimitive);

                return (getGradient() == rCompare.getGradient());
            }

            return false;
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(GradientTexturePrimitive3D, PRIMITIVE3D_ID_GRADIENTTEXTUREPRIMITIVE3D)



        BitmapTexturePrimitive3D::BitmapTexturePrimitive3D(
            const attribute::FillGraphicAttribute& rFillGraphicAttribute,
            const Primitive3DContainer& rChildren,
            const basegfx::B2DVector& rTextureSize,
            bool bModulate, bool bFilter)
        :   TexturePrimitive3D(rChildren, rTextureSize, bModulate, bFilter),
            maFillGraphicAttribute(rFillGraphicAttribute)
        {
        }

        bool BitmapTexturePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            if(TexturePrimitive3D::operator==(rPrimitive))
            {
                const BitmapTexturePrimitive3D& rCompare = static_cast<const BitmapTexturePrimitive3D&>(rPrimitive);

                return (getFillGraphicAttribute() == rCompare.getFillGraphicAttribute());
            }

            return false;
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(BitmapTexturePrimitive3D, PRIMITIVE3D_ID_BITMAPTEXTUREPRIMITIVE3D)



        TransparenceTexturePrimitive3D::TransparenceTexturePrimitive3D(
            const attribute::FillGradientAttribute& rGradient,
            const Primitive3DContainer& rChildren,
            const basegfx::B2DVector& rTextureSize)
        :   GradientTexturePrimitive3D(rGradient, rChildren, rTextureSize, false, false)
        {
        }

        bool TransparenceTexturePrimitive3D::operator==(const BasePrimitive3D& rPrimitive) const
        {
            return GradientTexturePrimitive3D::operator==(rPrimitive);
        }

        // provide unique ID
        ImplPrimitive3DIDBlock(TransparenceTexturePrimitive3D, PRIMITIVE3D_ID_TRANSPARENCETEXTUREPRIMITIVE3D)

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

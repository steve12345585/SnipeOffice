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
#include <svx/sdr/attribute/sdrallfillattributeshelper.hxx>
#include <sdr/primitive2d/sdrattributecreator.hxx>
#include <sdr/primitive2d/sdrdecompositiontools.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/utils/bgradient.hxx>
#include <drawinglayer/attribute/fillhatchattribute.hxx>
#include <drawinglayer/attribute/sdrfillgraphicattribute.hxx>
#include <vcl/graph.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer::attribute
{
        void SdrAllFillAttributesHelper::createPrimitive2DSequence(
            const basegfx::B2DRange& rPaintRange,
            const basegfx::B2DRange& rDefineRange)
        {
            // reset and remember new target range for object geometry
            maLastPaintRange = rPaintRange;
            maLastDefineRange = rDefineRange;

            if(isUsed())
            {
                maPrimitives = drawinglayer::primitive2d::Primitive2DContainer {
                    drawinglayer::primitive2d::createPolyPolygonFillPrimitive(
                        basegfx::B2DPolyPolygon(
                            basegfx::utils::createPolygonFromRect(
                                maLastPaintRange)),
                            maLastDefineRange,
                        maFillAttribute ? *maFillAttribute : drawinglayer::attribute::SdrFillAttribute(),
                        maFillGradientAttribute ? *maFillGradientAttribute : drawinglayer::attribute::FillGradientAttribute()) };
            }
        }

        SdrAllFillAttributesHelper::SdrAllFillAttributesHelper(const Color& rColor)
        {
            maFillAttribute = drawinglayer::attribute::SdrFillAttribute(
                    0.0,
                    rColor.GetRGBColor().getBColor(),
                    drawinglayer::attribute::FillGradientAttribute(),
                    drawinglayer::attribute::FillHatchAttribute(),
                    drawinglayer::attribute::SdrFillGraphicAttribute());
        }

        SdrAllFillAttributesHelper::SdrAllFillAttributesHelper(const SfxItemSet& rSet)
        :   maFillAttribute(
                    drawinglayer::primitive2d::createNewSdrFillAttribute(rSet)),
            maFillGradientAttribute(
                    drawinglayer::primitive2d::createNewTransparenceGradientAttribute(rSet))
        {
        }

        SdrAllFillAttributesHelper::~SdrAllFillAttributesHelper()
        {
        }

        bool SdrAllFillAttributesHelper::isUsed() const
        {
            // only depends on fill, FillGradientAttribute alone defines no fill
            return maFillAttribute && !maFillAttribute->isDefault();
        }

        bool SdrAllFillAttributesHelper::isTransparent() const
        {
            if(hasSdrFillAttribute() && 0.0 != maFillAttribute->getTransparence())
            {
                return true;
            }

            if(maFillGradientAttribute && !maFillGradientAttribute->isDefault())
            {
                return true;
            }

            if(hasSdrFillAttribute())
            {
                const Graphic& rGraphic = getFillAttribute().getFillGraphic().getFillGraphic();

                return rGraphic.IsSupportedGraphic() && rGraphic.IsTransparent();
            }

            return false;
        }

        const drawinglayer::attribute::SdrFillAttribute& SdrAllFillAttributesHelper::getFillAttribute() const
        {
            if(!maFillAttribute)
            {
                const_cast< SdrAllFillAttributesHelper* >(this)->maFillAttribute.emplace();
            }

            return *maFillAttribute;
        }

        const drawinglayer::attribute::FillGradientAttribute& SdrAllFillAttributesHelper::getFillGradientAttribute() const
        {
            if(!maFillGradientAttribute)
            {
                const_cast< SdrAllFillAttributesHelper* >(this)->maFillGradientAttribute.emplace();
            }

            return *maFillGradientAttribute;
        }

        const drawinglayer::primitive2d::Primitive2DContainer& SdrAllFillAttributesHelper::getPrimitive2DSequence(
            const basegfx::B2DRange& rPaintRange,
            const basegfx::B2DRange& rDefineRange) const
        {
            if(!maPrimitives.empty() && (maLastPaintRange != rPaintRange || maLastDefineRange != rDefineRange))
            {
                const_cast< SdrAllFillAttributesHelper* >(this)->maPrimitives.clear();
            }

            if(maPrimitives.empty())
            {
                const_cast< SdrAllFillAttributesHelper* >(this)->createPrimitive2DSequence(rPaintRange, rDefineRange);
            }

            return maPrimitives;
        }

        basegfx::BColor SdrAllFillAttributesHelper::getAverageColor(const basegfx::BColor& rFallback) const
        {
            basegfx::BColor aRetval(rFallback);

            if(maFillAttribute && !maFillAttribute->isDefault())
            {
                const drawinglayer::attribute::FillGradientAttribute& rFillGradientAttribute = maFillAttribute->getGradient();
                const drawinglayer::attribute::FillHatchAttribute& rFillHatchAttribute = maFillAttribute->getHatch();
                const drawinglayer::attribute::SdrFillGraphicAttribute& rSdrFillGraphicAttribute = maFillAttribute->getFillGraphic();
                const drawinglayer::attribute::FillGradientAttribute& rFillTransparenceGradientAttribute = getFillGradientAttribute();
                double fTransparence(maFillAttribute->getTransparence());

                if(!rFillTransparenceGradientAttribute.isDefault())
                {
                    const double fTransA(rFillTransparenceGradientAttribute.getColorStops().front().getStopColor().luminance());
                    const double fTransB(rFillTransparenceGradientAttribute.getColorStops().back().getStopColor().luminance());

                    fTransparence = (fTransA + fTransB) * 0.5;
                }

                if(!rFillGradientAttribute.isDefault())
                {
                    // gradient fill
                    const basegfx::BColor aStart(rFillGradientAttribute.getColorStops().front().getStopColor());
                    const basegfx::BColor aEnd(rFillGradientAttribute.getColorStops().back().getStopColor());

                    aRetval = basegfx::interpolate(aStart, aEnd, 0.5);
                }
                else if(!rFillHatchAttribute.isDefault())
                {
                    // hatch fill
                    const basegfx::BColor& rColor = rFillHatchAttribute.getColor();

                    if(rFillHatchAttribute.isFillBackground())
                    {
                        const basegfx::BColor& rBackgroundColor = maFillAttribute->getColor();

                        // mix colors 50%/50%
                        aRetval = basegfx::interpolate(rColor, rBackgroundColor, 0.5);
                    }
                    else
                    {
                        // mix color with fallback color
                        aRetval = basegfx::interpolate(rColor, rFallback, 0.5);
                    }
                }
                else if(!rSdrFillGraphicAttribute.isDefault())
                {
                    // graphic fill

                    // not used yet by purpose (see SwPageFrm::GetDrawBackgrdColor()),
                    // use fallback (already set)
                }
                else
                {
                    // color fill
                    aRetval = maFillAttribute->getColor();
                }

                if(!basegfx::fTools::equalZero(fTransparence))
                {
                    // blend into transparency
                    aRetval = basegfx::interpolate(aRetval, rFallback, fTransparence);
                }
            }

            return aRetval.clamp();
        }

        bool SdrAllFillAttributesHelper::needCompleteRepaint() const
        {
            if(!isUsed() || !hasSdrFillAttribute())
            {
                // not used or no fill
                return false;
            }

            const drawinglayer::attribute::SdrFillAttribute& rSdrFillAttribute = getFillAttribute();

            if(!rSdrFillAttribute.getHatch().isDefault())
            {
                // hatch is always top-left aligned, needs no full refreshes
                return false;
            }

            if(!rSdrFillAttribute.getGradient().isDefault())
            {
                // gradients always scale with the object
                return true;
            }

            if(!rSdrFillAttribute.getFillGraphic().isDefault())
            {
                // some graphic constellations may not need this, but since most do
                // (stretch to fill, all but top-left aligned, ...) claim to do by default
                return true;
            }

            // color fill
            return false;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

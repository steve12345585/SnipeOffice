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

#include <drawinglayer/attribute/sdrfillattribute.hxx>
#include <basegfx/color/bcolor.hxx>
#include <drawinglayer/attribute/sdrfillgraphicattribute.hxx>
#include <drawinglayer/attribute/fillhatchattribute.hxx>
#include <drawinglayer/attribute/fillgradientattribute.hxx>
#include <utility>


namespace drawinglayer::attribute
{
        class ImpSdrFillAttribute
        {
        public:
            // fill definitions
            double                              mfTransparence;     // [0.0 .. 1.0], 0.0==no transp.
            basegfx::BColor                     maColor;            // fill color
            FillGradientAttribute               maGradient;         // fill gradient (if used)
            FillHatchAttribute                  maHatch;            // fill hatch (if used)
            SdrFillGraphicAttribute             maFillGraphic;      // fill graphic (if used)

        public:
            ImpSdrFillAttribute(
                double fTransparence,
                const basegfx::BColor& rColor,
                FillGradientAttribute aGradient,
                FillHatchAttribute aHatch,
                SdrFillGraphicAttribute aFillGraphic)
            :   mfTransparence(fTransparence),
                maColor(rColor),
                maGradient(std::move(aGradient)),
                maHatch(std::move(aHatch)),
                maFillGraphic(std::move(aFillGraphic))
            {
            }

            ImpSdrFillAttribute()
            :   mfTransparence(0.0)
            {
            }

            // data read access
            double getTransparence() const { return mfTransparence; }
            const basegfx::BColor& getColor() const { return maColor; }
            const FillGradientAttribute& getGradient() const { return maGradient; }
            const FillHatchAttribute& getHatch() const { return maHatch; }
            const SdrFillGraphicAttribute& getFillGraphic() const { return maFillGraphic; }

            // compare operator
            bool operator==(const ImpSdrFillAttribute& rCandidate) const
            {
                return(getTransparence() == rCandidate.getTransparence()
                    && getColor() == rCandidate.getColor()
                    && getGradient() == rCandidate.getGradient()
                    && getHatch() == rCandidate.getHatch()
                    && getFillGraphic() == rCandidate.getFillGraphic());
            }
        };

        namespace
        {
            SdrFillAttribute::ImplType& theGlobalDefault()
            {
                static SdrFillAttribute::ImplType SINGLETON;
                return SINGLETON;
            }
            SdrFillAttribute::ImplType& slideBackgroundFillGlobalDefault()
            {
                static SdrFillAttribute::ImplType SINGLETON2;
                return SINGLETON2;
            }
        }

        SdrFillAttribute::SdrFillAttribute(
            double fTransparence,
            const basegfx::BColor& rColor,
            const FillGradientAttribute& rGradient,
            const FillHatchAttribute& rHatch,
            const SdrFillGraphicAttribute& rFillGraphic)
        :   mpSdrFillAttribute(ImpSdrFillAttribute(
                fTransparence, rColor, rGradient, rHatch, rFillGraphic))
        {
        }

        SdrFillAttribute::SdrFillAttribute(bool bSlideBackgroundFill)
        :   mpSdrFillAttribute(bSlideBackgroundFill
                ? slideBackgroundFillGlobalDefault()
                : theGlobalDefault())
        {
        }

        SdrFillAttribute::SdrFillAttribute(const SdrFillAttribute&) = default;

        SdrFillAttribute::SdrFillAttribute(SdrFillAttribute&&) = default;

        SdrFillAttribute::~SdrFillAttribute() = default;

        bool SdrFillAttribute::isDefault() const
        {
            return mpSdrFillAttribute.same_object(theGlobalDefault());
        }

        bool SdrFillAttribute::isSlideBackgroundFill() const
        {
            return mpSdrFillAttribute.same_object(slideBackgroundFillGlobalDefault());
        }

        SdrFillAttribute& SdrFillAttribute::operator=(const SdrFillAttribute&) = default;

        SdrFillAttribute& SdrFillAttribute::operator=(SdrFillAttribute&&) = default;

        bool SdrFillAttribute::operator==(const SdrFillAttribute& rCandidate) const
        {
            // tdf#87509 default attr is always != non-default attr, even with same values
            if(rCandidate.isDefault() != isDefault())
                return false;

            return rCandidate.mpSdrFillAttribute == mpSdrFillAttribute;
        }

        double SdrFillAttribute::getTransparence() const
        {
            return mpSdrFillAttribute->getTransparence();
        }

        const basegfx::BColor& SdrFillAttribute::getColor() const
        {
            return mpSdrFillAttribute->getColor();
        }

        const FillGradientAttribute& SdrFillAttribute::getGradient() const
        {
            return mpSdrFillAttribute->getGradient();
        }

        const FillHatchAttribute& SdrFillAttribute::getHatch() const
        {
            return mpSdrFillAttribute->getHatch();
        }

        const SdrFillGraphicAttribute& SdrFillAttribute::getFillGraphic() const
        {
            return mpSdrFillAttribute->getFillGraphic();
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

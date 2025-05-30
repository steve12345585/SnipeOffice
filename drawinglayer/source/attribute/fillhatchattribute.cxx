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

#include <drawinglayer/attribute/fillhatchattribute.hxx>
#include <basegfx/color/bcolor.hxx>


namespace drawinglayer::attribute
{
        class ImpFillHatchAttribute
        {
        public:
            // data definitions
            HatchStyle                              meStyle;
            double                                  mfDistance;
            double                                  mfAngle;
            basegfx::BColor                         maColor;
            sal_uInt32                              mnMinimalDiscreteDistance;

            bool                                    mbFillBackground : 1;

            ImpFillHatchAttribute(
                HatchStyle eStyle,
                double fDistance,
                double fAngle,
                const basegfx::BColor& rColor,
                sal_uInt32 nMinimalDiscreteDistance,
                bool bFillBackground)
            :   meStyle(eStyle),
                mfDistance(fDistance),
                mfAngle(fAngle),
                maColor(rColor),
                mnMinimalDiscreteDistance(nMinimalDiscreteDistance),
                mbFillBackground(bFillBackground)
            {
            }

            ImpFillHatchAttribute()
            :   meStyle(HatchStyle::Single),
                mfDistance(0.0),
                mfAngle(0.0),
                mnMinimalDiscreteDistance(3), // same as VCL
                mbFillBackground(false)
            {
            }

            // data read access
            HatchStyle getStyle() const { return meStyle; }
            double getDistance() const { return mfDistance; }
            double getAngle() const { return mfAngle; }
            const basegfx::BColor& getColor() const { return maColor; }
            sal_uInt32 getMinimalDiscreteDistance() const { return mnMinimalDiscreteDistance; }
            bool isFillBackground() const { return mbFillBackground; }

            bool operator==(const ImpFillHatchAttribute& rCandidate) const
            {
                return (getStyle() == rCandidate.getStyle()
                    && getDistance() == rCandidate.getDistance()
                    && getAngle() == rCandidate.getAngle()
                    && getColor() == rCandidate.getColor()
                    && getMinimalDiscreteDistance() == rCandidate.getMinimalDiscreteDistance()
                    && isFillBackground() == rCandidate.isFillBackground());
            }
        };

        namespace
        {
            FillHatchAttribute::ImplType& theGlobalDefault()
            {
                static FillHatchAttribute::ImplType SINGLETON;
                return SINGLETON;
            }
        }

        FillHatchAttribute::FillHatchAttribute(
            HatchStyle eStyle,
            double fDistance,
            double fAngle,
            const basegfx::BColor& rColor,
            sal_uInt32 nMinimalDiscreteDistance,
            bool bFillBackground)
        :   mpFillHatchAttribute(ImpFillHatchAttribute(
                eStyle, fDistance, fAngle, rColor,
                nMinimalDiscreteDistance, bFillBackground))
        {
        }

        FillHatchAttribute::FillHatchAttribute()
        :   mpFillHatchAttribute(theGlobalDefault())
        {
        }

        FillHatchAttribute::FillHatchAttribute(const FillHatchAttribute&) = default;

        FillHatchAttribute::FillHatchAttribute(FillHatchAttribute&&) = default;

        FillHatchAttribute::~FillHatchAttribute() = default;

        bool FillHatchAttribute::isDefault() const
        {
            return mpFillHatchAttribute.same_object(theGlobalDefault());
        }

        FillHatchAttribute& FillHatchAttribute::operator=(const FillHatchAttribute&) = default;

        FillHatchAttribute& FillHatchAttribute::operator=(FillHatchAttribute&&) = default;

        bool FillHatchAttribute::operator==(const FillHatchAttribute& rCandidate) const
        {
            // tdf#87509 default attr is always != non-default attr, even with same values
            if(rCandidate.isDefault() != isDefault())
                return false;

            return rCandidate.mpFillHatchAttribute == mpFillHatchAttribute;
        }

        // data read access
        HatchStyle FillHatchAttribute::getStyle() const
        {
            return mpFillHatchAttribute->getStyle();
        }

        double FillHatchAttribute::getDistance() const
        {
            return mpFillHatchAttribute->getDistance();
        }

        double FillHatchAttribute::getAngle() const
        {
            return mpFillHatchAttribute->getAngle();
        }

        const basegfx::BColor& FillHatchAttribute::getColor() const
        {
            return mpFillHatchAttribute->getColor();
        }

        sal_uInt32 FillHatchAttribute::getMinimalDiscreteDistance() const
        {
            return mpFillHatchAttribute->getMinimalDiscreteDistance();
        }

        bool FillHatchAttribute::isFillBackground() const
        {
            return mpFillHatchAttribute->isFillBackground();
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

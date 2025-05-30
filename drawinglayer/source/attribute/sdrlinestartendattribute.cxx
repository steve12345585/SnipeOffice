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

#include <drawinglayer/attribute/sdrlinestartendattribute.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <utility>


namespace drawinglayer::attribute
{
        class ImpSdrLineStartEndAttribute
        {
        public:
            // line arrow definitions
            basegfx::B2DPolyPolygon                 maStartPolyPolygon;     // start Line PolyPolygon
            basegfx::B2DPolyPolygon                 maEndPolyPolygon;       // end Line PolyPolygon
            double                                  mfStartWidth;           // 1/100th mm
            double                                  mfEndWidth;             // 1/100th mm

            bool                                    mbStartActive : 1;     // start of Line is active
            bool                                    mbEndActive : 1;       // end of Line is active
            bool                                    mbStartCentered : 1;   // Line is centered on line start point
            bool                                    mbEndCentered : 1;     // Line is centered on line end point

            ImpSdrLineStartEndAttribute(
                basegfx::B2DPolyPolygon aStartPolyPolygon,
                basegfx::B2DPolyPolygon aEndPolyPolygon,
                double fStartWidth,
                double fEndWidth,
                bool bStartActive,
                bool bEndActive,
                bool bStartCentered,
                bool bEndCentered)
            :   maStartPolyPolygon(std::move(aStartPolyPolygon)),
                maEndPolyPolygon(std::move(aEndPolyPolygon)),
                mfStartWidth(fStartWidth),
                mfEndWidth(fEndWidth),
                mbStartActive(bStartActive),
                mbEndActive(bEndActive),
                mbStartCentered(bStartCentered),
                mbEndCentered(bEndCentered)
            {
            }

            ImpSdrLineStartEndAttribute()
            :   mfStartWidth(0.0),
                mfEndWidth(0.0),
                mbStartActive(false),
                mbEndActive(false),
                mbStartCentered(false),
                mbEndCentered(false)
            {
            }

            // data read access
            const basegfx::B2DPolyPolygon& getStartPolyPolygon() const { return maStartPolyPolygon; }
            const basegfx::B2DPolyPolygon& getEndPolyPolygon() const { return maEndPolyPolygon; }
            double getStartWidth() const { return mfStartWidth; }
            double getEndWidth() const { return mfEndWidth; }
            bool isStartActive() const { return mbStartActive; }
            bool isEndActive() const { return mbEndActive; }
            bool isStartCentered() const { return mbStartCentered; }
            bool isEndCentered() const { return mbEndCentered; }

            bool operator==(const ImpSdrLineStartEndAttribute& rCandidate) const
            {
                return (getStartPolyPolygon() == rCandidate.getStartPolyPolygon()
                    && getEndPolyPolygon() == rCandidate.getEndPolyPolygon()
                    && getStartWidth() == rCandidate.getStartWidth()
                    && getEndWidth() == rCandidate.getEndWidth()
                    && isStartActive() == rCandidate.isStartActive()
                    && isEndActive() == rCandidate.isEndActive()
                    && isStartCentered() == rCandidate.isStartCentered()
                    && isEndCentered() == rCandidate.isEndCentered());
            }
        };

        namespace
        {
            SdrLineStartEndAttribute::ImplType& theGlobalDefault()
            {
                static SdrLineStartEndAttribute::ImplType SINGLETON;
                return SINGLETON;
            }
        }

        SdrLineStartEndAttribute::SdrLineStartEndAttribute(
            const basegfx::B2DPolyPolygon& rStartPolyPolygon,
            const basegfx::B2DPolyPolygon& rEndPolyPolygon,
            double fStartWidth,
            double fEndWidth,
            bool bStartActive,
            bool bEndActive,
            bool bStartCentered,
            bool bEndCentered)
        :   mpSdrLineStartEndAttribute(ImpSdrLineStartEndAttribute(
                rStartPolyPolygon, rEndPolyPolygon, fStartWidth, fEndWidth, bStartActive, bEndActive, bStartCentered, bEndCentered))
        {
        }

        SdrLineStartEndAttribute::SdrLineStartEndAttribute()
        :   mpSdrLineStartEndAttribute(theGlobalDefault())
        {
        }

        SdrLineStartEndAttribute::SdrLineStartEndAttribute(const SdrLineStartEndAttribute&) = default;

        SdrLineStartEndAttribute::SdrLineStartEndAttribute(SdrLineStartEndAttribute&&) = default;

        SdrLineStartEndAttribute::~SdrLineStartEndAttribute() = default;

        bool SdrLineStartEndAttribute::isDefault() const
        {
            return mpSdrLineStartEndAttribute.same_object(theGlobalDefault());
        }

        SdrLineStartEndAttribute& SdrLineStartEndAttribute::operator=(const SdrLineStartEndAttribute&) = default;

        SdrLineStartEndAttribute& SdrLineStartEndAttribute::operator=(SdrLineStartEndAttribute&&) = default;

        bool SdrLineStartEndAttribute::operator==(const SdrLineStartEndAttribute& rCandidate) const
        {
            // tdf#87509 default attr is always != non-default attr, even with same values
            if(rCandidate.isDefault() != isDefault())
                return false;

            return rCandidate.mpSdrLineStartEndAttribute == mpSdrLineStartEndAttribute;
        }

        const basegfx::B2DPolyPolygon& SdrLineStartEndAttribute::getStartPolyPolygon() const
        {
            return mpSdrLineStartEndAttribute->getStartPolyPolygon();
        }

        const basegfx::B2DPolyPolygon& SdrLineStartEndAttribute::getEndPolyPolygon() const
        {
            return mpSdrLineStartEndAttribute->getEndPolyPolygon();
        }

        double SdrLineStartEndAttribute::getStartWidth() const
        {
            return mpSdrLineStartEndAttribute->getStartWidth();
        }

        double SdrLineStartEndAttribute::getEndWidth() const
        {
            return mpSdrLineStartEndAttribute->getEndWidth();
        }

        bool SdrLineStartEndAttribute::isStartActive() const
        {
            return mpSdrLineStartEndAttribute->isStartActive();
        }

        bool SdrLineStartEndAttribute::isEndActive() const
        {
            return mpSdrLineStartEndAttribute->isEndActive();
        }

        bool SdrLineStartEndAttribute::isStartCentered() const
        {
            return mpSdrLineStartEndAttribute->isStartCentered();
        }

        bool SdrLineStartEndAttribute::isEndCentered() const
        {
            return mpSdrLineStartEndAttribute->isEndCentered();
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

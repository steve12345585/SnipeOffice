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

#include <drawinglayer/attribute/linestartendattribute.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <utility>


namespace drawinglayer::attribute
{
        class ImpLineStartEndAttribute
        {
        public:
            // data definitions
            double                                  mfWidth;                // absolute line StartEndGeometry base width
            basegfx::B2DPolyPolygon                 maPolyPolygon;          // the StartEndGeometry PolyPolygon

            bool                                    mbCentered : 1;         // use centered to lineStart/End point?

            ImpLineStartEndAttribute(
                double fWidth,
                basegfx::B2DPolyPolygon aPolyPolygon,
                bool bCentered)
            :   mfWidth(fWidth),
                maPolyPolygon(std::move(aPolyPolygon)),
                mbCentered(bCentered)
            {
            }

            ImpLineStartEndAttribute()
            :   mfWidth(0.0),
                mbCentered(false)
            {
            }

            // data read access
            double getWidth() const { return mfWidth; }
            const basegfx::B2DPolyPolygon& getB2DPolyPolygon() const { return maPolyPolygon; }
            bool isCentered() const { return mbCentered; }

            bool operator==(const ImpLineStartEndAttribute& rCandidate) const
            {
                return (basegfx::fTools::equal(getWidth(), rCandidate.getWidth())
                    && getB2DPolyPolygon() == rCandidate.getB2DPolyPolygon()
                    && isCentered() == rCandidate.isCentered());
            }
        };

        namespace
        {
            LineStartEndAttribute::ImplType& theGlobalDefault()
            {
                static LineStartEndAttribute::ImplType SINGLETON;
                return SINGLETON;
            }
        }

        LineStartEndAttribute::LineStartEndAttribute(
            double fWidth,
            const basegfx::B2DPolyPolygon& rPolyPolygon,
            bool bCentered)
        :   mpLineStartEndAttribute(ImpLineStartEndAttribute(
                fWidth, rPolyPolygon, bCentered))
        {
        }

        LineStartEndAttribute::LineStartEndAttribute()
        :   mpLineStartEndAttribute(theGlobalDefault())
        {
        }

        LineStartEndAttribute::LineStartEndAttribute(const LineStartEndAttribute&) = default;

        LineStartEndAttribute::~LineStartEndAttribute() = default;

        bool LineStartEndAttribute::isDefault() const
        {
            return mpLineStartEndAttribute.same_object(theGlobalDefault());
        }

        LineStartEndAttribute& LineStartEndAttribute::operator=(const LineStartEndAttribute&) = default;

        bool LineStartEndAttribute::operator==(const LineStartEndAttribute& rCandidate) const
        {
            // tdf#87509 default attr is always != non-default attr, even with same values
            if(rCandidate.isDefault() != isDefault())
                return false;

            return rCandidate.mpLineStartEndAttribute == mpLineStartEndAttribute;
        }

        double LineStartEndAttribute::getWidth() const
        {
            return mpLineStartEndAttribute->getWidth();
        }

        const basegfx::B2DPolyPolygon& LineStartEndAttribute::getB2DPolyPolygon() const
        {
            return mpLineStartEndAttribute->getB2DPolyPolygon();
        }

        bool LineStartEndAttribute::isCentered() const
        {
            return mpLineStartEndAttribute->isCentered();
        }

        bool LineStartEndAttribute::isActive() const
        {
            return (0.0 != getWidth()
                && 0 != getB2DPolyPolygon().count()
                && 0 != getB2DPolyPolygon().getB2DPolygon(0).count());
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

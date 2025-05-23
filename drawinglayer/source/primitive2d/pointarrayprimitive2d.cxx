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

#include <drawinglayer/primitive2d/pointarrayprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>


using namespace com::sun::star;


namespace drawinglayer::primitive2d
{
        PointArrayPrimitive2D::PointArrayPrimitive2D(
            std::vector< basegfx::B2DPoint >&& rPositions,
            const basegfx::BColor& rRGBColor)
        :   maPositions(std::move(rPositions)),
            maRGBColor(rRGBColor)
        {
        }

        bool PointArrayPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(BasePrimitive2D::operator==(rPrimitive))
            {
                const PointArrayPrimitive2D& rCompare = static_cast<const PointArrayPrimitive2D&>(rPrimitive);

                return (getPositions() == rCompare.getPositions()
                    && getRGBColor() == rCompare.getRGBColor());
            }

            return false;
        }

        basegfx::B2DRange PointArrayPrimitive2D::getB2DRange(const geometry::ViewInformation2D& /*rViewInformation*/) const
        {
            if(maB2DRange.isEmpty())
            {
                basegfx::B2DRange aNewRange;

                // get the basic range from the position vector
                for (auto const& pos : getPositions())
                {
                    aNewRange.expand(pos);
                }

                // assign to buffered value
                const_cast< PointArrayPrimitive2D* >(this)->maB2DRange = aNewRange;
            }

            return maB2DRange;
        }

        // provide unique ID
        sal_uInt32 PointArrayPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_POINTARRAYPRIMITIVE2D;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

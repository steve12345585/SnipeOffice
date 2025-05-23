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

#pragma once

#include <basegfx/vector/b3dvector.hxx>
#include <basegfx/point/b3dpoint.hxx>
#include <basegfx/tuple/b3dtuple.hxx>
#include <basegfx/range/basicrange.hxx>
#include <basegfx/basegfxdllapi.h>

namespace basegfx
{
    class B3DHomMatrix;

    class SAL_WARN_UNUSED B3DRange
    {
        typedef ::basegfx::BasicRange< double, DoubleTraits >   MyBasicRange;

        MyBasicRange            maRangeX;
        MyBasicRange            maRangeY;
        MyBasicRange            maRangeZ;

    public:
        B3DRange() {}

        explicit B3DRange(const B3DTuple& rTuple)
        :   maRangeX(rTuple.getX()),
            maRangeY(rTuple.getY()),
            maRangeZ(rTuple.getZ())
        {
        }

        B3DRange(double x1,
                 double y1,
                 double z1,
                 double x2,
                 double y2,
                 double z2)
        :   maRangeX(x1),
            maRangeY(y1),
            maRangeZ(z1)
        {
            maRangeX.expand(x2);
            maRangeY.expand(y2);
            maRangeZ.expand(z2);
        }

        B3DRange(const B3DTuple& rTuple1,
                 const B3DTuple& rTuple2)
        :   maRangeX(rTuple1.getX()),
            maRangeY(rTuple1.getY()),
            maRangeZ(rTuple1.getZ())
        {
            expand(rTuple2);
        }

        bool isEmpty() const
        {
            return (
                maRangeX.isEmpty()
                || maRangeY.isEmpty()
                || maRangeZ.isEmpty()
                );
        }

        void reset()
        {
            maRangeX.reset();
            maRangeY.reset();
            maRangeZ.reset();
        }

        bool operator==( const B3DRange& rRange ) const
        {
            return (maRangeX == rRange.maRangeX
                && maRangeY == rRange.maRangeY
                && maRangeZ == rRange.maRangeZ);
        }

        bool operator!=( const B3DRange& rRange ) const
        {
            return (maRangeX != rRange.maRangeX
                || maRangeY != rRange.maRangeY
                || maRangeZ != rRange.maRangeZ);
        }

        double getMinX() const
        {
            return maRangeX.getMinimum();
        }

        double getMinY() const
        {
            return maRangeY.getMinimum();
        }

        double getMinZ() const
        {
            return maRangeZ.getMinimum();
        }

        double getMaxX() const
        {
            return maRangeX.getMaximum();
        }

        double getMaxY() const
        {
            return maRangeY.getMaximum();
        }

        double getMaxZ() const
        {
            return maRangeZ.getMaximum();
        }

        double getWidth() const
        {
            return maRangeX.getRange();
        }

        double getHeight() const
        {
            return maRangeY.getRange();
        }

        double getDepth() const
        {
            return maRangeZ.getRange();
        }

        B3DVector getRange() const
        {
            return B3DVector(
                maRangeX.getRange(),
                maRangeY.getRange(),
                maRangeZ.getRange()
                );
        }

        B3DPoint getCenter() const
        {
            return B3DPoint(
                maRangeX.getCenter(),
                maRangeY.getCenter(),
                maRangeZ.getCenter()
                );
        }

        bool overlaps(const B3DRange& rRange) const
        {
            return (
                maRangeX.overlaps(rRange.maRangeX)
                && maRangeY.overlaps(rRange.maRangeY)
                && maRangeZ.overlaps(rRange.maRangeZ)
                );
        }

        void expand(const B3DTuple& rTuple)
        {
            maRangeX.expand(rTuple.getX());
            maRangeY.expand(rTuple.getY());
            maRangeZ.expand(rTuple.getZ());
        }

        void expand(const B3DRange& rRange)
        {
            maRangeX.expand(rRange.maRangeX);
            maRangeY.expand(rRange.maRangeY);
            maRangeZ.expand(rRange.maRangeZ);
        }

        void grow(double fValue)
        {
            maRangeX.grow(fValue);
            maRangeY.grow(fValue);
            maRangeZ.grow(fValue);
        }

        /// clamp value on range
        B3DTuple clamp(const B3DTuple& rTuple) const
        {
            return B3DTuple(
                maRangeX.clamp(rTuple.getX()),
                maRangeY.clamp(rTuple.getY()),
                maRangeZ.clamp(rTuple.getZ()));
        }

         BASEGFX_DLLPUBLIC void transform(const B3DHomMatrix& rMatrix);

        /** Transform Range by given transformation matrix.

            This operation transforms the Range by transforming all eight possible
            extrema points (corners) of the given range and building a new one.
            This means that the range will grow evtl. when a shear and/or rotation
            is part of the transformation.
        */
        B3DRange& operator*=( const ::basegfx::B3DHomMatrix& rMat );

        /** Get a range filled with (0.0, 0.0, 0.0, 1.0, 1.0, 1.0) */
        static const B3DRange& getUnitB3DRange();
    };

    /** Transform B3DRange by given transformation matrix (see operator*=())
    */
    B3DRange operator*( const B3DHomMatrix& rMat, const B3DRange& rB2DRange );

} // end of namespace basegfx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

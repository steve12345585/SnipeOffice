/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

namespace basegfx
{
template <typename TYPE> class Tuple3D
{
protected:
    TYPE mnX;
    TYPE mnY;
    TYPE mnZ;

public:
    /** Create a 3D Tuple

            @param x
            This parameter is used to initialize the X-coordinate
            of the 3D Tuple.

            @param y
            This parameter is used to initialize the Y-coordinate
            of the 3D Tuple.

            @param z
            This parameter is used to initialize the Z-coordinate
            of the 3D Tuple.
        */
    Tuple3D(TYPE x, TYPE y, TYPE z)
        : mnX(x)
        , mnY(y)
        , mnZ(z)
    {
    }

    /// Get X-Coordinate of 3D Tuple
    TYPE getX() const { return mnX; }

    /// Get Y-Coordinate of 3D Tuple
    TYPE getY() const { return mnY; }

    /// Get Z-Coordinate of 3D Tuple
    TYPE getZ() const { return mnZ; }

    /// Set X-Coordinate of 3D Tuple
    void setX(TYPE fX) { mnX = fX; }

    /// Set Y-Coordinate of 3D Tuple
    void setY(TYPE fY) { mnY = fY; }

    /// Set Z-Coordinate of 3D Tuple
    void setZ(TYPE fZ) { mnZ = fZ; }

    // operators

    Tuple3D& operator+=(const Tuple3D& rTup)
    {
        mnX += rTup.mnX;
        mnY += rTup.mnY;
        mnZ += rTup.mnZ;
        return *this;
    }

    Tuple3D& operator-=(const Tuple3D& rTup)
    {
        mnX -= rTup.mnX;
        mnY -= rTup.mnY;
        mnZ -= rTup.mnZ;
        return *this;
    }

    Tuple3D& operator/=(const Tuple3D& rTup)
    {
        mnX /= rTup.mnX;
        mnY /= rTup.mnY;
        mnZ /= rTup.mnZ;
        return *this;
    }

    Tuple3D& operator*=(const Tuple3D& rTup)
    {
        mnX *= rTup.mnX;
        mnY *= rTup.mnY;
        mnZ *= rTup.mnZ;
        return *this;
    }

    Tuple3D& operator*=(TYPE t)
    {
        mnX *= t;
        mnY *= t;
        mnZ *= t;
        return *this;
    }

    Tuple3D& operator/=(TYPE t)
    {
        mnX /= t;
        mnY /= t;
        mnZ /= t;
        return *this;
    }

    bool operator==(const Tuple3D& rTup) const
    {
        return mnX == rTup.mnX && mnY == rTup.mnY && mnZ == rTup.mnZ;
    }

    bool operator!=(const Tuple3D& rTup) const { return !operator==(rTup); }
};

} // end of namespace basegfx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

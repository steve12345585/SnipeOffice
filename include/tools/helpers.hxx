/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <com/sun/star/uno/Reference.h>
#include <sal/config.h>
#include <sal/types.h>
#include <tools/long.hxx>
#include <limits>
#include <string>
#include <type_traits>

inline sal_uInt32 AlignedWidth4Bytes(sal_uInt32 nWidthBits)
{
    if (nWidthBits > SAL_MAX_UINT32 - 31)
        nWidthBits = SAL_MAX_UINT32;
    else
        nWidthBits += 31;
    return (nWidthBits >> 5) << 2;
}

//valid range:  (-180,180]
template <typename T>
[[nodiscard]] inline typename std::enable_if<std::is_signed<T>::value, T>::type
NormAngle180(T angle)
{
    while (angle <= -180)
        angle += 360;
    while (angle > 180)
        angle -= 360;
    return angle;
}

//valid range:  [0,360)
template <typename T> [[nodiscard]] inline T NormAngle360(T angle)
{
    while (angle < 0)
        angle += 360;
    while (angle >= 360)
        angle -= 360;
    return angle;
}

// get hash from interface
// TODO: UNIT TEST
[[nodiscard]] inline
std::string GetInterfaceHash(const ::css::uno::Reference<::css::uno::XInterface>& xIf)
{
    if (xIf.is())
    {
        auto ptr = reinterpret_cast<sal_uIntPtr>(xIf.get());
        return std::to_string(ptr);
    }
    return {};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_SVL_TYPEDWHICH_HXX
#define INCLUDED_SVL_TYPEDWHICH_HXX

#include <sal/config.h>
#include <sal/types.h>
#include <type_traits>

/**
 * A very thin wrapper around the sal_uInt16 WhichId whose purpose is mostly to carry type information,
 * so that we Put() and Get() the right subclasses of SfxPoolItem for each WhichId.
 */
template <class T> class TypedWhichId final
{
public:
    explicit constexpr TypedWhichId(sal_uInt16 nWhich)
        : mnWhich(nWhich)
    {
    }

    /** Up-casting conversion constructor
    */
    template <class derived_type>
    constexpr TypedWhichId(TypedWhichId<derived_type> other,
                           std::enable_if_t<std::is_base_of_v<T, derived_type>, int> = 0)
        : mnWhich(sal_uInt16(other))
    {
    }

    constexpr operator sal_uInt16() const { return mnWhich; }

private:
    sal_uInt16 mnWhich;
};

template <class T> constexpr bool operator==(TypedWhichId<T> const& lhs, TypedWhichId<T> rhs)
{
    return sal_uInt16(lhs) == sal_uInt16(rhs);
}
template <class T> constexpr bool operator!=(TypedWhichId<T> const& lhs, TypedWhichId<T> rhs)
{
    return sal_uInt16(lhs) != sal_uInt16(rhs);
}
template <class T> constexpr bool operator==(sal_uInt16 lhs, TypedWhichId<T> const& rhs)
{
    return lhs == sal_uInt16(rhs);
}
template <class T> constexpr bool operator!=(sal_uInt16 lhs, TypedWhichId<T> const& rhs)
{
    return lhs != sal_uInt16(rhs);
}
template <class T> constexpr bool operator==(TypedWhichId<T> const& lhs, sal_uInt16 rhs)
{
    return sal_uInt16(lhs) == rhs;
}
template <class T> constexpr bool operator!=(TypedWhichId<T> const& lhs, sal_uInt16 rhs)
{
    return sal_uInt16(lhs) != rhs;
}

#endif // INCLUDED_SVL_TYPEDWHICH_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

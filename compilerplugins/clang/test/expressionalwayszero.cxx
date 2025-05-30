/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <o3tl/typed_flags_set.hxx>

constexpr auto ONE = 1;
constexpr auto TWO = 2;

enum class Enum1 {
    ONE = 1,
    TWO = 2,
};
namespace o3tl {
    template<> struct typed_flags<Enum1> : is_typed_flags<Enum1, 0x3> {};
}

enum class Enum2 {
    ZERO = 0,
    ONE = 1,
    TWO = 2,
};
namespace o3tl {
    template<> struct typed_flags<Enum2> : is_typed_flags<Enum2, 0x3> {};
}

int main()
{
    auto v1 = ONE & TWO; // expected-error {{expression always evaluates to zero, lhs=1 rhs=2 [loplugin:expressionalwayszero]}}
    (void)v1;
    auto v2 = Enum1::ONE & Enum1::TWO; // expected-error {{expression always evaluates to zero, lhs=1 rhs=2 [loplugin:expressionalwayszero]}}
    (void)v2;

    auto v3 = Enum2::ONE;
    auto v4 = v3 & Enum2::ZERO; // expected-error {{expression always evaluates to zero, lhs=unknown rhs=0 [loplugin:expressionalwayszero]}}
    (void)v4;

    auto v5 = Enum2::ONE;
    v5 &= Enum2::ZERO; // expected-error {{expression always evaluates to zero, lhs=unknown rhs=0 [loplugin:expressionalwayszero]}}
    (void)v5;
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

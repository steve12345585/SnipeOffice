/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sal/config.h"

#include <compare>

struct S
{
    void* p;
};

int main()
{
    S s{
        0 // expected-error {{NullToPointer ValueDependentIsNotNull ZeroLiteral -> nullptr [loplugin:nullptr]}}
    };
    (void)s;

    (void)(std::strong_order(0, 1) < 0);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

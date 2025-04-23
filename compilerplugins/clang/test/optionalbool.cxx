/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "config_clang.h"
#include <optional>

namespace test1
{
std::optional<bool> get_optional_bool();
void foo1()
{
    // expected-error@+1 {{using conversion call to convert std::optional<bool> to bool probably does not do what you expect, rather use has_value() or value_or() [loplugin:optionalbool]}}
    bool foo(get_optional_bool());
    (void)foo;

    // no warning expected
    if (std::optional<bool> b = get_optional_bool())
        return;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

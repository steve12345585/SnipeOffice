/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined _WIN32 // TODO, see corresponding TODO in compilerplugins/clang/unusedfields.cxx
// expected-no-diagnostics
#else

#include <string_view>

namespace something
{
// expected-error@+1 {{write [loplugin:unusedvarsglobal]}}
extern const std::u16string_view literal1;
}
const std::u16string_view something::literal1(u"xxx");

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_O3TL_CPPUNITTRAITSHELPER_HXX
#define INCLUDED_O3TL_CPPUNITTRAITSHELPER_HXX

#include <sal/config.h>

#include <cstdint>
#include <string>

#include <cppunit/TestAssert.h>

// ostream << char16_t is deleted since C++20 (but just keep outputting numeric values):
template <> inline std::string CppUnit::assertion_traits<char16_t>::toString(char16_t const& x)
{
    return assertion_traits<std::uint_least16_t>::toString(std::uint_least16_t(x));
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

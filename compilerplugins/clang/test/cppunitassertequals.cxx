/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sal/config.h"

#include <cstddef>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include "rtl/ustring.hxx"

#define TEST1 CPPUNIT_ASSERT(b1 == b2)
#define TEST2(x) x

struct T { bool operator ==(T); };

void test(
    bool b1, bool b2, OUString const & s1, OUString const & s2, T t, void * p, std::nullptr_t n,
    double d, int i)
{
    CppUnit::Asserter::failIf(b1,"");
    CPPUNIT_ASSERT(b1 && b2); // expected-error {{rather split into two CPPUNIT_ASSERT [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT((b1 && b2)); // expected-error {{rather split into two CPPUNIT_ASSERT [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(b1 || b2)); // expected-error {{rather split into two CPPUNIT_ASSERT [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(b1 && b2));
    CPPUNIT_ASSERT(!!(b1 && b2)); // expected-error {{rather split into two CPPUNIT_ASSERT [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_MESSAGE("", b1 && b2); // expected-error {{rather split into two CPPUNIT_ASSERT_MESSAGE [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(b1 == b2); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(b1 != b2);
    CPPUNIT_ASSERT((b1 == b2)); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(b1 != b2)); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator != call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(b1 == b2));
    CPPUNIT_ASSERT(!!(b1 == b2)); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_MESSAGE("", b1 == b2); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL_MESSAGE when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(s1 == s2); // expected-error-re {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'const {{(rtl::)?}}OUString' and 'const {{(rtl::)?}}OUString' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(s1 != s2);
    CPPUNIT_ASSERT((s1 == s2)); // expected-error-re {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'const {{(rtl::)?}}OUString' and 'const {{(rtl::)?}}OUString' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(s1 != s2)); // expected-error-re {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'const {{(rtl::)?}}OUString' and 'const {{(rtl::)?}}OUString' (or rewrite as an explicit operator != call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT(!(s1 == s2));
    CPPUNIT_ASSERT(!!(s1 == s2)); // expected-error-re {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'const {{(rtl::)?}}OUString' and 'const {{(rtl::)?}}OUString' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    TEST1; // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    TEST2(CPPUNIT_ASSERT(b1 == b2)); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}
    TEST2(TEST1); // expected-error {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'bool' and 'bool' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}

    // Useful when testing an equality iterator itself:
    CPPUNIT_ASSERT(operator ==(s1, s1));
    CPPUNIT_ASSERT(t.operator ==(t));

    // `P == nullptr` for P of pointer type is awkward to write with CPPUNIT_ASSERT_EQUAL, and the
    // actual pointer values that would be printed if CPPUNIT_ASSERT_EQUAL failed would likely not be
    // very meaningful, so let it use CPPUNIT_ASSERT (but stick to CPPUNIT_ASSERT_EQUAL for
    // consistency in the unlikely case that P is of type std::nullptr_t):
    CPPUNIT_ASSERT(p == nullptr);
    CPPUNIT_ASSERT(n == nullptr); // expected-error-re {{rather call CPPUNIT_ASSERT_EQUAL when comparing 'std::nullptr_t'{{( \(aka 'nullptr_t'\))?}} and '{{(std::)?}}nullptr_t' (or rewrite as an explicit operator == call when the operator itself is the topic) [loplugin:cppunitassertequals]}}

    // There might even be good reasons(?) not to warn inside explicit casts:
    CPPUNIT_ASSERT(bool(b1 && b2));
    CPPUNIT_ASSERT(bool(b1 == b2));
    CPPUNIT_ASSERT(bool(s1 == s2));

    CPPUNIT_ASSERT_EQUAL(b1, true); // expected-error {{CPPUNIT_ASSERT_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_EQUAL_MESSAGE("foo", b1, true); // expected-error {{CPPUNIT_ASSERT_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_EQUAL(true, b1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("foo", true, b1);
    CPPUNIT_ASSERT_EQUAL(s1, OUString("xxx")); // expected-error {{CPPUNIT_ASSERT_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_EQUAL_MESSAGE("foo", s1, OUString("xxx")); // expected-error {{CPPUNIT_ASSERT_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_EQUAL(OUString("xxx"), s1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("foo", OUString("xxx"), s1);

    CPPUNIT_ASSERT_EQUAL(d, 1.0); // expected-error {{CPPUNIT_ASSERT_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_EQUAL(1.0, d);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(d, 1.0, 0.1); // expected-error {{CPPUNIT_ASSERT_DOUBLES_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("foo", d, 1.0, 0.1); // expected-error {{CPPUNIT_ASSERT_DOUBLES_EQUALS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, d, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("foo", 1.0, d, 0.1);

    CPPUNIT_ASSERT_LESS(i, 1); // expected-error {{CPPUNIT_ASSERT_LESS parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_LESSEQUAL(i, 1); // expected-error {{CPPUNIT_ASSERT_LESSEQUAL parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_GREATER(i, 1); // expected-error {{CPPUNIT_ASSERT_GREATER parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
    CPPUNIT_ASSERT_GREATEREQUAL(i, 1); // expected-error {{CPPUNIT_ASSERT_GREATEREQUAL parameters look switched, expected value should be first param [loplugin:cppunitassertequals]}}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

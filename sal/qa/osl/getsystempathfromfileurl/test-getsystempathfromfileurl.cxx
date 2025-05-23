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

#include <sal/config.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <osl/file.hxx>

#if defined(_WIN32)
#define MY_PATH_IN "/c:/foo/bar"
#define MY_PATH_OUT "c:\\foo\\bar"
#define MY_PATH_OUT_CONT MY_PATH_OUT "\\"
#define MY_PATH_OUT_REL "foo\\bar"
#else
#define MY_PATH_IN "/foo/bar"
#define MY_PATH_OUT MY_PATH_IN
#define MY_PATH_OUT_CONT MY_PATH_OUT "/"
#define MY_PATH_OUT_REL "foo/bar"
#endif

namespace {

class Test: public CppUnit::TestFixture {
private:
    CPPUNIT_TEST_SUITE(Test);
    CPPUNIT_TEST(testBadScheme);
    CPPUNIT_TEST(testNoScheme);
    CPPUNIT_TEST(testBadAuthority);
    CPPUNIT_TEST(testLocalhost1Authority);
    CPPUNIT_TEST(testLocalhost2Authority);
    CPPUNIT_TEST(testLocalhost3Authority);
    CPPUNIT_TEST(testNoAuthority);
    CPPUNIT_TEST(testEmptyPath);
    CPPUNIT_TEST(testHomeAbbreviation);
    CPPUNIT_TEST(testOtherHomeAbbreviation);
    CPPUNIT_TEST(testRelative);
    CPPUNIT_TEST(testEscape);
    CPPUNIT_TEST(testBadEscape2f);
    CPPUNIT_TEST(testBadEscape2F);
    CPPUNIT_TEST(testBad0);
    CPPUNIT_TEST(testBadEscape0);
    CPPUNIT_TEST(testBadQuery);
    CPPUNIT_TEST(testBadFragment);
    CPPUNIT_TEST_SUITE_END();

    void testBadScheme();
    void testNoScheme();
    void testBadAuthority();
    void testLocalhost1Authority();
    void testLocalhost2Authority();
    void testLocalhost3Authority();
    void testNoAuthority();
    void testEmptyPath();
    void testHomeAbbreviation();
    void testOtherHomeAbbreviation();
    void testRelative();
    void testEscape();
    void testBadEscape2f();
    void testBadEscape2F();
    void testBad0();
    void testBadEscape0();
    void testBadQuery();
    void testBadFragment();
};

void Test::testBadScheme() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"foo:bar"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testNoScheme() {
#if !defined(_WIN32) //TODO
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"//" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT ""_ustr, p);
#endif
}

void Test::testBadAuthority() {
#if defined UNX
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://baz" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
#endif
}

void Test::testLocalhost1Authority() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://localhost" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT ""_ustr, p);
}

void Test::testLocalhost2Authority() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://LOCALHOST" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT ""_ustr, p);
}

void Test::testLocalhost3Authority() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://127.0.0.1" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT ""_ustr, p);
}

void Test::testNoAuthority() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"file:" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT ""_ustr, p);
}

void Test::testEmptyPath() {
#if defined UNX
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"file://"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"/"_ustr, p);
#endif
}

void Test::testHomeAbbreviation() {
#if defined UNX
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"file:///~"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
        // could theoretically fail due to osl::Security::getHomeDir problem
    e = osl::FileBase::getSystemPathFromFileURL(u"file:///~/foo%2525/bar"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
        // could theoretically fail due to osl::Security::getHomeDir problem
    CPPUNIT_ASSERT(p.endsWith("/foo%25/bar"));
#endif
}

void Test::testOtherHomeAbbreviation() {
#if defined UNX
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file:///~baz" MY_PATH_IN ""_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e); // not supported for now
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
#endif
}

void Test::testRelative() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(u"foo/bar"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT(p.endsWith(MY_PATH_OUT_REL));
}

void Test::testEscape() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "/b%61z"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, e);
    CPPUNIT_ASSERT_EQUAL(u"" MY_PATH_OUT_CONT "baz" ""_ustr, p);
}

void Test::testBadEscape2f() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "/b%2fz"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testBadEscape2F() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "/b%2Fz"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testBad0() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        OUString(RTL_CONSTASCII_USTRINGPARAM("file://" MY_PATH_IN "/b\x00z")),
        p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testBadEscape0() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "/b%00z"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testBadQuery() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "?baz"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

void Test::testBadFragment() {
    OUString p;
    auto e = osl::FileBase::getSystemPathFromFileURL(
        u"file://" MY_PATH_IN "#baz"_ustr, p);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_INVAL, e);
    CPPUNIT_ASSERT_EQUAL(OUString(), p);
}

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <sal/types.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <sal/macros.h>

namespace test::oustring {

class Convert: public CppUnit::TestFixture
{
private:
    void convertToString();

    CPPUNIT_TEST_SUITE(Convert);
    CPPUNIT_TEST(convertToString);
    CPPUNIT_TEST_SUITE_END();
};

}

CPPUNIT_TEST_SUITE_REGISTRATION(test::oustring::Convert);

namespace {

struct TestConvertToString
{
    sal_Unicode aSource[100];
    sal_Int32 nLength;
    rtl_TextEncoding nEncoding;
    sal_uInt32 nFlags;
    char const * pStrict;
    char const * pRelaxed;
};

void testConvertToString(TestConvertToString const & rTest)
{
    const OUString aSource(rTest.aSource, rTest.nLength);
    OString aStrict(RTL_CONSTASCII_STRINGPARAM("12345"));
    bool bSuccess = aSource.convertToString(&aStrict, rTest.nEncoding,
                                            rTest.nFlags);
    OString aRelaxed(OUStringToOString(aSource, rTest.nEncoding,
                                                 rTest.nFlags));

    OStringBuffer aPrefix;
    aPrefix.append("{");
    for (sal_Int32 i = 0; i < rTest.nLength; ++i)
    {
        aPrefix.append("U+");
        aPrefix.append(static_cast< sal_Int32 >(rTest.aSource[i]), 16);
        if (i + 1 < rTest.nLength)
            aPrefix.append(",");
    }
    aPrefix.append("}, ");
    aPrefix.append(static_cast< sal_Int32 >(rTest.nEncoding));
    aPrefix.append(", 0x");
    aPrefix.append(static_cast< sal_Int32 >(rTest.nFlags), 16);
    aPrefix.append(" -> ");

    if (bSuccess)
    {
        if (rTest.pStrict == nullptr || aStrict != rTest.pStrict)
        {
            OStringBuffer aMessage(aPrefix);
            aMessage.append("strict = \"");
            aMessage.append(aStrict);
            aMessage.append("\"");
            CPPUNIT_ASSERT_MESSAGE(aMessage.getStr(), false);
        }
    }
    else
    {
        if (aStrict != "12345")
        {
            OStringBuffer aMessage(aPrefix);
            aMessage.append("modified output");
            CPPUNIT_ASSERT_MESSAGE(aMessage.getStr(), false);
        }
        if (rTest.pStrict != nullptr)
        {
            OStringBuffer aMessage(aPrefix);
            aMessage.append("failed");
            CPPUNIT_ASSERT_MESSAGE(aMessage.getStr(), false);
        }
    }
    if (aRelaxed != rTest.pRelaxed)
    {
        OStringBuffer aMessage(aPrefix);
        aMessage.append("relaxed = \"");
        aMessage.append(aRelaxed);
        aMessage.append("\"");
        CPPUNIT_ASSERT_MESSAGE(aMessage.getStr(), false);
    }
}

}

void test::oustring::Convert::convertToString()
{
    TestConvertToString const aTests[]
        = { { { 0 },
              0,
              RTL_TEXTENCODING_ASCII_US,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR
                  | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR,
              "",
              "" },
            { { 0 },
              0,
              RTL_TEXTENCODING_ASCII_US,
              OUSTRING_TO_OSTRING_CVTFLAGS,
              "",
              "" },
            { { 0x0041,0x0042,0x0043 },
              3,
              RTL_TEXTENCODING_ASCII_US,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR
                  | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR,
              "ABC",
              "ABC" },
            { { 0x0041,0x0042,0x0043 },
              3,
              RTL_TEXTENCODING_ASCII_US,
              OUSTRING_TO_OSTRING_CVTFLAGS,
              "ABC",
              "ABC" },
            { { 0xB800 },
              1,
              RTL_TEXTENCODING_ISO_2022_JP,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR
                  | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR,
              nullptr,
              "" },
            { { 0x3001,  0xB800 },
              2,
              RTL_TEXTENCODING_ISO_2022_JP,
              OUSTRING_TO_OSTRING_CVTFLAGS,
              "\x1b\x24\x42\x21\x22\x1b\x28\x42\x3f",
              "\x1b\x24\x42\x21\x22\x1b\x28\x42\x3f" },
            { { 0x0041,0x0100,0x0042 },
              3,
              RTL_TEXTENCODING_ISO_8859_1,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR
                  | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR,
              nullptr,
              "A" },
            { { 0x0041,0x0100,0x0042 },
              3,
              RTL_TEXTENCODING_ISO_8859_1,
              OUSTRING_TO_OSTRING_CVTFLAGS,
              "A?B",
              "A?B" } };
    for (auto const& aTest : aTests)
        testConvertToString(aTest);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <config_locales.h>

#include <sal/config.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <o3tl/cppunittraitshelper.hxx>
#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <rtl/tencinfo.h>
#include <rtl/textcvt.h>
#include <rtl/textenc.h>
#include <sal/types.h>
#include <sal/macros.h>
#include <osl/diagnose.h>

namespace {

struct SingleByteCharSet {
    rtl_TextEncoding m_nEncoding;
    sal_Unicode m_aMap[256];
};

void testSingleByteCharSet(SingleByteCharSet const & rSet) {
    char aText[256];
    sal_Unicode aUnicode[256];
    sal_Size nNumber = 0;
    for (int i = 0; i < 256; ++i) {
        if (rSet.m_aMap[i] != 0xFFFF) {
            aText[nNumber++] = static_cast< char >(i);
        }
    }
    {
        rtl_TextToUnicodeConverter aConverter
            = rtl_createTextToUnicodeConverter(rSet.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::number(rSet.m_nEncoding) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        rtl_TextToUnicodeContext aContext
            = rtl_createTextToUnicodeContext(aConverter);
        CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
        sal_Size nSize;
        sal_uInt32 nInfo;
        sal_Size nConverted;
        nSize = rtl_convertTextToUnicode(
            aConverter, aContext, aText, nNumber, aUnicode, nNumber,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
            &nInfo, &nConverted);
        CPPUNIT_ASSERT_EQUAL(nNumber, nSize);
        CPPUNIT_ASSERT_EQUAL(sal_uInt32(0), nInfo);
        CPPUNIT_ASSERT_EQUAL(nNumber, nConverted);
        rtl_destroyTextToUnicodeContext(aConverter, aContext);
        rtl_destroyTextToUnicodeConverter(aConverter);
    }
    {
        int j = 0;
        for (int i = 0; i < 256; ++i) {
            if (rSet.m_aMap[i] != 0xFFFF && aUnicode[j] != rSet.m_aMap[i]) {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("rSet.m_aMap[" + OUString::number(i) + "] == " +
                                                                  OUString::number(rSet.m_aMap[i], 16)),
                                                         RTL_TEXTENCODING_UTF8).getStr(),
                                       u'\xFFFF', rSet.m_aMap[i]);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aUnicode[" + OUString::number(j) + "] == " +
                                                                  OUString::number(aUnicode[j], 16) +
                                                                  ", rSet.m_aMap[" + OUString::number(i) + "] == " +
                                                                  OUString::number(rSet.m_aMap[i], 16)),
                                                         RTL_TEXTENCODING_UTF8).getStr(),
                                       rSet.m_aMap[i], aUnicode[j]);
            }
            if (rSet.m_aMap[i] != 0xFFFF)
                j++;
        }
    }
    if (rSet.m_nEncoding == RTL_TEXTENCODING_ASCII_US) {
        nNumber = 128;
    }
    {
        rtl_UnicodeToTextConverter aConverter
            = rtl_createUnicodeToTextConverter(rSet.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rSet.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        rtl_UnicodeToTextContext aContext
            = rtl_createUnicodeToTextContext(aConverter);
        CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
        sal_Size nSize;
        sal_uInt32 nInfo;
        sal_Size nConverted;
        nSize = rtl_convertUnicodeToText(
            aConverter, aContext, aUnicode, nNumber, aText, nNumber,
            (RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR
             | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR),
            &nInfo, &nConverted);
        CPPUNIT_ASSERT_EQUAL(nNumber, nSize);
        CPPUNIT_ASSERT_EQUAL(sal_uInt32(0), nInfo);
        CPPUNIT_ASSERT_EQUAL(nNumber, nConverted);
        rtl_destroyUnicodeToTextContext(aConverter, aContext);
        rtl_destroyUnicodeToTextConverter(aConverter);
    }
    {
        int j = 0;
        for (int i = 0; i < 256; ++i) {
            if (rSet.m_aMap[i] != 0xFFFF
                && aText[j] != static_cast< char >(i))
            {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("rSet.m_aMap[" + OUString::number(i) + "] == " +
                                                                  OUString::number(rSet.m_aMap[i], 16)),
                                                         RTL_TEXTENCODING_UTF8).getStr(),
                                       u'\xFFFF', rSet.m_aMap[i]);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aText[" + OUString::number(j) + "] == " +
                                                                  OUString::number(i, 16)),
                                                         RTL_TEXTENCODING_UTF8).getStr(),
                                       static_cast< char >(i), aText[j]);
            }
            if (rSet.m_aMap[i] != 0xFFFF)
                j++;
        }
    }
    for (int i = 0; i < 256; ++i) {
        if (rSet.m_aMap[i] == 0xFFFF) {
            aText[0] = static_cast< char >(i);
            rtl_TextToUnicodeConverter aConverter
                = rtl_createTextToUnicodeConverter(rSet.m_nEncoding);
            CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rSet.m_nEncoding)) + ") failed"),
                                                     RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
            rtl_TextToUnicodeContext aContext
                = rtl_createTextToUnicodeContext(aConverter);
            CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
            sal_Size nSize;
            sal_uInt32 nInfo;
            sal_Size nConverted;
            nSize = rtl_convertTextToUnicode(
                aConverter, aContext, aText, 1, aUnicode, 1,
                (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
                 | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
                 | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
                &nInfo, &nConverted);

            sal_uInt32 nExpectedInfo = (RTL_TEXTTOUNICODE_INFO_ERROR | RTL_TEXTTOUNICODE_INFO_UNDEFINED);

            CPPUNIT_ASSERT_EQUAL(sal_Size(0), nSize);
            CPPUNIT_ASSERT_EQUAL(nExpectedInfo, nInfo);
            CPPUNIT_ASSERT_EQUAL(sal_Size(1), nConverted);

            rtl_destroyTextToUnicodeContext(aConverter, aContext);
            rtl_destroyTextToUnicodeConverter(aConverter);
        }
    }
}

int const TEST_STRING_SIZE = 1000;

struct ComplexCharSetTest {
    rtl_TextEncoding m_nEncoding;
    char const * m_pText;
    sal_Size m_nTextSize;
    sal_Unicode m_aUnicode[TEST_STRING_SIZE];
    sal_Size m_nUnicodeSize;
    bool m_bNoContext;
    bool m_bForward;
    bool m_bReverse;
    bool m_bGlobalSignature;
    sal_uInt32 m_nReverseUndefined;
};

void doComplexCharSetTest(ComplexCharSetTest const & rTest) {
    if (rTest.m_bForward) {
        sal_Unicode aUnicode[TEST_STRING_SIZE];
        rtl_TextToUnicodeConverter aConverter
            = rtl_createTextToUnicodeConverter(rTest.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rTest.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        rtl_TextToUnicodeContext aContext
            = rtl_createTextToUnicodeContext(aConverter);
        CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
        sal_Size nSize;
        sal_uInt32 nInfo;
        sal_Size nConverted;
        nSize = rtl_convertTextToUnicode(
            aConverter, aContext,
            reinterpret_cast< char const * >(rTest.m_pText),
            rTest.m_nTextSize, aUnicode, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH
             | (rTest.m_bGlobalSignature ?
                RTL_TEXTTOUNICODE_FLAGS_GLOBAL_SIGNATURE : 0)),
            &nInfo, &nConverted);
        CPPUNIT_ASSERT_EQUAL(rTest.m_nUnicodeSize, nSize);
        CPPUNIT_ASSERT_EQUAL(sal_uInt32(0), nInfo);
        CPPUNIT_ASSERT_EQUAL(rTest.m_nTextSize, nConverted);

        rtl_destroyTextToUnicodeContext(aConverter, aContext);
        rtl_destroyTextToUnicodeConverter(aConverter);

        for (sal_Size i = 0; i < rTest.m_nUnicodeSize; ++i) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(aUnicode[i], 16) +
                                                              ", rTest.m_aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(rTest.m_aUnicode[i], 16)),
                                                     RTL_TEXTENCODING_UTF8).getStr(),
                                   rTest.m_aUnicode[i], aUnicode[i]);
        }
    }
    if (rTest.m_bForward) {
        sal_Unicode aUnicode[TEST_STRING_SIZE];
        rtl_TextToUnicodeConverter aConverter
            = rtl_createTextToUnicodeConverter(rTest.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rTest.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        rtl_TextToUnicodeContext aContext
            = rtl_createTextToUnicodeContext(aConverter);
        CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
        if (aContext != reinterpret_cast<rtl_TextToUnicodeContext>(1)) {
            sal_Size nInput = 0;
            sal_Size nOutput = 0;
            for (bool bFlush = true; nInput < rTest.m_nTextSize || bFlush;) {
                sal_Size nSrcBytes = 1;
                sal_uInt32 nFlags
                    = (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
                       | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
                       | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR
                       | (rTest.m_bGlobalSignature ?
                          RTL_TEXTTOUNICODE_FLAGS_GLOBAL_SIGNATURE : 0));
                if (nInput >= rTest.m_nTextSize) {
                    nSrcBytes = 0;
                    nFlags |= RTL_TEXTTOUNICODE_FLAGS_FLUSH;
                    bFlush = false;
                }
                sal_uInt32 nInfo;
                sal_Size nConverted;
                sal_Size nSize = rtl_convertTextToUnicode(
                    aConverter, aContext,
                    rTest.m_pText + nInput,
                    nSrcBytes, aUnicode + nOutput, TEST_STRING_SIZE - nOutput,
                    nFlags, &nInfo, &nConverted);
                nOutput += nSize;
                nInput += nConverted;
                CPPUNIT_ASSERT_EQUAL(sal_uInt32(0),
                                     (nInfo & ~RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL));
            }
            CPPUNIT_ASSERT_EQUAL(rTest.m_nUnicodeSize, nOutput);
            CPPUNIT_ASSERT_EQUAL(rTest.m_nTextSize, nInput);

            for (sal_Size i = 0; i < rTest.m_nUnicodeSize; ++i) {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aUnicode[" + OUString::number(i) + "] == " +
                                                                  OUString::number(aUnicode[i], 16) +
                                                                  ", rTest.m_aUnicode[" + OUString::number(i) + "] == " +
                                                                  OUString::number(rTest.m_aUnicode[i], 16)),
                                                         RTL_TEXTENCODING_UTF8).getStr(),
                                       rTest.m_aUnicode[i], aUnicode[i]);
            }
        }
        rtl_destroyTextToUnicodeContext(aConverter, aContext);
        rtl_destroyTextToUnicodeConverter(aConverter);
    }
    if (rTest.m_bNoContext && rTest.m_bForward) {
        sal_Unicode aUnicode[TEST_STRING_SIZE] = { 0, };
        int nSize = 0;
        rtl_TextToUnicodeConverter aConverter
            = rtl_createTextToUnicodeConverter(rTest.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rTest.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        for (sal_Size i = 0;;) {
            if (i == rTest.m_nTextSize) {
                goto done;
            }
            char c1 = rTest.m_pText[i++];
            sal_Unicode aUC[2];
            sal_uInt32 nInfo = 0;
            sal_Size nCvtBytes;
            sal_Size nChars = rtl_convertTextToUnicode(
                aConverter, nullptr, &c1, 1, aUC, 2,
                (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
                 | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
                 | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR
                 | (rTest.m_bGlobalSignature ?
                    RTL_TEXTTOUNICODE_FLAGS_GLOBAL_SIGNATURE : 0)),
                &nInfo, &nCvtBytes);
            if ((nInfo & RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL) != 0) {
                char sBuffer[10];
                sBuffer[0] = c1;
                sal_uInt16 nLen = 1;
                while ((nInfo & RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL) != 0
                       && nLen < 10)
                {
                    if (i == rTest.m_nTextSize) {
                        goto done;
                    }
                    c1 = rTest.m_pText[i++];
                    sBuffer[nLen++] = c1;
                    nChars = rtl_convertTextToUnicode(
                        aConverter, nullptr, sBuffer, nLen, aUC, 2,
                        (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
                         | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
                         | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR
                         | (rTest.m_bGlobalSignature ?
                            RTL_TEXTTOUNICODE_FLAGS_GLOBAL_SIGNATURE : 0)),
                        &nInfo, &nCvtBytes);
                }
                if (nChars == 1 && nInfo == 0) {
                    OSL_ASSERT(nCvtBytes == nLen);
                    aUnicode[nSize++] = aUC[0];
                } else if (nChars == 2 && nInfo == 0) {
                    OSL_ASSERT(nCvtBytes == nLen);
                    aUnicode[nSize++] = aUC[0];
                    aUnicode[nSize++] = aUC[1];
                } else {
                    OSL_ASSERT(
                        (nInfo & RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL) == 0
                        && nChars == 0 && nInfo != 0);
                    aUnicode[nSize++] = sBuffer[0];
                    i -= nLen - 1;
                }
            } else if (nChars == 1 && nInfo == 0) {
                OSL_ASSERT(nCvtBytes == 1);
                aUnicode[nSize++] = aUC[0];
            } else if (nChars == 2 && nInfo == 0) {
                OSL_ASSERT(nCvtBytes == 1);
                aUnicode[nSize++] = aUC[0];
                aUnicode[nSize++] = aUC[1];
            } else {
                OSL_ASSERT(nChars == 0 && nInfo != 0);
                aUnicode[nSize++] = c1;
            }
        }
    done:
        rtl_destroyTextToUnicodeConverter(aConverter);
        for (sal_Size i = 0; i < rTest.m_nUnicodeSize; ++i) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(aUnicode[i], 16) +
                                                              ", rTest.m_aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(rTest.m_aUnicode[i], 16)),
                                                     RTL_TEXTENCODING_UTF8).getStr(),
                                   rTest.m_aUnicode[i], aUnicode[i]);
        }
    }
    if (rTest.m_bReverse) {
        char aText[TEST_STRING_SIZE];
        rtl_UnicodeToTextConverter aConverter
            = rtl_createUnicodeToTextConverter(rTest.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rTest.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        rtl_UnicodeToTextContext aContext
            = rtl_createUnicodeToTextContext(aConverter);
        CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", aContext != nullptr);
        sal_Size nSize;
        sal_uInt32 nInfo;
        sal_Size nConverted;
        nSize = rtl_convertUnicodeToText(
            aConverter, aContext, rTest.m_aUnicode, rTest.m_nUnicodeSize, aText,
            TEST_STRING_SIZE,
            (rTest.m_nReverseUndefined | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR
             | RTL_UNICODETOTEXT_FLAGS_FLUSH
             | (rTest.m_bGlobalSignature ?
                RTL_UNICODETOTEXT_FLAGS_GLOBAL_SIGNATURE : 0)),
            &nInfo, &nConverted);
        CPPUNIT_ASSERT_EQUAL(rTest.m_nTextSize, nSize);
        if (nInfo != 0)
        {
            CPPUNIT_ASSERT_EQUAL(RTL_UNICODETOTEXT_INFO_UNDEFINED, nInfo);
            CPPUNIT_ASSERT_MESSAGE("rTest.m_nReverseUndefined should not be RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR",
                                   rTest.m_nReverseUndefined != RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR);
        }
        CPPUNIT_ASSERT_EQUAL(rTest.m_nUnicodeSize, nConverted);
        rtl_destroyUnicodeToTextContext(aConverter, aContext);
        rtl_destroyUnicodeToTextConverter(aConverter);
        for (sal_Size i = 0; i < rTest.m_nTextSize; ++i) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aText[" + OUString::number(i) + "] == " +
                                                              OUString::number(aText[i], 16) +
                                                              ", rTest.m_pText[" + OUString::number(i) + "] == " +
                                                              OUString::number(rTest.m_pText[i], 16)),
                                                     RTL_TEXTENCODING_UTF8).getStr(),
                                   rTest.m_pText[i], aText[i]);
        }
    }
}

void doComplexCharSetCutTest(ComplexCharSetTest const & rTest) {
    if (rTest.m_bNoContext) {
        sal_Unicode aUnicode[TEST_STRING_SIZE];
        rtl_TextToUnicodeConverter aConverter
            = rtl_createTextToUnicodeConverter(rTest.m_nEncoding);
        CPPUNIT_ASSERT_MESSAGE(OUStringToOString(Concat2View("rtl_createTextToUnicodeConverter(" + OUString::createFromAscii(rtl_getMimeCharsetFromTextEncoding(rTest.m_nEncoding)) + ") failed"),
                                                 RTL_TEXTENCODING_UTF8).getStr(),
                               aConverter != nullptr);
        sal_Size nSize;
        sal_uInt32 nInfo;
        sal_Size nConverted;
        nSize = rtl_convertTextToUnicode(
            aConverter, nullptr, reinterpret_cast< char const * >(rTest.m_pText),
            rTest.m_nTextSize, aUnicode, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
            &nInfo, &nConverted);

        CPPUNIT_ASSERT_EQUAL(rTest.m_nUnicodeSize, nSize);
        if (nInfo != RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL)
        {
            CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_ERROR | RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL,
                                 nInfo);
        }
        CPPUNIT_ASSERT_MESSAGE("nConverted should be less than rTest.m_nTextSize", nConverted < rTest.m_nTextSize);

        rtl_destroyTextToUnicodeConverter(aConverter);
        for (sal_Size i = 0; i < nSize; ++i) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OUStringToOString(Concat2View("aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(aUnicode[i], 16) +
                                                              ", rTest.m_aUnicode[" + OUString::number(i) + "] == " +
                                                              OUString::number(rTest.m_aUnicode[i], 16)),
                                                     RTL_TEXTENCODING_UTF8).getStr(),
                                   rTest.m_aUnicode[i], aUnicode[i]);
        }
    }
}

class Test: public CppUnit::TestFixture {
public:
    void testSingleByte();

    void testComplex();

    void testComplexCut();

    void testInvalidUtf7();

    void testInvalidUtf8();

    void testInvalidUnicode();

    void testSRCBUFFERTOSMALL();

    void testMime();

    void testWindows();

    void testInfo();

    CPPUNIT_TEST_SUITE(Test);
    CPPUNIT_TEST(testSingleByte);
    CPPUNIT_TEST(testComplex);
    CPPUNIT_TEST(testComplexCut);
    CPPUNIT_TEST(testInvalidUtf7);
    CPPUNIT_TEST(testInvalidUtf8);
    CPPUNIT_TEST(testInvalidUnicode);
    CPPUNIT_TEST(testSRCBUFFERTOSMALL);
    CPPUNIT_TEST(testMime);
    CPPUNIT_TEST(testWindows);
    CPPUNIT_TEST(testInfo);
    CPPUNIT_TEST_SUITE_END();
};

void Test::testSingleByte() {
    static SingleByteCharSet const data[]
        = { { RTL_TEXTENCODING_MS_1250,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0xFFFF,0x201E,0x2026,0x2020,0x2021,
                0xFFFF,0x2030,0x0160,0x2039,0x015A,0x0164,0x017D,0x0179,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0xFFFF,0x2122,0x0161,0x203A,0x015B,0x0165,0x017E,0x017A,
                0x00A0,0x02C7,0x02D8,0x0141,0x00A4,0x0104,0x00A6,0x00A7,
                0x00A8,0x00A9,0x015E,0x00AB,0x00AC,0x00AD,0x00AE,0x017B,
                0x00B0,0x00B1,0x02DB,0x0142,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x0105,0x015F,0x00BB,0x013D,0x02DD,0x013E,0x017C,
                0x0154,0x00C1,0x00C2,0x0102,0x00C4,0x0139,0x0106,0x00C7,
                0x010C,0x00C9,0x0118,0x00CB,0x011A,0x00CD,0x00CE,0x010E,
                0x0110,0x0143,0x0147,0x00D3,0x00D4,0x0150,0x00D6,0x00D7,
                0x0158,0x016E,0x00DA,0x0170,0x00DC,0x00DD,0x0162,0x00DF,
                0x0155,0x00E1,0x00E2,0x0103,0x00E4,0x013A,0x0107,0x00E7,
                0x010D,0x00E9,0x0119,0x00EB,0x011B,0x00ED,0x00EE,0x010F,
                0x0111,0x0144,0x0148,0x00F3,0x00F4,0x0151,0x00F6,0x00F7,
                0x0159,0x016F,0x00FA,0x0171,0x00FC,0x00FD,0x0163,0x02D9 } },
            { RTL_TEXTENCODING_MS_1251,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
                0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
                0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0xFFFF,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
                0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
                0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
                0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
                0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
                0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
                0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
                0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
                0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
                0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
                0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
                0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
                0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F } },
            { RTL_TEXTENCODING_MS_1252,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0x02C6,0x2030,0x0160,0x2039,0x0152,0xFFFF,0x017D,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x02DC,0x2122,0x0161,0x203A,0x0153,0xFFFF,0x017E,0x0178,
                0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
                0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,
                0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
                0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,
                0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,
                0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
                0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,
                0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x00FD,0x00FE,0x00FF } },
            { RTL_TEXTENCODING_MS_1253,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0xFFFF,0x2030,0xFFFF,0x2039,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0xFFFF,0x2122,0xFFFF,0x203A,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x00A0,0x0385,0x0386,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0xFFFF,0x00AB,0x00AC,0x00AD,0x00AE,0x2015,
                0x00B0,0x00B1,0x00B2,0x00B3,0x0384,0x00B5,0x00B6,0x00B7,
                0x0388,0x0389,0x038A,0x00BB,0x038C,0x00BD,0x038E,0x038F,
                0x0390,0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,
                0x0398,0x0399,0x039A,0x039B,0x039C,0x039D,0x039E,0x039F,
                0x03A0,0x03A1,0xFFFF,0x03A3,0x03A4,0x03A5,0x03A6,0x03A7,
                0x03A8,0x03A9,0x03AA,0x03AB,0x03AC,0x03AD,0x03AE,0x03AF,
                0x03B0,0x03B1,0x03B2,0x03B3,0x03B4,0x03B5,0x03B6,0x03B7,
                0x03B8,0x03B9,0x03BA,0x03BB,0x03BC,0x03BD,0x03BE,0x03BF,
                0x03C0,0x03C1,0x03C2,0x03C3,0x03C4,0x03C5,0x03C6,0x03C7,
                0x03C8,0x03C9,0x03CA,0x03CB,0x03CC,0x03CD,0x03CE,0xFFFF } },
            { RTL_TEXTENCODING_MS_1254,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0x02C6,0x2030,0x0160,0x2039,0x0152,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x02DC,0x2122,0x0161,0x203A,0x0153,0xFFFF,0xFFFF,0x0178,
                0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
                0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,
                0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
                0x011E,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,
                0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x0130,0x015E,0x00DF,
                0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
                0x011F,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,
                0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x0131,0x015F,0x00FF } },
            { RTL_TEXTENCODING_APPLE_ROMAN,
              {   0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,
                  0x08,  0x09,  0x0A,  0x0B,  0x0C,  0x0D,  0x0E,  0x0F,
                  0x10,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,  0x17,
                  0x18,  0x19,  0x1A,  0x1B,  0x1C,  0x1D,  0x1E,  0x1F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,  0x7F,
                0x00C4,0x00C5,0x00C7,0x00C9,0x00D1,0x00D6,0x00DC,0x00E1,
                0x00E0,0x00E2,0x00E4,0x00E3,0x00E5,0x00E7,0x00E9,0x00E8,
                0x00EA,0x00EB,0x00ED,0x00EC,0x00EE,0x00EF,0x00F1,0x00F3,
                0x00F2,0x00F4,0x00F6,0x00F5,0x00FA,0x00F9,0x00FB,0x00FC,
                0x2020,0x00B0,0x00A2,0x00A3,0x00A7,0x2022,0x00B6,0x00DF,
                0x00AE,0x00A9,0x2122,0x00B4,0x00A8,0x2260,0x00C6,0x00D8,
                0x221E,0x00B1,0x2264,0x2265,0x00A5,0x00B5,0x2202,0x2211,
                0x220F,0x03C0,0x222B,0x00AA,0x00BA,0x03A9,0x00E6,0x00F8,
                0x00BF,0x00A1,0x00AC,0x221A,0x0192,0x2248,0x2206,0x00AB,
                0x00BB,0x2026,0x00A0,0x00C0,0x00C3,0x00D5,0x0152,0x0153,
                0x2013,0x2014,0x201C,0x201D,0x2018,0x2019,0x00F7,0x25CA,
                0x00FF,0x0178,0x2044,0x20AC,0x2039,0x203A,0xFB01,0xFB02,
                0x2021,0x00B7,0x201A,0x201E,0x2030,0x00C2,0x00CA,0x00C1,
                0x00CB,0x00C8,0x00CD,0x00CE,0x00CF,0x00CC,0x00D3,0x00D4,
                0xF8FF,0x00D2,0x00DA,0x00DB,0x00D9,0x0131,0x02C6,0x02DC,
                0x00AF,0x02D8,0x02D9,0x02DA,0x00B8,0x02DD,0x02DB,0x02C7 } },
            { RTL_TEXTENCODING_IBM_437,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,
                0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
                0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,
                0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
                0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,
                0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
                0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
                0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
                0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,
                0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
                0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,
                0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
                0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,
                0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
                0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,
                0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0 } },

            { RTL_TEXTENCODING_ASCII_US,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021, // !
                0x02C6,0x2030,0x0160,0x2039,0x0152,0xFFFF,0x017D,0xFFFF, // !
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014, // !
                0x02DC,0x2122,0x0161,0x203A,0x0153,0xFFFF,0x017E,0x0178, // !
                  0xA0,  0xA1,  0xA2,  0xA3,  0xA4,  0xA5,  0xA6,  0xA7,
                  0xA8,  0xA9,  0xAA,  0xAB,  0xAC,  0xAD,  0xAE,  0xAF,
                  0xB0,  0xB1,  0xB2,  0xB3,  0xB4,  0xB5,  0xB6,  0xB7,
                  0xB8,  0xB9,  0xBA,  0xBB,  0xBC,  0xBD,  0xBE,  0xBF,
                  0xC0,  0xC1,  0xC2,  0xC3,  0xC4,  0xC5,  0xC6,  0xC7,
                  0xC8,  0xC9,  0xCA,  0xCB,  0xCC,  0xCD,  0xCE,  0xCF,
                  0xD0,  0xD1,  0xD2,  0xD3,  0xD4,  0xD5,  0xD6,  0xD7,
                  0xD8,  0xD9,  0xDA,  0xDB,  0xDC,  0xDD,  0xDE,  0xDF,
                  0xE0,  0xE1,  0xE2,  0xE3,  0xE4,  0xE5,  0xE6,  0xE7,
                  0xE8,  0xE9,  0xEA,  0xEB,  0xEC,  0xED,  0xEE,  0xEF,
                  0xF0,  0xF1,  0xF2,  0xF3,  0xF4,  0xF5,  0xF6,  0xF7,
                  0xF8,  0xF9,  0xFA,  0xFB,  0xFC,  0xFD,  0xFE,  0xFF } },
            { RTL_TEXTENCODING_ISO_8859_1,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
                0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,
                0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
                0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,
                0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,
                0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
                0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,
                0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x00FD,0x00FE,0x00FF } },
            { RTL_TEXTENCODING_ISO_8859_2,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0x0104,0x02D8,0x0141,0x00A4,0x013D,0x015A,0x00A7,
                0x00A8,0x0160,0x015E,0x0164,0x0179,0x00AD,0x017D,0x017B,
                0x00B0,0x0105,0x02DB,0x0142,0x00B4,0x013E,0x015B,0x02C7,
                0x00B8,0x0161,0x015F,0x0165,0x017A,0x02DD,0x017E,0x017C,
                0x0154,0x00C1,0x00C2,0x0102,0x00C4,0x0139,0x0106,0x00C7,
                0x010C,0x00C9,0x0118,0x00CB,0x011A,0x00CD,0x00CE,0x010E,
                0x0110,0x0143,0x0147,0x00D3,0x00D4,0x0150,0x00D6,0x00D7,
                0x0158,0x016E,0x00DA,0x0170,0x00DC,0x00DD,0x0162,0x00DF,
                0x0155,0x00E1,0x00E2,0x0103,0x00E4,0x013A,0x0107,0x00E7,
                0x010D,0x00E9,0x0119,0x00EB,0x011B,0x00ED,0x00EE,0x010F,
                0x0111,0x0144,0x0148,0x00F3,0x00F4,0x0151,0x00F6,0x00F7,
                0x0159,0x016F,0x00FA,0x0171,0x00FC,0x00FD,0x0163,0x02D9 } },
            { RTL_TEXTENCODING_ISO_8859_3,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0x0126,0x02D8,0x00A3,0x00A4,0xFFFF,0x0124,0x00A7,
                0x00A8,0x0130,0x015E,0x011E,0x0134,0x00AD,0xFFFF,0x017B,
                0x00B0,0x0127,0x00B2,0x00B3,0x00B4,0x00B5,0x0125,0x00B7,
                0x00B8,0x0131,0x015F,0x011F,0x0135,0x00BD,0xFFFF,0x017C,
                0x00C0,0x00C1,0x00C2,0xFFFF,0x00C4,0x010A,0x0108,0x00C7,
                0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
                0xFFFF,0x00D1,0x00D2,0x00D3,0x00D4,0x0120,0x00D6,0x00D7,
                0x011C,0x00D9,0x00DA,0x00DB,0x00DC,0x016C,0x015C,0x00DF,
                0x00E0,0x00E1,0x00E2,0xFFFF,0x00E4,0x010B,0x0109,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
                0xFFFF,0x00F1,0x00F2,0x00F3,0x00F4,0x0121,0x00F6,0x00F7,
                0x011D,0x00F9,0x00FA,0x00FB,0x00FC,0x016D,0x015D,0x02D9 } },

            { RTL_TEXTENCODING_ISO_8859_6,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0xFFFF,0xFFFF,0xFFFF,0x00A4,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x060C,0x00AD,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0x061B,0xFFFF,0xFFFF,0xFFFF,0x061F,
                0xFFFF,0x0621,0x0622,0x0623,0x0624,0x0625,0x0626,0x0627,
                0x0628,0x0629,0x062A,0x062B,0x062C,0x062D,0x062E,0x062F,
                0x0630,0x0631,0x0632,0x0633,0x0634,0x0635,0x0636,0x0637,
                0x0638,0x0639,0x063A,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x0640,0x0641,0x0642,0x0643,0x0644,0x0645,0x0646,0x0647,
                0x0648,0x0649,0x064A,0x064B,0x064C,0x064D,0x064E,0x064F,
                0x0650,0x0651,0x0652,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF } },

            { RTL_TEXTENCODING_ISO_8859_8,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0xFFFF,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00D7,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00F7,0x00BB,0x00BC,0x00BD,0x00BE,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x2017,
                0x05D0,0x05D1,0x05D2,0x05D3,0x05D4,0x05D5,0x05D6,0x05D7,
                0x05D8,0x05D9,0x05DA,0x05DB,0x05DC,0x05DD,0x05DE,0x05DF,
                0x05E0,0x05E1,0x05E2,0x05E3,0x05E4,0x05E5,0x05E6,0x05E7,
                0x05E8,0x05E9,0x05EA,0xFFFF,0xFFFF,0x200E,0x200F,0xFFFF } },

            { RTL_TEXTENCODING_TIS_620,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
                0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
                0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
                0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
                0x00A0,0x0E01,0x0E02,0x0E03,0x0E04,0x0E05,0x0E06,0x0E07, // !
                0x0E08,0x0E09,0x0E0A,0x0E0B,0x0E0C,0x0E0D,0x0E0E,0x0E0F,
                0x0E10,0x0E11,0x0E12,0x0E13,0x0E14,0x0E15,0x0E16,0x0E17,
                0x0E18,0x0E19,0x0E1A,0x0E1B,0x0E1C,0x0E1D,0x0E1E,0x0E1F,
                0x0E20,0x0E21,0x0E22,0x0E23,0x0E24,0x0E25,0x0E26,0x0E27,
                0x0E28,0x0E29,0x0E2A,0x0E2B,0x0E2C,0x0E2D,0x0E2E,0x0E2F,
                0x0E30,0x0E31,0x0E32,0x0E33,0x0E34,0x0E35,0x0E36,0x0E37,
                0x0E38,0x0E39,0x0E3A,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0E3F,
                0x0E40,0x0E41,0x0E42,0x0E43,0x0E44,0x0E45,0x0E46,0x0E47,
                0x0E48,0x0E49,0x0E4A,0x0E4B,0x0E4C,0x0E4D,0x0E4E,0x0E4F,
                0x0E50,0x0E51,0x0E52,0x0E53,0x0E54,0x0E55,0x0E56,0x0E57,
                0x0E58,0x0E59,0x0E5A,0x0E5B,0xFFFF,0xFFFF,0xFFFF,0xFFFF } },
            { RTL_TEXTENCODING_MS_874,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x2026,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x00A0,0x0E01,0x0E02,0x0E03,0x0E04,0x0E05,0x0E06,0x0E07,
                0x0E08,0x0E09,0x0E0A,0x0E0B,0x0E0C,0x0E0D,0x0E0E,0x0E0F,
                0x0E10,0x0E11,0x0E12,0x0E13,0x0E14,0x0E15,0x0E16,0x0E17,
                0x0E18,0x0E19,0x0E1A,0x0E1B,0x0E1C,0x0E1D,0x0E1E,0x0E1F,
                0x0E20,0x0E21,0x0E22,0x0E23,0x0E24,0x0E25,0x0E26,0x0E27,
                0x0E28,0x0E29,0x0E2A,0x0E2B,0x0E2C,0x0E2D,0x0E2E,0x0E2F,
                0x0E30,0x0E31,0x0E32,0x0E33,0x0E34,0x0E35,0x0E36,0x0E37,
                0x0E38,0x0E39,0x0E3A,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0E3F,
                0x0E40,0x0E41,0x0E42,0x0E43,0x0E44,0x0E45,0x0E46,0x0E47,
                0x0E48,0x0E49,0x0E4A,0x0E4B,0x0E4C,0x0E4D,0x0E4E,0x0E4F,
                0x0E50,0x0E51,0x0E52,0x0E53,0x0E54,0x0E55,0x0E56,0x0E57,
                0x0E58,0x0E59,0x0E5A,0x0E5B,0xFFFF,0xFFFF,0xFFFF,0xFFFF } },
            { RTL_TEXTENCODING_MS_1255,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0x02C6,0x2030,0xFFFF,0x2039,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x02DC,0x2122,0xFFFF,0x203A,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x00A0,0x00A1,0x00A2,0x00A3,0x20AA,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00D7,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00F7,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
                0x05B0,0x05B1,0x05B2,0x05B3,0x05B4,0x05B5,0x05B6,0x05B7,
                0x05B8,0x05B9,0xFFFF,0x05BB,0x05BC,0x05BD,0x05BE,0x05BF,
                0x05C0,0x05C1,0x05C2,0x05C3,0x05F0,0x05F1,0x05F2,0x05F3,
                0x05F4,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x05D0,0x05D1,0x05D2,0x05D3,0x05D4,0x05D5,0x05D6,0x05D7,
                0x05D8,0x05D9,0x05DA,0x05DB,0x05DC,0x05DD,0x05DE,0x05DF,
                0x05E0,0x05E1,0x05E2,0x05E3,0x05E4,0x05E5,0x05E6,0x05E7,
                0x05E8,0x05E9,0x05EA,0xFFFF,0xFFFF,0x200E,0x200F,0xFFFF } },
            { RTL_TEXTENCODING_MS_1256,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0x067E,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0x02C6,0x2030,0x0679,0x2039,0x0152,0x0686,0x0698,0x0688,
                0x06AF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x06A9,0x2122,0x0691,0x203A,0x0153,0x200C,0x200D,0x06BA,
                0x00A0,0x060C,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x06BE,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x061B,0x00BB,0x00BC,0x00BD,0x00BE,0x061F,
                0x06C1,0x0621,0x0622,0x0623,0x0624,0x0625,0x0626,0x0627,
                0x0628,0x0629,0x062A,0x062B,0x062C,0x062D,0x062E,0x062F,
                0x0630,0x0631,0x0632,0x0633,0x0634,0x0635,0x0636,0x00D7,
                0x0637,0x0638,0x0639,0x063A,0x0640,0x0641,0x0642,0x0643,
                0x00E0,0x0644,0x00E2,0x0645,0x0646,0x0647,0x0648,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x0649,0x064A,0x00EE,0x00EF,
                0x064B,0x064C,0x064D,0x064E,0x00F4,0x064F,0x0650,0x00F7,
                0x0651,0x00F9,0x0652,0x00FB,0x00FC,0x200E,0x200F,0x06D2 } },
            { RTL_TEXTENCODING_MS_1257,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0xFFFF,0x201E,0x2026,0x2020,0x2021,
                0xFFFF,0x2030,0xFFFF,0x2039,0xFFFF,0x00A8,0x02C7,0x00B8,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0xFFFF,0x2122,0xFFFF,0x203A,0xFFFF,0x00AF,0x02DB,0xFFFF,
                0x00A0,0xFFFF,0x00A2,0x00A3,0x00A4,0xFFFF,0x00A6,0x00A7,
                0x00D8,0x00A9,0x0156,0x00AB,0x00AC,0x00AD,0x00AE,0x00C6,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00F8,0x00B9,0x0157,0x00BB,0x00BC,0x00BD,0x00BE,0x00E6,
                0x0104,0x012E,0x0100,0x0106,0x00C4,0x00C5,0x0118,0x0112,
                0x010C,0x00C9,0x0179,0x0116,0x0122,0x0136,0x012A,0x013B,
                0x0160,0x0143,0x0145,0x00D3,0x014C,0x00D5,0x00D6,0x00D7,
                0x0172,0x0141,0x015A,0x016A,0x00DC,0x017B,0x017D,0x00DF,
                0x0105,0x012F,0x0101,0x0107,0x00E4,0x00E5,0x0119,0x0113,
                0x010D,0x00E9,0x017A,0x0117,0x0123,0x0137,0x012B,0x013C,
                0x0161,0x0144,0x0146,0x00F3,0x014D,0x00F5,0x00F6,0x00F7,
                0x0173,0x0142,0x015B,0x016B,0x00FC,0x017C,0x017E,0x02D9 } },
            { RTL_TEXTENCODING_MS_1258,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x20AC,0xFFFF,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
                0x02C6,0x2030,0xFFFF,0x2039,0x0152,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x02DC,0x2122,0xFFFF,0x203A,0x0153,0xFFFF,0xFFFF,0x0178,
                0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
                0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
                0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
                0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
                0x00C0,0x00C1,0x00C2,0x0102,0x00C4,0x00C5,0x00C6,0x00C7,
                0x00C8,0x00C9,0x00CA,0x00CB,0x0300,0x00CD,0x00CE,0x00CF,
                0x0110,0x00D1,0x0309,0x00D3,0x00D4,0x01A0,0x00D6,0x00D7,
                0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x01AF,0x0303,0x00DF,
                0x00E0,0x00E1,0x00E2,0x0103,0x00E4,0x00E5,0x00E6,0x00E7,
                0x00E8,0x00E9,0x00EA,0x00EB,0x0301,0x00ED,0x00EE,0x00EF,
                0x0111,0x00F1,0x0323,0x00F3,0x00F4,0x01A1,0x00F6,0x00F7,
                0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x01B0,0x20AB,0x00FF } },
            { RTL_TEXTENCODING_KOI8_U, // RFC 2319
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x2500,0x2502,0x250C,0x2510,0x2514,0x2518,0x251C,0x2524,
                0x252C,0x2534,0x253C,0x2580,0x2584,0x2588,0x258C,0x2590,
                0x2591,0x2592,0x2593,0x2320,0x25A0,0x2219,0x221A,0x2248,
                0x2264,0x2265,0x00A0,0x2321,0x00B0,0x00B2,0x00B7,0x00F7,
                0x2550,0x2551,0x2552,0x0451,0x0454,0x2554,0x0456,0x0457,
                0x2557,0x2558,0x2559,0x255A,0x255B,0x0491,0x255D,0x255E,
                0x255F,0x2560,0x2561,0x0401,0x0404,0x2563,0x0406,0x0407,
                0x2566,0x2567,0x2568,0x2569,0x256A,0x0490,0x256C,0x00A9,
                0x044E,0x0430,0x0431,0x0446,0x0434,0x0435,0x0444,0x0433,
                0x0445,0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,
                0x043F,0x044F,0x0440,0x0441,0x0442,0x0443,0x0436,0x0432,
                0x044C,0x044B,0x0437,0x0448,0x044D,0x0449,0x0447,0x044A,
                0x042E,0x0410,0x0411,0x0426,0x0414,0x0415,0x0424,0x0413,
                0x0425,0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,
                0x041F,0x042F,0x0420,0x0421,0x0422,0x0423,0x0416,0x0412,
                0x042C,0x042B,0x0417,0x0428,0x042D,0x0429,0x0427,0x042A } },
            { RTL_TEXTENCODING_ADOBE_STANDARD,
              { 0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x2019,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x2018,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x00A1,0x00A2,0x00A3,0x2215,0x00A5,0x0192,0x00A7,
                0x00A4,0x0027,0x201C,0x00AB,0x2039,0x203A,0xFB01,0xFB02,
                0xFFFF,0x2013,0x2020,0x2021,0x00B7,0xFFFF,0x00B6,0x2022,
                0x201A,0x201E,0x201D,0x00BB,0x2026,0x2030,0xFFFF,0x00BF,
                0xFFFF,0x0060,0x00B4,0x02C6,0x02DC,0x00AF,0x02D8,0x02D9,
                0x00A8,0xFFFF,0x02DA,0x00B8,0xFFFF,0x02DD,0x02DB,0x02C7,
                0x2014,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x00C6,0xFFFF,0x00AA,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x0141,0x00D8,0x0152,0x00BA,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0x00E6,0xFFFF,0xFFFF,0xFFFF,0x0131,0xFFFF,0xFFFF,
                0x0142,0x00F8,0x0153,0x00DF,0xFFFF,0xFFFF,0xFFFF,0xFFFF } },
            { RTL_TEXTENCODING_ADOBE_SYMBOL,
              { 0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x0020,0x0021,0x2200,0x0023,0x2203,0x0025,0x0026,0x220B,
                0x0028,0x0029,0x2217,0x002B,0x002C,0x2212,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x2245,0x0391,0x0392,0x03A7,0x0394,0x0395,0x03A6,0x0393,
                0x0397,0x0399,0x03D1,0x039A,0x039B,0x039C,0x039D,0x039F,
                0x03A0,0x0398,0x03A1,0x03A3,0x03A4,0x03A5,0x03C2,0x03A9,
                0x039E,0x03A8,0x0396,0x005B,0x2234,0x005D,0x22A5,0x005F,
                0xF8E5,0x03B1,0x03B2,0x03C7,0x03B4,0x03B5,0x03C6,0x03B3,
                0x03B7,0x03B9,0x03D5,0x03BA,0x03BB,0x03BC,0x03BD,0x03BF,
                0x03C0,0x03B8,0x03C1,0x03C3,0x03C4,0x03C5,0x03D6,0x03C9,
                0x03BE,0x03C8,0x03B6,0x007B,0x007C,0x007D,0x223C,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0x20AC,0x03D2,0x2032,0x2264,0x2215,0x221E,0x0192,0x2663,
                0x2666,0x2665,0x2660,0x2194,0x2190,0x2191,0x2192,0x2193,
                0x00B0,0x00B1,0x2033,0x2265,0x00D7,0x221D,0x2202,0x2022,
                0x00F7,0x2260,0x2261,0x2248,0x2026,0x23AF,0x23D0,0x21B5,
                0x2135,0x2111,0x211C,0x2118,0x2297,0x2295,0x2205,0x2229,
                0x222A,0x2283,0x2287,0x2284,0x2282,0x2286,0x2208,0x2209,
                0x2220,0x2207,0xF6DA,0xF6D9,0xF6DB,0x220F,0x221A,0x22C5,
                0x00AC,0x2227,0x2228,0x21D4,0x21D0,0x21D1,0x21D2,0x21D3,
                0x25CA,0x2329,0xF8E8,0xF8E9,0xF8EA,0x2211,0x239B,0x239C,
                0x239D,0x23A1,0x23A2,0x23A3,0x23A7,0x23A8,0x23A9,0x23AA,
                0xFFFF,0x232A,0x222B,0x2320,0x23AE,0x2321,0x239E,0x239F,
                0x23A0,0x23A4,0x23A5,0x23A6,0x23AB,0x23AC,0x23AD,0xFFFF } },
            { RTL_TEXTENCODING_ADOBE_DINGBATS,
              { 0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
// 20
                0x0020,0x2701,0x2702,0x2703,0x2704,0x260E,0x2706,0x2707,
                0x2708,0x2709,0x261B,0x261E,0x270C,0x270D,0x270E,0x270F,
                0x2710,0x2711,0x2712,0x2713,0x2714,0x2715,0x2716,0x2717,
                0x2718,0x2719,0x271A,0x271B,0x271C,0x271D,0x271E,0x271F,
// 40
                0x2720,0x2721,0x2722,0x2723,0x2724,0x2725,0x2726,0x2727,
                0x2605,0x2729,0x272A,0x272B,0x272C,0x272D,0x272E,0x272F,
                0x2730,0x2731,0x2732,0x2733,0x2734,0x2735,0x2736,0x2737,
                0x2738,0x2739,0x273A,0x273B,0x273C,0x273D,0x273E,0x273F,
// 60
                0x2740,0x2741,0x2742,0x2743,0x2744,0x2745,0x2746,0x2747,
                0x2748,0x2749,0x274A,0x274B,0x25CF,0x274D,0x25A0,0x274F,
                0x2750,0x2751,0x2752,0x25B2,0x25BC,0x25C6,0x2756,0x25D7,
                0x2758,0x2759,0x275A,0x275B,0x275C,0x275D,0x275E,0xFFFF,
// 80
                0xF8D7,0xF8D8,0xF8D9,0xF8DA,0xF8DB,0xF8DC,0xF8DD,0xF8DE,
                0xF8DF,0xF8E0,0xF8E1,0xF8E2,0xF8E3,0xF8E4,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
// A0
                0xFFFF,0x2761,0x2762,0x2763,0x2764,0x2765,0x2766,0x2767,
                0x2663,0x2666,0x2665,0x2660,0x2460,0x2461,0x2462,0x2463,
                0x2464,0x2465,0x2466,0x2467,0x2468,0x2469,0x2776,0x2777,
                0x2778,0x2779,0x277A,0x277B,0x277C,0x277D,0x277E,0x277F,
// C0
                0x2780,0x2781,0x2782,0x2783,0x2784,0x2785,0x2786,0x2787,
                0x2788,0x2789,0x278A,0x278B,0x278C,0x278D,0x278E,0x278F,
                0x2790,0x2791,0x2792,0x2793,0x2794,0x2795,0x2796,0x2797,
                0x2798,0x2799,0x279A,0x279B,0x279C,0x279D,0x279E,0x279F,
// E0
                0x27A0,0x27A1,0x27A2,0x27A3,0x27A4,0x27A5,0x27A6,0x27A7,
                0x27A8,0x27A9,0x27AA,0x27AB,0x27AC,0x27AD,0x27AE,0x27AF,
                0xFFFF,0x27B1,0x27B2,0x27B3,0x27B4,0x27B5,0x27B6,0x27B7,
                0x27B8,0x27B9,0x27BA,0x27BB,0x27BC,0x27BD,0x27BE,0xFFFF } },
            { RTL_TEXTENCODING_PT154,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x0496,0x0492,0x04EE,0x0493,0x201E,0x2026,0x04B6,0x04AE,
                0x04B2,0x04AF,0x04A0,0x04E2,0x04A2,0x049A,0x04BA,0x04B8,
                0x0497,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
                0x04B3,0x04B7,0x04A1,0x04E3,0x04A3,0x049B,0x04BB,0x04B9,
                0x00A0,0x040E,0x045E,0x0408,0x04E8,0x0498,0x04B0,0x00A7,
                0x0401,0x00A9,0x04D8,0x00AB,0x00AC,0x04EF,0x00AE,0x049C,
                0x00B0,0x04B1,0x0406,0x0456,0x0499,0x04E9,0x00B6,0x00B7,
                0x0451,0x2116,0x04D9,0x00BB,0x0458,0x04AA,0x04AB,0x049D,
                0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
                0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
                0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
                0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
                0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
                0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
                0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
                0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F } },
            { RTL_TEXTENCODING_KAMENICKY,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x010C,0x00FC,0x00E9,0x010F,0x00E4,0x010E,0x0164,0x010D,
                0x011B,0x011A,0x0139,0x00CD,0x013E,0x013A,0x00C4,0x00C1,
                0x00C9,0x017E,0x017D,0x00F4,0x00F6,0x00D3,0x016F,0x00DA,
                0x00FD,0x00D6,0x00DC,0x0160,0x013D,0x00DD,0x0158,0x0165,
                0x00E1,0x00ED,0x00F3,0x00FA,0x0148,0x0147,0x016E,0x00D4,
                0x0161,0x0159,0x0155,0x0154,0x00BC,0x00A7,0x00AB,0x00BB,
                0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
                0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
                0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
                0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
                0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
                0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
                0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,
                0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
                0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,
                0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0 } },
            { RTL_TEXTENCODING_MAZOVIA,
              { 0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,
                0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
                0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,
                0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
                0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
                0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
                0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,
                0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,
                0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
                0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
                0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
                0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,
                0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
                0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,
                0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
                0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x0105,0x00E7,
                0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x0107,0x00C4,0x0104,
                0x0118,0x0119,0x0142,0x00F4,0x00F6,0x0106,0x00FB,0x00F9,
                0x015A,0x00D6,0x00DC,0x00A2,0x0141,0x00A5,0x015B,0x0192,
                0x0179,0x017B,0x00F3,0x00D3,0x0144,0x0143,0x017A,0x017C,
                0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
                0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
                0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
                0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
                0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
                0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
                0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
                0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,
                0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
                0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,
                0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0 } } };
    for (auto const& aDatum : data)
    {
        testSingleByteCharSet(aDatum);
    }
}

void Test::testComplex() {
    static ComplexCharSetTest const data[]
        = { { RTL_TEXTENCODING_ASCII_US,
              RTL_CONSTASCII_STRINGPARAM("\x01\"3De$~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E },
              7,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_EUC_CN,
              RTL_CONSTASCII_STRINGPARAM("\x01\"3De$~\xA1\xB9\xF0\xC5"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E,
                0x300D,0x9E4B },
              9,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_TW,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x01\"3De$~\xC5\xF0\x8E\xA4\xDC\xD9"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E,
                0x4ED9,0xD87E,0xDD68 },
              10,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_GB_18030,
              RTL_CONSTASCII_STRINGPARAM("\x01\"3De$~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E },
              7,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_GB_18030,
              RTL_CONSTASCII_STRINGPARAM("\x81\x40\xFE\xFE"),
              { 0x4E02,0xE4C5 },
              2,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_GB_18030,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x81\x30\xB1\x33\x81\x30\xD3\x30\x81\x36\xA5\x31"),
              { 0x028A,0x0452,0x200F },
              3,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_GB_18030,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xFE\x50\xFE\x51\xFE\x52\xFE\x53\xFE\x54\xFE\x55\xFE\x56"
                  "\xFE\x57\xFE\x58\xFE\x59\xFE\x5A\xFE\x5B\xFE\x5C\xFE\x5D"
                  "\xFE\x5E\xFE\x5F\xFE\x60\xFE\x61\xFE\x62\xFE\x63\xFE\x64"
                  "\xFE\x65\xFE\x66\xFE\x67\xFE\x68\xFE\x69\xFE\x6A\xFE\x6B"
                  "\xFE\x6C\xFE\x6D\xFE\x6E\xFE\x6F\xFE\x70\xFE\x71\xFE\x72"
                  "\xFE\x73\xFE\x74\xFE\x75\xFE\x76\xFE\x77\xFE\x78\xFE\x79"
                  "\xFE\x7A\xFE\x7B\xFE\x7C\xFE\x7D\xFE\x7E\xFE\x80\xFE\x81"
                  "\xFE\x82\xFE\x83\xFE\x84\xFE\x85\xFE\x86\xFE\x87\xFE\x88"
                  "\xFE\x89\xFE\x8A\xFE\x8B\xFE\x8C\xFE\x8D\xFE\x8E\xFE\x8F"
                  "\xFE\x90\xFE\x91\xFE\x92\xFE\x93\xFE\x94\xFE\x95\xFE\x96"
                  "\xFE\x97\xFE\x98\xFE\x99\xFE\x9A\xFE\x9B\xFE\x9C\xFE\x9D"
                  "\xFE\x9E\xFE\x9F\xFE\xA0"),
              { 0x2E81,0xE816,0xE817,0xE818,0x2E84,0x3473,0x3447,0x2E88,
                0x2E8B,0xE81E,0x359E,0x361A,0x360E,0x2E8C,0x2E97,0x396E,
                0x3918,0xE826,0x39CF,0x39DF,0x3A73,0x39D0,0xE82B,0xE82C,
                0x3B4E,0x3C6E,0x3CE0,0x2EA7,0xE831,0xE832,0x2EAA,0x4056,
                0x415F,0x2EAE,0x4337,0x2EB3,0x2EB6,0x2EB7,0xE83B,0x43B1,
                0x43AC,0x2EBB,0x43DD,0x44D6,0x4661,0x464C,0xE843,0x4723,
                0x4729,0x477C,0x478D,0x2ECA,0x4947,0x497A,0x497D,0x4982,
                0x4983,0x4985,0x4986,0x499F,0x499B,0x49B7,0x49B6,0xE854,
                0xE855,0x4CA3,0x4C9F,0x4CA0,0x4CA1,0x4C77,0x4CA2,0x4D13,
                0x4D14,0x4D15,0x4D16,0x4D17,0x4D18,0x4D19,0x4DAE,0xE864 },
              80,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM("\x01\"3De$\\~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x005C,0x007E },
              8,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM("\x1B(B\x01\"3De$\\~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x005C,0x007E },
              8,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM("\x1B(J\x01\"3De$\\~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x00A5,0x00AF },
              8,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM("\x1B$B\x26\x21\x27\x71\x1B(B"),
              { 0x0391,0x044F },
              2,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ko
            { RTL_TEXTENCODING_ISO_2022_KR,
              RTL_CONSTASCII_STRINGPARAM("\x1B$)C\x01\"3De$\\~"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x005C,0x007E },
              8,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_KR,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x1B$)C\x0E\x25\x21\x0F\x0D\x0Ax\x0E\x48\x7E\x0F"),
              { 0x2170,0x000D,0x000A,0x0078,0xD79D },
              5,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_ISO_2022_CN,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x01\"3De$\\~\x1B$)G\x0E\x45\x70\x1B$*H\x1BN\x22\x22"
                      "\x45\x70\x0F\x1B$)A\x0E\x26\x21\x0F"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x005C,0x007E,
                0x4ED9,0x531F,0x4ED9,0x0391 },
              12,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_CN,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x01\"3De$\\~\x1B$)A\x0E\x26\x21\x1B$*H\x1BN\x22\x22"
                      "\x26\x21\x0F\x0D\x0A\x1B$)A\x0E\x26\x21\x0F"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x005C,0x007E,
                0x0391,0x531F,0x0391,0x000D,0x000A,0x0391 },
              14,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            // The following does not work as long as Big5-HKSCS maps to
            // Unicode PUA instead of Plane 2.  Use the next two tests
            // instead:
//          { RTL_TEXTENCODING_BIG5_HKSCS,
//            RTL_CONSTASCII_STRINGPARAM(
//                "\x01\"3De$~\x88\x56\xF9\xFE\xFA\x5E\xA1\x40\xF9\xD5"),
//            { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E,0x0100,
//              0xFFED,0xD849,0xDD13,0x3000,0x9F98 },
//            13,
//            true,
//            true,
//            true,
//            false,
//            RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x01\"3De$~\x88\x56\xF9\xFE\xFA\x5E\xA1\x40\xF9\xD5"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E,0x0100,
                0xFFED,0xE01E,0x3000,0x9F98 },
              12,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x01\"3De$~\x88\x56\xF9\xFE\xFA\x5E\xA1\x40\xF9\xD5"),
              { 0x0001,0x0022,0x0033,0x0044,0x0065,0x0024,0x007E,0x0100,
                0xFFED,0xD849,0xDD13,0x3000,0x9F98 },
              13,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC6\xA1\xC6\xCF\xC6\xD3\xC6\xD5\xC6\xD7\xC6\xDE\xC6\xDF"
                  "\xC6\xFE\xC7\x40\xC7\x7E\xC7\xA1\xC7\xFE"),
              { 0x2460,0xF6E0,0xF6E4,0xF6E6,0xF6E8,0xF6EF,0xF6F0,0x3058,
                0x3059,0x30A4,0x30A5,0x041A },
              12,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM("\x81\x40\x84\xFE"),
              { 0xEEB8,0xF12B },
              2,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x81\x40\x8D\xFE\x8E\x40\xA0\xFE\xC6\xA1\xC8\xFE\xFA\x40"
                  "\xFE\xFE"),
              { 0xEEB8,0xF6B0,0xE311,0xEEB7,0xF6B1,0xF848,0xE000,0xE310 },
              8,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM("\xAD\xC5\x94\x55"),
              { 0x5029,0x7250 },
              2,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM("\xFA\x5F\xA0\xE4"),
              { 0x5029,0x7250 },
              2,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM("\xA0\x40\xA0\x7E\xA0\xA1\xA0\xFE"),
              { 0xEE1B,0xEE59,0xEE5A,0xEEB7 },
              4,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5,
              RTL_CONSTASCII_STRINGPARAM("\xA1\x45"),
              { 0x2027 },
              1,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC6\xCF\xC6\xD3\xC6\xD5\xC6\xD7\xC6\xDE\xC6\xDF"),
              { 0x306B,0x306F,0x3071,0x3073,0x307A,0x307B },
              6,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC7\xFD\xC7\xFE\xC8\x40\xC8\x7E\xC8\xA1\xC8\xFE"),
              { 0xF7AA,0xF7AB,0xF7AC,0xF7EA,0xF7EB,0xF848 },
              6,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5,
              RTL_CONSTASCII_STRINGPARAM("\xA0\x40\xA0\x7E\xA0\xA1\xA0\xFE"),
              { 0xEE1B,0xEE59,0xEE5A,0xEEB7 },
              4,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            { RTL_TEXTENCODING_MS_950,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC6\xA1\xC6\xFE\xC7\x40\xC7\x7E\xC7\xA1\xC7\xFE\xC8\x40"
                  "\xC8\x7E\xC8\xA1\xC8\xFE"),
              { 0xF6B1,0xF70E,0xF70F,0xF74D,0xF74E,0xF7AB,0xF7AC,0xF7EA,
                0xF7EB,0xF848 },
              10,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_950,
              RTL_CONSTASCII_STRINGPARAM("\xA0\x40\xA0\x7E\xA0\xA1\xA0\xFE"),
              { 0xEE1B,0xEE59,0xEE5A,0xEEB7 },
              4,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            // Test Unicode beyond BMP:

            // FIXME The second m_bForward test (requiring a context) does not
            // work for UTF7:
//          { RTL_TEXTENCODING_UTF7,
//            RTL_CONSTASCII_STRINGPARAM("+2EndEw-"),
//            { 0xD849,0xDD13 },
//            2,
//            true,
//            true,
//            true,
//            false,
//            RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xF0\xA2\x94\x93"),
              { 0xD849,0xDD13 },
              2,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_GB_18030,
              RTL_CONSTASCII_STRINGPARAM("\x95\x39\xC5\x37"),
              { 0xD849,0xDD13 },
              2,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_BIG5_HKSCS,
              RTL_CONSTASCII_STRINGPARAM("\xFA\x5E"),
              { 0xD849,0xDD13 },
              2,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            // Test GBK (aka CP936):

            { RTL_TEXTENCODING_GBK,
              RTL_CONSTASCII_STRINGPARAM("\xFD\x7C\xC1\xFA\xFD\x9B"),
              { 0x9F76,0x9F99,0x9FA5 },
              3,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            { RTL_TEXTENCODING_MS_936,
              RTL_CONSTASCII_STRINGPARAM("\xFD\x7C\xC1\xFA\xFD\x9B"),
              { 0x9F76,0x9F99,0x9FA5 },
              3,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_GBK,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xFE\x50\xFE\x54\xFE\x55\xFE\x56"
                  "\xFE\x57\xFE\x58\xFE\x5A\xFE\x5B\xFE\x5C\xFE\x5D"
                  "\xFE\x5E\xFE\x5F\xFE\x60\xFE\x62\xFE\x63\xFE\x64"
                  "\xFE\x65\xFE\x68\xFE\x69\xFE\x6A\xFE\x6B"
                  "\xFE\x6E\xFE\x6F\xFE\x70\xFE\x71\xFE\x72"
                  "\xFE\x73\xFE\x74\xFE\x75\xFE\x77\xFE\x78\xFE\x79"
                  "\xFE\x7A\xFE\x7B\xFE\x7C\xFE\x7D\xFE\x80\xFE\x81"
                  "\xFE\x82\xFE\x83\xFE\x84\xFE\x85\xFE\x86\xFE\x87\xFE\x88"
                  "\xFE\x89\xFE\x8A\xFE\x8B\xFE\x8C\xFE\x8D\xFE\x8E\xFE\x8F"
                  "\xFE\x92\xFE\x93\xFE\x94\xFE\x95\xFE\x96"
                  "\xFE\x97\xFE\x98\xFE\x99\xFE\x9A\xFE\x9B\xFE\x9C\xFE\x9D"
                  "\xFE\x9E\xFE\x9F"),
              { 0x2E81,0x2E84,0x3473,0x3447,0x2E88,0x2E8B,0x359E,0x361A,
                0x360E,0x2E8C,0x2E97,0x396E,0x3918,0x39CF,0x39DF,0x3A73,
                0x39D0,0x3B4E,0x3C6E,0x3CE0,0x2EA7,0x2EAA,0x4056,0x415F,
                0x2EAE,0x4337,0x2EB3,0x2EB6,0x2EB7,0x43B1,0x43AC,0x2EBB,
                0x43DD,0x44D6,0x4661,0x464C,0x4723,0x4729,0x477C,0x478D,
                0x2ECA,0x4947,0x497A,0x497D,0x4982,0x4983,0x4985,0x4986,
                0x499F,0x499B,0x49B7,0x49B6,0x4CA3,0x4C9F,0x4CA0,0x4CA1,
                0x4C77,0x4CA2,0x4D13,0x4D14,0x4D15,0x4D16,0x4D17,0x4D18,
                0x4D19,0x4DAE },
              66,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("?"),
              { 0xFF0D },
              1,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_QUESTIONMARK },
#endif
            // Test of "JIS X 0208 row 13" (taken from CP932; added to
            // ISO-2022-JP and EUC-JP; 74 of the 83 characters introduce
            // mappings to new Unicode characters):
            { RTL_TEXTENCODING_MS_932,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x87\x40\x87\x41\x87\x42\x87\x43\x87\x44\x87\x45\x87\x46"
                  "\x87\x47\x87\x48\x87\x49\x87\x4A\x87\x4B\x87\x4C\x87\x4D"
                  "\x87\x4E\x87\x4F\x87\x50\x87\x51\x87\x52\x87\x53\x87\x54"
                  "\x87\x55\x87\x56\x87\x57\x87\x58\x87\x59\x87\x5A\x87\x5B"
                  "\x87\x5C\x87\x5D\x87\x5F\x87\x60\x87\x61\x87\x62\x87\x63"
                  "\x87\x64\x87\x65\x87\x66\x87\x67\x87\x68\x87\x69\x87\x6A"
                  "\x87\x6B\x87\x6C\x87\x6D\x87\x6E\x87\x6F\x87\x70\x87\x71"
                  "\x87\x72\x87\x73\x87\x74\x87\x75\x87\x7E\x87\x80\x87\x81"
                  "\x87\x82\x87\x83\x87\x84\x87\x85\x87\x86\x87\x87\x87\x88"
                  "\x87\x89\x87\x8A\x87\x8B\x87\x8C\x87\x8D\x87\x8E\x87\x8F"
                  "\x87\x90\x87\x91\x87\x92\x87\x93\x87\x94\x87\x95\x87\x96"
                  "\x87\x97\x87\x98\x87\x99\x87\x9A\x87\x9B\x87\x9C"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x2252,0x2261,
                0x222B,0x222E,0x2211,0x221A,0x22A5,0x2220,0x221F,0x22BF,0x2235,
                0x2229,0x222A },
              83,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM("\x00\xFA\x6F\xFA\x71"),
              {0x0000, 0x4F92, 0x4F9A},
              3,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x87\x40\x87\x41\x87\x42\x87\x43\x87\x44\x87\x45\x87\x46"
                  "\x87\x47\x87\x48\x87\x49\x87\x4A\x87\x4B\x87\x4C\x87\x4D"
                  "\x87\x4E\x87\x4F\x87\x50\x87\x51\x87\x52\x87\x53\x87\x54"
                  "\x87\x55\x87\x56\x87\x57\x87\x58\x87\x59\x87\x5A\x87\x5B"
                  "\x87\x5C\x87\x5D\x87\x5F\x87\x60\x87\x61\x87\x62\x87\x63"
                  "\x87\x64\x87\x65\x87\x66\x87\x67\x87\x68\x87\x69\x87\x6A"
                  "\x87\x6B\x87\x6C\x87\x6D\x87\x6E\x87\x6F\x87\x70\x87\x71"
                  "\x87\x72\x87\x73\x87\x74\x87\x75\x87\x7E\x87\x80\x87\x81"
                  "\x87\x82\x87\x83\x87\x84\x87\x85\x87\x86\x87\x87\x87\x88"
                  "\x87\x89\x87\x8A\x87\x8B\x87\x8C\x87\x8D\x87\x8E\x87\x8F"
                  "\x87\x90\x87\x91\x87\x92\x87\x93\x87\x94\x87\x95\x87\x96"
                  "\x87\x97\x87\x98\x87\x99\x87\x9A\x87\x9B\x87\x9C"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x2252,0x2261,
                0x222B,0x222E,0x2211,0x221A,0x22A5,0x2220,0x221F,0x22BF,0x2235,
                0x2229,0x222A },
              83,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x1B$B\x2D\x21\x2D\x22\x2D\x23\x2D\x24\x2D\x25\x2D\x26"
                  "\x2D\x27\x2D\x28\x2D\x29\x2D\x2A\x2D\x2B\x2D\x2C\x2D\x2D"
                  "\x2D\x2E\x2D\x2F\x2D\x30\x2D\x31\x2D\x32\x2D\x33\x2D\x34"
                  "\x2D\x35\x2D\x36\x2D\x37\x2D\x38\x2D\x39\x2D\x3A\x2D\x3B"
                  "\x2D\x3C\x2D\x3D\x2D\x3E\x2D\x40\x2D\x41\x2D\x42\x2D\x43"
                  "\x2D\x44\x2D\x45\x2D\x46\x2D\x47\x2D\x48\x2D\x49\x2D\x4A"
                  "\x2D\x4B\x2D\x4C\x2D\x4D\x2D\x4E\x2D\x4F\x2D\x50\x2D\x51"
                  "\x2D\x52\x2D\x53\x2D\x54\x2D\x55\x2D\x56\x2D\x5F\x2D\x60"
                  "\x2D\x61\x2D\x62\x2D\x63\x2D\x64\x2D\x65\x2D\x66\x2D\x67"
                  "\x2D\x68\x2D\x69\x2D\x6A\x2D\x6B\x2D\x6C\x2D\x6D\x2D\x6E"
                  "\x2D\x6F\x2D\x70\x2D\x71\x2D\x72\x2D\x73\x2D\x74\x2D\x75"
                  "\x2D\x76\x2D\x77\x2D\x78\x2D\x79\x2D\x7A\x2D\x7B\x2D\x7C"
                  "\x1B(B"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x2252,0x2261,
                0x222B,0x222E,0x2211,0x221A,0x22A5,0x2220,0x221F,0x22BF,0x2235,
                0x2229,0x222A },
              83,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_2022_JP,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x1B$B\x2D\x21\x2D\x22\x2D\x23\x2D\x24\x2D\x25\x2D\x26"
                  "\x2D\x27\x2D\x28\x2D\x29\x2D\x2A\x2D\x2B\x2D\x2C\x2D\x2D"
                  "\x2D\x2E\x2D\x2F\x2D\x30\x2D\x31\x2D\x32\x2D\x33\x2D\x34"
                  "\x2D\x35\x2D\x36\x2D\x37\x2D\x38\x2D\x39\x2D\x3A\x2D\x3B"
                  "\x2D\x3C\x2D\x3D\x2D\x3E\x2D\x40\x2D\x41\x2D\x42\x2D\x43"
                  "\x2D\x44\x2D\x45\x2D\x46\x2D\x47\x2D\x48\x2D\x49\x2D\x4A"
                  "\x2D\x4B\x2D\x4C\x2D\x4D\x2D\x4E\x2D\x4F\x2D\x50\x2D\x51"
                  "\x2D\x52\x2D\x53\x2D\x54\x2D\x55\x2D\x56\x2D\x5F\x2D\x60"
                  "\x2D\x61\x2D\x62\x2D\x63\x2D\x64\x2D\x65\x2D\x66\x2D\x67"
                  "\x2D\x68\x2D\x69\x2D\x6A\x2D\x6B\x2D\x6C\x2D\x6D\x2D\x6E"
                  "\x2D\x6F\x2D\x73\x2D\x74\x2D\x78\x2D\x79\x1B(B"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x222E,0x2211,
                0x221F,0x22BF },
              74,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xAD\xA1\xAD\xA2\xAD\xA3\xAD\xA4\xAD\xA5\xAD\xA6\xAD\xA7"
                  "\xAD\xA8\xAD\xA9\xAD\xAA\xAD\xAB\xAD\xAC\xAD\xAD\xAD\xAE"
                  "\xAD\xAF\xAD\xB0\xAD\xB1\xAD\xB2\xAD\xB3\xAD\xB4\xAD\xB5"
                  "\xAD\xB6\xAD\xB7\xAD\xB8\xAD\xB9\xAD\xBA\xAD\xBB\xAD\xBC"
                  "\xAD\xBD\xAD\xBE\xAD\xC0\xAD\xC1\xAD\xC2\xAD\xC3\xAD\xC4"
                  "\xAD\xC5\xAD\xC6\xAD\xC7\xAD\xC8\xAD\xC9\xAD\xCA\xAD\xCB"
                  "\xAD\xCC\xAD\xCD\xAD\xCE\xAD\xCF\xAD\xD0\xAD\xD1\xAD\xD2"
                  "\xAD\xD3\xAD\xD4\xAD\xD5\xAD\xD6\xAD\xDF\xAD\xE0\xAD\xE1"
                  "\xAD\xE2\xAD\xE3\xAD\xE4\xAD\xE5\xAD\xE6\xAD\xE7\xAD\xE8"
                  "\xAD\xE9\xAD\xEA\xAD\xEB\xAD\xEC\xAD\xED\xAD\xEE\xAD\xEF"
                  "\xAD\xF0\xAD\xF1\xAD\xF2\xAD\xF3\xAD\xF4\xAD\xF5\xAD\xF6"
                  "\xAD\xF7\xAD\xF8\xAD\xF9\xAD\xFA\xAD\xFB\xAD\xFC"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x2252,0x2261,
                0x222B,0x222E,0x2211,0x221A,0x22A5,0x2220,0x221F,0x22BF,0x2235,
                0x2229,0x222A },
              83,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xAD\xA1\xAD\xA2\xAD\xA3\xAD\xA4\xAD\xA5\xAD\xA6\xAD\xA7"
                  "\xAD\xA8\xAD\xA9\xAD\xAA\xAD\xAB\xAD\xAC\xAD\xAD\xAD\xAE"
                  "\xAD\xAF\xAD\xB0\xAD\xB1\xAD\xB2\xAD\xB3\xAD\xB4\xAD\xB5"
                  "\xAD\xB6\xAD\xB7\xAD\xB8\xAD\xB9\xAD\xBA\xAD\xBB\xAD\xBC"
                  "\xAD\xBD\xAD\xBE\xAD\xC0\xAD\xC1\xAD\xC2\xAD\xC3\xAD\xC4"
                  "\xAD\xC5\xAD\xC6\xAD\xC7\xAD\xC8\xAD\xC9\xAD\xCA\xAD\xCB"
                  "\xAD\xCC\xAD\xCD\xAD\xCE\xAD\xCF\xAD\xD0\xAD\xD1\xAD\xD2"
                  "\xAD\xD3\xAD\xD4\xAD\xD5\xAD\xD6\xAD\xDF\xAD\xE0\xAD\xE1"
                  "\xAD\xE2\xAD\xE3\xAD\xE4\xAD\xE5\xAD\xE6\xAD\xE7\xAD\xE8"
                  "\xAD\xE9\xAD\xEA\xAD\xEB\xAD\xEC\xAD\xED\xAD\xEE\xAD\xEF"
                  "\xAD\xF3\xAD\xF4\xAD\xF8\xAD\xF9"),
              { 0x2460,0x2461,0x2462,0x2463,0x2464,0x2465,0x2466,0x2467,0x2468,
                0x2469,0x246A,0x246B,0x246C,0x246D,0x246E,0x246F,0x2470,0x2471,
                0x2472,0x2473,0x2160,0x2161,0x2162,0x2163,0x2164,0x2165,0x2166,
                0x2167,0x2168,0x2169,0x3349,0x3314,0x3322,0x334D,0x3318,0x3327,
                0x3303,0x3336,0x3351,0x3357,0x330D,0x3326,0x3323,0x332B,0x334A,
                0x333B,0x339C,0x339D,0x339E,0x338E,0x338F,0x33C4,0x33A1,0x337B,
                0x301D,0x301F,0x2116,0x33CD,0x2121,0x32A4,0x32A5,0x32A6,0x32A7,
                0x32A8,0x3231,0x3232,0x3239,0x337E,0x337D,0x337C,0x222E,0x2211,
                0x221F,0x22BF },
              74,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("\xB9\xF5"),
              { 0x9ED2 },
              1,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            // Test ISO-8859-x/MS-125x range 0x80--9F:

            { RTL_TEXTENCODING_ISO_8859_1,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_2,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_3,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_4,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_5,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_6,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_7,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_8,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_9,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_14,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISO_8859_15,
              RTL_CONSTASCII_STRINGPARAM(
                  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E"
                  "\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D"
                  "\x9E\x9F"),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_874,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1250,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1251,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1252,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1253,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1254,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1255,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1256,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1257,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_1258,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,
                0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,0x0090,0x0091,
                0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,
                0x009B,0x009C,0x009D,0x009E,0x009F },
              32,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_MS_949,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xB0\xA1\xB0\xA2\x81\x41\x81\x42\xB0\xA3\x81\x43\x81\x44"
                  "\xB0\xA4\xB0\xA5\xB0\xA6\xB0\xA7\x81\x45\x81\x46\x81\x47"
                  "\x81\x48\x81\x49\xB0\xA8\xB0\xA9\xB0\xAA\xB0\xAB\xB0\xAC"
                  "\xB0\xAD\xB0\xAE\xB0\xAF\x81\x4A\xB0\xB0\xB0\xB1\xB0\xB2"),
              { 0xAC00,0xAC01,0xAC02,0xAC03,0xAC04,0xAC05,0xAC06,0xAC07,0xAC08,
                0xAC09,0xAC0A,0xAC0B,0xAC0C,0xAC0D,0xAC0E,0xAC0F,0xAC10,0xAC11,
                0xAC12,0xAC13,0xAC14,0xAC15,0xAC16,0xAC17,0xAC18,0xAC19,0xAC1A,
                0xAC1B },
              28,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_949,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC9\xA1\xC9\xA2\xC9\xA3\xC9\xFC\xC9\xFD\xC9\xFE"
                  "\xFE\xA1\xFE\xA2\xFE\xA3\xFE\xFC\xFE\xFD\xFE\xFE"),
              { 0xE000,0xE001,0xE002,0xE05B,0xE05C,0xE05D,
                0xE05E,0xE05F,0xE060,0xE0B9,0xE0BA,0xE0BB },
              12,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_ko
            { RTL_TEXTENCODING_EUC_KR,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xB0\xA1\xB0\xA2"              "\xB0\xA3"
                  "\xB0\xA4\xB0\xA5\xB0\xA6\xB0\xA7"
                                  "\xB0\xA8\xB0\xA9\xB0\xAA\xB0\xAB\xB0\xAC"
                  "\xB0\xAD\xB0\xAE\xB0\xAF"      "\xB0\xB0\xB0\xB1\xB0\xB2"),
              { 0xAC00,0xAC01,              0xAC04,              0xAC07,0xAC08,
                0xAC09,0xAC0A,                                   0xAC10,0xAC11,
                0xAC12,0xAC13,0xAC14,0xAC15,0xAC16,0xAC17,       0xAC19,0xAC1A,
                0xAC1B },
              18,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_KR,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xB0\xA1\xB0\xA2"              "\xB0\xA3"
                  "\xB0\xA4\xB0\xA5\xB0\xA6\xB0\xA7"
                                  "\xB0\xA8\xB0\xA9\xB0\xAA\xB0\xAB\xB0\xAC"
                  "\xB0\xAD\xB0\xAE\xB0\xAF"      "\xB0\xB0\xB0\xB1\xB0\xB2"),
              { 0xAC00,0xAC01,0xAC02,0xAC03,0xAC04,0xAC05,0xAC06,0xAC07,0xAC08,
                0xAC09,0xAC0A,0xAC0B,0xAC0C,0xAC0D,0xAC0E,0xAC0F,0xAC10,0xAC11,
                0xAC12,0xAC13,0xAC14,0xAC15,0xAC16,0xAC17,0xAC18,0xAC19,0xAC1A,
                0xAC1B },
              28,
              true,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_EUC_KR,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC9\xA1\xC9\xA2\xC9\xA3\xC9\xFC\xC9\xFD\xC9\xFE"
                  "\xFE\xA1\xFE\xA2\xFE\xA3\xFE\xFC\xFE\xFD\xFE\xFE"),
              { 0xE000,0xE001,0xE002,0xE05B,0xE05C,0xE05D,
                0xE05E,0xE05F,0xE060,0xE0B9,0xE0BA,0xE0BB },
              12,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            // Test UTF-8:

            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\x00"),
              { 0x0000 },
              1,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xEF\xBB\xBF"),
              { 0xFEFF },
              1,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xEF\xBB\xBF\xEF\xBB\xBF"),
              { 0xFEFF,0xFEFF },
              2,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xEF\xBB\xBF"),
              { 0 },
              0,
              false,
              true,
              true,
              true,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xEF\xBB\xBF\xEF\xBB\xBF"),
              { 0xFEFF },
              1,
              false,
              true,
              true,
              true,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\x01\x02\x7E\x7F"),
              { 0x0001,0x0002,0x007E,0x007F },
              4,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_UTF8,
              RTL_CONSTASCII_STRINGPARAM("\xEF\xBF\xBF"),
              {0xFFFF},
              1,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            // Test Java UTF-8:

            { RTL_TEXTENCODING_JAVA_UTF8,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xEF\xBB\xBF\xC0\x80\x01\x20\x41\x7F\xED\xA0\x80"
                  "\xED\xB0\x80"),
              { 0xFEFF,0x0000,0x0001,0x0020,0x0041,0x007F,0xD800,0xDC00 },
              8,
              false,
              true,
              true,
              true,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            // Bug #112949#:
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM("\x81\x63"),
              { 0x2026 },
              1,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM("\xA0\xFD\xFE\xFF"),
              { 0x00A0, 0x00A9, 0x2122, 0x2026 },
              4,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x00A0, 0x00A9, 0x2122 },
              3,
              false,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
#endif
            { RTL_TEXTENCODING_MS_932,
              RTL_CONSTASCII_STRINGPARAM("\x81\x63"),
              { 0x2026 },
              1,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_932,
              RTL_CONSTASCII_STRINGPARAM("\xA0\xFD\xFE\xFF"),
              { 0x00A0, 0x00A9, 0x2122, 0x2026 },
              4,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_932,
              RTL_CONSTASCII_STRINGPARAM(""),
              { 0x00A0, 0x00A9, 0x2122 },
              3,
              false,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_IGNORE },
            { RTL_TEXTENCODING_APPLE_JAPANESE,
              RTL_CONSTASCII_STRINGPARAM("\xA0\xFD\xFE\x81\x63"),
              { 0x00A0, 0x00A9, 0x2122, 0x2026 },
              4,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_APPLE_JAPANESE,
              RTL_CONSTASCII_STRINGPARAM("\xFF"),
              { 0x2026 },
              1,
              false,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            { RTL_TEXTENCODING_ADOBE_STANDARD,
              RTL_CONSTASCII_STRINGPARAM("\x20\x2D\xA4\xB4\xC5"),
              { 0x0020, 0x002D, 0x2215, 0x00B7, 0x00AF },
              5,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ADOBE_STANDARD,
              RTL_CONSTASCII_STRINGPARAM("\x20\x2D\xA4\xB4\xC5"),
              { 0x00A0, 0x00AD, 0x2044, 0x2219, 0x02C9 },
              5,
              false,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

            { RTL_TEXTENCODING_ADOBE_SYMBOL,
              RTL_CONSTASCII_STRINGPARAM("\x20\x44\x57\x6D\xA4"),
              { 0x0020, 0x0394, 0x03A9, 0x03BC, 0x2215 },
              5,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ADOBE_SYMBOL,
              RTL_CONSTASCII_STRINGPARAM("\x20\x44\x57\x6D\xA4"),
              { 0x00A0, 0x2206, 0x2126, 0x00B5, 0x2044 },
              5,
              false,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },

#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            // Bug #i62310#:
            { RTL_TEXTENCODING_SHIFT_JIS,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xF0\x40\xF0\x7E\xF0\x80\xF0\xFC\xF1\x40\xF9\xFC"),
              { 0xE000, 0xE03E, 0xE03F, 0xE0BB, 0xE0BC, 0xE757 },
              6,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
            // Bug #i73103#:
            { RTL_TEXTENCODING_MS_1258,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC0\x41\xDE\xE3\xD2\xD4\xEC\xFD\xF2"),
              { 0x00C0, 0x0041, 0x0303, 0x0103, 0x0309, 0x00D4, 0x0301, 0x01B0,
                0x0323 },
              9,
              true,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_MS_1258,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xC0\x41\xDE\xE3\xD2\xD4\xEC\xFD\xF2"),
              { 0x00C0, 0x00C3, 0x1EB3, 0x1ED0, 0x1EF1 },
              5,
              false,
              false,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#if WITH_LOCALE_ALL || WITH_LOCALE_FOR_SCRIPT_Deva
            { RTL_TEXTENCODING_ISCII_DEVANAGARI,
              RTL_CONSTASCII_STRINGPARAM(
                  "\xD7\xE6\x20\xD4\xCF\xE8\xD6\x20"
                  "\xC8\xD8\xD1\xE1\x20\xB3\xCA\xDC"
                  "\xCF\xC4\xDA\xD7\x20\xD8\xDB\xA2"
                  "\xC4\xDE\x20\xB1\xCF\x20\xCC\xDD"
                  "\xD7\xD1\xCC\xDA\xC6\x20\xC4\xE5"
                  "\xC6\xE5\xA2\x20\xB3\xE1\x20\xB3"
                  "\xBD\xE8\xBD\xCF\xC8\xC6\x20\xB3"
                  "\xE5\x20\xC9\xBD\xB3\xDA\xCF\x20"
                  "\xB8\xDD\xB3\xE1\x20\xC3\xE1\x20"
                  "\xEA"),
              { 0x0938, 0x094C, 0x0020, 0x0935, 0x0930, 0x094D, 0x0937, 0x0020,
                0x092A, 0x0939, 0x0932, 0x0947, 0x0020, 0x0915, 0x092C, 0x0940,
                0x0930, 0x0926, 0x093E, 0x0938, 0x0020, 0x0939, 0x093F, 0x0902,
                0x0926, 0x0942, 0x0020, 0x0914, 0x0930, 0x0020, 0x092E, 0x0941,
                0x0938, 0x0932, 0x092E, 0x093E, 0x0928, 0x0020, 0x0926, 0x094B,
                0x0928, 0x094B, 0x0902, 0x0020, 0x0915, 0x0947, 0x0020, 0x0915,
                0x091F, 0x094D, 0x091F, 0x0930, 0x092A, 0x0928, 0x0020, 0x0915,
                0x094B, 0x0020, 0x092B, 0x091F, 0x0915, 0x093E, 0x0930, 0x0020,
                0x091A, 0x0941, 0x0915, 0x0947, 0x0020, 0x0925, 0x0947, 0x0020,
                0x0964 },
              73,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_ISCII_DEVANAGARI,
              RTL_CONSTASCII_STRINGPARAM("\xE8\xE8\xE8\xE9\xA1\xE9\xEA\xE9"),
              { 0x094D, 0x200C, 0x094D, 0x200D, 0x0950, 0x93D },
              6,
              false,
              true,
              true,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR }
#endif
        };
    for (auto const& aDatum : data)
    {
        doComplexCharSetTest(aDatum);
    }
}

void Test::testComplexCut() {
#if WITH_LOCALE_ALL || WITH_LOCALE_ja || WITH_LOCALE_zh
    static ComplexCharSetTest const data[]
        = {
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("\x8E"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("\x8F"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_JP,
              RTL_CONSTASCII_STRINGPARAM("\x8F\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            { RTL_TEXTENCODING_EUC_CN,
              RTL_CONSTASCII_STRINGPARAM("\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
#endif
/* ,
            { RTL_TEXTENCODING_EUC_TW,
              RTL_CONSTASCII_STRINGPARAM("\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_TW,
              RTL_CONSTASCII_STRINGPARAM("\x8E"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_TW,
              RTL_CONSTASCII_STRINGPARAM("\x8E\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR },
            { RTL_TEXTENCODING_EUC_TW,
              RTL_CONSTASCII_STRINGPARAM("\x8E\xA1\xA1"),
              { 0 },
              0,
              true,
              true,
              false,
              false,
              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR } */ };
    for (auto const& aDatum : data)
    {
        doComplexCharSetCutTest(aDatum);
    }
#endif
}

void Test::testInvalidUtf7() {
    auto const converter = rtl_createTextToUnicodeConverter(RTL_TEXTENCODING_UTF7);
    CPPUNIT_ASSERT(converter != nullptr);
    sal_Unicode buf[TEST_STRING_SIZE];
    sal_uInt32 info;
    sal_Size converted;
    auto const size = rtl_convertTextToUnicode(
        converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\x80"), buf, TEST_STRING_SIZE,
        (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
         | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
        &info, &converted);
    CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
    CPPUNIT_ASSERT_EQUAL(u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
    CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
    CPPUNIT_ASSERT_EQUAL(sal_Size(1), converted);
    rtl_destroyTextToUnicodeConverter(converter);
}

void Test::testInvalidUtf8() {
    // UTF-8, invalid bytes:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\x80\xBF\xFE\xFF"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(4), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD\uFFFD\uFFFD\uFFFD"_ustr,
            OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(4), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, non-shortest two-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xC0\x80"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(2), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, cut two-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xC0"), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(0), size);
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOSMALL, info);
        CPPUNIT_ASSERT(converted <= 1);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, non-shortest three-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xE0\x9F\xBF"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(3), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, cut three-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xE0\x80"), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(0), size);
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOSMALL, info);
        CPPUNIT_ASSERT(converted <= 2);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, cut three-byte sequence followed by more:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xE0\x80."), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(2), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD."_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(3), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, surrogates:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr,
            RTL_CONSTASCII_STRINGPARAM("\xED\xA0\x80\xED\xB0\x80"), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(2), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(6), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, non-shortest four-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xF0\x8F\xBF\xBF"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(4), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, too-large four-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\xF4\x90\x80\x80"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(4), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, five-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr,
            RTL_CONSTASCII_STRINGPARAM("\xFB\xBF\xBF\xBF\xBF"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(5), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // UTF-8, six-byte sequence:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr,
            RTL_CONSTASCII_STRINGPARAM("\xFD\xBF\xBF\xBF\xBF\xBF"),
            buf, TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(6), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // Java UTF-8, U+0000:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_JAVA_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, RTL_CONSTASCII_STRINGPARAM("\0"), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
    // Java UTF-8, U+10000:
    {
        auto const converter = rtl_createTextToUnicodeConverter(
            RTL_TEXTENCODING_JAVA_UTF8);
        CPPUNIT_ASSERT(converter != nullptr);
        static constexpr OString input(u8"\U00010000"_ostr);
        sal_Unicode buf[TEST_STRING_SIZE];
        sal_uInt32 info;
        sal_Size converted;
        auto const size = rtl_convertTextToUnicode(
            converter, nullptr, input.getStr(), input.getLength(), buf,
            TEST_STRING_SIZE,
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT
             | RTL_TEXTTOUNICODE_FLAGS_FLUSH),
            &info, &converted);
        CPPUNIT_ASSERT_EQUAL(sal_Size(1), size);
        CPPUNIT_ASSERT_EQUAL(
            u"\uFFFD"_ustr, OUString(buf, sal_Int32(size)));
        CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_INVALID, info);
        CPPUNIT_ASSERT_EQUAL(sal_Size(4), converted);
        rtl_destroyTextToUnicodeConverter(converter);
    }
}

void Test::testInvalidUnicode() {
    auto const converter = rtl_createUnicodeToTextConverter(RTL_TEXTENCODING_UTF8);
    CPPUNIT_ASSERT(converter != nullptr);
    sal_Unicode const input[] = {0xDC00}; // lone low surrogate
    char buf[TEST_STRING_SIZE];
    sal_uInt32 info;
    sal_Size converted;
    auto const size = rtl_convertUnicodeToText(
        converter, nullptr, input, SAL_N_ELEMENTS(input), buf, TEST_STRING_SIZE,
        (RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR | RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR
         | RTL_UNICODETOTEXT_FLAGS_FLUSH),
        &info, &converted);
    CPPUNIT_ASSERT_EQUAL(sal_Size(0), size);
    CPPUNIT_ASSERT_EQUAL(RTL_UNICODETOTEXT_INFO_ERROR | RTL_UNICODETOTEXT_INFO_INVALID, info);
    CPPUNIT_ASSERT_EQUAL(sal_Size(1), converted);
    rtl_destroyTextToUnicodeConverter(converter);
}

void Test::testSRCBUFFERTOSMALL() {
    rtl_TextToUnicodeConverter cv = rtl_createTextToUnicodeConverter(
        RTL_TEXTENCODING_EUC_JP);
    CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeConverter(EUC-JP) failed",
                           cv != nullptr);
    rtl_TextToUnicodeContext cx = rtl_createTextToUnicodeContext(cv);
    CPPUNIT_ASSERT_MESSAGE("rtl_createTextToUnicodeContext failed", cx != nullptr);
    char src = '\xA1';
    sal_Unicode dst[10];
    sal_uInt32 info;
    sal_Size cvt;
    CPPUNIT_ASSERT_EQUAL(
        sal_Size(0),
        rtl_convertTextToUnicode(
            cv, cx, &src, 1, dst, SAL_N_ELEMENTS(dst),
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR |
             RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR |
             RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR),
            &info, &cvt));
    CPPUNIT_ASSERT_EQUAL(RTL_TEXTTOUNICODE_INFO_SRCBUFFERTOOSMALL, info);
    CPPUNIT_ASSERT(cvt <= 1);
    rtl_destroyTextToUnicodeContext(cv, cx);
    rtl_destroyTextToUnicodeConverter(cv);
}

void Test::testMime() {
    struct Data {
        char const * mime;
        rtl_TextEncoding encoding;
        bool reverse;
    };
    static Data const data[] = {
        { "GBK", RTL_TEXTENCODING_GBK, false },
        { "CP936", RTL_TEXTENCODING_GBK, false },
        { "MS936", RTL_TEXTENCODING_GBK, false },
        { "windows-936", RTL_TEXTENCODING_GBK, false },

        { "GB18030", RTL_TEXTENCODING_GB_18030, false },

        { "TIS-620", RTL_TEXTENCODING_TIS_620, true },
        { "ISO-8859-11", RTL_TEXTENCODING_TIS_620, false }, // not registered

        { "CP874", RTL_TEXTENCODING_MS_874, false }, // not registered
        { "MS874", RTL_TEXTENCODING_MS_874, false }, // not registered
        { "windows-874", RTL_TEXTENCODING_MS_874, true }, // not registered

        { "ISO_8859-8:1988", RTL_TEXTENCODING_ISO_8859_8, false },
        { "iso-ir-138", RTL_TEXTENCODING_ISO_8859_8, false },
        { "ISO_8859-8", RTL_TEXTENCODING_ISO_8859_8, false },
        { "ISO-8859-8", RTL_TEXTENCODING_ISO_8859_8, true },
        { "hebrew", RTL_TEXTENCODING_ISO_8859_8, false },
        { "csISOLatinHebrew", RTL_TEXTENCODING_ISO_8859_8, false },

        { "windows-1255", RTL_TEXTENCODING_MS_1255, true },

        { "IBM862", RTL_TEXTENCODING_IBM_862, true },
        { "cp862", RTL_TEXTENCODING_IBM_862, false },
        { "862", RTL_TEXTENCODING_IBM_862, false },
        { "csPC862LatinHebrew", RTL_TEXTENCODING_IBM_862, false },

        { "ISO_8859-6:1987", RTL_TEXTENCODING_ISO_8859_6, false },
        { "iso-ir-127", RTL_TEXTENCODING_ISO_8859_6, false },
        { "ISO_8859-6", RTL_TEXTENCODING_ISO_8859_6, false },
        { "ISO-8859-6", RTL_TEXTENCODING_ISO_8859_6, true },
        { "ECMA-114", RTL_TEXTENCODING_ISO_8859_6, false },
        { "ASMO-708", RTL_TEXTENCODING_ISO_8859_6, false },
        { "arabic", RTL_TEXTENCODING_ISO_8859_6, false },
        { "csISOLatinArabic", RTL_TEXTENCODING_ISO_8859_6, false },

        { "windows-1256", RTL_TEXTENCODING_MS_1256, true },

        { "IBM864", RTL_TEXTENCODING_IBM_864, true },
        { "cp864", RTL_TEXTENCODING_IBM_864, false },
        { "csIBM864", RTL_TEXTENCODING_IBM_864, false },

        { "KOI8-R", RTL_TEXTENCODING_KOI8_R, false },
        { "csKOI8R", RTL_TEXTENCODING_KOI8_R, false },
        { "koi8-r", RTL_TEXTENCODING_KOI8_R, true },

        { "KOI8-U", RTL_TEXTENCODING_KOI8_U, true },

        { "IBM860", RTL_TEXTENCODING_IBM_860, true },
        { "cp860", RTL_TEXTENCODING_IBM_860, false },
        { "860", RTL_TEXTENCODING_IBM_860, false },
        { "csIBM860", RTL_TEXTENCODING_IBM_860, false },

        { "IBM861", RTL_TEXTENCODING_IBM_861, true },
        { "cp861", RTL_TEXTENCODING_IBM_861, false },
        { "861", RTL_TEXTENCODING_IBM_861, false },
        { "cp-is", RTL_TEXTENCODING_IBM_861, false },
        { "csIBM861", RTL_TEXTENCODING_IBM_861, false },

        { "IBM863", RTL_TEXTENCODING_IBM_863, true },
        { "cp863", RTL_TEXTENCODING_IBM_863, false },
        { "863", RTL_TEXTENCODING_IBM_863, false },
        { "csIBM863", RTL_TEXTENCODING_IBM_863, false },

        { "IBM865", RTL_TEXTENCODING_IBM_865, true },
        { "cp865", RTL_TEXTENCODING_IBM_865, false },
        { "865", RTL_TEXTENCODING_IBM_865, false },
        { "csIBM865", RTL_TEXTENCODING_IBM_865, false },

        { "Latin-9", RTL_TEXTENCODING_ISO_8859_15, false },

        { "KS_C_5601-1987", RTL_TEXTENCODING_MS_949, false },
        { "iso-ir-149", RTL_TEXTENCODING_MS_949, false },
        { "KS_C_5601-1989", RTL_TEXTENCODING_MS_949, false },
        { "KSC_5601", RTL_TEXTENCODING_MS_949, false },
        { "korean", RTL_TEXTENCODING_MS_949, false },
        { "csKSC56011987", RTL_TEXTENCODING_MS_949, false },
        { nullptr, RTL_TEXTENCODING_MS_949, true },

        { "Adobe-Standard-Encoding", RTL_TEXTENCODING_ADOBE_STANDARD, false },
        { "csAdobeStandardEncoding", RTL_TEXTENCODING_ADOBE_STANDARD, false },
        { "Adobe-Symbol-Encoding", RTL_TEXTENCODING_ADOBE_SYMBOL, false },
        { "csHPPSMath", RTL_TEXTENCODING_ADOBE_SYMBOL, false },

        { "PTCP154", RTL_TEXTENCODING_PT154, true },
        { "csPTCP154", RTL_TEXTENCODING_PT154, false },
        { "PT154", RTL_TEXTENCODING_PT154, false },
        { "CP154", RTL_TEXTENCODING_PT154, false },
        { "Cyrillic-Asian", RTL_TEXTENCODING_PT154, false }
    };
    for (auto const[pMime,nEncoding,bReverse] : data)
    {
        if (pMime == nullptr)
        {
            OSL_ASSERT(bReverse);
            CPPUNIT_ASSERT_EQUAL(static_cast< char const * >(nullptr),
                                 rtl_getMimeCharsetFromTextEncoding(nEncoding));
        }
        else
        {
            CPPUNIT_ASSERT_EQUAL(nEncoding, rtl_getTextEncodingFromMimeCharset(pMime));
            if (bReverse)
            {
                CPPUNIT_ASSERT_EQUAL(OString(pMime),
                                     OString( rtl_getMimeCharsetFromTextEncoding(nEncoding)));
            }
        }
    }
}

void Test::testWindows() {
    struct Data {
        sal_uInt32 codePage;
        rtl_TextEncoding encoding;
        bool reverse;
    };
    static Data const data[] = {
        { 42, RTL_TEXTENCODING_SYMBOL, true },
        { 437, RTL_TEXTENCODING_IBM_437, true },
        { 708, RTL_TEXTENCODING_ISO_8859_6, false },
        { 737, RTL_TEXTENCODING_IBM_737, true },
        { 775, RTL_TEXTENCODING_IBM_775, true },
        { 850, RTL_TEXTENCODING_IBM_850, true },
        { 852, RTL_TEXTENCODING_IBM_852, true },
        { 855, RTL_TEXTENCODING_IBM_855, true },
        { 857, RTL_TEXTENCODING_IBM_857, true },
        { 860, RTL_TEXTENCODING_IBM_860, true },
        { 861, RTL_TEXTENCODING_IBM_861, true },
        { 862, RTL_TEXTENCODING_IBM_862, true },
        { 863, RTL_TEXTENCODING_IBM_863, true },
        { 864, RTL_TEXTENCODING_IBM_864, true },
        { 865, RTL_TEXTENCODING_IBM_865, true },
        { 866, RTL_TEXTENCODING_IBM_866, true },
        { 869, RTL_TEXTENCODING_IBM_869, true },
        { 874, RTL_TEXTENCODING_MS_874, true },
        { 932, RTL_TEXTENCODING_MS_932, true },
        { 936, RTL_TEXTENCODING_MS_936, true },
        { 949, RTL_TEXTENCODING_MS_949, true },
        { 950, RTL_TEXTENCODING_MS_950, true },
        { 1250, RTL_TEXTENCODING_MS_1250, true },
        { 1251, RTL_TEXTENCODING_MS_1251, true },
        { 1252, RTL_TEXTENCODING_MS_1252, true },
        { 1253, RTL_TEXTENCODING_MS_1253, true },
        { 1254, RTL_TEXTENCODING_MS_1254, true },
        { 1255, RTL_TEXTENCODING_MS_1255, true },
        { 1256, RTL_TEXTENCODING_MS_1256, true },
        { 1257, RTL_TEXTENCODING_MS_1257, true },
        { 1258, RTL_TEXTENCODING_MS_1258, true },
        { 1361, RTL_TEXTENCODING_MS_1361, true },
        { 10000, RTL_TEXTENCODING_APPLE_ROMAN, true },
        { 10001, RTL_TEXTENCODING_APPLE_JAPANESE, true },
        { 10002, RTL_TEXTENCODING_APPLE_CHINTRAD, true },
        { 10003, RTL_TEXTENCODING_APPLE_KOREAN, true },
        { 10004, RTL_TEXTENCODING_APPLE_ARABIC, true },
        { 10005, RTL_TEXTENCODING_APPLE_HEBREW, true },
        { 10006, RTL_TEXTENCODING_APPLE_GREEK, true },
        { 10007, RTL_TEXTENCODING_APPLE_CYRILLIC, true },
        { 10008, RTL_TEXTENCODING_APPLE_CHINSIMP, true },
        { 10010, RTL_TEXTENCODING_APPLE_ROMANIAN, true },
        { 10017, RTL_TEXTENCODING_APPLE_UKRAINIAN, true },
        { 10029, RTL_TEXTENCODING_APPLE_CENTEURO, true },
        { 10079, RTL_TEXTENCODING_APPLE_ICELAND, true },
        { 10081, RTL_TEXTENCODING_APPLE_TURKISH, true },
        { 10082, RTL_TEXTENCODING_APPLE_CROATIAN, true },
        { 20127, RTL_TEXTENCODING_ASCII_US, true },
        { 20866, RTL_TEXTENCODING_KOI8_R, true },
        { 21866, RTL_TEXTENCODING_KOI8_U, true },
        { 28591, RTL_TEXTENCODING_ISO_8859_1, true },
        { 28592, RTL_TEXTENCODING_ISO_8859_2, true },
        { 28593, RTL_TEXTENCODING_ISO_8859_3, true },
        { 28594, RTL_TEXTENCODING_ISO_8859_4, true },
        { 28595, RTL_TEXTENCODING_ISO_8859_5, true },
        { 28596, RTL_TEXTENCODING_ISO_8859_6, true },
        { 28597, RTL_TEXTENCODING_ISO_8859_7, true },
        { 28598, RTL_TEXTENCODING_ISO_8859_8, true },
        { 28599, RTL_TEXTENCODING_ISO_8859_9, true },
        { 28605, RTL_TEXTENCODING_ISO_8859_15, true },
        { 50220, RTL_TEXTENCODING_ISO_2022_JP, true },
        { 50225, RTL_TEXTENCODING_ISO_2022_KR, true },
        { 51932, RTL_TEXTENCODING_EUC_JP, true },
        { 51936, RTL_TEXTENCODING_EUC_CN, true },
        { 51949, RTL_TEXTENCODING_EUC_KR, true },
        { 65000, RTL_TEXTENCODING_UTF7, true },
        { 65001, RTL_TEXTENCODING_UTF8, true },
        { 1200, RTL_TEXTENCODING_DONTKNOW, false }, // UTF_16LE
        { 1201, RTL_TEXTENCODING_DONTKNOW, false }, // UTF_16LE
        { 0, RTL_TEXTENCODING_DONTKNOW, true },
        { 0, RTL_TEXTENCODING_UCS4, true },
        { 0, RTL_TEXTENCODING_UCS2, true },
        { 57002, RTL_TEXTENCODING_ISCII_DEVANAGARI, true }
    };
    for (auto const[nCodePage,nEncoding,bReverse] : data)
    {
        OSL_ASSERT(nCodePage != 0 || bReverse);
        if (nCodePage != 0)
        {
            CPPUNIT_ASSERT_EQUAL( nEncoding, rtl_getTextEncodingFromWindowsCodePage(nCodePage));
        }
        if (bReverse)
        {
            CPPUNIT_ASSERT_EQUAL( nCodePage, rtl_getWindowsCodePageFromTextEncoding(nEncoding));
        }
    }
}

void Test::testInfo() {
    struct Data {
        rtl_TextEncoding encoding;
        sal_uInt32 flag;
        bool value;
    };
    static Data const data[] = {
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
        { RTL_TEXTENCODING_APPLE_JAPANESE, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_EUC_JP, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_ISO_2022_JP, RTL_TEXTENCODING_INFO_CONTEXT, true },
        { RTL_TEXTENCODING_ISO_2022_JP, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_SHIFT_JIS, RTL_TEXTENCODING_INFO_ASCII, false },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ko
        { RTL_TEXTENCODING_APPLE_KOREAN, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_EUC_KR, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_ISO_2022_KR, RTL_TEXTENCODING_INFO_CONTEXT, true },
        { RTL_TEXTENCODING_ISO_2022_KR, RTL_TEXTENCODING_INFO_ASCII, false },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
        { RTL_TEXTENCODING_APPLE_CHINTRAD, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_BIG5, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_BIG5_HKSCS, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_EUC_CN, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_EUC_TW, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_GBK, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_GB_18030, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_GB_18030, RTL_TEXTENCODING_INFO_UNICODE, true },
        { RTL_TEXTENCODING_ISO_2022_CN, RTL_TEXTENCODING_INFO_CONTEXT, true },
        { RTL_TEXTENCODING_ISO_2022_CN, RTL_TEXTENCODING_INFO_ASCII, false },
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_FOR_SCRIPT_Deva
        { RTL_TEXTENCODING_ISCII_DEVANAGARI, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_ISCII_DEVANAGARI, RTL_TEXTENCODING_INFO_MIME, false },
#endif
        { RTL_TEXTENCODING_MS_1361, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_MS_874, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_MS_932, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_MS_936, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_MS_949, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_MS_950, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_KOI8_R, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_KOI8_R, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_KOI8_U, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_KOI8_U, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_IBM_860, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_IBM_861, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_IBM_863, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_IBM_865, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_ADOBE_STANDARD, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_ADOBE_STANDARD, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_ADOBE_STANDARD, RTL_TEXTENCODING_INFO_SYMBOL, false },
        { RTL_TEXTENCODING_ADOBE_SYMBOL, RTL_TEXTENCODING_INFO_ASCII, false },
        { RTL_TEXTENCODING_ADOBE_SYMBOL, RTL_TEXTENCODING_INFO_MIME, true },
        { RTL_TEXTENCODING_ADOBE_SYMBOL, RTL_TEXTENCODING_INFO_SYMBOL, true },
        { RTL_TEXTENCODING_PT154, RTL_TEXTENCODING_INFO_ASCII, true },
        { RTL_TEXTENCODING_PT154, RTL_TEXTENCODING_INFO_MIME, true }
    };
    for (auto const[nEncoding, nFlag, bValue] : data)
    {
        rtl_TextEncodingInfo info;
        info.StructSize = sizeof info;
        CPPUNIT_ASSERT(rtl_getTextEncodingInfo(nEncoding, &info));
        CPPUNIT_ASSERT_EQUAL(bValue, ((info.Flags & nFlag) != 0));
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

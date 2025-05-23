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

#include <rtl/uri.hxx>
#include <osl/thread.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace Stringtest
{

    class Convert : public CppUnit::TestFixture
    {
    public:
        OString convertToOString(OUString const& _suStr)
            {
                return OUStringToOString(_suStr, osl_getThreadTextEncoding()/*RTL_TEXTENCODING_ASCII_US*/);
            }

        void showContent(OUString const& _suStr)
            {
                OString sStr = convertToOString(_suStr);
                printf("%s\n", sStr.getStr());
            }

        void test_FromUTF8_001()
            {
                // string --> ustring
                OUString suStrUTF8 = OStringToOUString("h%C3%A4llo", RTL_TEXTENCODING_ASCII_US);

                // UTF8 --> real ustring
                OUString suStr_UriDecodeToIuri      = rtl::Uri::decode(suStrUTF8, rtl_UriDecodeToIuri, RTL_TEXTENCODING_UTF8);
                showContent(suStr_UriDecodeToIuri);

                // string --> ustring
                OString sStr("h\xE4llo", strlen("h\xE4llo"));
                OUString suString = OStringToOUString(sStr, RTL_TEXTENCODING_ISO_8859_15);

                CPPUNIT_ASSERT_EQUAL_MESSAGE("Strings must be equal", suString, suStr_UriDecodeToIuri);

                // ustring --> ustring (UTF8)
                OUString suStr2 = rtl::Uri::encode(suStr_UriDecodeToIuri, rtl_UriCharClassUnoParamValue, rtl_UriEncodeKeepEscapes, RTL_TEXTENCODING_UTF8);
                showContent(suStr2);

                CPPUNIT_ASSERT_EQUAL_MESSAGE("Strings must be equal", suStr2, suStrUTF8);
                // suStr should be equal to suStr2
            }

        CPPUNIT_TEST_SUITE( Convert );
        CPPUNIT_TEST( test_FromUTF8_001 );
        CPPUNIT_TEST_SUITE_END( );
    };

}

CPPUNIT_TEST_SUITE_REGISTRATION( Stringtest::Convert );

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

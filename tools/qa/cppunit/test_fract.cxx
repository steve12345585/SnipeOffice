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

#include <rtl/math.hxx>
#include <tools/fract.hxx>

namespace tools
{

class FractionTest : public CppUnit::TestFixture
{
public:

    void testFraction()
    {
        const Fraction aFract(1082130431,1073741824);
        CPPUNIT_ASSERT_MESSAGE( "Fraction #1 not approximately equal to 1.007812499068677",
                                rtl::math::approxEqual(static_cast<double>(aFract),1.007812499068677) );

        Fraction aFract2( aFract );
        aFract2.ReduceInaccurate(8);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #2 not 1",
                                sal_Int32(1), aFract2.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #2 not 1",
                                sal_Int32(1), aFract2.GetDenominator() );

        Fraction aFract3( 0x7AAAAAAA, 0x35555555 );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 cancellation wrong",
                                sal_Int32(0x7AAAAAAA), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 cancellation wrong",
                                sal_Int32(0x35555555), aFract3.GetDenominator() );
        aFract3.ReduceInaccurate(30);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 ReduceInaccurate erroneously cut precision",
                                sal_Int32(0x7AAAAAAA), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 ReduceInaccurate erroneously cut precision",
                                sal_Int32(0x35555555), aFract3.GetDenominator() );

        aFract3.ReduceInaccurate(29);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 29 bits failed",
                                sal_Int32(0x3D555555), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 29 bits failed",
                                sal_Int32(0x1AAAAAAA), aFract3.GetDenominator() );

        aFract3.ReduceInaccurate(9);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 9 bits failed",
                                sal_Int32(0x0147), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 9 bits failed",
                                sal_Int32(0x008E), aFract3.GetDenominator() );

        aFract3.ReduceInaccurate(1);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 1 bit failed",
                                sal_Int32(2), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 1 bit failed",
                                sal_Int32(1), aFract3.GetDenominator() );

        aFract3.ReduceInaccurate(0);
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 0 bits failed",
                                sal_Int32(2), aFract3.GetNumerator() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Fraction #3 reduce to 0 bits failed",
                                sal_Int32(1), aFract3.GetDenominator() );

    }

    void testMinLongDouble() {
        Fraction f(double(SAL_MIN_INT32));
        CPPUNIT_ASSERT_EQUAL(SAL_MIN_INT32, f.GetNumerator());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(1), f.GetDenominator());
    }

    void testCreateFromDoubleIn32BitsPlatform() {
        // This pass in 64 bits but fail in 32 bits
        Fraction f(0.960945);
        CPPUNIT_ASSERT_EQUAL(true, f.IsValid());
    }

    CPPUNIT_TEST_SUITE(FractionTest);
    CPPUNIT_TEST(testFraction);
    CPPUNIT_TEST(testMinLongDouble);
    CPPUNIT_TEST(testCreateFromDoubleIn32BitsPlatform);
    CPPUNIT_TEST_SUITE_END();
};


CPPUNIT_TEST_SUITE_REGISTRATION(FractionTest);
} // namespace tools

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

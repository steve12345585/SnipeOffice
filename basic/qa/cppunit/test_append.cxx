/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "basictest.hxx"

namespace
{
    class EnableTest : public test::BootstrapFixture
    {
        public:
        EnableTest() : BootstrapFixture(true, false) {};
        void testDimEnable();
        void testWin64();
        void testEnableRuntime();
        // Adds code needed to register the test suite
        CPPUNIT_TEST_SUITE(EnableTest);

        // Declares the method as a test to call
        CPPUNIT_TEST(testDimEnable);
        CPPUNIT_TEST(testWin64);
        CPPUNIT_TEST(testEnableRuntime);

        // End of test suite definition
        CPPUNIT_TEST_SUITE_END();
    };

OUString sTestEnableRuntime(
    u"Function doUnitTest as Integer\n"
    "Dim Enable as Integer\n"
    "Enable = 1\n"
    "Enable = Enable + 2\n"
    "doUnitTest = Enable\n"
    "End Function\n"_ustr
);

OUString sTestDimEnable(
    u"Sub doUnitTest\n"
    "Dim Enable as String\n"
    "End Sub\n"_ustr
);

void EnableTest::testEnableRuntime()
{
    MacroSnippet myMacro(sTestEnableRuntime);
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testEnableRuntime fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(3), pNew->GetInteger());
}

void EnableTest::testDimEnable()
{
    MacroSnippet myMacro(sTestDimEnable);
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("Dim causes compile error", !myMacro.HasError() );
}

void EnableTest::testWin64()
{
    MacroSnippet myMacro(u"   #If Win64\n"
        "Declare PtrSafe Function aht_apiGetOpenFileName Lib \"comdlg32.dll\""
        "\n"
        "#End if\n"_ustr);
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("#if Win64 Declare PtrSafe causes compile error", !myMacro.HasError() );
}

  // Put the test suite in the registry
  CPPUNIT_TEST_SUITE_REGISTRATION(EnableTest);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

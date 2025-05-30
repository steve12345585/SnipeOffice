/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "basictest.hxx"

#include <com/sun/star/awt/WindowDescriptor.hpp>
#include <com/sun/star/table/TableBorder.hpp>
#include <basic/sbuno.hxx>

namespace
{
    using namespace com::sun::star;
    class Nested_Struct : public test::BootstrapFixture
    {
        public:
        Nested_Struct(): BootstrapFixture(true, false) {};
        void testAssign1();
        void testAssign1Alt(); // result is uno-ised and tested
        void testOldAssign();
        void testOldAssignAlt(); // result is uno-ised and tested
        void testUnfixedVarAssign();
        void testUnfixedVarAssignAlt(); // result is uno-ised and tested
        void testFixedVarAssign();
        void testFixedVarAssignAlt(); // result is uno-ised and tested
        void testUnoAccess(); // fdo#60117 specific test
        void testTdf134576();

        // Adds code needed to register the test suite
        CPPUNIT_TEST_SUITE(Nested_Struct);

        // Declares the method as a test to call
        CPPUNIT_TEST(testAssign1);
        CPPUNIT_TEST(testAssign1Alt);
        CPPUNIT_TEST(testOldAssign);
        CPPUNIT_TEST(testOldAssignAlt);
        CPPUNIT_TEST(testUnfixedVarAssign);
        CPPUNIT_TEST(testUnfixedVarAssignAlt);
        CPPUNIT_TEST(testFixedVarAssign);
        CPPUNIT_TEST(testFixedVarAssignAlt);
        CPPUNIT_TEST(testUnoAccess);
        CPPUNIT_TEST(testTdf134576);

        // End of test suite definition
        CPPUNIT_TEST_SUITE_END();
    };

// tests the new behaviour, we should be able to
// directly modify the value of the nested 'HorizontalLine' struct
OUString sTestSource1(
    u"Function doUnitTest() as Integer\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\"\n"
    "b0.HorizontalLine.OuterLineWidth = 9\n"
    "doUnitTest = b0.HorizontalLine.OuterLineWidth\n"
    "End Function\n"_ustr
);

OUString sTestSource1Alt(
    u"Function doUnitTest() as Object\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\"\n"
    "b0.HorizontalLine.OuterLineWidth = 9\n"
    "doUnitTest = b0\n"
    "End Function\n"_ustr
);

// tests the old behaviour, we should still be able
// to use the old workaround of
// a) creating a new instance BorderLine,
// b) cloning the new instance with the value of b0.HorizontalLine
// c) modifying the new instance
// d) setting b0.HorizontalLine with the value of the new instance
OUString sTestSource2(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\", l as new \"com.sun.star.table.BorderLine\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "doUnitTest = b0.HorizontalLine.OuterLineWidth\n"
"End Function\n"_ustr
);

OUString sTestSource2Alt(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\", l as new \"com.sun.star.table.BorderLine\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "doUnitTest = b0\n"
"End Function\n"_ustr
);
// it should be legal to assign a variant to a struct ( and copy by val )
// make sure we aren't copying by reference, we make sure that l is not
// a reference copy of b0.HorizontalLine, each one should have an
// OuterLineWidth of 4 & 9 respectively and we should be returning
// 13 the sum of the two ( hopefully unique values if we haven't copied by reference )
OUString sTestSource3(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "l.OuterLineWidth = 4\n"
    "doUnitTest = b0.HorizontalLine.OuterLineWidth + l.OuterLineWidth\n"
"End Function\n"_ustr
);

OUString sTestSource3Alt(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "l.OuterLineWidth = 4\n"
    "Dim result(1)\n"
    "result(0) = b0\n"
    "result(1) = l\n"
    "doUnitTest = result\n"
"End Function\n"_ustr
);

// nearly the same as above but this time for a fixed type
// variable
OUString sTestSource4(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\", l as new \"com.sun.star.table.BorderLine\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "l.OuterLineWidth = 4\n"
    "doUnitTest = b0.HorizontalLine.OuterLineWidth + l.OuterLineWidth\n"
"End Function\n"_ustr
);

OUString sTestSource4Alt(
    u"Function doUnitTest()\n"
    "Dim b0 as new \"com.sun.star.table.TableBorder\", l as new \"com.sun.star.table.BorderLine\"\n"
    "l = b0.HorizontalLine\n"
    "l.OuterLineWidth = 9\n"
    "b0.HorizontalLine = l\n"
    "l.OuterLineWidth = 4\n"
    "Dim result(1)\n"
    "result(0) = b0\n"
    "result(1) = l\n"
    "doUnitTest = result\n"
"End Function\n"_ustr
);

// Although basic might appear to correctly change nested struct elements
// fdo#60117 shows that basic can be fooled ( and even the watch(ed) variable
// in the debugger shows the expected values )
// We need to additionally check the actual uno struct to see if the
// changes made are *really* reflected in the object
OUString sTestSource5(
    u"Function doUnitTest() as Object\n"
    "Dim aWinDesc as new \"com.sun.star.awt.WindowDescriptor\"\n"
    "Dim aRect as new \"com.sun.star.awt.Rectangle\"\n"
    "aRect.X = 200\n"
    "aWinDesc.Bounds = aRect\n"
    "doUnitTest = aWinDesc\n"
"End Function\n"_ustr
);


void Nested_Struct::testAssign1()
{
    MacroSnippet myMacro( sTestSource1 );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testAssign1 fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(9), pNew->GetInteger());
}

void Nested_Struct::testAssign1Alt()
{
    MacroSnippet myMacro( sTestSource1Alt );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testAssign1Alt fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    uno::Any aRet = sbxToUnoValue( pNew.get() );
    table::TableBorder aBorder;
    aRet >>= aBorder;

    int result = aBorder.HorizontalLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL( 9, result );
}

void Nested_Struct::testOldAssign()
{
    MacroSnippet myMacro( sTestSource2 );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testOldAssign fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(9), pNew->GetInteger());
}

void Nested_Struct::testOldAssignAlt()
{
    MacroSnippet myMacro( sTestSource2Alt );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testOldAssign fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    uno::Any aRet = sbxToUnoValue( pNew.get() );
    table::TableBorder aBorder;
    aRet >>= aBorder;

    int result = aBorder.HorizontalLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL( 9, result );
}

void Nested_Struct::testUnfixedVarAssign()
{
    MacroSnippet myMacro( sTestSource3 );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testUnfixedVarAssign fails with compile error",!myMacro.HasError() );
    // forces a broadcast
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(13), pNew->GetInteger());
}

void Nested_Struct::testUnfixedVarAssignAlt()
{
    MacroSnippet myMacro( sTestSource3Alt );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testUnfixedVarAssignAlt fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    uno::Any aRet = sbxToUnoValue( pNew.get() );

    uno::Sequence< uno::Any > aResult;
    bool bRes = aRet >>= aResult;
    CPPUNIT_ASSERT_EQUAL(true, bRes );

    int result = aResult.getLength();
    // should have 2 elements in a sequence returned
    CPPUNIT_ASSERT_EQUAL(2, result );

    table::TableBorder aBorder;
    aResult[0] >>= aBorder;

    table::BorderLine aBorderLine;
    aResult[1] >>= aBorderLine;
    result = aBorder.HorizontalLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL(9, result );
    result = aBorderLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL(4, result );
}

void Nested_Struct::testFixedVarAssign()
{
    MacroSnippet myMacro( sTestSource4 );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testFixedVarAssign fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(13), pNew->GetInteger());
}

void Nested_Struct::testFixedVarAssignAlt()
{
    MacroSnippet myMacro( sTestSource4Alt );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testFixedVarAssignAlt fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    uno::Any aRet = sbxToUnoValue( pNew.get() );

    uno::Sequence< uno::Any > aResult;
    bool bRes = aRet >>= aResult;
    CPPUNIT_ASSERT_EQUAL(true, bRes );

    int result = aResult.getLength();
    // should have 2 elements in a sequence returned
    CPPUNIT_ASSERT_EQUAL(2, result );

    table::TableBorder aBorder;
    aResult[0] >>= aBorder;

    table::BorderLine aBorderLine;
    aResult[1] >>= aBorderLine;
    result = aBorder.HorizontalLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL(9, result );
    result = aBorderLine.OuterLineWidth;
    CPPUNIT_ASSERT_EQUAL(4, result );
}

void Nested_Struct::testUnoAccess()
{
    MacroSnippet myMacro( sTestSource5 );
    myMacro.Compile();
    CPPUNIT_ASSERT_MESSAGE("testUnoAccess fails with compile error",!myMacro.HasError() );
    SbxVariableRef pNew = myMacro.Run();
    uno::Any aRet = sbxToUnoValue( pNew.get() );
    awt::WindowDescriptor aWinDesc;
    aRet >>= aWinDesc;

    int result = aWinDesc.Bounds.X;
    CPPUNIT_ASSERT_EQUAL(200, result );
}

void Nested_Struct::testTdf134576()
{
    MacroSnippet myMacro(u"Function doUnitTest()\n"
                        "  On Error Resume Next\n"
                        "  For Each a In b\n"
                        "    c.d\n"
                        "  Next\n"
                        "  doUnitTest = 1\n"
                        "End Function\n"_ustr);

    myMacro.Compile();
    CPPUNIT_ASSERT(!myMacro.HasError());

    // Without the fix in place, it would have crashed here
    SbxVariableRef pNew = myMacro.Run();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(1), pNew->GetInteger());
}

  // Put the test suite in the registry
  CPPUNIT_TEST_SUITE_REGISTRATION(Nested_Struct);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

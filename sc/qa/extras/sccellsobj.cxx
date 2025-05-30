/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/container/xelementaccess.hxx>
#include <test/container/xenumerationaccess.hxx>

#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/sheet/XCellRangesQuery.hpp>
#include <com/sun/star/sheet/XSheetCellRanges.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <cppu/unotype.hxx>

using namespace css;
using namespace css::uno;

namespace sc_apitest
{
class ScCellsObj : public UnoApiTest,
                   public apitest::XElementAccess,
                   public apitest::XEnumerationAccess
{
public:
    ScCellsObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScCellsObj);

    // XElementAccess
    CPPUNIT_TEST(testHasElements);
    CPPUNIT_TEST(testGetElementType);

    // XEnumerationAccess
    CPPUNIT_TEST(testCreateEnumeration);

    CPPUNIT_TEST_SUITE_END();
};

ScCellsObj::ScCellsObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XElementAccess(cppu::UnoType<table::XCell>::get())
{
}

uno::Reference<uno::XInterface> ScCellsObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("no calc document", xDoc.is());

    uno::Reference<sheet::XSpreadsheets> xSheets(xDoc->getSheets(), uno::UNO_SET_THROW);
    uno::Reference<container::XIndexAccess> xIA(xSheets, uno::UNO_QUERY_THROW);
    uno::Reference<table::XCellRange> xCellRange(xIA->getByIndex(0), uno::UNO_QUERY_THROW);

    uno::Reference<table::XCell> xCell0(xCellRange->getCellByPosition(0, 0), uno::UNO_SET_THROW);
    uno::Reference<text::XTextRange> xTextRange0(xCell0, uno::UNO_QUERY_THROW);
    xTextRange0->setString(u"ScCellsObj test 1"_ustr);

    uno::Reference<table::XCell> xCell1(xCellRange->getCellByPosition(5, 1), uno::UNO_SET_THROW);
    xCell1->setValue(15);

    uno::Reference<table::XCell> xCell2(xCellRange->getCellByPosition(3, 9), uno::UNO_SET_THROW);
    uno::Reference<text::XTextRange> xTextRange2(xCell2, uno::UNO_QUERY_THROW);
    xTextRange2->setString(u"ScCellsObj test 2"_ustr);

    uno::Reference<sheet::XCellRangesQuery> xCRQ(xCellRange, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSheetCellRanges> xSCR(xCRQ->queryVisibleCells(), uno::UNO_SET_THROW);

    return xSCR->getCells();
}

void ScCellsObj::setUp()
{
    UnoApiTest::setUp();
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScCellsObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

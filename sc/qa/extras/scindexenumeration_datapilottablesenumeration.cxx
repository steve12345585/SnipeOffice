/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/container/xenumeration.hxx>

#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/sheet/XDataPilotDescriptor.hpp>
#include <com/sun/star/sheet/XDataPilotTables.hpp>
#include <com/sun/star/sheet/XDataPilotTablesSupplier.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

using namespace css;
using namespace css::uno;

namespace sc_apitest
{
class ScIndexEnumeration_DataPilotTablesEnumeration : public UnoApiTest,
                                                      public apitest::XEnumeration
{
public:
    ScIndexEnumeration_DataPilotTablesEnumeration();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScIndexEnumeration_DataPilotTablesEnumeration);

    // XEnumeration
    CPPUNIT_TEST(testHasMoreElements);
    CPPUNIT_TEST(testNextElement);

    CPPUNIT_TEST_SUITE_END();
};

ScIndexEnumeration_DataPilotTablesEnumeration::ScIndexEnumeration_DataPilotTablesEnumeration()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
{
}

uno::Reference<uno::XInterface> ScIndexEnumeration_DataPilotTablesEnumeration::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("no calc document", xDoc.is());

    uno::Reference<sheet::XSpreadsheets> xSheets(xDoc->getSheets(), uno::UNO_SET_THROW);
    uno::Reference<container::XIndexAccess> xIA(xSheets, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheet> xSheet0(xIA->getByIndex(0), uno::UNO_QUERY_THROW);

    for (auto i = 1; i < 4; ++i)
    {
        xSheet0->getCellByPosition(i, 0)->setFormula("Col" + OUString::number(i));
        xSheet0->getCellByPosition(0, i)->setFormula("Row" + OUString::number(i));
    }

    for (auto x = 1; x < 4; ++x)
        for (auto y = 1; y < 4; ++y)
            xSheet0->getCellByPosition(x, y)->setValue(x * (y + 1));

    uno::Reference<sheet::XDataPilotTablesSupplier> xDPTS(xSheet0, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XDataPilotTables> xDPT(xDPTS->getDataPilotTables(), uno::UNO_SET_THROW);
    uno::Reference<sheet::XDataPilotDescriptor> xDPD(xDPT->createDataPilotDescriptor(),
                                                     uno::UNO_SET_THROW);
    xDPD->setSourceRange(table::CellRangeAddress(0, 0, 0, 4, 4));
    xDPT->insertNewByName(u"DataPilotTable"_ustr, table::CellAddress(0, 5, 5), xDPD);

    uno::Reference<container::XEnumerationAccess> xEA(xDPT, uno::UNO_QUERY_THROW);

    return xEA->createEnumeration();
}

void ScIndexEnumeration_DataPilotTablesEnumeration::setUp()
{
    UnoApiTest::setUp();
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScIndexEnumeration_DataPilotTablesEnumeration);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

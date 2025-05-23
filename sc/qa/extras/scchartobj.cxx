/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/beans/xpropertyset.hxx>
#include <test/container/xnamed.hxx>
#include <test/document/xembeddedobjectsupplier.hxx>
#include <test/table/xtablechart.hxx>
#include <test/lang/xserviceinfo.hxx>

#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/sheet/XCellRangeAddressable.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/table/XTableChart.hpp>
#include <com/sun/star/table/XTableCharts.hpp>
#include <com/sun/star/table/XTableChartsSupplier.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>

using namespace css;

namespace sc_apitest
{
class ScChartObj : public UnoApiTest,
                   public apitest::XEmbeddedObjectSupplier,
                   public apitest::XNamed,
                   public apitest::XPropertySet,
                   public apitest::XServiceInfo,
                   public apitest::XTableChart
{
public:
    ScChartObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScChartObj);

    // XEmbeddedObjectSupplier
    CPPUNIT_TEST(testGetEmbeddedObject);

    // XNamed
    CPPUNIT_TEST(testGetName);
    CPPUNIT_TEST(testSetNameThrowsException);

    // XPropertySet
    CPPUNIT_TEST(testGetPropertySetInfo);

    // XServiceInfo
    CPPUNIT_TEST(testGetImplementationName);
    CPPUNIT_TEST(testGetSupportedServiceNames);
    CPPUNIT_TEST(testSupportsService);

    // XTableChart
    CPPUNIT_TEST(testGetSetHasColumnHeaders);
    CPPUNIT_TEST(testGetSetHasRowHeaders);
    CPPUNIT_TEST(testGetSetRanges);

    CPPUNIT_TEST_SUITE_END();
};

ScChartObj::ScChartObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XNamed(u"ScChartObj"_ustr)
    , XServiceInfo(u"ScChartObj"_ustr, u"com.sun.star.table.TableChart"_ustr)
{
}

uno::Reference<uno::XInterface> ScChartObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheets> xSheets(xDoc->getSheets(), uno::UNO_SET_THROW);
    uno::Reference<container::XIndexAccess> xIA(xSheets, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheet> xSheet0(xIA->getByIndex(0), uno::UNO_QUERY_THROW);

    xSheet0->getCellByPosition(1, 0)->setFormula(u"JAN"_ustr);
    xSheet0->getCellByPosition(2, 0)->setFormula(u"FEB"_ustr);
    xSheet0->getCellByPosition(3, 0)->setFormula(u"MAR"_ustr);
    xSheet0->getCellByPosition(4, 0)->setFormula(u"APR"_ustr);
    xSheet0->getCellByPosition(5, 0)->setFormula(u"MAY"_ustr);
    xSheet0->getCellByPosition(6, 0)->setFormula(u"JUN"_ustr);
    xSheet0->getCellByPosition(7, 0)->setFormula(u"JUL"_ustr);
    xSheet0->getCellByPosition(8, 0)->setFormula(u"AUG"_ustr);
    xSheet0->getCellByPosition(9, 0)->setFormula(u"SEP"_ustr);
    xSheet0->getCellByPosition(10, 0)->setFormula(u"OCT"_ustr);
    xSheet0->getCellByPosition(11, 0)->setFormula(u"NOV"_ustr);
    xSheet0->getCellByPosition(12, 0)->setFormula(u"DEC"_ustr);
    xSheet0->getCellByPosition(13, 0)->setFormula(u"SUM"_ustr);

    xSheet0->getCellByPosition(0, 1)->setFormula(u"Smith"_ustr);
    xSheet0->getCellByPosition(1, 1)->setValue(42);
    xSheet0->getCellByPosition(2, 1)->setValue(58.9);
    xSheet0->getCellByPosition(3, 1)->setValue(-66.5);
    xSheet0->getCellByPosition(4, 1)->setValue(43.4);
    xSheet0->getCellByPosition(5, 1)->setValue(44.5);
    xSheet0->getCellByPosition(6, 1)->setValue(45.3);
    xSheet0->getCellByPosition(7, 1)->setValue(-67.3);
    xSheet0->getCellByPosition(8, 1)->setValue(30.5);
    xSheet0->getCellByPosition(9, 1)->setValue(23.2);
    xSheet0->getCellByPosition(10, 1)->setValue(-97.3);
    xSheet0->getCellByPosition(11, 1)->setValue(22.4);
    xSheet0->getCellByPosition(11, 1)->setValue(23.5);
    xSheet0->getCellByPosition(13, 1)->setFormula(u"SUM(B2:M2"_ustr);

    xSheet0->getCellByPosition(0, 2)->setFormula(u"Jones"_ustr);
    xSheet0->getCellByPosition(1, 2)->setValue(21);
    xSheet0->getCellByPosition(2, 2)->setValue(40.9);
    xSheet0->getCellByPosition(3, 2)->setValue(-57.5);
    xSheet0->getCellByPosition(4, 2)->setValue(-23.4);
    xSheet0->getCellByPosition(5, 2)->setValue(34.5);
    xSheet0->getCellByPosition(6, 2)->setValue(59.3);
    xSheet0->getCellByPosition(7, 2)->setValue(27.3);
    xSheet0->getCellByPosition(8, 2)->setValue(-38.5);
    xSheet0->getCellByPosition(9, 2)->setValue(43.2);
    xSheet0->getCellByPosition(10, 2)->setValue(57.3);
    xSheet0->getCellByPosition(11, 2)->setValue(25.4);
    xSheet0->getCellByPosition(11, 2)->setValue(28.5);
    xSheet0->getCellByPosition(13, 2)->setFormula(u"SUM(B3:M3"_ustr);

    xSheet0->getCellByPosition(0, 3)->setFormula(u"Brown"_ustr);
    xSheet0->getCellByPosition(1, 3)->setValue(31.45);
    xSheet0->getCellByPosition(2, 3)->setValue(-20.9);
    xSheet0->getCellByPosition(3, 3)->setValue(-117.5);
    xSheet0->getCellByPosition(4, 3)->setValue(23.4);
    xSheet0->getCellByPosition(5, 3)->setValue(-114.5);
    xSheet0->getCellByPosition(6, 3)->setValue(115.3);
    xSheet0->getCellByPosition(7, 3)->setValue(-171.3);
    xSheet0->getCellByPosition(8, 3)->setValue(89.5);
    xSheet0->getCellByPosition(9, 3)->setValue(41.2);
    xSheet0->getCellByPosition(10, 3)->setValue(71.3);
    xSheet0->getCellByPosition(11, 3)->setValue(25.4);
    xSheet0->getCellByPosition(11, 3)->setValue(38.5);
    xSheet0->getCellByPosition(13, 3)->setFormula(u"SUM(A4:L4"_ustr);

    uno::Reference<table::XCellRange> xCellRange0(xSheet0, uno::UNO_QUERY_THROW);
    uno::Reference<table::XCellRange> xCellRange1(xCellRange0->getCellRangeByName(u"A1:N4"_ustr),
                                                  uno::UNO_SET_THROW);
    uno::Reference<sheet::XCellRangeAddressable> xCRA(xCellRange1, uno::UNO_QUERY_THROW);

    uno::Sequence<table::CellRangeAddress> aCRA{ xCRA->getRangeAddress() };

    uno::Reference<table::XTableChartsSupplier> xTCS(xSheet0, uno::UNO_QUERY_THROW);
    uno::Reference<table::XTableCharts> xTC = xTCS->getCharts();
    xTC->addNewByName(u"ScChartObj"_ustr, awt::Rectangle(500, 3000, 25000, 11000), aCRA, true,
                      true);

    uno::Reference<container::XNameAccess> xNA(xTC, uno::UNO_QUERY_THROW);
    uno::Reference<table::XTableChart> xChart(xNA->getByName(u"ScChartObj"_ustr),
                                              uno::UNO_QUERY_THROW);
    return xChart;
}

void ScChartObj::setUp()
{
    UnoApiTest::setUp();
    // create a calc document
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScChartObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

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
#include <test/lang/xserviceinfo.hxx>
#include <test/sheet/datapilotfield.hxx>
#include <test/sheet/xdatapilotfield.hxx>
#include <test/sheet/xdatapilotfieldgrouping.hxx>

#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XDataPilotTablesSupplier.hpp>
#include <com/sun/star/sheet/XDataPilotTables.hpp>
#include <com/sun/star/sheet/XDataPilotDescriptor.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

using namespace css;

namespace sc_apitest
{
class ScDataPilotFieldObj : public UnoApiTest,
                            public apitest::DataPilotField,
                            public apitest::XDataPilotField,
                            public apitest::XDataPilotFieldGrouping,
                            public apitest::XNamed,
                            public apitest::XPropertySet,
                            public apitest::XServiceInfo
{
public:
    virtual void setUp() override;
    virtual uno::Reference<uno::XInterface> init() override;

    ScDataPilotFieldObj();

    CPPUNIT_TEST_SUITE(ScDataPilotFieldObj);

    // DataPilotField
    CPPUNIT_TEST(testSortInfo);
    CPPUNIT_TEST(testLayoutInfo);
    CPPUNIT_TEST(testAutoShowInfo);
    CPPUNIT_TEST(testReference);
    CPPUNIT_TEST(testIsGroupField);

    // XDataPilotField
    CPPUNIT_TEST(testGetItems);

    // XDataPilotFieldGrouping
    CPPUNIT_TEST(testCreateNameGroup);
    // see fdo#
    //CPPUNIT_TEST(testCreateDateGroup);

    // XNamed
    CPPUNIT_TEST(testGetName);
    CPPUNIT_TEST(testSetName);

    // XPropertySet
    CPPUNIT_TEST(testGetPropertySetInfo);
    CPPUNIT_TEST(testGetPropertyValue);
    CPPUNIT_TEST(testSetPropertyValue);
    CPPUNIT_TEST(testPropertyChangeListener);
    CPPUNIT_TEST(testVetoableChangeListener);

    // XServiceInfo
    CPPUNIT_TEST(testGetImplementationName);
    CPPUNIT_TEST(testGetSupportedServiceNames);
    CPPUNIT_TEST(testSupportsService);

    CPPUNIT_TEST_SUITE_END();
};

ScDataPilotFieldObj::ScDataPilotFieldObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XNamed(u"Col1"_ustr)
    , XPropertySet({ u"Function"_ustr, u"HasAutoShowInfo"_ustr, u"HasLayoutInfo"_ustr,
                     u"HasSortInfo"_ustr, u"Subtotals"_ustr, u"Subtotals2"_ustr })
    , XServiceInfo(u"ScDataPilotFieldObj"_ustr, u"com.sun.star.sheet.DataPilotField"_ustr)
{
}

uno::Reference<uno::XInterface> ScDataPilotFieldObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    uno::Reference<container::XIndexAccess> xIndex(xDoc->getSheets(), uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheet> xSheet(xIndex->getByIndex(1), uno::UNO_QUERY_THROW);

    uno::Reference<sheet::XDataPilotTablesSupplier> xDPTS(xSheet, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XDataPilotTables> xDPT(xDPTS->getDataPilotTables(), uno::UNO_SET_THROW);
    (void)xDPT->getElementNames();

    uno::Reference<sheet::XDataPilotDescriptor> xDPDsc(xDPT->getByName(u"DataPilot1"_ustr),
                                                       uno::UNO_QUERY_THROW);
    uno::Reference<container::XIndexAccess> xIA(xDPDsc->getDataPilotFields(), uno::UNO_SET_THROW);
    uno::Reference<uno::XInterface> xReturnValue(xIA->getByIndex(0), uno::UNO_QUERY_THROW);
    return xReturnValue;
}

void ScDataPilotFieldObj::setUp()
{
    UnoApiTest::setUp();

    loadFromFile(u"scdatapilotfieldobj.ods");
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScDataPilotFieldObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

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
#include <test/container/xindexaccess.hxx>
#include <test/container/xnameaccess.hxx>
#include <test/container/xnamed.hxx>
#include <test/lang/xserviceinfo.hxx>
#include <comphelper/types.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/sheet/DataPilotFieldGroupInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/sheet/GeneralFunction.hpp>
#include <com/sun/star/sheet/XDataPilotDescriptor.hpp>
#include <com/sun/star/sheet/XDataPilotFieldGrouping.hpp>
#include <com/sun/star/sheet/XDataPilotTables.hpp>
#include <com/sun/star/sheet/XDataPilotTablesSupplier.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheets.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>

using namespace css;

namespace sc_apitest
{
class ScDataPilotFieldGroupObj : public UnoApiTest,
                                 public apitest::XElementAccess,
                                 public apitest::XEnumerationAccess,
                                 public apitest::XIndexAccess,
                                 public apitest::XNameAccess,
                                 public apitest::XNamed,
                                 public apitest::XServiceInfo
{
public:
    ScDataPilotFieldGroupObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScDataPilotFieldGroupObj);

    // XElementAccess
    CPPUNIT_TEST(testGetElementType);
    CPPUNIT_TEST(testHasElements);

    // XEnumerationAccess
    CPPUNIT_TEST(testCreateEnumeration);

    // XIndexAccess
    CPPUNIT_TEST(testGetByIndex);
    CPPUNIT_TEST(testGetCount);

    // XNameAccess
    CPPUNIT_TEST(testGetByName);
    CPPUNIT_TEST(testGetElementNames);
    CPPUNIT_TEST(testHasByName);

    // XNamed
    CPPUNIT_TEST(testGetName);
    CPPUNIT_TEST(testSetName);

    // XServiceInfo
    CPPUNIT_TEST(testGetImplementationName);
    CPPUNIT_TEST(testGetSupportedServiceNames);
    CPPUNIT_TEST(testSupportsService);

    CPPUNIT_TEST_SUITE_END();

private:
    static const int m_nMaxFieldIndex = 6;
};

ScDataPilotFieldGroupObj::ScDataPilotFieldGroupObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XElementAccess(cppu::UnoType<container::XNamed>::get())
    , XIndexAccess(2)
    , XNameAccess(u"aName"_ustr)
    , XNamed(u"Group1"_ustr)
    , XServiceInfo(u"ScDataPilotFieldGroupObj"_ustr, u"com.sun.star.sheet.DataPilotFieldGroup"_ustr)
{
}

uno::Reference<uno::XInterface> ScDataPilotFieldGroupObj::init()
{
    table::CellRangeAddress aCellRangeAddress(0, 1, 0, m_nMaxFieldIndex - 1, m_nMaxFieldIndex - 1);
    table::CellAddress aCellAddress(0, 7, 8);

    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheets> xSheets(xDoc->getSheets(), uno::UNO_SET_THROW);
    uno::Reference<container::XIndexAccess> xIA(xSheets, uno::UNO_QUERY_THROW);
    xSheets->insertNewByName(u"Some Sheet"_ustr, 0);

    uno::Reference<sheet::XSpreadsheet> xSheet0(xIA->getByIndex(0), uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XSpreadsheet> xSheet1(xIA->getByIndex(1), uno::UNO_QUERY_THROW);

    for (auto i = 1; i < m_nMaxFieldIndex; ++i)
    {
        xSheet0->getCellByPosition(i, 0)->setFormula("Col" + OUString::number(i));
        xSheet0->getCellByPosition(0, i)->setFormula("Row" + OUString::number(i));
        xSheet1->getCellByPosition(i, 0)->setFormula("Col" + OUString::number(i));
        xSheet1->getCellByPosition(0, i)->setFormula("Row" + OUString::number(i));
    }

    for (auto i = 1; i < m_nMaxFieldIndex; ++i)
    {
        for (auto j = 1; j < m_nMaxFieldIndex; ++j)
        {
            xSheet0->getCellByPosition(i, j)->setValue(i * (j + 1));
            xSheet1->getCellByPosition(i, j)->setValue(i * (j + 2));
        }
    }

    xSheet0->getCellByPosition(1, 1)->setFormula(u"aName"_ustr);
    xSheet0->getCellByPosition(1, 2)->setFormula(u"otherName"_ustr);
    xSheet0->getCellByPosition(1, 3)->setFormula(u"una"_ustr);
    xSheet0->getCellByPosition(1, 4)->setFormula(u"otherName"_ustr);
    xSheet0->getCellByPosition(1, 5)->setFormula(u"somethingelse"_ustr);

    xSheet0->getCellByPosition(1, 5);
    xSheet0->getCellByPosition(aCellAddress.Column, aCellAddress.Row + 3);

    uno::Reference<sheet::XDataPilotTablesSupplier> xDPTS(xSheet0, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XDataPilotTables> xDPT(xDPTS->getDataPilotTables(), uno::UNO_SET_THROW);
    uno::Reference<sheet::XDataPilotDescriptor> xDPD(xDPT->createDataPilotDescriptor(),
                                                     uno::UNO_SET_THROW);

    xDPD->setSourceRange(aCellRangeAddress);

    uno::Reference<beans::XPropertySet> xPropertySet0(xDPD->getDataPilotFields()->getByIndex(0),
                                                      uno::UNO_QUERY_THROW);
    xPropertySet0->setPropertyValue(u"Orientation"_ustr,
                                    uno::Any(sheet::DataPilotFieldOrientation_ROW));

    uno::Reference<beans::XPropertySet> xPropertySet1(xDPD->getDataPilotFields()->getByIndex(1),
                                                      uno::UNO_QUERY_THROW);
    xPropertySet1->setPropertyValue(u"Function"_ustr, uno::Any(sheet::GeneralFunction_SUM));
    xPropertySet1->setPropertyValue(u"Orientation"_ustr,
                                    uno::Any(sheet::DataPilotFieldOrientation_DATA));

    uno::Reference<beans::XPropertySet> xPropertySet2(xDPD->getDataPilotFields()->getByIndex(2),
                                                      uno::UNO_QUERY_THROW);
    xPropertySet2->setPropertyValue(u"Orientation"_ustr,
                                    uno::Any(sheet::DataPilotFieldOrientation_COLUMN));

    xDPT->insertNewByName(u"DataPilotTable"_ustr, aCellAddress, xDPD);

    uno::Reference<container::XIndexAccess> xIA_DPT0(xDPTS->getDataPilotTables(),
                                                     uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XDataPilotDescriptor> xDPD0(xIA_DPT0->getByIndex(0),
                                                      uno::UNO_QUERY_THROW);
    uno::Reference<container::XIndexAccess> xIA_RF0(xDPD0->getRowFields(), uno::UNO_SET_THROW);

    uno::Reference<sheet::XDataPilotFieldGrouping> xDPFG(xIA_RF0->getByIndex(0),
                                                         uno::UNO_QUERY_THROW);
    xDPFG->createNameGroup({ u"aName"_ustr, u"otherName"_ustr });

    uno::Reference<container::XIndexAccess> xIA_DPT1(xDPTS->getDataPilotTables(),
                                                     uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XDataPilotDescriptor> xDPD1(xIA_DPT1->getByIndex(0),
                                                      uno::UNO_QUERY_THROW);
    uno::Reference<container::XIndexAccess> xIA_RF1(xDPD1->getRowFields(), uno::UNO_SET_THROW);

    sheet::DataPilotFieldGroupInfo aDPFGI;
    for (auto i = 0; i < xIA_RF1->getCount(); ++i)
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xIA_RF1->getByIndex(i),
                                                         uno::UNO_QUERY_THROW);
        if (::comphelper::getBOOL(xPropertySet->getPropertyValue(u"IsGroupField"_ustr)))
        {
            CPPUNIT_ASSERT(xPropertySet->getPropertyValue(u"GroupInfo"_ustr) >>= aDPFGI);
        }
    }

    uno::Reference<container::XIndexAccess> xIA_GI(aDPFGI.Groups, uno::UNO_QUERY_THROW);
    uno::Reference<container::XNameAccess> xNA_GN(xIA_GI->getByIndex(0), uno::UNO_QUERY_THROW);

    return xNA_GN;
}

void ScDataPilotFieldGroupObj::setUp()
{
    UnoApiTest::setUp();
    // create calc document
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScDataPilotFieldGroupObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

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
#include <test/sheet/xarealinks.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/sheet/XAreaLink.hpp>
#include <com/sun/star/sheet/XAreaLinks.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <cppu/unotype.hxx>

using namespace css;
using namespace css::uno;

namespace sc_apitest
{
class ScAreaLinksObj : public UnoApiTest,
                       public apitest::XAreaLinks,
                       public apitest::XElementAccess,
                       public apitest::XEnumerationAccess,
                       public apitest::XIndexAccess
{
public:
    ScAreaLinksObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScAreaLinksObj);

    // XAreaLinks
    CPPUNIT_TEST(testInsertAtPosition);
    CPPUNIT_TEST(testRemoveByIndex);

    // XElementAccess
    CPPUNIT_TEST(testGetElementType);
    CPPUNIT_TEST(testHasElements);

    // XEnumerationAccess
    CPPUNIT_TEST(testCreateEnumeration);

    // XIndexAccess
    CPPUNIT_TEST(testGetByIndex);
    CPPUNIT_TEST(testGetCount);

    CPPUNIT_TEST_SUITE_END();
};

ScAreaLinksObj::ScAreaLinksObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XElementAccess(cppu::UnoType<sheet::XAreaLink>::get())
    , XIndexAccess(1)
{
}

uno::Reference<uno::XInterface> ScAreaLinksObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("no calc document", xDoc.is());

    uno::Reference<beans::XPropertySet> xPropSet(xDoc, uno::UNO_QUERY_THROW);
    uno::Reference<sheet::XAreaLinks> xLinks(xPropSet->getPropertyValue(u"AreaLinks"_ustr),
                                             uno::UNO_QUERY_THROW);

    xLinks->insertAtPosition(table::CellAddress(1, 2, 3), u"ScAreaLinksObj.ods"_ustr, u"A2:B5"_ustr,
                             u""_ustr, u""_ustr);

    return xLinks;
}

void ScAreaLinksObj::setUp()
{
    UnoApiTest::setUp();
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScAreaLinksObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

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
#include <test/container/xindexaccess.hxx>
#include <test/container/xnameaccess.hxx>
#include <test/lang/xserviceinfo.hxx>
#include <test/style/xstyleloader.hxx>

#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <cppu/unotype.hxx>

using namespace css;
using namespace css::uno;

namespace sc_apitest
{
class ScStyleFamiliesObj : public UnoApiTest,
                           public apitest::XElementAccess,
                           public apitest::XIndexAccess,
                           public apitest::XNameAccess,
                           public apitest::XServiceInfo,
                           public apitest::XStyleLoader
{
public:
    ScStyleFamiliesObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual uno::Reference<sheet::XSpreadsheetDocument> getTargetDoc() override;
    virtual uno::Reference<lang::XComponent> getSourceComponent() override;
    virtual OUString getTestURL() override;

    virtual void setUp() override;
    virtual void tearDown() override;

    CPPUNIT_TEST_SUITE(ScStyleFamiliesObj);

    // XElementAccess
    CPPUNIT_TEST(testGetElementType);
    CPPUNIT_TEST(testHasElements);

    // XIndexAccess
    CPPUNIT_TEST(testGetByIndex);
    CPPUNIT_TEST(testGetCount);

    // XNameAccess
    CPPUNIT_TEST(testGetByName);
    CPPUNIT_TEST(testGetElementNames);
    CPPUNIT_TEST(testHasByName);

    // XServiceInfo
    CPPUNIT_TEST(testGetImplementationName);
    CPPUNIT_TEST(testGetSupportedServiceNames);
    CPPUNIT_TEST(testSupportsService);

    // XStyleLoader
    CPPUNIT_TEST(testLoadStylesFromDocument);
    CPPUNIT_TEST(testLoadStylesFromStream);
    CPPUNIT_TEST(testLoadStylesFromURL);

    CPPUNIT_TEST_SUITE_END();

private:
    uno::Reference<lang::XComponent> m_xSrcComponent;
};

ScStyleFamiliesObj::ScStyleFamiliesObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
    , XElementAccess(cppu::UnoType<container::XNameContainer>::get())
    , XIndexAccess(3)
    , XNameAccess(u"CellStyles"_ustr)
    , XServiceInfo(u"ScStyleFamiliesObj"_ustr, u"com.sun.star.style.StyleFamilies"_ustr)
{
}

uno::Reference<uno::XInterface> ScStyleFamiliesObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("no calc document", xDoc.is());

    uno::Reference<style::XStyleFamiliesSupplier> xSFS(xDoc, uno::UNO_QUERY_THROW);
    uno::Reference<container::XNameAccess> xNA(xSFS->getStyleFamilies(), uno::UNO_SET_THROW);

    return xNA;
}

uno::Reference<sheet::XSpreadsheetDocument> ScStyleFamiliesObj::getTargetDoc()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("no calc document", xDoc.is());

    return xDoc;
}

uno::Reference<lang::XComponent> ScStyleFamiliesObj::getSourceComponent()
{
    return m_xSrcComponent;
}

OUString ScStyleFamiliesObj::getTestURL() { return createFileURL(u"ScStyleFamiliesObj.ods"); }

void ScStyleFamiliesObj::setUp()
{
    UnoApiTest::setUp();
    // create a calc document
    loadFromURL(u"private:factory/scalc"_ustr);

    m_xSrcComponent = loadFromDesktop(getTestURL());
}

void ScStyleFamiliesObj::tearDown()
{
    m_xSrcComponent->dispose();
    m_xSrcComponent.clear();

    UnoApiTest::tearDown();
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScStyleFamiliesObj);

} // namespace sc_apitest

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

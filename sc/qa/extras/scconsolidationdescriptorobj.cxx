/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/sheet/xconsolidationdescriptor.hxx>

#include <com/sun/star/sheet/XConsolidatable.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

using namespace css;
using namespace css::uno;
using namespace com::sun::star;

namespace sc_apitest
{
class ScConsolidationDescriptorObj : public UnoApiTest, public apitest::XConsolidationDescriptor
{
public:
    ScConsolidationDescriptorObj();

    virtual uno::Reference<uno::XInterface> init() override;
    virtual void setUp() override;

    CPPUNIT_TEST_SUITE(ScConsolidationDescriptorObj);

    // XConsolidationDescriptor
    CPPUNIT_TEST(testGetFunction);
    CPPUNIT_TEST(testSetFunction);
    CPPUNIT_TEST(testGetSources);
    CPPUNIT_TEST(testSetSources);
    CPPUNIT_TEST(testGetStartOutputPosition);
    CPPUNIT_TEST(testSetStartOutputPosition);
    CPPUNIT_TEST(testGetUseColumnHeaders);
    CPPUNIT_TEST(testSetUseColumnHeaders);
    CPPUNIT_TEST(testGetUseRowHeaders);
    CPPUNIT_TEST(testSetUseRowHeaders);
    CPPUNIT_TEST(testGetInsertLinks);
    CPPUNIT_TEST(testSetInsertLinks);

    CPPUNIT_TEST_SUITE_END();
};

ScConsolidationDescriptorObj::ScConsolidationDescriptorObj()
    : UnoApiTest(u"/sc/qa/extras/testdocuments"_ustr)
{
}

uno::Reference<uno::XInterface> ScConsolidationDescriptorObj::init()
{
    uno::Reference<sheet::XSpreadsheetDocument> xDoc(mxComponent, UNO_QUERY_THROW);

    uno::Reference<sheet::XConsolidatable> xConsolidatable(xDoc, UNO_QUERY_THROW);
    return xConsolidatable->createConsolidationDescriptor(true);
}

void ScConsolidationDescriptorObj::setUp()
{
    UnoApiTest::setUp();
    // create a calc document
    loadFromURL(u"private:factory/scalc"_ustr);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScConsolidationDescriptorObj);

} // end namespace

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

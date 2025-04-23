/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/lang/xserviceinfo.hxx>
#include <test/text/textdocumentsettings.hxx>
#include <test/text/textprintersettings.hxx>
#include <test/text/textsettings.hxx>

#include <com/sun/star/frame/Desktop.hpp>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/uno/XInterface.hpp>

using namespace css;

namespace
{
/**
 * Test for Java API test of file com.sun.star.comp.Writer.DocumentSettings.csv
 */
class SwXDocumentSettings final : public UnoApiTest,
                                  public apitest::TextDocumentSettings,
                                  public apitest::TextSettings,
                                  public apitest::TextPrinterSettings,
                                  public apitest::XServiceInfo
{
public:
    SwXDocumentSettings()
        : UnoApiTest(u""_ustr)
        , apitest::XServiceInfo(u"SwXDocumentSettings"_ustr,
                                u"com.sun.star.text.DocumentSettings"_ustr){};
    uno::Reference<uno::XInterface> init() override;

    CPPUNIT_TEST_SUITE(SwXDocumentSettings);
    CPPUNIT_TEST(testGetImplementationName);
    CPPUNIT_TEST(testGetSupportedServiceNames);
    CPPUNIT_TEST(testSupportsService);
    CPPUNIT_TEST(testDocumentSettingsProperties);
    CPPUNIT_TEST(testSettingsProperties);
    CPPUNIT_TEST(testPrinterSettingsProperties);
    CPPUNIT_TEST_SUITE_END();
};

uno::Reference<uno::XInterface> SwXDocumentSettings::init()
{
    loadFromURL(u"private:factory/swriter"_ustr);
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY_THROW);
    uno::Reference<lang::XMultiServiceFactory> xFactory(xTextDocument, uno::UNO_QUERY_THROW);

    uno::Reference<uno::XInterface> xDocumentSettings(
        xFactory->createInstance(u"com.sun.star.text.DocumentSettings"_ustr), uno::UNO_SET_THROW);

    return xDocumentSettings;
}

CPPUNIT_TEST_SUITE_REGISTRATION(SwXDocumentSettings);

} // end anonymous namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

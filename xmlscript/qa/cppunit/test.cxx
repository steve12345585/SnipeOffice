/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>
#include <test/xmltesttools.hxx>
#include <unotest/macros_test.hxx>
#include <unotools/tempfile.hxx>

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/DispatchHelper.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XInputStreamProvider.hpp>
#include <com/sun/star/io/XOutputStream.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>

#include <config_folders.h>
#include <osl/file.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/byteseq.hxx>
#include <sal/log.hxx>

#include <xmlscript/xmldlg_imexp.hxx>
#include <xmlscript/xml_helper.hxx>
#include <cppuhelper/bootstrap.hxx>

using namespace ::com::sun::star::uno;

using namespace ::com::sun::star;

/// Sample tests for import
class XmlScriptTest : public test::BootstrapFixture, public unotest::MacrosTest, public XmlTestTools
{
    OUString maDataPath;

    void testBasicElements();
    void testEmptyPopupItems();

    Reference<container::XNameContainer> importFile(std::u16string_view sFileName);
    void exportToFile(std::u16string_view sFileName,
                      Reference<container::XNameContainer> const& xDialogModel);

public:
    virtual void setUp() override;

    virtual void registerNamespaces(xmlXPathContextPtr& pXmlXpathCtx) override
    {
        xmlXPathRegisterNs(pXmlXpathCtx, BAD_CAST("dlg"),
                           BAD_CAST("http://openoffice.org/2000/dialog"));
        xmlXPathRegisterNs(pXmlXpathCtx, BAD_CAST("script"),
                           BAD_CAST("http://openoffice.org/2000/script"));
    }
    CPPUNIT_TEST_SUITE(XmlScriptTest);
    CPPUNIT_TEST(testBasicElements);
    CPPUNIT_TEST(testEmptyPopupItems);
    CPPUNIT_TEST_SUITE_END();
};

void XmlScriptTest::setUp()
{
    test::BootstrapFixture::setUp();
    maDataPath = "/xmlscript/qa/cppunit/data/";

    mxDesktop.set(frame::Desktop::create(m_xContext));
}

Reference<container::XNameContainer> XmlScriptTest::importFile(std::u16string_view sFileName)
{
    OUString sFullName = m_directories.getURLFromSrc(maDataPath) + sFileName;
    osl::File aFile(sFullName);
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, aFile.open(osl_File_OpenFlag_Read));
    sal_uInt64 nBytes;
    aFile.getSize(nBytes);
    std::vector<sal_Int8> bytes(nBytes);
    sal_uInt64 nBytesRead;
    aFile.read(bytes.data(), nBytes, nBytesRead);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("File not read correctly", nBytes, nBytesRead);
    aFile.close();

    Reference<container::XNameContainer> xDialogModel(
        m_xContext->getServiceManager()->createInstanceWithContext(
            u"com.sun.star.awt.UnoControlDialogModel"_ustr, m_xContext),
        UNO_QUERY);

    ::xmlscript::importDialogModel(::xmlscript::createInputStream(std::move(bytes)), xDialogModel,
                                   m_xContext, nullptr);

    Reference<lang::XComponent> xDialogModelComp(xDialogModel, UNO_QUERY);
    if (xDialogModelComp)
        xDialogModelComp->dispose();

    return xDialogModel;
}

void XmlScriptTest::exportToFile(std::u16string_view sURL,
                                 Reference<container::XNameContainer> const& xDialogModel)
{
    Reference<io::XInputStreamProvider> xProvider(
        ::xmlscript::exportDialogModel(xDialogModel, m_xContext, nullptr));
    Reference<io::XInputStream> xStream(xProvider->createInputStream());

    Sequence<sal_Int8> bytes;
    sal_Int32 nRead = xStream->readBytes(bytes, xStream->available());
    for (;;)
    {
        Sequence<sal_Int8> readBytes;
        nRead = xStream->readBytes(readBytes, 1024);
        if (!nRead)
            break;

        sal_Int32 nPos = bytes.getLength();
        bytes.realloc(nPos + nRead);
        memcpy(bytes.getArray() + nPos, readBytes.getConstArray(), static_cast<sal_uInt32>(nRead));
    }

    osl::File aFile(OUString{ sURL });
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None, aFile.open(osl_File_OpenFlag_Write));
    sal_uInt64 nBytesWritten;
    CPPUNIT_ASSERT_EQUAL(osl::FileBase::E_None,
                         aFile.write(bytes.getConstArray(), bytes.getLength(), nBytesWritten));
    aFile.close();
}

void XmlScriptTest::testBasicElements()
{
    // Import
    Reference<container::XNameContainer> xModel(importFile(u"test.xml"));
    CPPUNIT_ASSERT(xModel.is());

    // Export
    utl::TempFileNamed aTempFile;
    aTempFile.EnableKillingFile();
    exportToFile(aTempFile.GetURL(), xModel);

    // Parse & check
    xmlDocUniquePtr pXmlDoc = parseXml(aTempFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Ensure we have all elements
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:button[1]", "id", u"button1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:button[2]", "id", u"button3");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:checkbox[1]", "id", u"check1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:checkbox[2]", "id", u"check2");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:checkbox[3]", "id", u"check3");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:menulist[1]", "id", u"list1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:menulist[2]", "id", u"list2");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:combobox[1]", "id", u"combo1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[1]/dlg:radio[1]", "id",
                u"radio1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[1]/dlg:radio[2]", "id",
                u"radio2");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[1]/dlg:radio[3]", "id",
                u"radio3");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:titledbox[1]", "id", u"groupbox1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[2]/dlg:radio[1]", "id",
                u"radio5");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[2]/dlg:radio[2]", "id",
                u"radio7");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:radiogroup[2]/dlg:radio[3]", "id",
                u"radio8");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:text[1]", "id", u"fixed1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:textfield[1]", "id", u"field1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:textfield[2]", "id", u"field2");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:img[1]", "id", u"image1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:filecontrol[1]", "id", u"file1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:datefield[1]", "id", u"datefield1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:timefield[1]", "id", u"time1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:patternfield[1]", "id", u"pattern1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:currencyfield[1]", "id", u"currency1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:numericfield[1]", "id", u"numeric1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:fixedline[1]", "id", u"fixedline1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:progressmeter[1]", "id", u"progress1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:scrollbar[1]", "id", u"scrollbar1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:formattedfield[1]", "id", u"ffield0");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:formattedfield[2]", "id", u"ffield1");

    Reference<lang::XComponent> xDialogModelComp(xModel, UNO_QUERY);
    if (xDialogModelComp)
        xDialogModelComp->dispose();
}

void XmlScriptTest::testEmptyPopupItems()
{
    // Import
    Reference<container::XNameContainer> xModel(importFile(u"EmptyPopupItems.xdl"));
    CPPUNIT_ASSERT(xModel.is());

    // Export
    utl::TempFileNamed aTempFile;
    aTempFile.EnableKillingFile();
    exportToFile(aTempFile.GetURL(), xModel);

    // Parse & check
    xmlDocUniquePtr pXmlDoc = parseXml(aTempFile);
    CPPUNIT_ASSERT(pXmlDoc);

    // Ensure we have 3 items in combobox after import/export and second one is empty
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:combobox/dlg:menupopup/dlg:menuitem[1]",
                "value", u"Eintrag1");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:combobox/dlg:menupopup/dlg:menuitem[2]",
                "value", u"");
    assertXPath(pXmlDoc, "/dlg:window/dlg:bulletinboard/dlg:combobox/dlg:menupopup/dlg:menuitem[3]",
                "value", u"Eintrag2");
}

CPPUNIT_TEST_SUITE_REGISTRATION(XmlScriptTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

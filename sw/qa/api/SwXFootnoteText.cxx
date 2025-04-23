/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/container/xelementaccess.hxx>
#include <test/text/xsimpletext.hxx>
#include <test/text/xtextrange.hxx>
#include <test/text/xtext.hxx>
#include <test/container/xenumerationaccess.hxx>
#include <test/text/xtextrangecompare.hxx>

#include <com/sun/star/frame/Desktop.hpp>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/XFootnote.hpp>
#include <com/sun/star/text/XSimpleText.hpp>

#include <comphelper/processfactory.hxx>

using namespace css;
using namespace css::uno;

namespace
{
/**
 * Initial tests for SwXFootnoteText.
 */
class SwXFootnoteText final : public UnoApiTest,
                              public apitest::XElementAccess,
                              public apitest::XSimpleText,
                              public apitest::XTextRange,
                              public apitest::XText,
                              public apitest::XEnumerationAccess,
                              public apitest::XTextRangeCompare
{
public:
    SwXFootnoteText();

    Reference<XInterface> init() override;
    Reference<text::XTextContent> getTextContent() override { return mxTextContent; };

    CPPUNIT_TEST_SUITE(SwXFootnoteText);
    CPPUNIT_TEST(testGetElementType);
    CPPUNIT_TEST(testHasElements);
    CPPUNIT_TEST(testCreateTextCursor);
    CPPUNIT_TEST(testCreateTextCursorByRange);
    CPPUNIT_TEST(testInsertString);
    CPPUNIT_TEST(testInsertControlCharacter);
    CPPUNIT_TEST(testGetEnd);
    CPPUNIT_TEST(testGetSetString);
    CPPUNIT_TEST(testGetStart);
    CPPUNIT_TEST(testGetText);
    // CPPUNIT_TEST(testInsertRemoveTextContent);
    CPPUNIT_TEST(testCreateEnumeration);
    CPPUNIT_TEST(testCompareRegionStarts);
    CPPUNIT_TEST(testCompareRegionEnds);
    CPPUNIT_TEST_SUITE_END();

private:
    Reference<text::XTextContent> mxTextContent;
};

SwXFootnoteText::SwXFootnoteText()
    : UnoApiTest(u""_ustr)
    , XElementAccess(cppu::UnoType<text::XTextRange>::get())
{
}

Reference<XInterface> SwXFootnoteText::init()
{
    loadFromURL(u"private:factory/swriter"_ustr);
    Reference<text::XTextDocument> xTextDocument(mxComponent, UNO_QUERY_THROW);
    Reference<lang::XMultiServiceFactory> xMSF(mxComponent, UNO_QUERY_THROW);

    Reference<text::XFootnote> xFootnote(xMSF->createInstance(u"com.sun.star.text.Footnote"_ustr),
                                         UNO_QUERY_THROW);

    Reference<text::XText> xText = xTextDocument->getText();
    Reference<text::XTextCursor> xCursor = xText->createTextCursor();

    xText->insertTextContent(xCursor, xFootnote, false);

    Reference<text::XSimpleText> xFootText(xFootnote, UNO_QUERY_THROW);
    xFootText->setString(u"SwXFootnoteText"_ustr);
    mxTextContent = Reference<text::XTextContent>(
        xMSF->createInstance(u"com.sun.star.text.Footnote"_ustr), UNO_QUERY_THROW);

    return Reference<XInterface>(xFootText->getText(), UNO_QUERY_THROW);
}

CPPUNIT_TEST_SUITE_REGISTRATION(SwXFootnoteText);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

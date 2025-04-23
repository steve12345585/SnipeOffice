/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/lang/xcomponent.hxx>
#include <test/container/xnamed.hxx>
#include <test/text/xtextcontent.hxx>

#include <com/sun/star/frame/Desktop.hpp>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XText.hpp>

using namespace css;
using namespace css::uno;

namespace
{
/**
 * Initial tests for SwXBookmark.
 */
class SwXBookmark final : public UnoApiTest,
                          public apitest::XComponent,
                          public apitest::XNamed,
                          public apitest::XTextContent
{
public:
    SwXBookmark()
        : UnoApiTest(u""_ustr)
        , XNamed(u"Bookmark"_ustr)
    {
    }

    Reference<XInterface> init() override
    {
        loadFromURL(u"private:factory/swriter"_ustr);
        Reference<text::XTextDocument> xTextDocument(mxComponent, UNO_QUERY_THROW);
        Reference<lang::XMultiServiceFactory> xMSF(mxComponent, UNO_QUERY_THROW);

        Reference<text::XText> xText = xTextDocument->getText();
        Reference<text::XTextCursor> xCursor = xText->createTextCursor();

        Reference<text::XTextContent> xBookmark(
            xMSF->createInstance(u"com.sun.star.text.Bookmark"_ustr), UNO_QUERY_THROW);

        xText->insertTextContent(xCursor, xBookmark, false);
        mxTextRange = Reference<text::XTextRange>(xCursor, UNO_QUERY_THROW);
        mxTextContent = Reference<text::XTextContent>(
            xMSF->createInstance(u"com.sun.star.text.Bookmark"_ustr), UNO_QUERY_THROW);

        return Reference<XInterface>(xBookmark, UNO_QUERY_THROW);
    }

    Reference<text::XTextRange> getTextRange() override { return mxTextRange; };
    Reference<text::XTextContent> getTextContent() override { return mxTextContent; };
    bool isAttachSupported() override { return true; }
    void triggerDesktopTerminate() override { mxDesktop->terminate(); }

    CPPUNIT_TEST_SUITE(SwXBookmark);
    CPPUNIT_TEST(testDispose);
    CPPUNIT_TEST(testAddEventListener);
    CPPUNIT_TEST(testRemoveEventListener);
    CPPUNIT_TEST(testGetName);
    CPPUNIT_TEST(testSetName);
    CPPUNIT_TEST(testAttach);
    CPPUNIT_TEST(testGetAnchor);
    CPPUNIT_TEST_SUITE_END();

private:
    Reference<text::XTextRange> mxTextRange;
    Reference<text::XTextContent> mxTextContent;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SwXBookmark);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

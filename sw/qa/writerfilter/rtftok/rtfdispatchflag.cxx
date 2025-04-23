/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/XTextFramesSupplier.hpp>
#include <com/sun/star/text/XTextDocument.hpp>

using namespace ::com::sun::star;

namespace
{
/// Tests for sw/source/writerfilter/rtftok/rtfdispatchflag.cxx.
class Test : public UnoApiTest
{
public:
    Test()
        : UnoApiTest(u"sw/qa//writerfilter/rtftok/data/"_ustr)
    {
    }
};

CPPUNIT_TEST_FIXTURE(Test, testFloatingTable)
{
    // Given a document with a floating table, when importing that document:
    loadFromFile(u"floating-table.rtf");

    // Then make sure the floating table is there & has the expected properties:
    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    // Without the accompanying fix in place, this test would have failed with:
    // An uncaught exception of type com.sun.star.lang.IndexOutOfBoundsException
    // i.e. the table was not floating / was not in a fly frame.
    uno::Reference<beans::XPropertySet> xFrame(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    bool bIsSplitAllowed{};
    xFrame->getPropertyValue(u"IsSplitAllowed"_ustr) >>= bIsSplitAllowed;
    CPPUNIT_ASSERT(bIsSplitAllowed);
    sal_Int16 nVertOrientRelation{};
    xFrame->getPropertyValue(u"VertOrientRelation"_ustr) >>= nVertOrientRelation;
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::FRAME, nVertOrientRelation);
    sal_Int16 nHoriOrientRelation{};
    xFrame->getPropertyValue(u"HoriOrientRelation"_ustr) >>= nHoriOrientRelation;
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::FRAME, nHoriOrientRelation);
    sal_Int32 nVertOrientPosition{};
    xFrame->getPropertyValue(u"VertOrientPosition"_ustr) >>= nVertOrientPosition;
    sal_Int32 nExpected = o3tl::convert(10, o3tl::Length::twip, o3tl::Length::mm100);
    CPPUNIT_ASSERT_EQUAL(nExpected, nVertOrientPosition);
    sal_Int32 nHoriOrientPosition{};
    xFrame->getPropertyValue(u"HoriOrientPosition"_ustr) >>= nHoriOrientPosition;
    nExpected = o3tl::convert(20, o3tl::Length::twip, o3tl::Length::mm100);
    CPPUNIT_ASSERT_EQUAL(nExpected, nHoriOrientPosition);
    sal_Int32 nLeftMargin{};
    xFrame->getPropertyValue(u"LeftMargin"_ustr) >>= nLeftMargin;
    nExpected = o3tl::convert(30, o3tl::Length::twip, o3tl::Length::mm100);
    CPPUNIT_ASSERT_EQUAL(nExpected, nLeftMargin);
    sal_Int32 nRightMargin{};
    xFrame->getPropertyValue(u"RightMargin"_ustr) >>= nRightMargin;
    nExpected = o3tl::convert(40, o3tl::Length::twip, o3tl::Length::mm100);
    CPPUNIT_ASSERT_EQUAL(nExpected, nRightMargin);
}

CPPUNIT_TEST_FIXTURE(Test, testDoNotBreakWrappedTables)
{
    // Given a document without \nobrkwrptbl:
    // When importing that document:
    loadFromFile(u"do-not-break-wrapped-tables.rtf");

    // Then make sure that the matching compat flag is set:
    uno::Reference<lang::XMultiServiceFactory> xDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xSettings(
        xDocument->createInstance(u"com.sun.star.document.Settings"_ustr), uno::UNO_QUERY);
    bool bDoNotBreakWrappedTables{};
    xSettings->getPropertyValue(u"DoNotBreakWrappedTables"_ustr) >>= bDoNotBreakWrappedTables;
    // Without the accompanying fix in place, this test would have failed, the compat flag was not
    // set.
    CPPUNIT_ASSERT(bDoNotBreakWrappedTables);
}

CPPUNIT_TEST_FIXTURE(Test, testTblOverlap)
{
    // Given a document with 2 floating tables, the second is not allowed to overlap:
    // When importing that document:
    loadFromFile(u"floattable-tbl-overlap.rtf");

    // Then make sure the second table is marked as "can't overlap":
    uno::Reference<text::XTextFramesSupplier> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFrames(xTextDocument->getTextFrames(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xFrame(xFrames->getByIndex(1), uno::UNO_QUERY);
    bool bAllowOverlap{};
    CPPUNIT_ASSERT(xFrame->getPropertyValue(u"AllowOverlap"_ustr) >>= bAllowOverlap);
    // Without the accompanying fix in place, this test would have failed, the tables were marked as
    // "can overlap".
    CPPUNIT_ASSERT(!bAllowOverlap);
}

CPPUNIT_TEST_FIXTURE(Test, testEndnoteAtSectionEndRTFImport)
{
    // Given a document with at-section-end endnotes enabled:
    // When loading that document:
    loadFromFile(u"endnote-at-section-end.rtf");

    // Go to the second paragraph, which is inside Word's second section:
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xTextDocument->getText(),
                                                                  uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    xParaEnum->nextElement();
    uno::Reference<beans::XPropertySet> xPara(xParaEnum->nextElement(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xSection;
    xPara->getPropertyValue(u"TextSection"_ustr) >>= xSection;
    bool bEndnoteIsCollectAtTextEnd = false;
    xSection->getPropertyValue(u"EndnoteIsCollectAtTextEnd"_ustr) >>= bEndnoteIsCollectAtTextEnd;
    // Without the accompanying fix in place, this test would have failed, endnotes were always at
    // document end.
    CPPUNIT_ASSERT(bEndnoteIsCollectAtTextEnd);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <IDocumentLayoutAccess.hxx>
#include <rootfrm.hxx>
#include <pagefrm.hxx>
#include <tabfrm.hxx>
#include <sortedobjs.hxx>
#include <anchoredobject.hxx>
#include <flyfrm.hxx>
#include <flyfrms.hxx>
#include <docsh.hxx>
#include <wrtsh.hxx>

namespace
{
/// Covers sw/source/core/layout/tabfrm.cxx fixes.
class Test : public SwModelTestBase
{
public:
    Test()
        : SwModelTestBase(u"/sw/qa/core/layout/data/"_ustr)
    {
    }
};

CPPUNIT_TEST_FIXTURE(Test, testTablePrintAreaLeft)
{
    // Given a document with a header containing an image, and also with an overlapping table:
    createSwDoc("table-print-area-left.docx");
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // When laying out that document & parsing the left margin of the table:
    SwTwips nTablePrintLeft = getXPath(pXmlDoc, "//tab/infos/prtBounds", "left").toInt32();

    // Then make sure it has ~no left margin:
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 5
    // - Actual  : 10646
    // i.e. the table was shifted outside the page, was invisible.
    CPPUNIT_ASSERT_EQUAL(static_cast<SwTwips>(5), nTablePrintLeft);
}

CPPUNIT_TEST_FIXTURE(Test, testTableMissingJoin)
{
    // Given a document with a table on page 2:
    // When laying out that document:
    createSwDoc("table-missing-join.docx");

    // Then make sure that the table fits page 2:
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage1 = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage1);
    auto pPage2 = pPage1->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage2);
    SwFrame* pBody = pPage2->FindBodyCont();
    auto pTab = pBody->GetLower()->DynCastTabFrame();
    // Without the accompanying fix in place, this test would have failed, the table continued on
    // page 3.
    CPPUNIT_ASSERT(!pTab->HasFollow());
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyInInlineTable)
{
    // Outer inline table on pages 1 -> 2 -> 3, inner floating table on pages 2 -> 3:
    // When laying out that document:
    createSwDoc("floattable-in-inlinetable.docx");

    // Then make sure that the outer table is not missing on page 3:
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage1 = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage1);
    {
        SwFrame* pBody = pPage1->FindBodyCont();
        auto pTab = pBody->GetLower()->DynCastTabFrame();
        CPPUNIT_ASSERT(!pTab->GetPrecede());
        CPPUNIT_ASSERT(pTab->GetFollow());
    }
    auto pPage2 = pPage1->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage2);
    {
        SwFrame* pBody = pPage2->FindBodyCont();
        auto pTab = pBody->GetLower()->DynCastTabFrame();
        CPPUNIT_ASSERT(pTab->GetPrecede());
        // Without the accompanying fix in place, this test would have failed, the outer table was
        // missing on page 3.
        CPPUNIT_ASSERT(pTab->GetFollow());
    }
    auto pPage3 = pPage2->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage3);
    {
        SwFrame* pBody = pPage3->FindBodyCont();
        auto pTab = pBody->GetLower()->DynCastTabFrame();
        CPPUNIT_ASSERT(pTab->GetPrecede());
        CPPUNIT_ASSERT(!pTab->GetFollow());
    }
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyNestedRowSpan)
{
    // Given a document with nested floating tables and a row with rowspan cells at page boundary:
    // When loading that document:
    // Without the accompanying fix in place, this test would have resulted in a layout loop.
    createSwDoc("floattable-nested-rowspan.docx");

    // Then make sure the resulting page count matches Word:
    CPPUNIT_ASSERT_EQUAL(6, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyTableJoin)
{
    // Given a document with a multi-page floating table:
    // When loading this document:
    createSwDoc("floattable-table-join.docx");

    // Then make sure this document doesn't crash the layout and has a floating table split on 4
    // pages:
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage1 = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage1);
    CPPUNIT_ASSERT(pPage1->GetSortedObjs());
    {
        SwSortedObjs& rPageObjs = *pPage1->GetSortedObjs();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rPageObjs.size());
        auto pFly = rPageObjs[0]->DynCastFlyFrame()->DynCastFlyAtContentFrame();
        CPPUNIT_ASSERT(pFly);
        // Start of the chain.
        CPPUNIT_ASSERT(!pFly->GetPrecede());
        CPPUNIT_ASSERT(pFly->HasFollow());
    }
    auto pPage2 = pPage1->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage2);
    CPPUNIT_ASSERT(pPage2->GetSortedObjs());
    {
        SwSortedObjs& rPageObjs = *pPage2->GetSortedObjs();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rPageObjs.size());
        auto pFly = rPageObjs[0]->DynCastFlyFrame()->DynCastFlyAtContentFrame();
        CPPUNIT_ASSERT(pFly);
        CPPUNIT_ASSERT(pFly->GetPrecede());
        CPPUNIT_ASSERT(pFly->HasFollow());
    }
    auto pPage3 = pPage2->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage3);
    CPPUNIT_ASSERT(pPage3->GetSortedObjs());
    {
        SwSortedObjs& rPageObjs = *pPage3->GetSortedObjs();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rPageObjs.size());
        auto pFly = rPageObjs[0]->DynCastFlyFrame()->DynCastFlyAtContentFrame();
        CPPUNIT_ASSERT(pFly);
        CPPUNIT_ASSERT(pFly->GetPrecede());
        CPPUNIT_ASSERT(pFly->HasFollow());
    }
    auto pPage4 = pPage3->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage4);
    CPPUNIT_ASSERT(pPage4->GetSortedObjs());
    SwSortedObjs& rPageObjs = *pPage4->GetSortedObjs();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rPageObjs.size());
    auto pFly = rPageObjs[0]->DynCastFlyFrame()->DynCastFlyAtContentFrame();
    CPPUNIT_ASSERT(pFly);
    // End of the chain.
    CPPUNIT_ASSERT(pFly->GetPrecede());
    CPPUNIT_ASSERT(!pFly->HasFollow());
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyWrappedByTable)
{
    // Given a document with a floating table, wrapped by an inline table:
    // When laying out the document:
    createSwDoc("floattable-wrapped-by-table.docx");

    // Then make sure the inline table wraps around the floating table:
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage);
    // Get the top of the inline table, ignoring margins:
    CPPUNIT_ASSERT(pPage->GetSortedObjs());
    SwSortedObjs& rPageObjs = *pPage->GetSortedObjs();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rPageObjs.size());
    auto pFly = rPageObjs[0]->DynCastFlyFrame()->DynCastFlyAtContentFrame();
    CPPUNIT_ASSERT(pFly);
    // Get the bottom of the floating table, ignoring margins:
    SwTwips nFloatingBottom = pFly->getFrameArea().Top() + pFly->getFramePrintArea().Height();
    SwFrame* pBody = pPage->FindBodyCont();
    auto pTab = pBody->GetLower()->GetNext()->DynCastTabFrame();
    SwTwips nInlineTop = pTab->getFrameArea().Top() + pTab->getFramePrintArea().Top();
    // Make sure the inline table is on the right of the floating one, not below it:
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected less than: 7287
    // - Actual  : 7287
    // i.e. the inline table was under the floating one, not on the right of it.
    CPPUNIT_ASSERT_LESS(nFloatingBottom, nInlineTop);
}

CPPUNIT_TEST_FIXTURE(Test, testInlineTableThenSplitFly)
{
    // Given a document with a floating table ("right") and an inline table ("left"):
    // When laying out the document:
    createSwDoc("floattable-not-wrapped-by-table.docx");

    // Then make sure the inline table is on the left (small negative offset):
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage);
    SwFrame* pBody = pPage->FindBodyCont();
    auto pTab = pBody->GetLower()->GetNext()->DynCastTabFrame();
    SwTwips nInlineLeft = pTab->getFramePrintArea().Left();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected less than: 0
    // - Actual  : 6958
    // i.e. "left" was on the right, its horizontal margin was not a small negative value but a
    // large positive one.
    CPPUNIT_ASSERT_LESS(static_cast<SwTwips>(0), nInlineLeft);
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyWrappedByTableNested)
{
    // Given a document with 3 tables, one inline toplevel and two inner ones (one inline, one
    // floating):
    // When laying out that document:
    // Without the accompanying fix in place, this test would have failed here with a layout loop.
    createSwDoc("floattable-wrapped-by-table-nested.docx");

    // Than make sure we have 3 tables, but only one of them is floating:
    SwDoc* pDoc = getSwDoc();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), pDoc->GetTableFrameFormats()->GetFormatCount());
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), pDoc->GetSpzFrameFormats()->GetFormatCount());
}

CPPUNIT_TEST_FIXTURE(Test, testSplitFlyHeader)
{
    // Given a document with 8 pages: a first page ending in a manual page break, then a multi-page
    // floating table on pages 2..8:
    createSwDoc("floattable-header.docx");
    CPPUNIT_ASSERT_EQUAL(8, getPages());

    // When creating a new paragraph at doc start:
    SwDocShell* pDocShell = getSwDocShell();
    SwWrtShell* pWrtShell = pDocShell->GetWrtShell();
    pWrtShell->SttEndDoc(/*bStt=*/true);
    pWrtShell->SplitNode();
    // Without the accompanying fix in place, this test would have crashed here.
    calcLayout();

    // Then make sure we get one more page, since the first page is now 2 pages:
    CPPUNIT_ASSERT_EQUAL(9, getPages());
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

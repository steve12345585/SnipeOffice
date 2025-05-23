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
#include <docsh.hxx>
#include <formatflysplit.hxx>
#include <frmmgr.hxx>
#include <pagefrm.hxx>
#include <rootfrm.hxx>
#include <view.hxx>
#include <wrtsh.hxx>
#include <bodyfrm.hxx>
#include <sectfrm.hxx>

/// Covers sw/source/core/layout/ftnfrm.cxx fixes.
class Test : public SwModelTestBase
{
public:
    Test()
        : SwModelTestBase(u"/sw/qa/core/layout/data/"_ustr)
    {
    }
};

CPPUNIT_TEST_FIXTURE(Test, testFlySplitFootnoteLayout)
{
    // Given a document with a split fly (to host a table):
    createSwDoc();
    SwDoc* pDoc = getSwDoc();
    SwWrtShell* pWrtShell = getSwDocShell()->GetWrtShell();
    SwFlyFrameAttrMgr aMgr(true, pWrtShell, Frmmgr_Type::TEXT, nullptr);
    RndStdIds eAnchor = RndStdIds::FLY_AT_PARA;
    pWrtShell->StartAllAction();
    aMgr.InsertFlyFrame(eAnchor, aMgr.GetPos(), aMgr.GetSize());
    pWrtShell->EndAllAction();
    pWrtShell->StartAllAction();
    sw::FrameFormats<sw::SpzFrameFormat*>& rFlys = *pDoc->GetSpzFrameFormats();
    sw::SpzFrameFormat* pFly = rFlys[0];
    SwAttrSet aSet(pFly->GetAttrSet());
    aSet.Put(SwFormatFlySplit(true));
    pDoc->SetAttr(aSet, *pFly);
    pWrtShell->EndAllAction();
    pWrtShell->UnSelectFrame();
    pWrtShell->LeaveSelFrameMode();
    pWrtShell->GetView().AttrChangedNotify(nullptr);
    pWrtShell->MoveSection(GoCurrSection, fnSectionEnd);

    // When inserting a footnote:
    pWrtShell->InsertFootnote(OUString());

    // Then make sure the footnote frame and its container is created:
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage = dynamic_cast<SwPageFrame*>(pLayout->Lower());
    CPPUNIT_ASSERT(pPage);
    // Without the accompanying fix in place, this test would have failed, the footnote frame was
    // not created, the footnote reference was empty.
    CPPUNIT_ASSERT(pPage->FindFootnoteCont());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf158713_footnoteInHeadline)
{
    // Given a file with table-with-headline split across multiple pages,
    // and a footnote in the table's repeated heading row:
    createSwDoc("tdf158713_footnoteInHeadline.odt");

    // delete first paragraph, so table now fits all on the first page - no more "follow table"...
    dispatchCommand(mxComponent, u".uno:Delete"_ustr, {});

    // ensure the footnote text has not been removed from the layout
    xmlDocUniquePtr pLayout = parseLayoutDump();
    assertXPath(pLayout, "/root/page/ftncont/ftn", 1);
}

CPPUNIT_TEST_FIXTURE(Test, testInlineEndnoteAndFootnote)
{
    // Given a DOC file with an endnote and then a footnote:
    createSwDoc("inline-endnote-and-footnote.doc");

    // When laying out that document:
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Then make sure the footnote is below the endnote:
    // Without the accompanying fix in place, this test would have failed with:
    // - xpath should match exactly 1 node
    // i.e. the endnote was also in the footnote container, not at the end of the body text.
    sal_Int32 nEndnoteTop
        = getXPath(pXmlDoc, "/root/page/body/section/column/ftncont/ftn/infos/bounds", "top")
              .toInt32();
    sal_Int32 nFootnoteTop
        = getXPath(pXmlDoc, "/root/page/ftncont/ftn/infos/bounds", "top").toInt32();
    // Endnote at the end of body text, footnote at page bottom.
    CPPUNIT_ASSERT_LESS(nFootnoteTop, nEndnoteTop);
}

CPPUNIT_TEST_FIXTURE(Test, testInlineEndnoteAndSection)
{
    // Given a document ending with a section, ContinuousEndnotes is true:
    createSwDoc("inline-endnote-and-section.odt");

    // When laying out that document:
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Then make sure the endnote section is after the section at the end of the document, not
    // inside it:
    int nToplevelSections = countXPathNodes(pXmlDoc, "/root/page/body/section");
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 2
    // - Actual  : 1
    // and we even crashed on shutdown.
    CPPUNIT_ASSERT_EQUAL(2, nToplevelSections);
}

CPPUNIT_TEST_FIXTURE(Test, testInlineEndnotePosition)
{
    // Given a document, ContinuousEndnotes is true:
    createSwDoc("inline-endnote-position.docx");

    // When laying out that document:
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();

    // Then make sure the endnote separator (line + spacing around it) is large enough, so the
    // endnote text below the separator has the correct position:
    sal_Int32 nEndnoteContTopMargin
        = getXPath(pXmlDoc, "//column/ftncont/infos/prtBounds", "top").toInt32();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 269
    // - Actual  : 124
    // i.e. the top margin wasn't the default font size with its spacing, but the Writer default,
    // which shifted endnote text up, incorrectly.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(269), nEndnoteContTopMargin);
}

CPPUNIT_TEST_FIXTURE(Test, testInlineEndnoteSectionDelete)
{
    // Given a document, ContinuousEndnotes is true, 3 pages, endnodes start on page 2:
    // When laying out that document:
    createSwDoc("inline-endnote-section-delete.docx");

    // First page: just body text:
    SwDoc* pDoc = getSwDoc();
    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    auto pPage = pLayout->Lower()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage->GetLower()->IsBodyFrame());
    auto pBodyFrame = static_cast<SwBodyFrame*>(pPage->GetLower());
    CPPUNIT_ASSERT(!pBodyFrame->GetLastLower()->IsSctFrame());
    // Second page: ends with endnotes:
    pPage = pPage->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage->GetLower()->IsBodyFrame());
    pBodyFrame = static_cast<SwBodyFrame*>(pPage->GetLower());
    CPPUNIT_ASSERT(pBodyFrame->GetLastLower()->IsSctFrame());
    auto pSection = static_cast<SwSectionFrame*>(pBodyFrame->GetLastLower());
    CPPUNIT_ASSERT(pSection->IsEndNoteSection());
    // Third page: just endnotes:
    pPage = pPage->GetNext()->DynCastPageFrame();
    CPPUNIT_ASSERT(pPage->GetLower()->IsBodyFrame());
    pBodyFrame = static_cast<SwBodyFrame*>(pPage->GetLower());
    CPPUNIT_ASSERT(pBodyFrame->GetLower()->IsSctFrame());
    pSection = static_cast<SwSectionFrame*>(pBodyFrame->GetLower());
    CPPUNIT_ASSERT(pSection->IsEndNoteSection());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

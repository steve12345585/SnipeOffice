/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <memory>
#include <bookmark.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentLinksAdministration.hxx>
#include <IDocumentState.hxx>
#include <doc.hxx>
#include <ndtxt.hxx>
#include <pam.hxx>
#include <swserv.hxx>
#include <sfx2/linkmgr.hxx>
#include <sfx2/viewsh.hxx>
#include <UndoBookmark.hxx>
#include <unobookmark.hxx>
#include <utility>
#include <xmloff/odffields.hxx>
#include <libxml/xmlwriter.h>
#include <comphelper/random.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/anytostring.hxx>
#include <sal/log.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <edtwin.hxx>
#include <DateFormFieldButton.hxx>
#include <DropDownFormFieldButton.hxx>
#include <DocumentContentOperationsManager.hxx>
#include <comphelper/lok.hxx>
#include <txtfrm.hxx>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>
#include <rtl/strbuf.hxx>
#include <strings.hrc>
#include <tools/json_writer.hxx>

using namespace ::sw::mark;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace sw::mark
{

    SwPosition FindFieldSep(Fieldmark const& rMark)
    {
        auto [/*const SwPosition&*/ rStartPos, rEndPos] = rMark.GetMarkStartEnd();
        SwNodes const& rNodes(rStartPos.GetNodes());
        SwNodeOffset const nStartNode(rStartPos.GetNodeIndex());
        SwNodeOffset const nEndNode(rEndPos.GetNodeIndex());
        int nFields(0);
        std::optional<SwPosition> ret;
        for (SwNodeOffset n = nEndNode; nStartNode <= n; --n)
        {
            SwNode *const pNode(rNodes[n]);
            if (pNode->IsTextNode())
            {
                SwTextNode & rTextNode(*pNode->GetTextNode());
                sal_Int32 const nStart(n == nStartNode
                        ? rStartPos.GetContentIndex() + 1
                        : 0);
                sal_Int32 const nEnd(n == nEndNode
                        // subtract 1 to ignore the end char
                        ? rEndPos.GetContentIndex() - 1
                        : rTextNode.Len());
                for (sal_Int32 i = nEnd; nStart < i; --i)
                {
                    const sal_Unicode c(rTextNode.GetText()[i - 1]);
                    switch (c)
                    {
                        case CH_TXT_ATR_FIELDSTART:
                            --nFields;
                            assert(0 <= nFields);
                            break;
                        case CH_TXT_ATR_FIELDEND:
                            ++nFields;
                            // fields in field result could happen by manual
                            // editing, although the field update deletes them
                            break;
                        case CH_TXT_ATR_FIELDSEP:
                            if (nFields == 0)
                            {
                                assert(!ret); // one per field
                                ret.emplace(rTextNode, i - 1);
#ifndef DBG_UTIL
                                return *ret;
#endif
                            }
                            break;
                    }
                }
            }
            else if (pNode->IsEndNode() && !pNode->StartOfSectionNode()->IsSectionNode())
            {
                assert(nStartNode <= pNode->StartOfSectionIndex());
                // fieldmark cannot overlap node section, unless it's a section
                n = pNode->StartOfSectionIndex();
            }
            else
            {
                assert(pNode->IsNoTextNode() || pNode->IsSectionNode()
                    || (pNode->IsEndNode() && pNode->StartOfSectionNode()->IsSectionNode()));
            }
        }
        assert(ret); // must have found it
        return *ret;
    }
} // namespace sw::mark

namespace
{
    void lcl_FixPosition(SwPosition& rPos)
    {
        // make sure the position has 1) the proper node, and 2) a proper index
        SwTextNode* pTextNode = rPos.GetNode().GetTextNode();
        if(pTextNode == nullptr && rPos.GetContentIndex() > 0)
        {
            SAL_INFO(
                "sw.core",
                "illegal position: " << rPos.GetContentIndex()
                    << " without proper TextNode");
            rPos.nContent.Assign(nullptr, 0);
        }
        else if(pTextNode != nullptr && rPos.GetContentIndex() > pTextNode->Len())
        {
            SAL_INFO(
                "sw.core",
                "illegal position: " << rPos.GetContentIndex()
                    << " is beyond " << pTextNode->Len());
            rPos.nContent.Assign(pTextNode, pTextNode->Len());
        }
    }

    void lcl_AssertFieldMarksSet(const Fieldmark& rField,
        const sal_Unicode aStartMark,
        const sal_Unicode aEndMark)
    {
        if (aEndMark != CH_TXT_ATR_FORMELEMENT)
        {
            SwPosition const& rStart(rField.GetMarkStart());
            assert(rStart.GetNode().GetTextNode()->GetText()[rStart.GetContentIndex()] == aStartMark); (void) rStart; (void) aStartMark;
            SwPosition const sepPos(sw::mark::FindFieldSep(rField));
            assert(sepPos.GetNode().GetTextNode()->GetText()[sepPos.GetContentIndex()] == CH_TXT_ATR_FIELDSEP); (void) sepPos;
        }
        else
        {   // must be m_pPos1 < m_pPos2 because of asymmetric SplitNode update
            assert(rField.GetMarkPos().GetContentIndex() + 1 == rField.GetOtherMarkPos().GetContentIndex());
        }
        SwPosition const& rEnd(rField.GetMarkEnd());
        assert(rEnd.GetNode().GetTextNode()->GetText()[rEnd.GetContentIndex() - 1] == aEndMark); (void) rEnd;
    }

    void lcl_SetFieldMarks(Fieldmark& rField,
        SwDoc& io_rDoc,
        const sal_Unicode aStartMark,
        const sal_Unicode aEndMark,
        SwPosition const*const pSepPos)
    {
        io_rDoc.GetIDocumentUndoRedo().StartUndo(SwUndoId::UI_REPLACE, nullptr);
        OUString startChar(aStartMark);
        if (aEndMark != CH_TXT_ATR_FORMELEMENT
            && rField.GetMarkStart() == rField.GetMarkEnd())
        {
            // do only 1 InsertString call - to expand existing bookmarks at the
            // position over the whole field instead of just aStartMark
            startChar += OUStringChar(CH_TXT_ATR_FIELDSEP) + OUStringChar(aEndMark);
        }

        SwPosition start = rField.GetMarkStart();
        if (aEndMark != CH_TXT_ATR_FORMELEMENT)
        {
            SwPaM aStartPaM(start);
            io_rDoc.getIDocumentContentOperations().InsertString(aStartPaM, startChar);
            start.AdjustContent( -startChar.getLength() ); // restore, it was moved by InsertString
            // do not manipulate via reference directly but call SetMarkStartPos
            // which works even if start and end pos were the same
            rField.SetMarkStartPos( start );
            SwPosition& rEnd = rField.GetMarkEnd(); // note: retrieve after
            // setting start, because if start==end it can go stale, see SetMarkPos()
            assert(pSepPos == nullptr || (start < *pSepPos && *pSepPos <= rEnd));
            if (startChar.getLength() == 1)
            {
                *aStartPaM.GetPoint() = pSepPos ? *pSepPos : rEnd;
                io_rDoc.getIDocumentContentOperations().InsertString(aStartPaM, OUString(CH_TXT_ATR_FIELDSEP));
                if (!pSepPos || rEnd < *pSepPos)
                {   // rEnd is not moved automatically if it's same as insert pos
                    rEnd.AdjustContent(1);
                }
            }
            assert(pSepPos == nullptr || (start < *pSepPos && *pSepPos <= rEnd));
        }
        else
        {
            assert(pSepPos == nullptr);
        }

        SwPosition& rEnd = rField.GetMarkEnd();
        if (aEndMark && startChar.getLength() == 1)
        {
            SwPaM aEndPaM(rEnd);
            io_rDoc.getIDocumentContentOperations().InsertString(aEndPaM, OUString(aEndMark));
            if (aEndMark != CH_TXT_ATR_FORMELEMENT)
            {
                rEnd.AdjustContent(1); // InsertString didn't move non-empty mark
            }
            else
            {   // InsertString moved the mark's end, not its start
                assert(rField.GetMarkPos().GetContentIndex() + 1 == rField.GetOtherMarkPos().GetContentIndex());
            }
        }
        lcl_AssertFieldMarksSet(rField, aStartMark, aEndMark);

        io_rDoc.GetIDocumentUndoRedo().EndUndo(SwUndoId::UI_REPLACE, nullptr);
    }

    void lcl_RemoveFieldMarks(const Fieldmark& rField,
        SwDoc& io_rDoc,
        const sal_Unicode aStartMark,
        const sal_Unicode aEndMark)
    {
        io_rDoc.GetIDocumentUndoRedo().StartUndo(SwUndoId::UI_REPLACE, nullptr);

        const SwPosition& rStart = rField.GetMarkStart();
        SwTextNode const*const pStartTextNode = rStart.GetNode().GetTextNode();
        assert(pStartTextNode);
        if (aEndMark != CH_TXT_ATR_FORMELEMENT)
        {
            (void) pStartTextNode;
            // check this before start / end because of the +1 / -1 ...
            SwPosition const sepPos(sw::mark::FindFieldSep(rField));
            io_rDoc.GetDocumentContentOperationsManager().DeleteDummyChar(rStart, aStartMark);
            io_rDoc.GetDocumentContentOperationsManager().DeleteDummyChar(sepPos, CH_TXT_ATR_FIELDSEP);
        }

        const SwPosition& rEnd = rField.GetMarkEnd();
        SwTextNode *const pEndTextNode = rEnd.GetNode().GetTextNode();
        assert(pEndTextNode);
        const sal_Int32 nEndPos = (rEnd == rStart)
                                   ? rEnd.GetContentIndex()
                                   : rEnd.GetContentIndex() - 1;
        assert(pEndTextNode->GetText()[nEndPos] == aEndMark);
        SwPosition const aEnd(*pEndTextNode, nEndPos);
        io_rDoc.GetDocumentContentOperationsManager().DeleteDummyChar(aEnd, aEndMark);

        io_rDoc.GetIDocumentUndoRedo().EndUndo(SwUndoId::UI_REPLACE, nullptr);
    }

    auto InvalidatePosition(SwPosition const& rPos) -> void
    {
        SwUpdateAttr const aHint(rPos.GetContentIndex(), rPos.GetContentIndex(), 0);
        rPos.GetNode().GetTextNode()->CallSwClientNotify(sw::UpdateAttrHint(&aHint, &aHint));
    }
}

namespace sw::mark
{
    MarkBase::MarkBase(const SwPaM& aPaM,
        ReferenceMarkerName aName)
        : m_oPos1(*aPaM.GetPoint())
        , m_aName(std::move(aName))
    {
        m_oPos1->SetOwner(this);
        lcl_FixPosition(*m_oPos1);
        if (aPaM.HasMark() && (*aPaM.GetMark() != *aPaM.GetPoint()))
        {
            MarkBase::SetOtherMarkPos(*(aPaM.GetMark()));
            lcl_FixPosition(*m_oPos2);
        }
    }

    void MarkBase::SetXBookmark(rtl::Reference<SwXBookmark> const& xBkmk)
    { m_wXBookmark = xBkmk.get(); }

    // For fieldmarks, the CH_TXT_ATR_FIELDSTART and CH_TXT_ATR_FIELDEND
    // themselves are part of the covered range. This is guaranteed by
    // TextFieldmark::InitDoc/lcl_AssureFieldMarksSet.
    bool MarkBase::IsCoveringPosition(const SwPosition& rPos) const
    {
        auto [/*const SwPosition&*/ rStartPos, rEndPos] = GetMarkStartEnd();
        return rStartPos <= rPos && rPos < rEndPos;
    }

    void MarkBase::SetMarkPos(const SwPosition& rNewPos)
    {
        m_oPos1.emplace(rNewPos);
        m_oPos1->SetOwner(this);
    }

    void MarkBase::SetOtherMarkPos(const SwPosition& rNewPos)
    {
        m_oPos2.emplace(rNewPos);
        m_oPos2->SetOwner(this);
    }

    OUString MarkBase::ToString( ) const
    {
        return "Mark: ( Name, [ Node1, Index1 ] ): ( " + m_aName.toString() + ", [ "
            + OUString::number( sal_Int32(GetMarkPos().GetNodeIndex()) )  + ", "
            + OUString::number( GetMarkPos().GetContentIndex( ) ) + " ] )";
    }

    void MarkBase::dumpAsXml(xmlTextWriterPtr pWriter) const
    {
        (void)xmlTextWriterStartElement(pWriter, BAD_CAST("MarkBase"));
        (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("name"), BAD_CAST(m_aName.toString().toUtf8().getStr()));
        (void)xmlTextWriterStartElement(pWriter, BAD_CAST("markPos"));
        GetMarkPos().dumpAsXml(pWriter);
        (void)xmlTextWriterEndElement(pWriter);
        if (IsExpanded())
        {
            (void)xmlTextWriterStartElement(pWriter, BAD_CAST("otherMarkPos"));
            GetOtherMarkPos().dumpAsXml(pWriter);
            (void)xmlTextWriterEndElement(pWriter);
        }
        (void)xmlTextWriterEndElement(pWriter);
    }

    MarkBase::~MarkBase()
    { }

    ReferenceMarkerName MarkBase::GenerateNewName(std::u16string_view rPrefix)
    {
        static bool bHack = (getenv("LIBO_ONEWAY_STABLE_ODF_EXPORT") != nullptr);

        if (bHack)
        {
            static sal_Int64 nIdCounter = SAL_CONST_INT64(6000000000);
            return ReferenceMarkerName(rPrefix + OUString::number(nIdCounter++));
        }
        else
        {
            static OUString sUniquePostfix;
            static sal_Int32 nCount = SAL_MAX_INT32;
            if(nCount == SAL_MAX_INT32)
            {
                unsigned int const n(comphelper::rng::uniform_uint_distribution(0,
                                    std::numeric_limits<unsigned int>::max()));
                sUniquePostfix = "_" + OUString::number(n);
                nCount = 0;
            }
            // putting the counter in front of the random parts will speed up string comparisons
            return ReferenceMarkerName(rPrefix + OUString::number(nCount++) + sUniquePostfix);
        }
    }

    void MarkBase::SwClientNotify(const SwModify&, const SfxHint& rHint)
    {
        CallSwClientNotify(rHint);
        if(SfxHintId::SwRemoveUnoObject == rHint.GetId())
           // invalidate cached uno object
            SetXBookmark(nullptr);
    }

    auto MarkBase::InvalidateFrames() -> void
    {
    }

    NavigatorReminder::NavigatorReminder(const SwPaM& rPaM)
        : MarkBase(rPaM, MarkBase::GenerateNewName(u"__NavigatorReminder__"))
    { }

    UnoMark::UnoMark(const SwPaM& aPaM)
        : MarkBase(aPaM, MarkBase::GenerateNewName(u"__UnoMark__"))
    { }

    DdeBookmark::DdeBookmark(const SwPaM& aPaM)
        : MarkBase(aPaM, MarkBase::GenerateNewName(u"__DdeLink__"))
    { }

    void DdeBookmark::SetRefObject(SwServerObject* pObj)
    {
        m_aRefObj = pObj;
    }

    void DdeBookmark::DeregisterFromDoc(SwDoc& rDoc)
    {
        if(m_aRefObj.is())
            rDoc.getIDocumentLinksAdministration().GetLinkManager().RemoveServer(m_aRefObj.get());
    }

    DdeBookmark::~DdeBookmark()
    {
        if( m_aRefObj.is() )
        {
            if(m_aRefObj->HasDataLinks())
            {
                ::sfx2::SvLinkSource* p = m_aRefObj.get();
                p->SendDataChanged();
            }
            m_aRefObj->SetNoServer();
        }
    }

    Bookmark::Bookmark(const SwPaM& aPaM,
        const vcl::KeyCode& rCode,
        const ReferenceMarkerName& rName)
        : DdeBookmark(aPaM)
        , m_aCode(rCode)
        , m_bHidden(false)
    {
        m_aName = rName;
    }

    void Bookmark::sendLOKDeleteCallback()
    {
        if (!comphelper::LibreOfficeKit::isActive() || GetMarkPos().GetDoc().IsClipBoard())
            return;

        SfxViewShell* pViewShell = SfxViewShell::Current();
        if (!pViewShell)
            return;

        ReferenceMarkerName fieldCommand = GetName();
        tools::JsonWriter aJson;
        aJson.put("commandName", ".uno:DeleteBookmark");
        aJson.put("success", true);
        {
            auto result = aJson.startNode("result");
            aJson.put("DeleteBookmark", fieldCommand.toString());
        }

        pViewShell->libreOfficeKitViewCallback(LOK_CALLBACK_UNO_COMMAND_RESULT, aJson.finishAndGetAsOString());
    }

    void Bookmark::InitDoc(SwDoc& io_rDoc,
            sw::mark::InsertMode const, SwPosition const*const)
    {
        if (io_rDoc.GetIDocumentUndoRedo().DoesUndo())
        {
            io_rDoc.GetIDocumentUndoRedo().AppendUndo(
                    std::make_unique<SwUndoInsBookmark>(*this));
        }
        io_rDoc.getIDocumentState().SetModified();
        InvalidateFrames();
    }

    void Bookmark::DeregisterFromDoc(SwDoc& io_rDoc)
    {
        DdeBookmark::DeregisterFromDoc(io_rDoc);

        if (io_rDoc.GetIDocumentUndoRedo().DoesUndo())
        {
            io_rDoc.GetIDocumentUndoRedo().AppendUndo(
                    std::make_unique<SwUndoDeleteBookmark>(*this));
        }
        io_rDoc.getIDocumentState().SetModified();
        InvalidateFrames();
    }

    // invalidate text frames in case it's hidden or Formatting Marks enabled
    auto Bookmark::InvalidateFrames() -> void
    {
        InvalidatePosition(GetMarkPos());
        if (IsExpanded())
        {
            InvalidatePosition(GetOtherMarkPos());
        }
    }

    void Bookmark::Hide(bool const isHide)
    {
        if (isHide != m_bHidden)
        {
            m_bHidden = isHide;
            InvalidateFrames();
        }
    }

    void Bookmark::SetHideCondition(OUString const& rHideCondition)
    {
        if (m_sHideCondition != rHideCondition)
        {
            m_sHideCondition = rHideCondition;
            // don't eval condition here yet - probably only needed for
            // UI editing condition and that doesn't exist yet
        }
    }

    ::sfx2::IXmlIdRegistry& Bookmark::GetRegistry()
    {
        SwDoc& rDoc( GetMarkPos().GetDoc() );
        return rDoc.GetXmlIdRegistry();
    }

    bool Bookmark::IsInClipboard() const
    {
        SwDoc& rDoc( GetMarkPos().GetDoc() );
        return rDoc.IsClipBoard();
    }

    bool Bookmark::IsInUndo() const
    {
        return false;
    }

    bool Bookmark::IsInContent() const
    {
        SwDoc& rDoc( GetMarkPos().GetDoc() );
        return !rDoc.IsInHeaderFooter( GetMarkPos().GetNode() );
    }

    uno::Reference< rdf::XMetadatable > Bookmark::MakeUnoObject()
    {
        SwDoc& rDoc( GetMarkPos().GetDoc() );
        const rtl::Reference< SwXBookmark> xMeta(
                SwXBookmark::CreateXBookmark(rDoc, this) );
        return xMeta;
    }

    Fieldmark::Fieldmark(const SwPaM& rPaM)
        : MarkBase(rPaM, MarkBase::GenerateNewName(u"__Fieldmark__"))
    {
        if(!IsExpanded())
            SetOtherMarkPos(GetMarkPos());
    }

    void Fieldmark::SetMarkStartPos( const SwPosition& rNewStartPos )
    {
        if ( GetMarkPos( ) <= GetOtherMarkPos( ) )
            return SetMarkPos( rNewStartPos );
        else
            return SetOtherMarkPos( rNewStartPos );
    }

    OUString Fieldmark::ToString( ) const
    {
        return "Fieldmark: ( Name, Type, [ Nd1, Id1 ], [ Nd2, Id2 ] ): ( " + m_aName.toString() + ", "
            + m_aFieldname + ", [ " + OUString::number( sal_Int32(GetMarkPos().GetNodeIndex( )) )
            + ", " + OUString::number( GetMarkPos( ).GetContentIndex( ) ) + " ], ["
            + OUString::number( sal_Int32(GetOtherMarkPos().GetNodeIndex( )) ) + ", "
            + OUString::number( GetOtherMarkPos( ).GetContentIndex( ) ) + " ] ) ";
    }

    void Fieldmark::Invalidate( )
    {
        // TODO: Does exist a better solution to trigger a format of the
        //       fieldmark portion? If yes, please use it.
        SwPaM aPaM( GetMarkPos(), GetOtherMarkPos() );
        aPaM.InvalidatePaM();
    }

    void Fieldmark::dumpAsXml(xmlTextWriterPtr pWriter) const
    {
        (void)xmlTextWriterStartElement(pWriter, BAD_CAST("Fieldmark"));
        (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("fieldname"), BAD_CAST(m_aFieldname.toUtf8().getStr()));
        (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("fieldHelptext"), BAD_CAST(m_aFieldHelptext.toUtf8().getStr()));
        MarkBase::dumpAsXml(pWriter);
        (void)xmlTextWriterStartElement(pWriter, BAD_CAST("parameters"));
        for (auto& rParam : m_vParams)
        {
            (void)xmlTextWriterStartElement(pWriter, BAD_CAST("parameter"));
            (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("name"), BAD_CAST(rParam.first.toUtf8().getStr()));
            (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("value"), BAD_CAST(comphelper::anyToString(rParam.second).toUtf8().getStr()));
            (void)xmlTextWriterEndElement(pWriter);
        }
        (void)xmlTextWriterEndElement(pWriter);
        (void)xmlTextWriterEndElement(pWriter);
    }

    TextFieldmark::TextFieldmark(const SwPaM& rPaM, const ReferenceMarkerName& rName)
        : Fieldmark(rPaM)
        , m_pDocumentContentOperationsManager(nullptr)
    {
        if ( !rName.isEmpty() )
            m_aName = rName;
    }

    TextFieldmark::~TextFieldmark()
    {
        if (!comphelper::LibreOfficeKit::isActive() || GetMarkPos().GetDoc().IsClipBoard())
            return;

        SfxViewShell* pViewShell = SfxViewShell::Current();
        if (!pViewShell)
            return;

        OUString fieldCommand;
        (*GetParameters())[ODF_CODE_PARAM] >>= fieldCommand;
        tools::JsonWriter aJson;
        aJson.put("commandName", ".uno:DeleteTextFormField");
        aJson.put("success", true);
        {
            auto result = aJson.startNode("result");
            aJson.put("DeleteTextFormField", fieldCommand);
        }

        pViewShell->libreOfficeKitViewCallback(LOK_CALLBACK_UNO_COMMAND_RESULT, aJson.finishAndGetAsOString());
    }

    void TextFieldmark::InitDoc(SwDoc& io_rDoc,
            sw::mark::InsertMode const eMode, SwPosition const*const pSepPos)
    {
        m_pDocumentContentOperationsManager = &io_rDoc.GetDocumentContentOperationsManager();
        if (eMode == sw::mark::InsertMode::New)
        {
            lcl_SetFieldMarks(*this, io_rDoc, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND, pSepPos);
        }
        else
        {
            lcl_AssertFieldMarksSet(*this, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND);
        }
    }

    void TextFieldmark::ReleaseDoc(SwDoc& rDoc)
    {
        IDocumentUndoRedo & rIDUR(rDoc.GetIDocumentUndoRedo());
        if (rIDUR.DoesUndo())
        {
            rIDUR.AppendUndo(std::make_unique<SwUndoDelTextFieldmark>(*this));
        }
        ::sw::UndoGuard const ug(rIDUR); // prevent SwUndoDeletes
        lcl_RemoveFieldMarks(*this, rDoc, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND);
        // notify layouts to unhide - for the entire fieldmark, as in InitDoc()
        SwPaM const tmp(GetMarkPos(), GetOtherMarkPos());
        sw::UpdateFramesForRemoveDeleteRedline(rDoc, tmp);
    }

    OUString TextFieldmark::GetContent() const
    {
        const SwTextNode& rTextNode = *GetMarkEnd().GetNode().GetTextNode();
        SwPosition const sepPos(sw::mark::FindFieldSep(*this));
        const sal_Int32 nStart(sepPos.GetContentIndex());
        const sal_Int32 nEnd(GetMarkEnd().GetContentIndex());

        OUString sContent;
        const sal_Int32 nLen = rTextNode.GetText().getLength();
        if (nStart + 1 < nLen && nEnd <= nLen && nEnd > nStart + 2)
            sContent = rTextNode.GetText().copy(nStart + 1, nEnd - nStart - 2);

        return sContent;
    }

    void TextFieldmark::ReplaceContent(const OUString& sNewContent)
    {
        if (!m_pDocumentContentOperationsManager)
            return;

        SwPosition const sepPos(sw::mark::FindFieldSep(*this));
        const sal_Int32 nStart(sepPos.GetContentIndex());
        const sal_Int32 nEnd(GetMarkEnd().GetContentIndex());

        const sal_Int32 nLen = GetMarkEnd().GetNode().GetTextNode()->GetText().getLength();
        if (nStart + 1 < nLen && nEnd <= nLen && nEnd > nStart + 2)
        {
            SwPaM aFieldPam(GetMarkStart().GetNode(), nStart + 1,
                            GetMarkStart().GetNode(), nEnd - 1);
            m_pDocumentContentOperationsManager->ReplaceRange(aFieldPam, sNewContent, false);
        }
        else
        {
            SwPaM aFieldStartPam(GetMarkStart().GetNode(), nStart + 1);
            m_pDocumentContentOperationsManager->InsertString(aFieldStartPam, sNewContent);
        }
        Invalidate();
    }
    bool TextFieldmark::HasDefaultContent() const
    {
        return GetContent() == vEnSpaces;
    }


    NonTextFieldmark::NonTextFieldmark(const SwPaM& rPaM)
        : Fieldmark(rPaM)
    { }

    void NonTextFieldmark::InitDoc(SwDoc& io_rDoc,
            sw::mark::InsertMode const eMode, SwPosition const*const pSepPos)
    {
        assert(pSepPos == nullptr);
        if (eMode == sw::mark::InsertMode::New)
        {
            lcl_SetFieldMarks(*this, io_rDoc, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FORMELEMENT, pSepPos);
        }
        else
        {
            lcl_AssertFieldMarksSet(*this, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FORMELEMENT);
        }
    }

    void NonTextFieldmark::ReleaseDoc(SwDoc& rDoc)
    {
        IDocumentUndoRedo & rIDUR(rDoc.GetIDocumentUndoRedo());
        if (rIDUR.DoesUndo())
        {
            rIDUR.AppendUndo(std::make_unique<SwUndoDelNoTextFieldmark>(*this));
        }
        ::sw::UndoGuard const ug(rIDUR); // prevent SwUndoDeletes
        lcl_RemoveFieldMarks(*this, rDoc,
                CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FORMELEMENT);
    }


    CheckboxFieldmark::CheckboxFieldmark(const SwPaM& rPaM, const ReferenceMarkerName& rName)
        : NonTextFieldmark(rPaM)
    {
        if (!rName.isEmpty())
            m_aName = rName;
    }

    void CheckboxFieldmark::SetChecked(bool checked)
    {
        if ( IsChecked() != checked )
        {
            (*GetParameters())[ODF_FORMCHECKBOX_RESULT] <<= checked;
            // mark document as modified
            SwDoc& rDoc( GetMarkPos().GetDoc() );
            rDoc.getIDocumentState().SetModified();
        }
    }

    bool CheckboxFieldmark::IsChecked() const
    {
        bool bResult = false;
        parameter_map_t::const_iterator pResult = GetParameters()->find(ODF_FORMCHECKBOX_RESULT);
        if(pResult != GetParameters()->end())
            pResult->second >>= bResult;
        return bResult;
    }

    OUString CheckboxFieldmark::GetContent() const
    {
        return IsChecked() ? "1" : "0";
    }

    void CheckboxFieldmark::ReplaceContent(const OUString& sNewContent)
    {
        SetChecked(sNewContent.toBoolean());
        Invalidate();
    }

    FieldmarkWithDropDownButton::FieldmarkWithDropDownButton(const SwPaM& rPaM)
        : NonTextFieldmark(rPaM)
        , m_pButton(nullptr)
    {
    }

    FieldmarkWithDropDownButton::~FieldmarkWithDropDownButton()
    {
        m_pButton.disposeAndClear();
    }

    void FieldmarkWithDropDownButton::RemoveButton()
    {
        if(m_pButton)
            m_pButton.disposeAndClear();
    }

    void FieldmarkWithDropDownButton::LaunchPopup()
    {
        if (!m_pButton)
            return;

        m_pButton->Invalidate();
        m_pButton->LaunchPopup();
    }

    DropDownFieldmark::DropDownFieldmark(const SwPaM& rPaM, const ReferenceMarkerName& rName)
        : FieldmarkWithDropDownButton(rPaM)
    {
        if (!rName.isEmpty())
            m_aName = rName;
    }

    DropDownFieldmark::~DropDownFieldmark()
    {
    }

    void DropDownFieldmark::ShowButton(SwEditWin* pEditWin)
    {
        if(pEditWin)
        {
            if(!m_pButton)
                m_pButton = VclPtr<DropDownFormFieldButton>::Create(pEditWin, *this);
            m_pButton->CalcPosAndSize(m_aPortionPaintArea);
            m_pButton->Show();
        }
    }

    void DropDownFieldmark::RemoveButton()
    {
        FieldmarkWithDropDownButton::RemoveButton();
    }

    /** GetContent
     *  @param pIndex The zero-based index to retrieve
     *                [in] if pIndex is null or negative, return the listbox's chosen result,
     *                     else return the indicated entry (or last entry for invalid choice).
     *                [out] the index of the returned result or -1 if error
     */
    OUString DropDownFieldmark::GetContent(sal_Int32* pIndex) const
    {
        sal_Int32 nIndex = pIndex ? *pIndex : -1;
        auto rParameters = *GetParameters();
        if (nIndex < 0)
            rParameters[ODF_FORMDROPDOWN_RESULT] >>= nIndex;

        uno::Sequence<OUString> aSeq;
        rParameters[ODF_FORMDROPDOWN_LISTENTRY] >>= aSeq;
        nIndex = std::min(nIndex, aSeq.getLength() - 1);

        if (nIndex < 0)
        {
            if (pIndex)
                *pIndex = -1;
            return OUString();
        }

        if (pIndex)
            *pIndex = nIndex;

        return aSeq[nIndex];
    }

    OUString DropDownFieldmark::GetContent() const
    {
        return GetContent(nullptr);
    }

    /** AddContent : INSERTS a new choice
     *  @param rText: The choice to add to the list choices.
     *
     *  @param pIndex [optional]
     *                [in] If pIndex is null or invalid, append to the end of the list.
     *                [out] Modified to point to the position of the choice if it already exists.
     */
    void DropDownFieldmark::AddContent(const OUString& rText, sal_Int32* pIndex)
    {
        uno::Sequence<OUString> aSeq;
        sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] >>= aSeq;

        // no duplicates: if it already exists, modify the given index to point to it
        const sal_Int32 nCurrentTextPos = comphelper::findValue(aSeq, rText);
        if (nCurrentTextPos != -1)
        {
            if (pIndex)
                *pIndex = nCurrentTextPos;
            return;
        }

        const sal_Int32 nLen = aSeq.getLength();
        const sal_Int32 nNewPos = pIndex && *pIndex > -1 ? std::min(*pIndex, nLen) : nLen;

        // need to shift list result index up if adding new entry before it
        sal_Int32 nResultIndex = -1;
        (*pParameters)[ODF_FORMDROPDOWN_RESULT] >>= nResultIndex;
        if (nNewPos <= nResultIndex)
            (*pParameters)[ODF_FORMDROPDOWN_RESULT] <<= nResultIndex + 1;

        auto aList = comphelper::sequenceToContainer<std::vector<OUString>>(aSeq);
        if (nNewPos < nLen)
            aList.insert(aList.begin() + nNewPos, rText);
        else
        {
            if (pIndex)
                *pIndex = nLen;
            aList.push_back(rText);
        }

        (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] <<= comphelper::containerToSequence(aList);
        Invalidate();
    }

    /**
     * ReplaceContent : changes the list result index or renames the existing choices
     * @param pText
     *               [in] If pIndex is null, change the list result index to this provided choice
     *                       (but do nothing if pText is an invalid choice)
     *                    else rename that entry.
     *
     * @param pIndex
     *               [in] If pText is null, change the list result index to this provided Index
     *                        (or the last position if it is an invalid choice)
     *                    else rename this entry (doing nothing for invalid indexes).
     *               [out] If pIndex is invalid, it is modified to use the last position.
     *
     * This function allows duplicate entries - which is also allowed in MS Word.
     */
    void DropDownFieldmark::ReplaceContent(const OUString* pText, sal_Int32* pIndex)
    {
        if (!pIndex && !pText)
            return;

        uno::Sequence<OUString> aSeq;
        sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] >>= aSeq;
        const sal_Int32 nLen = aSeq.getLength();

        if (!pText)
        {
            if (*pIndex < 0 || *pIndex >= nLen)
                *pIndex = nLen - 1;

            // select pIndex as the new value for the list box
            (*pParameters)[ODF_FORMDROPDOWN_RESULT] <<= *pIndex;
            Invalidate();
            return;
        }

        if (!pIndex)
        {
            const sal_Int32 nNewPos = comphelper::findValue(aSeq, *pText);
            if (nNewPos != -1)
            {
                (*pParameters)[ODF_FORMDROPDOWN_RESULT] <<= nNewPos;
                Invalidate();
            }
            return;
        }

        if (*pIndex > -1 && *pIndex < nLen)
        {
            auto aList = comphelper::sequenceToContainer<std::vector<OUString>>(aSeq);
            aList[*pIndex] = *pText;
            (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] <<= comphelper::containerToSequence(aList);
            Invalidate();
        }
    }

    void DropDownFieldmark::ReplaceContent(const OUString& rNewContent)
    {
        ReplaceContent(&rNewContent, nullptr);
    }

    /**
     * Remove everything if the given index is negative, else remove the given index (if valid).
     * If deleting the currently selected choice, reset the selection to the first choice.
     */
    void DropDownFieldmark::DelContent(sal_Int32 nDelIndex)
    {
        sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        uno::Sequence<OUString> aSeq;
        if (nDelIndex < 0)
        {
            pParameters->erase(ODF_FORMDROPDOWN_RESULT);
            (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] <<= aSeq;
            Invalidate();
            return;
        }

        (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] >>= aSeq;
        if (nDelIndex >= aSeq.getLength())
            return;

        // If deleting the current choice, select the first entry instead
        // else need to shift list result index down if deleting an entry before it
        sal_Int32 nResultIndex = -1;
        (*pParameters)[ODF_FORMDROPDOWN_RESULT] >>= nResultIndex;
        if (nDelIndex == nResultIndex)
            nResultIndex = 0;
        else if (nDelIndex < nResultIndex)
            --nResultIndex;

        comphelper::removeElementAt(aSeq, nDelIndex);
        if (nResultIndex != -1)
            (*pParameters)[ODF_FORMDROPDOWN_RESULT] <<= nResultIndex;
        (*pParameters)[ODF_FORMDROPDOWN_LISTENTRY] <<= aSeq;
        Invalidate();
    }

    void DropDownFieldmark::SetPortionPaintArea(const SwRect& rPortionPaintArea)
    {
        m_aPortionPaintArea = rPortionPaintArea;
        if(m_pButton)
        {
            m_pButton->Show();
            m_pButton->CalcPosAndSize(m_aPortionPaintArea);
        }
    }

    void DropDownFieldmark::SendLOKShowMessage(const SfxViewShell* pViewShell)
    {
        if (!comphelper::LibreOfficeKit::isActive())
            return;

        if (!pViewShell || pViewShell->isLOKMobilePhone())
            return;

        if (m_aPortionPaintArea.IsEmpty())
            return;

        OStringBuffer sPayload;
        sPayload = OString::Concat("{\"action\": \"show\","
                   " \"type\": \"drop-down\", \"textArea\": \"") +
                   m_aPortionPaintArea.SVRect().toString() + "\",";
        // Add field params to the message
        sPayload.append(" \"params\": { \"items\": [");

        // List items
        auto pParameters = this->GetParameters();
        auto pListEntriesIter = pParameters->find(ODF_FORMDROPDOWN_LISTENTRY);
        css::uno::Sequence<OUString> vListEntries;
        if (pListEntriesIter != pParameters->end())
        {
            pListEntriesIter->second >>= vListEntries;
            for (const OUString& sItem : vListEntries)
                sPayload.append("\"" + OUStringToOString(sItem, RTL_TEXTENCODING_UTF8) + "\", ");
            sPayload.setLength(sPayload.getLength() - 2);
        }
        sPayload.append("], ");

        // Selected item
        auto pSelectedItemIter = pParameters->find(ODF_FORMDROPDOWN_RESULT);
        sal_Int32 nSelection = -1;
        if (pSelectedItemIter != pParameters->end())
        {
            pSelectedItemIter->second >>= nSelection;
        }
        sPayload.append("\"selected\": \"" + OString::number(nSelection) + "\", ");

        // Placeholder text
        sPayload.append("\"placeholderText\": \"" + OUStringToOString(SwResId(STR_DROP_DOWN_EMPTY_LIST), RTL_TEXTENCODING_UTF8) + "\"}}");
        pViewShell->libreOfficeKitViewCallback(LOK_CALLBACK_FORM_FIELD_BUTTON, sPayload.toString());
    }

    void DropDownFieldmark::SendLOKHideMessage(const SfxViewShell* pViewShell)
    {
        pViewShell->libreOfficeKitViewCallback(LOK_CALLBACK_FORM_FIELD_BUTTON,
            "{\"action\": \"hide\", \"type\": \"drop-down\"}"_ostr);
    }

    DateFieldmark::DateFieldmark(const SwPaM& rPaM)
        : FieldmarkWithDropDownButton(rPaM)
        , m_pNumberFormatter(nullptr)
        , m_pDocumentContentOperationsManager(nullptr)
    {
    }

    DateFieldmark::~DateFieldmark()
    {
    }

    void DateFieldmark::InitDoc(SwDoc& io_rDoc,
            sw::mark::InsertMode eMode, SwPosition const*const pSepPos)
    {
        m_pNumberFormatter = io_rDoc.GetNumberFormatter();
        m_pDocumentContentOperationsManager = &io_rDoc.GetDocumentContentOperationsManager();
        if (eMode == sw::mark::InsertMode::New)
        {
            lcl_SetFieldMarks(*this, io_rDoc, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND, pSepPos);
        }
        else
        {
            lcl_AssertFieldMarksSet(*this, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND);
        }
    }

    void DateFieldmark::ReleaseDoc(SwDoc& rDoc)
    {
        IDocumentUndoRedo & rIDUR(rDoc.GetIDocumentUndoRedo());
        if (rIDUR.DoesUndo())
        {
            // TODO does this need a 3rd Undo class?
            rIDUR.AppendUndo(std::make_unique<SwUndoDelTextFieldmark>(*this));
        }
        ::sw::UndoGuard const ug(rIDUR); // prevent SwUndoDeletes
        lcl_RemoveFieldMarks(*this, rDoc, CH_TXT_ATR_FIELDSTART, CH_TXT_ATR_FIELDEND);
        // notify layouts to unhide - for the entire fieldmark, as in InitDoc()
        SwPaM const tmp(GetMarkPos(), GetOtherMarkPos());
        sw::UpdateFramesForRemoveDeleteRedline(rDoc, tmp);
    }

    void DateFieldmark::ShowButton(SwEditWin* pEditWin)
    {
        if(pEditWin)
        {
            if(!m_pButton)
                m_pButton = VclPtr<DateFormFieldButton>::Create(pEditWin, *this, m_pNumberFormatter);
            SwRect aPaintArea(m_aPaintAreaStart.TopLeft(), m_aPaintAreaEnd.BottomRight());
            m_pButton->CalcPosAndSize(aPaintArea);
            m_pButton->Show();
        }
    }

    void DateFieldmark::SetPortionPaintAreaStart(const SwRect& rPortionPaintArea)
    {
        if (rPortionPaintArea.IsEmpty())
            return;

        m_aPaintAreaStart = rPortionPaintArea;
        InvalidateCurrentDateParam();
    }

    void DateFieldmark::SetPortionPaintAreaEnd(const SwRect& rPortionPaintArea)
    {
        if (rPortionPaintArea.IsEmpty())
            return;

        if(m_aPaintAreaEnd == rPortionPaintArea &&
           m_pButton && m_pButton->IsVisible())
            return;

        m_aPaintAreaEnd = rPortionPaintArea;
        if(m_pButton)
        {
            m_pButton->Show();
            SwRect aPaintArea(m_aPaintAreaStart.TopLeft(), m_aPaintAreaEnd.BottomRight());
            m_pButton->CalcPosAndSize(aPaintArea);
            m_pButton->Invalidate();
        }
        InvalidateCurrentDateParam();
    }

    OUString DateFieldmark::GetContent() const
    {
        const SwTextNode* const pTextNode = GetMarkEnd().GetNode().GetTextNode();
        SwPosition const sepPos(sw::mark::FindFieldSep(*this));
        const sal_Int32 nStart(sepPos.GetContentIndex());
        const sal_Int32 nEnd  (GetMarkEnd().GetContentIndex());

        OUString sContent;
        if(nStart + 1 < pTextNode->GetText().getLength() && nEnd <= pTextNode->GetText().getLength() &&
           nEnd > nStart + 2)
            sContent = pTextNode->GetText().copy(nStart + 1, nEnd - nStart - 2);
        return sContent;
    }

    void DateFieldmark::ReplaceContent(const OUString& sNewContent)
    {
        if(!m_pDocumentContentOperationsManager)
            return;

        const SwTextNode* const pTextNode = GetMarkEnd().GetNode().GetTextNode();
        SwPosition const sepPos(sw::mark::FindFieldSep(*this));
        const sal_Int32 nStart(sepPos.GetContentIndex());
        const sal_Int32 nEnd  (GetMarkEnd().GetContentIndex());

        if(nStart + 1 < pTextNode->GetText().getLength() && nEnd <= pTextNode->GetText().getLength() &&
           nEnd > nStart + 2)
        {
            SwPaM aFieldPam(GetMarkStart().GetNode(), nStart + 1,
                            GetMarkStart().GetNode(), nEnd - 1);
            m_pDocumentContentOperationsManager->ReplaceRange(aFieldPam, sNewContent, false);
        }
        else
        {
            SwPaM aFieldStartPam(GetMarkStart().GetNode(), nStart + 1);
            m_pDocumentContentOperationsManager->InsertString(aFieldStartPam, sNewContent);
        }

    }

    std::pair<bool, double> DateFieldmark::GetCurrentDate() const
    {
        // Check current date param first
        std::pair<bool, double> aResult = ParseCurrentDateParam();
        if(aResult.first)
            return aResult;

        const sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        bool bFoundValidDate = false;
        double dCurrentDate = 0;
        OUString sDateFormat;
        auto pResult = pParameters->find(ODF_FORMDATE_DATEFORMAT);
        if (pResult != pParameters->end())
        {
            pResult->second >>= sDateFormat;
        }

        OUString sLang;
        pResult = pParameters->find(ODF_FORMDATE_DATEFORMAT_LANGUAGE);
        if (pResult != pParameters->end())
        {
            pResult->second >>= sLang;
        }

        // Get current content of the field
        OUString sContent = GetContent();

        sal_uInt32 nFormat = m_pNumberFormatter->GetEntryKey(sDateFormat, LanguageTag(sLang).getLanguageType());
        if (nFormat == NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            sal_Int32 nCheckPos = 0;
            SvNumFormatType nType;
            m_pNumberFormatter->PutEntry(sDateFormat,
                                         nCheckPos,
                                         nType,
                                         nFormat,
                                         LanguageTag(sLang).getLanguageType());
        }

        if (nFormat != NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            bFoundValidDate = m_pNumberFormatter->IsNumberFormat(sContent, nFormat, dCurrentDate);
        }
        return std::pair<bool, double>(bFoundValidDate, dCurrentDate);
    }

    void DateFieldmark::SetCurrentDate(double fDate)
    {
        // Replace current content with the selected date
        ReplaceContent(GetDateInCurrentDateFormat(fDate));

        // Also save the current date in a standard format
        sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        (*pParameters)[ODF_FORMDATE_CURRENTDATE] <<= GetDateInStandardDateFormat(fDate);
    }

    OUString DateFieldmark::GetDateInStandardDateFormat(double fDate) const
    {
        OUString sCurrentDate;
        sal_uInt32 nFormat = m_pNumberFormatter->GetEntryKey(ODF_FORMDATE_CURRENTDATE_FORMAT, ODF_FORMDATE_CURRENTDATE_LANGUAGE);
        if (nFormat == NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            sal_Int32 nCheckPos = 0;
            SvNumFormatType nType;
            OUString sFormat = ODF_FORMDATE_CURRENTDATE_FORMAT;
            m_pNumberFormatter->PutEntry(sFormat,
                                         nCheckPos,
                                         nType,
                                         nFormat,
                                         ODF_FORMDATE_CURRENTDATE_LANGUAGE);
        }

        if (nFormat != NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            const Color* pCol = nullptr;
            m_pNumberFormatter->GetOutputString(fDate, nFormat, sCurrentDate, &pCol, false);
        }
        return sCurrentDate;
    }

    std::pair<bool, double> DateFieldmark::ParseCurrentDateParam() const
    {
        bool bFoundValidDate = false;
        double dCurrentDate = 0;

        const sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        auto pResult = pParameters->find(ODF_FORMDATE_CURRENTDATE);
        OUString sCurrentDate;
        if (pResult != pParameters->end())
        {
            pResult->second >>= sCurrentDate;
        }
        if(!sCurrentDate.isEmpty())
        {
            sal_uInt32 nFormat = m_pNumberFormatter->GetEntryKey(ODF_FORMDATE_CURRENTDATE_FORMAT, ODF_FORMDATE_CURRENTDATE_LANGUAGE);
            if (nFormat == NUMBERFORMAT_ENTRY_NOT_FOUND)
            {
                sal_Int32 nCheckPos = 0;
                SvNumFormatType nType;
                OUString sFormat = ODF_FORMDATE_CURRENTDATE_FORMAT;
                m_pNumberFormatter->PutEntry(sFormat,
                                             nCheckPos,
                                             nType,
                                             nFormat,
                                             ODF_FORMDATE_CURRENTDATE_LANGUAGE);
            }

            if(nFormat != NUMBERFORMAT_ENTRY_NOT_FOUND)
            {
                bFoundValidDate = m_pNumberFormatter->IsNumberFormat(sCurrentDate, nFormat, dCurrentDate);
            }
        }
        return std::pair<bool, double>(bFoundValidDate, dCurrentDate);
    }


    OUString DateFieldmark::GetDateInCurrentDateFormat(double fDate) const
    {
        // Get current date format and language
        OUString sDateFormat;
        const sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
        auto pResult = pParameters->find(ODF_FORMDATE_DATEFORMAT);
        if (pResult != pParameters->end())
        {
            pResult->second >>= sDateFormat;
        }

        OUString sLang;
        pResult = pParameters->find(ODF_FORMDATE_DATEFORMAT_LANGUAGE);
        if (pResult != pParameters->end())
        {
            pResult->second >>= sLang;
        }

        // Fill the content with the specified format
        OUString sCurrentContent;
        sal_uInt32 nFormat = m_pNumberFormatter->GetEntryKey(sDateFormat, LanguageTag(sLang).getLanguageType());
        if (nFormat == NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            sal_Int32 nCheckPos = 0;
            SvNumFormatType nType;
            OUString sFormat = sDateFormat;
            m_pNumberFormatter->PutEntry(sFormat,
                                         nCheckPos,
                                         nType,
                                         nFormat,
                                         LanguageTag(sLang).getLanguageType());
        }

        if (nFormat != NUMBERFORMAT_ENTRY_NOT_FOUND)
        {
            const Color* pCol = nullptr;
            m_pNumberFormatter->GetOutputString(fDate, nFormat, sCurrentContent, &pCol, false);
        }
        return sCurrentContent;
    }

    void DateFieldmark::InvalidateCurrentDateParam()
    {
        std::pair<bool, double> aResult = ParseCurrentDateParam();
        if(!aResult.first)
            return;

        // Current date became invalid
        if(GetDateInCurrentDateFormat(aResult.second) != GetContent())
        {
            sw::mark::Fieldmark::parameter_map_t* pParameters = GetParameters();
            (*pParameters)[ODF_FORMDATE_CURRENTDATE] <<= OUString();
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

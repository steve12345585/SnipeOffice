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


#include "impedit.hxx"
#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>
#include <editeng/editview.hxx>
#include <editeng/editeng.hxx>
#include <edtspell.hxx>
#include <editeng/flditem.hxx>
#include <svl/intitem.hxx>
#include <svl/eitem.hxx>
#include <editeng/unolingu.hxx>
#include <com/sun/star/linguistic2/XDictionary.hpp>

using namespace com::sun::star::uno;
using namespace com::sun::star::linguistic2;


EditSpellWrapper::EditSpellWrapper(weld::Widget* pWindow,
        bool bIsStart, EditView* pView )
    : SvxSpellWrapper(pWindow, bIsStart, false/*bIsAllRight*/)
{
    SAL_WARN_IF( !pView, "editeng", "One view has to be abandoned!" );
    // Keep IgnoreList, delete ReplaceList...
    if (LinguMgr::GetChangeAllList().is())
        LinguMgr::GetChangeAllList()->clear();
    pEditView = pView;
}

void EditSpellWrapper::SpellStart( SvxSpellArea eArea )
{
    EditEngine& rEditEngine = pEditView->getEditEngine();
    ImpEditEngine& rImpEditEngine = pEditView->getImpEditEngine();
    SpellInfo* pSpellInfo = rImpEditEngine.GetSpellInfo();

    if ( eArea == SvxSpellArea::BodyStart )
    {
        // Is called when
        // a) Spell-Forward has arrived at the end and should restart at the top
        // IsEndDone() returns also true, when backward-spelling is started at the end!
        if ( IsEndDone() )
        {
            pSpellInfo->bSpellToEnd = false;
            pSpellInfo->aSpellTo = pSpellInfo->aSpellStart;
            pEditView->getImpl().SetEditSelection(rEditEngine.GetEditDoc().GetStartPaM());
        }
        else
        {
            pSpellInfo->bSpellToEnd = true;
            pSpellInfo->aSpellTo = rImpEditEngine.CreateEPaM(
                    rEditEngine.GetEditDoc().GetStartPaM() );
        }
    }
    else if ( eArea == SvxSpellArea::BodyEnd )
    {
        // Is called when
        // a) Spell-Forward is launched
        // IsStartDone() return also true, when forward-spelling is started at the beginning!
        if ( !IsStartDone() )
        {
            pSpellInfo->bSpellToEnd = true;
            pSpellInfo->aSpellTo = rImpEditEngine.CreateEPaM(
                    rEditEngine.GetEditDoc().GetEndPaM() );
        }
        else
        {
            pSpellInfo->bSpellToEnd = false;
            pSpellInfo->aSpellTo = pSpellInfo->aSpellStart;
            pEditView->getImpl().SetEditSelection(rEditEngine.GetEditDoc().GetEndPaM());
        }
    }
    else if ( eArea == SvxSpellArea::Body )
    {
        ;   // Is handled by the App through SpellNextDocument
    }
    else
    {
        OSL_FAIL( "SpellStart: Unknown Area!" );
    }
}

void EditSpellWrapper::SpellContinue()
{
    SetLast(pEditView->getImpEditEngine().ImpSpell(pEditView));
}

bool EditSpellWrapper::SpellMore()
{
    EditEngine& rEditEngine = pEditView->getEditEngine();
    ImpEditEngine& rImpEditEngine = pEditView->getImpEditEngine();
    SpellInfo* pSpellInfo = rImpEditEngine.GetSpellInfo();
    bool bMore = false;
    if ( pSpellInfo->bMultipleDoc )
    {
        bMore = rEditEngine.SpellNextDocument();
        if ( bMore )
        {
            // The text has been entered into the engine, when backwards then
            // it must be behind the selection.
            pEditView->getImpl().SetEditSelection(rEditEngine.GetEditDoc().GetStartPaM());
        }
    }
    return bMore;
}

void EditSpellWrapper::ReplaceAll( const OUString &rNewText )
{
    // Is called when the word is in ReplaceList of the spell checker
    pEditView->InsertText( rNewText );
    CheckSpellTo();
}

void EditSpellWrapper::CheckSpellTo()
{
    ImpEditEngine& rImpEditEngine = pEditView->getImpEditEngine();
    SpellInfo* pSpellInfo = rImpEditEngine.GetSpellInfo();
    EditPaM aPaM( pEditView->getImpl().GetEditSelection().Max() );
    EPaM aEPaM = rImpEditEngine.CreateEPaM( aPaM );
    if ( aEPaM.nPara == pSpellInfo->aSpellTo.nPara )
    {
        // Check if SpellToEnd still has a valid Index, if replace has been
        // performed in the paragraph.
        if ( pSpellInfo->aSpellTo.nIndex > aPaM.GetNode()->Len() )
            pSpellInfo->aSpellTo.nIndex = aPaM.GetNode()->Len();
    }
}

WrongList::WrongList() : mnInvalidStart(0), mnInvalidEnd(Valid) {}

void WrongList::SetRanges( std::vector<editeng::MisspellRange>&&rRanges )
{
    maRanges = std::move(rRanges);
}

bool WrongList::IsValid() const
{
    return mnInvalidStart == Valid;
}

void WrongList::SetValid()
{
    mnInvalidStart = Valid;
    mnInvalidEnd = 0;
}

void WrongList::SetInvalidRange( size_t nStart, size_t nEnd )
{
    if (mnInvalidStart == Valid || nStart < mnInvalidStart)
        mnInvalidStart = nStart;

    if (mnInvalidEnd < nEnd)
        mnInvalidEnd = nEnd;
}

void WrongList::ResetInvalidRange( size_t nStart, size_t nEnd )
{
    mnInvalidStart = nStart;
    mnInvalidEnd = nEnd;
}

void WrongList::TextInserted( size_t nPos, size_t nLength, bool bPosIsSep )
{
    if (IsValid())
    {
        mnInvalidStart = nPos;
        mnInvalidEnd = nPos + nLength;
    }
    else
    {
        if ( mnInvalidStart > nPos )
            mnInvalidStart = nPos;
        if ( mnInvalidEnd >= nPos )
            mnInvalidEnd = mnInvalidEnd + nLength;
        else
            mnInvalidEnd = nPos + nLength;
    }

    for (size_t i = 0, n = maRanges.size(); i < n; ++i)
    {
        editeng::MisspellRange& rWrong = maRanges[i];
        bool bRefIsValid = true;
        if (rWrong.mnEnd >= nPos)
        {
            // Move all Wrongs after the insert position...
            if (rWrong.mnStart > nPos)
            {
                rWrong.mnStart += nLength;
                rWrong.mnEnd += nLength;
            }
            // 1: Starts before and goes until nPos...
            else if (rWrong.mnEnd == nPos)
            {
                // Should be halted at a blank!
                if ( !bPosIsSep )
                    rWrong.mnEnd += nLength;
            }
            // 2: Starts before and goes until after nPos...
            else if ((rWrong.mnStart < nPos) && (rWrong.mnEnd > nPos))
            {
                rWrong.mnEnd += nLength;
                // When a separator remove and re-examine the Wrong
                if ( bPosIsSep )
                {
                    // Split Wrong...
                    editeng::MisspellRange aNewWrong(rWrong.mnStart, nPos);
                    rWrong.mnStart = nPos + 1;
                    maRanges.insert(maRanges.begin() + i, aNewWrong);
                    // Reference no longer valid after Insert, the other
                    // was inserted in front of this position
                    bRefIsValid = false;
                    ++i; // Not this again...
                }
            }
            // 3: Attribute starts at position ..
            else if (rWrong.mnStart == nPos)
            {
                rWrong.mnEnd += nLength;
                if ( bPosIsSep )
                    ++(rWrong.mnStart);
            }
        }
        SAL_WARN_IF(bRefIsValid && rWrong.mnStart >= rWrong.mnEnd, "editeng",
                "TextInserted, editeng::MisspellRange: Start >= End?!");
    }

    SAL_WARN_IF(DbgIsBuggy(), "editeng", "InsertWrong: WrongList broken!");
}

void WrongList::TextDeleted( size_t nPos, size_t nLength )
{
    size_t nEndPos = nPos + nLength;
    if (IsValid())
    {
        const size_t nNewInvalidStart = nPos ? nPos - 1 : 0;
        mnInvalidStart = nNewInvalidStart;
        mnInvalidEnd = nNewInvalidStart + 1;
    }
    else
    {
        if ( mnInvalidStart > nPos )
            mnInvalidStart = nPos;
        if ( mnInvalidEnd > nPos )
        {
            if (mnInvalidEnd > nEndPos)
                mnInvalidEnd = mnInvalidEnd - nLength;
            else
                mnInvalidEnd = nPos+1;
        }
    }

    for (WrongList::iterator i = maRanges.begin(); i != maRanges.end(); )
    {
        bool bDelWrong = false;
        if (i->mnEnd >= nPos)
        {
            // Move all Wrongs after the insert position...
            if (i->mnStart >= nEndPos)
            {
                i->mnStart -= nLength;
                i->mnEnd -= nLength;
            }
            // 1. Delete Internal Wrongs ...
            else if (i->mnStart >= nPos && i->mnEnd <= nEndPos)
            {
                bDelWrong = true;
            }
            // 2. Wrong begins before, ends inside or behind it ...
            else if (i->mnStart <= nPos && i->mnEnd > nPos)
            {
                if (i->mnEnd <= nEndPos)   // ends inside
                    i->mnEnd = nPos;
                else
                    i->mnEnd -= nLength; // ends after
            }
            // 3. Wrong begins inside, ending after ...
            else if (i->mnStart >= nPos && i->mnEnd > nEndPos)
            {
                i->mnStart = nEndPos - nLength;
                i->mnEnd -= nLength;
            }
        }
        SAL_WARN_IF(i->mnStart >= i->mnEnd, "editeng",
                "TextDeleted, editeng::MisspellRange: Start >= End?!");
        if ( bDelWrong )
        {
            i = maRanges.erase(i);
        }
        else
        {
            ++i;
        }
    }

    SAL_WARN_IF(DbgIsBuggy(), "editeng", "TextDeleted: WrongList broken!");
}

bool WrongList::NextWrong( size_t& rnStart, size_t& rnEnd ) const
{
    /*
        rnStart get the start position, is possibly adjusted wrt. Wrong start
        rnEnd does not have to be initialized.
    */
    for (auto const& range : maRanges)
    {
        if (range.mnEnd > rnStart)
        {
            rnStart = range.mnStart;
            rnEnd = range.mnEnd;
            return true;
        }
    }
    return false;
}

bool WrongList::HasWrong( size_t nStart, size_t nEnd ) const
{
    for (auto const& range : maRanges)
    {
        if (range.mnStart == nStart && range.mnEnd == nEnd)
            return true;
        else if (range.mnStart >= nStart)
            break;
    }
    return false;
}

bool WrongList::HasAnyWrong( size_t nStart, size_t nEnd ) const
{
    for (auto const& range : maRanges)
    {
        if (range.mnEnd >= nStart && range.mnStart < nEnd)
            return true;
        else if (range.mnStart >= nEnd)
            break;
    }
    return false;
}

void WrongList::ClearWrongs( size_t nStart, size_t nEnd,
            const ContentNode* pNode )
{
    for (WrongList::iterator i = maRanges.begin(); i != maRanges.end(); )
    {
        if (i->mnEnd > nStart && i->mnStart < nEnd)
        {
            if (i->mnEnd > nEnd) // Runs out
            {
                i->mnStart = nEnd;
                // Blanks?
                while (i->mnStart < o3tl::make_unsigned(pNode->Len()) &&
                       (pNode->GetChar(i->mnStart) == ' ' ||
                        pNode->IsFeature(i->mnStart)))
                {
                    ++i->mnStart;
                }
                ++i;
            }
            else
            {
                i = maRanges.erase(i);
                // no increment here
            }
        }
        else
        {
            ++i;
        }
    }

    SAL_WARN_IF(DbgIsBuggy(), "editeng", "ClearWrongs: WrongList broken!");
}

void WrongList::InsertWrong( size_t nStart, size_t nEnd )
{
    WrongList::iterator nPos = std::find_if(maRanges.begin(), maRanges.end(),
        [&nStart](const editeng::MisspellRange& rRange) { return rRange.mnStart >= nStart; });

    if (nPos != maRanges.end())
    {
        {
            // It can really only happen that the Wrong starts exactly here
            // and runs along, but not that there are several ranges ...
            // Exactly in the range is no one allowed to be, otherwise this
            // Method can not be called!
            SAL_WARN_IF((nPos->mnStart != nStart || nPos->mnEnd <= nEnd) && nPos->mnStart <= nEnd, "editeng", "InsertWrong: RangeMismatch!");
            if (nPos->mnStart == nStart && nPos->mnEnd > nEnd)
                nPos->mnStart = nEnd + 1;
        }
        maRanges.insert(nPos, editeng::MisspellRange(nStart, nEnd));
    }
    else
        maRanges.emplace_back(nStart, nEnd);

    SAL_WARN_IF(DbgIsBuggy(), "editeng", "InsertWrong: WrongList broken!");
}

void WrongList::MarkWrongsInvalid()
{
    if (!maRanges.empty())
        SetInvalidRange(maRanges.front().mnStart, maRanges.back().mnEnd);
}

WrongList* WrongList::Clone() const
{
    return new WrongList(*this);
}

// #i102062#
bool WrongList::operator==(const WrongList& rCompare) const
{
    // check direct members
    if(GetInvalidStart() != rCompare.GetInvalidStart()
        || GetInvalidEnd() != rCompare.GetInvalidEnd())
        return false;

    return std::equal(maRanges.begin(), maRanges.end(), rCompare.maRanges.begin(), rCompare.maRanges.end(),
        [](const editeng::MisspellRange& a, const editeng::MisspellRange& b) {
            return a.mnStart == b.mnStart && a.mnEnd == b.mnEnd; });
}

bool WrongList::empty() const
{
    return maRanges.empty();
}

void WrongList::push_back(const editeng::MisspellRange& rRange)
{
    maRanges.push_back(rRange);
}

editeng::MisspellRange& WrongList::back()
{
    return maRanges.back();
}

const editeng::MisspellRange& WrongList::back() const
{
    return maRanges.back();
}

WrongList::iterator WrongList::begin()
{
    return maRanges.begin();
}

WrongList::iterator WrongList::end()
{
    return maRanges.end();
}

WrongList::const_iterator WrongList::begin() const
{
    return maRanges.begin();
}

WrongList::const_iterator WrongList::end() const
{
    return maRanges.end();
}

bool WrongList::DbgIsBuggy() const
{
    // Check if the ranges overlap.
    bool bError = false;
    for (WrongList::const_iterator i = maRanges.begin(); !bError && (i != maRanges.end()); ++i)
    {
        bError = std::any_of(i + 1, maRanges.end(), [&i](const editeng::MisspellRange& rRange) {
            return i->mnStart <= rRange.mnEnd && rRange.mnStart <= i->mnEnd; });
    }
    return bError;
}


EdtAutoCorrDoc::EdtAutoCorrDoc(
    EditEngine* pE, ContentNode* pN, sal_Int32 nCrsr, sal_Unicode cIns) :
    mpEditEngine(pE),
    pCurNode(pN),
    nCursor(nCrsr),
    bAllowUndoAction(cIns != 0),
    bUndoAction(false) {}

EdtAutoCorrDoc::~EdtAutoCorrDoc()
{
    if ( bUndoAction )
        mpEditEngine->UndoActionEnd();
}

bool EdtAutoCorrDoc::Delete(sal_Int32 nStt, sal_Int32 nEnd)
{
    EditSelection aSel( EditPaM( pCurNode, nStt ), EditPaM( pCurNode, nEnd ) );
    mpEditEngine->DeleteSelection(aSel);
    SAL_WARN_IF(nCursor < nEnd, "editeng",
            "Cursor in the heart of the action?!");
    nCursor -= ( nEnd-nStt );
    bAllowUndoAction = false;
    return true;
}

bool EdtAutoCorrDoc::Insert(sal_Int32 nPos, const OUString& rTxt)
{
    EditSelection aSel(EditPaM( pCurNode, nPos ));
    mpEditEngine->InsertText(aSel, rTxt);
    SAL_WARN_IF(nCursor < nPos, "editeng",
            "Cursor in the heart of the action?!");
    nCursor = nCursor + rTxt.getLength();

    if ( bAllowUndoAction && ( rTxt.getLength() == 1 ) )
        ImplStartUndoAction();
    bAllowUndoAction = false;

    return true;
}

bool EdtAutoCorrDoc::Replace(sal_Int32 nPos, const OUString& rTxt)
{
    return ReplaceRange( nPos, rTxt.getLength(), rTxt );
}

bool EdtAutoCorrDoc::ReplaceRange(sal_Int32 nPos, sal_Int32 nSourceLength, const OUString& rTxt)
{
    // Actually a Replace introduce => corresponds to UNDO
    sal_Int32 nEnd = nPos+nSourceLength;
    if ( nEnd > pCurNode->Len() )
        nEnd = pCurNode->Len();

    // #i5925# First insert new text behind to be deleted text, for keeping attributes.
    mpEditEngine->InsertText(EditSelection(EditPaM(pCurNode, nEnd)), rTxt);
    mpEditEngine->DeleteSelection(
        EditSelection(EditPaM(pCurNode, nPos), EditPaM(pCurNode, nEnd)));

    if ( nPos == nCursor )
        nCursor = nCursor + rTxt.getLength();

    if ( bAllowUndoAction && ( rTxt.getLength() == 1 ) )
        ImplStartUndoAction();

    bAllowUndoAction = false;

    return true;
}

void EdtAutoCorrDoc::SetAttr(sal_Int32 nStt, sal_Int32 nEnd,
            sal_uInt16 nSlotId, SfxPoolItem& rItem)
{
    SfxItemPool* pPool = &mpEditEngine->GetEditDoc().GetItemPool();
    while ( pPool->GetSecondaryPool() &&
            pPool->GetName() != "EditEngineItemPool" )
    {
        pPool = pPool->GetSecondaryPool();

    }
    sal_uInt16 nWhich = pPool->GetWhichIDFromSlotID( nSlotId );
    if ( nWhich )
    {
        rItem.SetWhich( nWhich );

        SfxItemSet aSet = mpEditEngine->GetEmptyItemSet();
        aSet.Put( rItem );

        EditSelection aSel( EditPaM( pCurNode, nStt ), EditPaM( pCurNode, nEnd ) );
        aSel.Max().SetIndex( nEnd );    // ???
        mpEditEngine->SetAttribs( aSel, aSet, SetAttribsMode::Edge );
        bAllowUndoAction = false;
    }
}

bool EdtAutoCorrDoc::SetINetAttr(sal_Int32 nStt, sal_Int32 nEnd,
            const OUString& rURL)
{
    // Turn the Text into a command field ...
    EditSelection aSel( EditPaM( pCurNode, nStt ), EditPaM( pCurNode, nEnd ) );
    OUString aText = mpEditEngine->GetSelected(aSel);
    aSel = mpEditEngine->DeleteSelection(aSel);
    SAL_WARN_IF(nCursor < nEnd, "editeng",
            "Cursor in the heart of the action?!");
    nCursor -= ( nEnd-nStt );
    SvxFieldItem aField( SvxURLField( rURL, aText, SvxURLFormat::Repr ),
                                      EE_FEATURE_FIELD  );
    mpEditEngine->InsertField(aSel, aField);
    nCursor++;
    mpEditEngine->UpdateFieldsOnly();
    bAllowUndoAction = false;
    return true;
}

OUString const* EdtAutoCorrDoc::GetPrevPara(bool const)
{
    // Return previous paragraph, so that it can be determined,
    // whether the current word is at the beginning of a sentence.

    bAllowUndoAction = false;   // Not anymore ...

    EditDoc& rEditDoc = mpEditEngine->GetEditDoc();
    sal_Int32 nPos = rEditDoc.GetPos( pCurNode );

    // Special case: Bullet => Paragraph start => simply return NULL...
    const SfxBoolItem& rBulletState = mpEditEngine->GetParaAttrib( nPos, EE_PARA_BULLETSTATE );
    bool bBullet = rBulletState.GetValue();
    if ( !bBullet && (mpEditEngine->GetControlWord() & EEControlBits::OUTLINER) )
    {
        // The Outliner has still a Bullet at Level 0.
        const SfxInt16Item& rLevel = mpEditEngine->GetParaAttrib( nPos, EE_PARA_OUTLLEVEL );
        if ( rLevel.GetValue() == 0 )
            bBullet = true;
    }
    if ( bBullet )
        return nullptr;

    for ( sal_Int32 n = nPos; n; )
    {
        n--;
        ContentNode* pNode = rEditDoc.GetObject(n);
        if ( pNode->Len() )
            return & pNode->GetString();
    }
    return nullptr;

}

bool EdtAutoCorrDoc::ChgAutoCorrWord( sal_Int32& rSttPos,
            sal_Int32 nEndPos, SvxAutoCorrect& rACorrect,
            OUString* pPara )
{
    // Paragraph-start or a blank found, search for the word
    // shortcut in Auto
    bAllowUndoAction = false;   // Not anymore ...

    OUString aShort( pCurNode->Copy( rSttPos, nEndPos - rSttPos ) );
    bool bRet = false;

    if( aShort.isEmpty() ) {
        return bRet;
    }

    LanguageTag aLanguageTag( mpEditEngine->GetLanguage( EditPaM( pCurNode, rSttPos+1 ) ).nLang );
    sal_Int32 sttPos = rSttPos;
    auto pStatus = rACorrect.SearchWordsInList(pCurNode->GetString(),
                                               sttPos, nEndPos,
                                               *this, aLanguageTag);
    if( !pStatus ) {
        return bRet;
    }

    sal_Int32 minSttPos = sttPos;
    do {
        const SvxAutocorrWord* pFnd = pStatus->GetAutocorrWord();
        if( pFnd && pFnd->IsTextOnly() )
        {
            // replace also last colon of keywords surrounded by colons
            // (for example, ":name:")
            bool replaceLastChar = pFnd->GetShort()[0] == ':'
                                   && pFnd->GetShort().endsWith(":");

            // then replace
            EditSelection aSel( EditPaM( pCurNode, sttPos ),
                                EditPaM( pCurNode, nEndPos + (replaceLastChar ? 1 : 0) ));
            aSel = mpEditEngine->DeleteSelection(aSel);
            SAL_WARN_IF(nCursor < nEndPos, "editeng",
                    "Cursor in the heart of the action?!");
            nCursor -= ( nEndPos-sttPos );
            mpEditEngine->InsertText(aSel, pFnd->GetLong());
            nCursor = nCursor + pFnd->GetLong().getLength();
            nEndPos = sttPos + pFnd->GetLong().getLength();
            if( pPara ) {
                *pPara = pCurNode->GetString();
            }
            bRet = true;
            if( sttPos < minSttPos ) {
                minSttPos = sttPos;
            }
        }
        sttPos = rSttPos;
    } while( SvxAutoCorrect::SearchWordsNext(pCurNode->GetString(),
                                             sttPos, nEndPos, *pStatus) );
    rSttPos = minSttPos;

    return bRet;
}

bool EdtAutoCorrDoc::TransliterateRTLWord( sal_Int32& /*rSttPos*/,
            sal_Int32 /*nEndPos*/, bool /*bApply*/ )
{
    // Paragraph-start or a blank found, search for the word
    // shortcut in Auto
    bool bRet = false;

    return bRet;
}


LanguageType EdtAutoCorrDoc::GetLanguage( sal_Int32 nPos ) const
{
    return mpEditEngine->GetLanguage( EditPaM( pCurNode, nPos+1 ) ).nLang;
}

void EdtAutoCorrDoc::ImplStartUndoAction()
{
    sal_Int32 nPara = mpEditEngine->GetEditDoc().GetPos( pCurNode );
    ESelection aSel(nPara, nCursor);
    mpEditEngine->UndoActionStart( EDITUNDO_INSERT, aSel );
    bUndoAction = true;
    bAllowUndoAction = false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

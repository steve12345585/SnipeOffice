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

#include <config_wasm_strip.h>

#include <hintids.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <svl/itemiter.hxx>
#include <svl/languageoptions.hxx>
#include <editeng/splwrap.hxx>
#include <editeng/langitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/hangulhanja.hxx>
#include <i18nutil/transliteration.hxx>
#include <linguistic/misc.hxx>
#include <SwSmartTagMgr.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>
#include <officecfg/Office/Writer.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <sal/log.hxx>
#include <swmodule.hxx>
#include <splargs.hxx>
#include <viewopt.hxx>
#include <acmplwrd.hxx>
#include <doc.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <docsh.hxx>
#include <txtfld.hxx>
#include <txatbase.hxx>
#include <charatr.hxx>
#include <pam.hxx>
#include <hints.hxx>
#include <ndtxt.hxx>
#include <txtfrm.hxx>
#include <SwGrammarMarkUp.hxx>
#include <rootfrm.hxx>
#include <swscanner.hxx>

#include <breakit.hxx>
#include <UndoOverwrite.hxx>
#include <txatritr.hxx>
#include <redline.hxx>
#include <docary.hxx>
#include <scriptinfo.hxx>
#include <docstat.hxx>
#include <editsh.hxx>
#include <unotextmarkup.hxx>
#include <txtatr.hxx>
#include <fmtautofmt.hxx>
#include <istyleaccess.hxx>
#include <unicode/uchar.h>
#include <DocumentSettingManager.hxx>

#include <com/sun/star/i18n/WordType.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/i18n/XBreakIterator.hpp>

#include <vector>

#include <unotextrange.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::i18n;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::linguistic2;
using namespace ::com::sun::star::smarttags;

namespace
{
    void DetectAndMarkMissingDictionaries( SwDoc& rDoc,
                                           const uno::Reference< XSpellChecker1 >& xSpell,
                                           const LanguageType eActLang )
    {
        if( xSpell.is() && !xSpell->hasLanguage( eActLang.get() ) )
            rDoc.SetMissingDictionaries( true );
        else
            rDoc.SetMissingDictionaries( false );
    }
}

static bool lcl_HasComments(const SwTextNode& rNode)
{
    sal_Int32 nPosition = rNode.GetText().indexOf(CH_TXTATR_INWORD);
    while (nPosition != -1)
    {
        const SwTextAttr* pAttr = rNode.GetTextAttrForCharAt(nPosition);
        if (pAttr && pAttr->Which() == RES_TXTATR_ANNOTATION)
            return true;
        nPosition = rNode.GetText().indexOf(CH_TXTATR_INWORD, nPosition + 1);
    }
    return false;
}

// possible delimiter characters within URLs for word breaking
static bool lcl_IsDelim( const sal_Unicode c )
{
   return '#' == c || '$' == c || '%' == c || '&' == c || '+' == c ||
          ',' == c || '-' == c || '.' == c || '/' == c || ':' == c ||
          ';' == c || '=' == c || '?' == c || '@' == c || '_' == c;
}

// allow to check normal text with hyperlink by recognizing (parts of) URLs
static bool lcl_IsURL(std::u16string_view rWord,
    const SwTextNode &rNode, sal_Int32 nBegin, sal_Int32 nLen)
{
    // not a text with hyperlink
    if ( !rNode.GetTextAttrAt(nBegin, RES_TXTATR_INETFMT) )
        return false;

    // there is a dot in the word, which is not a period ("example.org")
    const size_t nPosAt = rWord.find('.');
    if (nPosAt != std::u16string_view::npos && nPosAt < rWord.length() - 1)
        return true;

    // an e-mail address ("user@example")
    if ( rWord.find('@') != std::u16string_view::npos )
        return true;

    const OUString& rText = rNode.GetText();

    // scheme (e.g. "http" in "http://" or "mailto" in "mailto:address"):
    // word is followed by 1) ':' + an alphanumeric character; 2) or ':' + a delimiter
    if ( nBegin + nLen + 2 <= rText.getLength() && ':' == rText[nBegin + nLen] )
    {
         sal_Unicode c = rText[nBegin + nLen + 1];
         if ( u_isalnum(c) || lcl_IsDelim(c) )
             return true;
    }

    // path, query, fragment (e.g. "path" in "example.org/path"):
    // word is preceded by 1) an alphanumeric character + a delimiter; 2) or two delimiters
    if ( 2 <= nBegin && lcl_IsDelim(rText[nBegin - 1]) )
    {
        sal_Unicode c = rText[nBegin - 2];
        if ( u_isalnum(c) || lcl_IsDelim(c) )
            return true;
    }

    return false;
}

/*
 * This has basically the same function as SwScriptInfo::MaskHiddenRanges,
 * only for deleted redlines
 */

static sal_Int32
lcl_MaskRedlines( const SwTextNode& rNode, OUStringBuffer& rText,
                         sal_Int32 nStt, sal_Int32 nEnd,
                         const sal_Unicode cChar )
{
    sal_Int32 nNumOfMaskedRedlines = 0;

    const SwDoc& rDoc = rNode.GetDoc();

    for ( SwRedlineTable::size_type nAct = rDoc.getIDocumentRedlineAccess().GetRedlinePos( rNode, RedlineType::Any ); nAct < rDoc.getIDocumentRedlineAccess().GetRedlineTable().size(); ++nAct )
    {
        const SwRangeRedline* pRed = rDoc.getIDocumentRedlineAccess().GetRedlineTable()[ nAct ];

        if ( pRed->Start()->GetNode() > rNode )
            break;

        if( RedlineType::Delete == pRed->GetType() )
        {
            sal_Int32 nRedlineEnd;
            sal_Int32 nRedlineStart;

            pRed->CalcStartEnd( rNode.GetIndex(), nRedlineStart, nRedlineEnd );

            if ( nRedlineEnd < nStt || nRedlineStart > nEnd )
                continue;

            while ( nRedlineStart < nRedlineEnd && nRedlineStart < nEnd )
            {
                if (nRedlineStart >= nStt)
                {
                    rText[nRedlineStart] = cChar;
                    ++nNumOfMaskedRedlines;
                }
                ++nRedlineStart;
            }
        }
    }

    return nNumOfMaskedRedlines;
}

/**
 * Used for spell checking. Deleted redlines and hidden characters are masked
 */
static bool
lcl_MaskRedlinesAndHiddenText( const SwTextNode& rNode, OUStringBuffer& rText,
                                      sal_Int32 nStt, sal_Int32 nEnd,
                                      const sal_Unicode cChar = CH_TXTATR_INWORD )
{
    sal_Int32 nRedlinesMasked = 0;
    sal_Int32 nHiddenCharsMasked = 0;

    const SwDoc& rDoc = rNode.GetDoc();
    const bool bShowChg = IDocumentRedlineAccess::IsShowChanges( rDoc.getIDocumentRedlineAccess().GetRedlineFlags() );

    // If called from word count or from spell checking, deleted redlines
    // should be masked:
    if ( bShowChg )
    {
        nRedlinesMasked = lcl_MaskRedlines( rNode, rText, nStt, nEnd, cChar );
    }

    const bool bHideHidden = !SwModule::get()->GetViewOption(rDoc.GetDocumentSettingManager().get(DocumentSettingId::HTML_MODE))->IsShowHiddenChar();

    // If called from word count, we want to mask the hidden ranges even
    // if they are visible:
    if ( bHideHidden )
    {
        nHiddenCharsMasked =
            SwScriptInfo::MaskHiddenRanges( rNode, rText, nStt, nEnd, cChar );
    }

    return (nRedlinesMasked > 0) || (nHiddenCharsMasked > 0);
}

/**
 * Used for spell checking. Calculates a rectangle for repaint.
 */
static SwRect lcl_CalculateRepaintRect(
        const SwTextFrame & rTextFrame, const SwTextNode & rNode,
        sal_Int32 const nChgStart, sal_Int32 const nChgEnd)
{
    TextFrameIndex const iChgStart(rTextFrame.MapModelToView(&rNode, nChgStart));
    TextFrameIndex const iChgEnd(rTextFrame.MapModelToView(&rNode, nChgEnd));

    SwRect aRect = rTextFrame.GetPaintArea();
    SwRect aTmp = rTextFrame.GetPaintArea();

    const SwTextFrame* pStartFrame = &rTextFrame;
    while( pStartFrame->HasFollow() &&
           iChgStart >= pStartFrame->GetFollow()->GetOffset())
        pStartFrame = pStartFrame->GetFollow();
    const SwTextFrame* pEndFrame = pStartFrame;
    while( pEndFrame->HasFollow() &&
           iChgEnd >= pEndFrame->GetFollow()->GetOffset())
        pEndFrame = pEndFrame->GetFollow();

    bool bSameFrame = true;

    if( rTextFrame.HasFollow() )
    {
        if( pEndFrame != pStartFrame )
        {
            bSameFrame = false;
            SwRect aStFrame( pStartFrame->GetPaintArea() );
            {
                SwRectFnSet aRectFnSet(pStartFrame);
                aRectFnSet.SetLeft( aTmp, aRectFnSet.GetLeft(aStFrame) );
                aRectFnSet.SetRight( aTmp, aRectFnSet.GetRight(aStFrame) );
                aRectFnSet.SetBottom( aTmp, aRectFnSet.GetBottom(aStFrame) );
            }
            aStFrame = pEndFrame->GetPaintArea();
            {
                SwRectFnSet aRectFnSet(pEndFrame);
                aRectFnSet.SetTop( aRect, aRectFnSet.GetTop(aStFrame) );
                aRectFnSet.SetLeft( aRect, aRectFnSet.GetLeft(aStFrame) );
                aRectFnSet.SetRight( aRect, aRectFnSet.GetRight(aStFrame) );
            }
            aRect.Union( aTmp );
            while( true )
            {
                pStartFrame = pStartFrame->GetFollow();
                if( pStartFrame == pEndFrame )
                    break;
                aRect.Union( pStartFrame->GetPaintArea() );
            }
        }
    }
    if( bSameFrame )
    {
        SwRectFnSet aRectFnSet(pStartFrame);
        if( aRectFnSet.GetTop(aTmp) == aRectFnSet.GetTop(aRect) )
            aRectFnSet.SetLeft( aRect, aRectFnSet.GetLeft(aTmp) );
        else
        {
            SwRect aStFrame( pStartFrame->GetPaintArea() );
            aRectFnSet.SetLeft( aRect, aRectFnSet.GetLeft(aStFrame) );
            aRectFnSet.SetRight( aRect, aRectFnSet.GetRight(aStFrame) );
            aRectFnSet.SetTop( aRect, aRectFnSet.GetTop(aTmp) );
        }

        if( aTmp.Height() > aRect.Height() )
            aRect.Height( aTmp.Height() );
    }

    return aRect;
}

/**
 * Used for automatic styles. Used during RstAttr.
 */
static bool lcl_HaveCommonAttributes( IStyleAccess& rStyleAccess,
                                      const SfxItemSet* pSet1,
                                      sal_uInt16 nWhichId,
                                      const SfxItemSet& rSet2,
                                      std::shared_ptr<SfxItemSet>& pStyleHandle )
{
    bool bRet = false;

    std::unique_ptr<SfxItemSet> pNewSet;

    if ( !pSet1 )
    {
        OSL_ENSURE( nWhichId, "lcl_HaveCommonAttributes not used correctly" );
        if ( SfxItemState::SET == rSet2.GetItemState( nWhichId, false ) )
        {
            pNewSet = rSet2.Clone();
            pNewSet->ClearItem( nWhichId );
        }
    }
    else if ( pSet1->Count() )
    {
        SfxItemIter aIter( *pSet1 );
        const SfxPoolItem* pItem = aIter.GetCurItem();
        do
        {
            if ( SfxItemState::SET == rSet2.GetItemState( pItem->Which(), false ) )
            {
                if ( !pNewSet )
                    pNewSet = rSet2.Clone();
                pNewSet->ClearItem( pItem->Which() );
            }

            pItem = aIter.NextItem();
        } while (pItem);
    }

    if ( pNewSet )
    {
        if ( pNewSet->Count() )
            pStyleHandle = rStyleAccess.getAutomaticStyle( *pNewSet, IStyleAccess::AUTO_STYLE_CHAR );
        bRet = true;
    }

    return bRet;
}

/** Delete all attributes
 *
 * 5 cases:
 * 1) The attribute is completely in the deletion range:
 *    -> delete it
 * 2) The end of the attribute is in the deletion range:
 *    -> delete it, then re-insert it with new end
 * 3) The start of the attribute is in the deletion range:
 *    -> delete it, then re-insert it with new start
 * 4) The attribute contains the deletion range:
 *       Split, i.e.,
 *    -> Delete, re-insert from old start to start of deletion range
 *    -> insert new attribute from end of deletion range to old end
 * 5) The attribute is outside the deletion range
 *    -> nothing to do
 *
 * @param nStt starting position
 * @param nLen length of the deletion
 * @param nWhich ???
 * @param pSet ???
 * @param bInclRefToxMark ???
 */

void SwTextNode::RstTextAttr(
    sal_Int32 nStt,
    const sal_Int32 nLen,
    const sal_uInt16 nWhich,
    const SfxItemSet* pSet,
    const bool bInclRefToxMark,
    const bool bExactRange )
{
    if ( !GetpSwpHints() )
        return;

    sal_Int32 nEnd = nStt + nLen;
    {
        // enlarge range for the reset of text attributes in case of an overlapping input field
        const SwTextInputField* pTextInputField = dynamic_cast<const SwTextInputField*>(GetTextAttrAt(nStt, RES_TXTATR_INPUTFIELD, ::sw::GetTextAttrMode::Parent));
        if ( pTextInputField == nullptr )
        {
            pTextInputField = dynamic_cast<const SwTextInputField*>(GetTextAttrAt(nEnd, RES_TXTATR_INPUTFIELD, ::sw::GetTextAttrMode::Parent));
        }
        if ( pTextInputField != nullptr )
        {
            if ( nStt > pTextInputField->GetStart() )
            {
                nStt = pTextInputField->GetStart();
            }
            if ( nEnd < *(pTextInputField->End()) )
            {
                nEnd = *(pTextInputField->End());
            }
        }
    }

    bool bChanged = false;

    // nMin and nMax initialized to maximum / minimum (inverse)
    sal_Int32 nMin = m_Text.getLength();
    sal_Int32 nMax = nStt;
    const bool bNoLen = nMin == 0;

    // We have to remember the "new" attributes that have
    // been introduced by splitting surrounding attributes (case 2,3,4).
    std::vector<SwTextAttr *> newAttributes;
    std::vector<SwTextAttr *> delAttributes;

    // iterate over attribute array until start of attribute is behind deletion range
    m_pSwpHints->SortIfNeedBe(); // trigger sorting now, we don't want it during iteration
    size_t i = 0;
    sal_Int32 nAttrStart = sal_Int32();
    SwTextAttr *pHt = nullptr;
    while ( (i < m_pSwpHints->Count())
            && ( ( ( nAttrStart = m_pSwpHints->GetWithoutResorting(i)->GetStart()) < nEnd )
                 || nLen==0 || (nEnd == nAttrStart && nAttrStart == m_Text.getLength()))
            && !bExactRange)
    {
        pHt = m_pSwpHints->GetWithoutResorting(i);

        // attributes without end stay in!
        // but consider <bInclRefToxMark> used by Undo
        const sal_Int32* const pAttrEnd = pHt->GetEnd();
        const bool bKeepAttrWithoutEnd =
            pAttrEnd == nullptr
            && ( !bInclRefToxMark
                 || ( RES_TXTATR_REFMARK != pHt->Which()
                      && RES_TXTATR_TOXMARK != pHt->Which()
                      && RES_TXTATR_META != pHt->Which()
                      && RES_TXTATR_METAFIELD != pHt->Which() ) );
        if ( bKeepAttrWithoutEnd )
        {

            i++;
            continue;
        }
        // attributes with content stay in
        if ( pHt->HasContent() )
        {
            ++i;
            continue;
        }

        // Default behavior is to process all attributes:
        bool bSkipAttr = false;
        std::shared_ptr<SfxItemSet> pStyleHandle;

        // 1. case: We want to reset only the attributes listed in pSet:
        if ( pSet )
        {
            bSkipAttr = SfxItemState::SET != pSet->GetItemState( pHt->Which(), false );
            if ( bSkipAttr && RES_TXTATR_AUTOFMT == pHt->Which() )
            {
                // if the current attribute is an autostyle, we have to check if the autostyle
                // and pSet have any attributes in common. If so, pStyleHandle will contain
                // a handle to AutoStyle / pSet:
                bSkipAttr = !lcl_HaveCommonAttributes( getIDocumentStyleAccess(), pSet, 0, *static_cast<const SwFormatAutoFormat&>(pHt->GetAttr()).GetStyleHandle(), pStyleHandle );
            }
        }
        else if ( nWhich )
        {
            // 2. case: We want to reset only the attributes with WhichId nWhich:
            bSkipAttr = nWhich != pHt->Which();
            if ( bSkipAttr && RES_TXTATR_AUTOFMT == pHt->Which() )
            {
                bSkipAttr = !lcl_HaveCommonAttributes( getIDocumentStyleAccess(), nullptr, nWhich, *static_cast<const SwFormatAutoFormat&>(pHt->GetAttr()).GetStyleHandle(), pStyleHandle );
            }
        }
        else if ( !bInclRefToxMark )
        {
            // 3. case: Reset all attributes except from ref/toxmarks:
            // skip hints with CH_TXTATR here
            // (deleting those is ONLY allowed for UNDO!)
            bSkipAttr = RES_TXTATR_REFMARK   == pHt->Which()
                     || RES_TXTATR_TOXMARK   == pHt->Which()
                     || RES_TXTATR_META      == pHt->Which()
                     || RES_TXTATR_METAFIELD == pHt->Which();
        }

        if ( bSkipAttr )
        {
            i++;
            continue;
        }

        if (nStt <= nAttrStart)     // Case: 1,3,5
        {
            const sal_Int32 nAttrEnd = pAttrEnd != nullptr
                                        ? *pAttrEnd
                                        : nAttrStart;
            if (nEnd > nAttrStart
                || (nEnd == nAttrEnd && nEnd == nAttrStart)) // Case: 1,3
            {
                if ( nMin > nAttrStart )
                    nMin = nAttrStart;
                if ( nMax < nAttrEnd )
                    nMax = nAttrEnd;
                // If only a no-extent hint is deleted, no resorting is needed
                bChanged = bChanged || nEnd > nAttrStart || bNoLen;
                if (nAttrEnd <= nEnd)   // Case: 1
                {
                    delAttributes.push_back(pHt);

                    if ( pStyleHandle )
                    {
                        SwTextAttr* pNew = MakeTextAttr( GetDoc(),
                                *pStyleHandle, nAttrStart, nAttrEnd );
                        newAttributes.push_back(pNew);
                    }
                }
                else    // Case: 3
                {
                    bChanged = true;
                    m_pSwpHints->NoteInHistory( pHt );
                    // UGLY: this may temporarily destroy the sorting!
                    pHt->SetStart(nEnd);
                    m_pSwpHints->NoteInHistory( pHt, true );

                    if ( pStyleHandle && nAttrStart < nEnd )
                    {
                        SwTextAttr* pNew = MakeTextAttr( GetDoc(),
                                *pStyleHandle, nAttrStart, nEnd );
                        newAttributes.push_back(pNew);
                    }
                }
            }
        }
        else if (pAttrEnd != nullptr)         // Case: 2,4,5
        {
            if (*pAttrEnd > nStt)       // Case: 2,4
            {
                if (*pAttrEnd < nEnd)   // Case: 2
                {
                    if ( nMin > nAttrStart )
                        nMin = nAttrStart;
                    if ( nMax < *pAttrEnd )
                        nMax = *pAttrEnd;
                    bChanged = true;

                    const sal_Int32 nAttrEnd = *pAttrEnd;

                    m_pSwpHints->NoteInHistory( pHt );
                    // UGLY: this may temporarily destroy the sorting!
                    pHt->SetEnd(nStt);
                    m_pSwpHints->NoteInHistory( pHt, true );

                    if ( pStyleHandle )
                    {
                        SwTextAttr* pNew = MakeTextAttr( GetDoc(),
                            *pStyleHandle, nStt, nAttrEnd );
                        newAttributes.push_back(pNew);
                    }
                }
                else if (nLen)  // Case: 4
                {
                    // for Length 0 both hints would be merged again by
                    // InsertHint, so leave them alone!
                    if ( nMin > nAttrStart )
                        nMin = nAttrStart;
                    if ( nMax < *pAttrEnd )
                        nMax = *pAttrEnd;
                    bChanged = true;
                    const sal_Int32 nTmpEnd = *pAttrEnd;
                    m_pSwpHints->NoteInHistory( pHt );
                    // UGLY: this may temporarily destroy the sorting!
                    pHt->SetEnd(nStt);
                    m_pSwpHints->NoteInHistory( pHt, true );

                    if ( pStyleHandle && nStt < nEnd )
                    {
                        SwTextAttr* pNew = MakeTextAttr( GetDoc(),
                            *pStyleHandle, nStt, nEnd );
                        newAttributes.push_back(pNew);
                    }

                    if( nEnd < nTmpEnd )
                    {
                        SwTextAttr* pNew = MakeTextAttr( GetDoc(),
                            pHt->GetAttr(), nEnd, nTmpEnd );
                        if ( pNew )
                        {
                            SwTextCharFormat* pCharFormat = dynamic_cast<SwTextCharFormat*>(pHt);
                            if ( pCharFormat )
                                static_txtattr_cast<SwTextCharFormat*>(pNew)->SetSortNumber(pCharFormat->GetSortNumber());

                            newAttributes.push_back(pNew);
                        }
                    }
                }
            }
        }
        ++i;
    }

    if (bExactRange)
    {
        // Only delete the hints which start at nStt and end at nEnd.
        for (i = 0; i < m_pSwpHints->Count(); ++i)
        {
            SwTextAttr* pHint = m_pSwpHints->Get(i);
            if ( (isTXTATR_WITHEND(pHint->Which()) && RES_TXTATR_AUTOFMT != pHint->Which())
                || pHint->GetStart() != nStt)
                continue;

            const sal_Int32* pHintEnd = pHint->GetEnd();
            if (!pHintEnd || *pHintEnd != nEnd)
                continue;

            delAttributes.push_back(pHint);
        }
    }

    if (bChanged && !delAttributes.empty())
    {   // Delete() calls GetStartOf() - requires sorted hints!
        m_pSwpHints->Resort();
    }

    // delay deleting the hints because it re-sorts the hints array
    for (SwTextAttr *const pDel : delAttributes)
    {
        m_pSwpHints->Delete(pDel);
        DestroyAttr(pDel);
    }

    // delay inserting the hints because it re-sorts the hints array
    for (SwTextAttr *const pNew : newAttributes)
    {
        InsertHint(pNew, SetAttrMode::NOHINTADJUST);
    }

    TryDeleteSwpHints();

    if (!bChanged)
        return;

    if ( HasHints() )
    {   // possibly sometimes Resort would be sufficient, but...
        m_pSwpHints->MergePortions(*this);
    }

    // TextFrame's respond to aHint, others to aNew
    SwUpdateAttr aHint(
        nMin,
        nMax,
        0);

    CallSwClientNotify(sw::UpdateAttrHint(nullptr, &aHint));
    CallSwClientNotify(SwFormatChangeHint(nullptr, GetFormatColl()));
}

static sal_Int32 clipIndexBounds(std::u16string_view aStr, sal_Int32 nPos)
{
    if (nPos < 0)
        return 0;
    if (nPos > sal_Int32(aStr.size()))
        return aStr.size();
    return nPos;
}

// Return current word:
// Search from left to right, so find the word before nPos.
// Except if at the start of the paragraph, then return the first word.
// If the first word consists only of whitespace, return an empty string.
OUString SwTextFrame::GetCurWord(SwPosition const& rPos) const
{
    TextFrameIndex const nPos(MapModelToViewPos(rPos));
    SwTextNode *const pTextNode(rPos.GetNode().GetTextNode());
    assert(pTextNode);
    OUString const& rText(GetText());
    assert(sal_Int32(nPos) <= rText.getLength()); // invalid index

    if (rText.isEmpty() || IsHiddenNow())
        return OUString();

    assert(g_pBreakIt && g_pBreakIt->GetBreakIter().is());
    const uno::Reference< XBreakIterator > &rxBreak = g_pBreakIt->GetBreakIter();
    sal_Int16 nWordType = WordType::DICTIONARY_WORD;
    lang::Locale aLocale( g_pBreakIt->GetLocale(pTextNode->GetLang(rPos.GetContentIndex())) );
    Boundary aBndry =
        rxBreak->getWordBoundary(rText, sal_Int32(nPos), aLocale, nWordType, true);

    // if no word was found use previous word (if any)
    if (aBndry.startPos == aBndry.endPos)
    {
        aBndry = rxBreak->previousWord(rText, sal_Int32(nPos), aLocale, nWordType);
    }

    // check if word was found and if it uses a symbol font, if so
    // enforce returning an empty string
    if (aBndry.endPos != aBndry.startPos
        && IsSymbolAt(TextFrameIndex(aBndry.startPos)))
    {
        aBndry.endPos = aBndry.startPos;
    }

    // can have -1 as start/end of bounds not found
    aBndry.startPos = clipIndexBounds(rText, aBndry.startPos);
    aBndry.endPos = clipIndexBounds(rText, aBndry.endPos);

    return  rText.copy(aBndry.startPos,
                       aBndry.endPos - aBndry.startPos);
}

SwScanner::SwScanner( const SwTextNode& rNd, const OUString& rText,
    const LanguageType* pLang, const ModelToViewHelper& rConvMap,
    sal_uInt16 nType, sal_Int32 nStart, sal_Int32 nEnd, bool bClp )
    : SwScanner(
        [&rNd](sal_Int32 const nBegin, sal_uInt16 const nScript, bool const bNoChar)
            { return rNd.GetLang(nBegin, bNoChar ? 0 : 1, nScript); }
        , rText, pLang, rConvMap, nType, nStart, nEnd, bClp)
{
}

SwScanner::SwScanner(std::function<LanguageType(sal_Int32, sal_Int32, bool)> aGetLangOfChar,
                     OUString aText, const LanguageType* pLang,
                     ModelToViewHelper aConvMap, sal_uInt16 nType, sal_Int32 nStart,
                     sal_Int32 nEnd, bool bClp)
    : m_pGetLangOfChar(std::move(aGetLangOfChar))
    , m_aPreDashReplacementText(std::move(aText))
    , m_pLanguage(pLang)
    , m_ModelToView(std::move(aConvMap))
    , m_nLength(0)
    , m_nOverriddenDashCount(0)
    , m_nWordType(nType)
    , m_bClip(bClp)
{
    m_nStartPos = m_nBegin = nStart;
    m_nEndPos = nEnd;

    //MSWord f.e has special emdash and endash behaviour in that they break
    //words for the purposes of word counting, while a hyphen etc. doesn't.

    //The default configuration treats emdash/endash as a word break, but
    //additional ones can be added in under tools->options
    if (m_nWordType == i18n::WordType::WORD_COUNT)
    {
        OUString sDashes = officecfg::Office::Writer::WordCount::AdditionalSeparators::get();
        OUStringBuffer aBuf(m_aPreDashReplacementText);
        for (sal_Int32 i = m_nStartPos; i < m_nEndPos; ++i)
        {
            if (i < 0)
                continue;
            sal_Unicode cChar = aBuf[i];
            if (sDashes.indexOf(cChar) != -1)
            {
                aBuf[i] = ' ';
                ++m_nOverriddenDashCount;
            }
        }
        m_aText = aBuf.makeStringAndClear();
    }
    else
        m_aText = m_aPreDashReplacementText;

    assert(m_aPreDashReplacementText.getLength() == m_aText.getLength());

    LanguageType aNewLang;
    if ( m_pLanguage )
    {
        aNewLang = *m_pLanguage;
    }
    else
    {
        ModelToViewHelper::ModelPosition aModelBeginPos =
            m_ModelToView.ConvertToModelPosition( m_nBegin );
        aNewLang = m_pGetLangOfChar(aModelBeginPos.mnPos, 0, true);
    }
    if (m_aCurrentLang != aNewLang)
    {
        m_aCurrentLang = aNewLang;
        moCharClass.reset();
    }
}

namespace
{
// tdf#45271 For Chinese and Japanese, count characters instead of words
sal_Int32
forceEachCJCodePointToWord(const OUString& rText, sal_Int32 nBegin, sal_Int32 nLen,
                           const ModelToViewHelper* pModelToView,
                           const std::function<LanguageType(sal_Int32, sal_Int32, bool)>& fnGetLangOfChar)
{
    if (nLen > 1)
    {
        const uno::Reference<XBreakIterator>& rxBreak = g_pBreakIt->GetBreakIter();

        sal_uInt16 nCurrScript = rxBreak->getScriptType(rText, nBegin);

        sal_Int32 indexUtf16 = nBegin;
        rText.iterateCodePoints(&indexUtf16);

        // First character is Asian
        if (nCurrScript == i18n::ScriptType::ASIAN)
        {
            auto aModelBeginPos = pModelToView->ConvertToModelPosition(nBegin);
            auto aCurrentLang = fnGetLangOfChar(aModelBeginPos.mnPos, nCurrScript, false);

            // tdf#150621 Korean words must be counted as-is
            if (primary(aCurrentLang) == primary(LANGUAGE_KOREAN))
            {
                return nLen;
            }

            // Word is Chinese or Japanese, and must be truncated to a single character
            return indexUtf16 - nBegin;
        }

        // First character was not Asian, consider appearance of any Asian character
        // to be the end of the word
        while (indexUtf16 < nBegin + nLen)
        {
            nCurrScript = rxBreak->getScriptType(rText, indexUtf16);
            if (nCurrScript == i18n::ScriptType::ASIAN)
            {
                auto aModelBeginPos = pModelToView->ConvertToModelPosition(indexUtf16);
                auto aCurrentLang = fnGetLangOfChar(aModelBeginPos.mnPos, nCurrScript, false);

                // tdf#150621 Korean words must be counted as-is.
                // Note that script changes intentionally do not delimit words for counting.
                if (primary(aCurrentLang) == primary(LANGUAGE_KOREAN))
                {
                    return nLen;
                }

                // Word tail contains Chinese or Japanese, and must be truncated
                return indexUtf16 - nBegin;
            }
            rText.iterateCodePoints(&indexUtf16);
        }
    }
    return nLen;
}
}

bool SwScanner::NextWord()
{
    m_nBegin = m_nBegin + m_nLength;
    Boundary aBound;

    while ( true )
    {
        // skip non-letter characters:
        while (m_nBegin < m_aText.getLength())
        {
            if (m_nBegin >= 0 && !u_isspace(m_aText[m_nBegin]))
            {
                if ( !m_pLanguage )
                {
                    const sal_uInt16 nNextScriptType = g_pBreakIt->GetBreakIter()->getScriptType( m_aText, m_nBegin );
                    ModelToViewHelper::ModelPosition aModelBeginPos =
                        m_ModelToView.ConvertToModelPosition( m_nBegin );
                    LanguageType aNewLang = m_pGetLangOfChar(aModelBeginPos.mnPos, nNextScriptType, false);
                    if (aNewLang != m_aCurrentLang)
                    {
                        m_aCurrentLang = aNewLang;
                        moCharClass.reset();
                    }
                }

                if ( m_nWordType != i18n::WordType::WORD_COUNT )
                {
                    if (!moCharClass)
                        moCharClass.emplace(LanguageTag( g_pBreakIt->GetLocale( m_aCurrentLang ) ));
                    if ( moCharClass->isLetterNumeric(OUString(m_aText[m_nBegin])) )
                        break;
                }
                else
                    break;
            }
            ++m_nBegin;
        }

        if ( m_nBegin >= m_aText.getLength() || m_nBegin >= m_nEndPos )
            return false;

        // get the word boundaries
        aBound = g_pBreakIt->GetBreakIter()->getWordBoundary( m_aText, m_nBegin,
                g_pBreakIt->GetLocale( m_aCurrentLang ), m_nWordType, true );
        OSL_ENSURE( aBound.endPos >= aBound.startPos, "broken aBound result" );

        // we don't want to include preceding text
        // to count words in text with mixed script punctuation correctly,
        // but we want to include preceding symbols (eg. percent sign, section sign,
        // degree sign defined by dict_word_hu to spell check their affixed forms).
        if (m_nWordType == i18n::WordType::WORD_COUNT && aBound.startPos < m_nBegin)
            aBound.startPos = m_nBegin;

        //no word boundaries could be found
        if(aBound.endPos == aBound.startPos)
            return false;

        //if a word before is found it has to be searched for the next
        if(aBound.endPos == m_nBegin)
            ++m_nBegin;
        else
            break;
    } // end while( true )

    // #i89042, as discussed with HDU: don't evaluate script changes for word count. Use whole word.
    if ( m_nWordType == i18n::WordType::WORD_COUNT )
    {
        m_nBegin = std::max(aBound.startPos, m_nBegin);
        m_nLength   = 0;
        if (aBound.endPos > m_nBegin)
            m_nLength = aBound.endPos - m_nBegin;
    }
    else
    {
        // we have to differentiate between these cases:
        if ( aBound.startPos <= m_nBegin )
        {
            OSL_ENSURE( aBound.endPos >= m_nBegin, "Unexpected aBound result" );

            // restrict boundaries to script boundaries and nEndPos
            const sal_uInt16 nCurrScript = g_pBreakIt->GetBreakIter()->getScriptType( m_aText, m_nBegin );
            OUString aTmpWord = m_aText.copy( m_nBegin, aBound.endPos - m_nBegin );
            const sal_Int32 nScriptEnd = m_nBegin +
                g_pBreakIt->GetBreakIter()->endOfScript( aTmpWord, 0, nCurrScript );
            const sal_Int32 nEnd = std::min( aBound.endPos, nScriptEnd );

            // restrict word start to last script change position
            sal_Int32 nScriptBegin = 0;
            if ( aBound.startPos < m_nBegin )
            {
                // search from nBegin backwards until the next script change
                aTmpWord = m_aText.copy( aBound.startPos,
                                       m_nBegin - aBound.startPos + 1 );
                nScriptBegin = aBound.startPos +
                    g_pBreakIt->GetBreakIter()->beginOfScript( aTmpWord, m_nBegin - aBound.startPos,
                                                    nCurrScript );
            }

            m_nBegin = std::max( aBound.startPos, nScriptBegin );
            m_nLength = nEnd - m_nBegin;
        }
        else
        {
            const sal_uInt16 nCurrScript = g_pBreakIt->GetBreakIter()->getScriptType( m_aText, aBound.startPos );
            OUString aTmpWord = m_aText.copy( aBound.startPos,
                                             aBound.endPos - aBound.startPos );
            const sal_Int32 nScriptEnd = aBound.startPos +
                g_pBreakIt->GetBreakIter()->endOfScript( aTmpWord, 0, nCurrScript );
            const sal_Int32 nEnd = std::min( aBound.endPos, nScriptEnd );
            m_nBegin = aBound.startPos;
            m_nLength = nEnd - m_nBegin;
        }
    }

    // optionally clip the result of getWordBoundaries:
    if ( m_bClip )
    {
        aBound.startPos = std::max( aBound.startPos, m_nStartPos );
        aBound.endPos = std::min( aBound.endPos, m_nEndPos );
        if (aBound.endPos < aBound.startPos)
        {
            m_nBegin = m_nEndPos;
            m_nLength = 0; // found word is outside of search interval
        }
        else
        {
            m_nBegin = aBound.startPos;
            m_nLength = aBound.endPos - m_nBegin;
        }
    }

    if( ! m_nLength )
        return false;

    if (m_nWordType == i18n::WordType::WORD_COUNT)
    {
        m_nLength = forceEachCJCodePointToWord(m_aText, m_nBegin, m_nLength, &m_ModelToView,
                                               m_pGetLangOfChar);
    }

    m_aPrevWord = m_aWord;
    m_aWord = m_aPreDashReplacementText.copy( m_nBegin, m_nLength );

    return true;
}

// Note: this is a clone of SwTextFrame::AutoSpell_, so keep them in sync when fixing things!
bool SwTextNode::Spell(SwSpellArgs* pArgs, bool bIsReadOnly)
{
    // modify string according to redline information and hidden text
    const OUString aOldText( m_Text );
    OUStringBuffer buf(m_Text);
    const bool bContainsComments = lcl_HasComments(*this);
    const bool bRestoreString =
        lcl_MaskRedlinesAndHiddenText(*this, buf, 0, m_Text.getLength());
    if (bRestoreString)
    {   // ??? UGLY: is it really necessary to modify m_Text here?
        m_Text = buf.makeStringAndClear();
    }

    sal_Int32 nBegin = ( &pArgs->pStartPos->GetNode() != this )
        ? 0
        : pArgs->pStartPos->GetContentIndex();

    sal_Int32 nEnd = ( &pArgs->pEndPos->GetNode() != this )
            ? m_Text.getLength()
            : pArgs->pEndPos->GetContentIndex();

    pArgs->xSpellAlt = nullptr;

    bool bIsEditableSect = false;
    if (bIsReadOnly)
    {
        // Enable spell checking in editable sections in read-only mode.
        if (SwSectionNode* pSectNode = GetTextNode()->FindSectionNode())
        {
            bIsEditableSect = pSectNode->GetSection().IsEditInReadonly();
        }
    }

    // 4 cases:

    // 1. IsWrongDirty = 0 and GetWrong = 0
    //      Everything is checked and correct
    // 2. IsWrongDirty = 0 and GetWrong = 1
    //      Everything is checked and errors are identified in the wrong list
    // 3. IsWrongDirty = 1 and GetWrong = 0
    //      Nothing has been checked
    // 4. IsWrongDirty = 1 and GetWrong = 1
    //      Text has been checked but there is an invalid range in the wrong list

    // Nothing has to be done for case 1.
    if ((IsWrongDirty() || GetWrong()) && (!bIsReadOnly || bIsEditableSect) && m_Text.getLength())
    {
        if (nBegin > m_Text.getLength())
        {
            nBegin = m_Text.getLength();
        }
        if (nEnd > m_Text.getLength())
        {
            nEnd = m_Text.getLength();
        }

        if(!IsWrongDirty())
        {
            const sal_Int32 nTemp = GetWrong()->NextWrong( nBegin );
            if(nTemp > nEnd)
            {
                // reset original text
                if ( bRestoreString )
                {
                    m_Text = aOldText;
                }
                return false;
            }
            if(nTemp > nBegin)
                nBegin = nTemp;

        }

        // In case 2. we pass the wrong list to the scanned, because only
        // the words in the wrong list have to be checked
        SwScanner aScanner( *this, m_Text, nullptr, ModelToViewHelper(),
                            WordType::DICTIONARY_WORD,
                            nBegin, nEnd );
        bool bNextWord = aScanner.NextWord();
        while( !pArgs->xSpellAlt.is() && bNextWord )
        {
            bool bCalledNextWord = false;

            const OUString& rWord = aScanner.GetWord();

            // get next language for next word, consider language attributes
            // within the word
            LanguageType eActLang = aScanner.GetCurrentLanguage();
            DetectAndMarkMissingDictionaries( GetTextNode()->GetDoc(), pArgs->xSpeller, eActLang );

            if( rWord.getLength() > 0 && LANGUAGE_NONE != eActLang &&
                !lcl_IsURL(rWord, *this, aScanner.GetBegin(), aScanner.GetLen() ) )
            {
                if (pArgs->xSpeller.is())
                {
                    SvxSpellWrapper::CheckSpellLang( pArgs->xSpeller, eActLang );
                    pArgs->xSpellAlt = pArgs->xSpeller->spell( rWord, static_cast<sal_uInt16>(eActLang),
                                            Sequence< PropertyValue >() );
                }
                if( pArgs->xSpellAlt.is() )
                {
                    if ( IsSymbolAt(aScanner.GetBegin()) ||
                        // redlines can leave "in word" character within word,
                        // we must remove them before spell checking
                        // to avoid false alarm
                        ( (bRestoreString || bContainsComments) && pArgs->xSpeller->isValid( rWord.replaceAll(OUStringChar(CH_TXTATR_INWORD), ""),
                            static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() ) ) )
                    {
                        pArgs->xSpellAlt = nullptr;
                    }
                    else
                    {
                        OUString sPrevWord = aScanner.GetPrevWord();
                        auto nWordBegin = aScanner.GetBegin();
                        auto nWordEnd = aScanner.GetEnd();
                        bNextWord = aScanner.NextWord();
                        const OUString& rActualWord = aScanner.GetPrevWord();
                        bCalledNextWord = true;
                        // check space separated word pairs in the dictionary, e.g. "vice versa"
                        if ( !((bNextWord && !linguistic::HasDigits(aScanner.GetWord()) &&
                            pArgs->xSpeller->isValid( rActualWord + " " + aScanner.GetWord(),
                                static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() )) ||
                           ( !sPrevWord.isEmpty() && !linguistic::HasDigits(sPrevWord) &&
                            pArgs->xSpeller->isValid( sPrevWord + " " + rActualWord,
                                static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() ))) )
                        {
                            // make sure the selection build later from the data
                            // below does not include "in word" character to the
                            // left and right in order to preserve those. Therefore
                            // count those "in words" in order to modify the
                            // selection accordingly.
                            const sal_Unicode* pChar = aScanner.GetPrevWord().getStr();
                            sal_Int32 nLeft = 0;
                            while (*pChar++ == CH_TXTATR_INWORD)
                                ++nLeft;
                            pChar = rActualWord.getLength() ? rActualWord.getStr() + rActualWord.getLength() - 1 : nullptr;
                            sal_Int32 nRight = 0;
                            while (pChar && *pChar-- == CH_TXTATR_INWORD)
                                ++nRight;

                            pArgs->pStartPos->Assign(*this, nWordEnd - nRight );
                            pArgs->pEndPos->Assign(*this, nWordBegin + nLeft );
                        }
                        else
                        {
                            pArgs->xSpellAlt = nullptr;
                        }
                    }
                }
            }

            if ( !bCalledNextWord )
                bNextWord = aScanner.NextWord();
        }
    }

    // reset original text
    if ( bRestoreString )
    {
        m_Text = aOldText;
    }

    return pArgs->xSpellAlt.is();
}

void SwTextNode::SetLanguageAndFont( const SwPaM &rPaM,
    LanguageType nLang, sal_uInt16 nLangWhichId,
    const vcl::Font *pFont,  sal_uInt16 nFontWhichId )
{
    SwEditShell *pEditShell = GetDoc().GetEditShell();
    if (!pEditShell)
        return;
    SfxItemSet aSet(pEditShell->GetAttrPool(), nLangWhichId, nLangWhichId );
    if (pFont)
        aSet.MergeRange(nFontWhichId, nFontWhichId); // Keep it sorted
    aSet.Put( SvxLanguageItem( nLang, nLangWhichId ) );

    OSL_ENSURE( pFont, "target font missing?" );
    if (pFont)
    {
        SvxFontItem aFontItem = static_cast<const SvxFontItem&>( aSet.Get( nFontWhichId ) );
        aFontItem.SetFamilyName(   pFont->GetFamilyName());
        aFontItem.SetFamily(       pFont->GetFamilyType());
        aFontItem.SetStyleName(    pFont->GetStyleName());
        aFontItem.SetPitch(        pFont->GetPitch());
        aFontItem.SetCharSet( pFont->GetCharSet() );
        aSet.Put( aFontItem );
    }

    GetDoc().getIDocumentContentOperations().InsertItemSet( rPaM, aSet );
    // SetAttr( aSet );    <- Does not set language attribute of empty paragraphs correctly,
    //                     <- because since there is no selection the flag to garbage
    //                     <- collect all attributes is set, and therefore attributes spanned
    //                     <- over empty selection are removed.

}

bool SwTextNode::Convert( SwConversionArgs &rArgs )
{
    // get range of text within node to be converted
    // (either all the text or the text within the selection
    // when the conversion was started)
    const sal_Int32 nTextBegin = ( &rArgs.pStartPos->GetNode() == this )
        ? std::min(rArgs.pStartPos->GetContentIndex(), m_Text.getLength())
        : 0;

    const sal_Int32 nTextEnd = ( &rArgs.pEndPos->GetNode() == this )
        ?  std::min(rArgs.pEndPos->GetContentIndex(), m_Text.getLength())
        :  m_Text.getLength();

    rArgs.aConvText.clear();

    // modify string according to redline information and hidden text
    const OUString aOldText( m_Text );
    OUStringBuffer buf(m_Text);
    const bool bRestoreString =
        lcl_MaskRedlinesAndHiddenText(*this, buf, 0, m_Text.getLength());
    if (bRestoreString)
    {   // ??? UGLY: is it really necessary to modify m_Text here?
        m_Text = buf.makeStringAndClear();
    }

    bool    bFound  = false;
    sal_Int32  nBegin  = nTextBegin;
    sal_Int32  nLen = 0;
    LanguageType nLangFound = LANGUAGE_NONE;
    if (m_Text.isEmpty())
    {
        if (rArgs.bAllowImplicitChangesForNotConvertibleText)
        {
            // create SwPaM with mark & point spanning empty paragraph
            //SwPaM aCurPaM( *this, *this, nBegin, nBegin + nLen ); <-- wrong c-tor, does sth different
            SwPaM aCurPaM( *this, 0 );

            SetLanguageAndFont( aCurPaM,
                    rArgs.nConvTargetLang, RES_CHRATR_CJK_LANGUAGE,
                    rArgs.pTargetFont, RES_CHRATR_CJK_FONT );
        }
    }
    else
    {
        SwLanguageIterator aIter( *this, nBegin );

        // Implicit changes require setting new attributes, which in turn destroys
        // the attribute sequence on that aIter iterates. We store the necessary
        // coordinates and apply those changes after iterating through the text.
        typedef std::pair<sal_Int32, sal_Int32> ImplicitChangesRange;
        std::vector<ImplicitChangesRange> aImplicitChanges;

        // find non zero length text portion of appropriate language
        do {
            nLangFound = aIter.GetLanguage();
            bool bLangOk =  (nLangFound == rArgs.nConvSrcLang) ||
                                (editeng::HangulHanjaConversion::IsChinese( nLangFound ) &&
                                 editeng::HangulHanjaConversion::IsChinese( rArgs.nConvSrcLang ));

            sal_Int32 nChPos = aIter.GetChgPos();
            // the position at the end of the paragraph is COMPLETE_STRING and
            // thus must be cut to the end of the actual string.
            assert(nChPos != -1);
            if (nChPos == -1 || nChPos == COMPLETE_STRING)
            {
                nChPos = m_Text.getLength();
            }

            nLen = nChPos - nBegin;
            bFound = bLangOk && nLen > 0;
            if (!bFound)
            {
                // create SwPaM with mark & point spanning the attributed text
                //SwPaM aCurPaM( *this, *this, nBegin, nBegin + nLen ); <-- wrong c-tor, does sth different
                SwPaM aCurPaM( *this, nBegin );
                aCurPaM.SetMark();
                aCurPaM.GetPoint()->SetContent(nBegin + nLen);

                // check script type of selected text
                if (SwEditShell *pEditShell = GetDoc().GetEditShell())
                {
                    pEditShell->Push();             // save current cursor on stack
                    pEditShell->SetSelection( aCurPaM );
                    bool bIsAsianScript = (SvtScriptType::ASIAN == pEditShell->GetScriptType());
                    pEditShell->Pop(SwCursorShell::PopMode::DeleteCurrent); // restore cursor from stack

                    if (!bIsAsianScript && rArgs.bAllowImplicitChangesForNotConvertibleText)
                    {
                        // Store for later use
                        aImplicitChanges.emplace_back(nBegin, nBegin+nLen);
                    }
                }
                nBegin = nChPos;    // start of next language portion
            }
        } while (!bFound && aIter.Next());  /* loop while nothing was found and still sth is left to be searched */

        // Apply implicit changes, if any, now that aIter is no longer used
        for (const auto& rImplicitChange : aImplicitChanges)
        {
            SwPaM aPaM( *this, rImplicitChange.first );
            aPaM.SetMark();
            aPaM.GetPoint()->SetContent( rImplicitChange.second );
            SetLanguageAndFont( aPaM, rArgs.nConvTargetLang, RES_CHRATR_CJK_LANGUAGE, rArgs.pTargetFont, RES_CHRATR_CJK_FONT );
        }

    }

    // keep resulting text within selection / range of text to be converted
    if (nBegin < nTextBegin)
        nBegin = nTextBegin;
    if (nBegin + nLen > nTextEnd)
        nLen = nTextEnd - nBegin;
    bool bInSelection = nBegin < nTextEnd;

    if (bFound && bInSelection)     // convertible text found within selection/range?
    {
        OSL_ENSURE( !m_Text.isEmpty(), "convertible text portion missing!" );
        rArgs.aConvText     = m_Text.copy(nBegin, nLen);
        rArgs.nConvTextLang = nLangFound;

        // position where to start looking in next iteration (after current ends)
        rArgs.pStartPos->Assign(*this, nBegin + nLen );
        // end position (when we have travelled over the whole document)
        rArgs.pEndPos->Assign(*this, nBegin );
    }

    // restore original text
    if ( bRestoreString )
    {
        m_Text = aOldText;
    }

    return !rArgs.aConvText.isEmpty();
}

// Note: this is a clone of SwTextNode::Spell, so keep them in sync when fixing things!
SwRect SwTextFrame::AutoSpell_(SwTextNode & rNode, sal_Int32 nActPos)
{
    SwRect aRect;
    assert(sw::FrameContainsNode(*this, rNode.GetIndex()));
    SwTextNode *const pNode(&rNode);
    if (!nActPos)
        nActPos = COMPLETE_STRING;

    SwAutoCompleteWord& rACW = SwDoc::GetAutoCompleteWords();

    // modify string according to redline information and hidden text
    const OUString aOldText( pNode->GetText() );
    OUStringBuffer buf(pNode->m_Text);
    const bool bContainsComments = lcl_HasComments(rNode);
    const bool bRestoreString =
        lcl_MaskRedlinesAndHiddenText(*pNode, buf, 0, pNode->GetText().getLength());
    if (bRestoreString)
    {   // ??? UGLY: is it really necessary to modify m_Text here? just for GetLang()?
        pNode->m_Text = buf.makeStringAndClear();
    }

    // a change of data indicates that at least one word has been modified

    sal_Int32 nBegin = 0;
    sal_Int32 nEnd = pNode->GetText().getLength();
    sal_Int32 nInsertPos = 0;
    sal_Int32 nChgStart = COMPLETE_STRING;
    sal_Int32 nChgEnd = 0;
    sal_Int32 nInvStart = COMPLETE_STRING;
    sal_Int32 nInvEnd = 0;

    const bool bAddAutoCmpl = pNode->IsAutoCompleteWordDirty() &&
                                  SwViewOption::IsAutoCompleteWords();

    if( pNode->GetWrong() )
    {
        nBegin = pNode->GetWrong()->GetBeginInv();
        if( COMPLETE_STRING != nBegin )
        {
            nEnd = std::max(pNode->GetWrong()->GetEndInv(), pNode->GetText().getLength());
        }

        // get word around nBegin, we start at nBegin - 1
        if ( COMPLETE_STRING != nBegin )
        {
            if ( nBegin )
                --nBegin;

            LanguageType eActLang = pNode->GetLang( nBegin );
            Boundary aBound =
                g_pBreakIt->GetBreakIter()->getWordBoundary( pNode->GetText(), nBegin,
                    g_pBreakIt->GetLocale( eActLang ),
                    WordType::DICTIONARY_WORD, true );
            nBegin = aBound.startPos;
        }

        // get the position in the wrong list
        nInsertPos = pNode->GetWrong()->GetWrongPos( nBegin );

        // sometimes we have to skip one entry
        if( nInsertPos < pNode->GetWrong()->Count() &&
            nBegin == pNode->GetWrong()->Pos( nInsertPos ) +
                      pNode->GetWrong()->Len( nInsertPos ) )
                nInsertPos++;
    }

    bool bFresh = nBegin < nEnd;
    bool bPending(false);

    if( bFresh )
    {
        uno::Reference< XSpellChecker1 > xSpell( ::GetSpellChecker() );
        SwDoc& rDoc = pNode->GetDoc();

        SwScanner aScanner( *pNode, pNode->GetText(), nullptr, ModelToViewHelper(),
                            WordType::DICTIONARY_WORD, nBegin, nEnd);

        bool bNextWord = aScanner.NextWord();
        while( bNextWord )
        {
            const OUString& rWord = aScanner.GetWord();
            nBegin = aScanner.GetBegin();
            sal_Int32 nLen = aScanner.GetLen();
            bool bCalledNextWord = false;

            // get next language for next word, consider language attributes
            // within the word
            LanguageType eActLang = aScanner.GetCurrentLanguage();
            DetectAndMarkMissingDictionaries( rDoc, xSpell, eActLang );

            bool bSpell = xSpell.is() && xSpell->hasLanguage( static_cast<sal_uInt16>(eActLang) );
            if( bSpell && !rWord.isEmpty() && !lcl_IsURL(rWord, *pNode, nBegin, nLen) )
            {
                // check for: bAlter => xHyphWord.is()
                OSL_ENSURE(!bSpell || xSpell.is(), "NULL pointer");
                if( !xSpell->isValid( rWord, static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() ) &&
                    // redlines can leave "in word" character within word,
                    // we must remove them before spell checking
                    // to avoid false alarm
                    ((!bRestoreString && !bContainsComments) || !xSpell->isValid( rWord.replaceAll(OUStringChar(CH_TXTATR_INWORD), ""),
                            static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() ) ) )
                {
                    OUString sPrevWord = aScanner.GetPrevWord();
                    bNextWord = aScanner.NextWord();
                    bCalledNextWord = true;
                    // check space separated word pairs in the dictionary, e.g. "vice versa"
                    if ( !((bNextWord && !linguistic::HasDigits(aScanner.GetWord()) &&
                            xSpell->isValid( aScanner.GetPrevWord() + " " + aScanner.GetWord(),
                                static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() )) ||
                           (!sPrevWord.isEmpty() && !linguistic::HasDigits(sPrevWord) &&
                            xSpell->isValid( sPrevWord + " " + aScanner.GetPrevWord(),
                                static_cast<sal_uInt16>(eActLang), Sequence< PropertyValue >() ))) )
                    {
                        sal_Int32 nSmartTagStt = nBegin;
                        sal_Int32 nDummy = 1;
                        if ( !pNode->GetSmartTags() || !pNode->GetSmartTags()->InWrongWord( nSmartTagStt, nDummy ) )
                        {
                            if( !pNode->GetWrong() )
                            {
                                pNode->SetWrong( std::make_unique<SwWrongList>( WRONGLIST_SPELL ) );
                                pNode->GetWrong()->SetInvalid( 0, nEnd );
                            }
                            SwWrongList::FreshState const eState(pNode->GetWrong()->Fresh(
                                nChgStart, nChgEnd, nBegin, nLen, nInsertPos, nActPos));
                            switch (eState)
                            {
                                case SwWrongList::FreshState::FRESH:
                                    pNode->GetWrong()->Insert(OUString(), nullptr, nBegin, nLen, nInsertPos++);
                                    break;
                                case SwWrongList::FreshState::CURSOR:
                                    bPending = true;
                                    [[fallthrough]]; // to mark as invalid
                                case SwWrongList::FreshState::NOTHING:
                                    nInvStart = nBegin;
                                    nInvEnd = nBegin + nLen;
                                    break;
                            }
                        }
                    }
                    else if( bAddAutoCmpl && rACW.GetMinWordLen() <= aScanner.GetPrevWord().getLength() )
                    {
                        // tdf#119695 only add the word if the cursor position is outside the word
                        // so that the incomplete words are not added as autocomplete candidates
                        bool bCursorOutsideWord = nActPos > nBegin + nLen || nActPos < nBegin;
                        if (bCursorOutsideWord)
                            rACW.InsertWord(aScanner.GetPrevWord(), rDoc);
                    }
                }
                else if( bAddAutoCmpl && rACW.GetMinWordLen() <= rWord.getLength() )
                {
                    // tdf#119695 only add the word if the cursor position is outside the word
                    // so that the incomplete words are not added as autocomplete candidates
                    bool bCursorOutsideWord = nActPos > nBegin + nLen || nActPos < nBegin;
                    if (bCursorOutsideWord)
                        rACW.InsertWord(rWord, rDoc);
                }
            }

            if ( !bCalledNextWord )
                 bNextWord = aScanner.NextWord();
        }
    }

    // reset original text
    // i63141 before calling GetCharRect(..) with formatting!
    if ( bRestoreString )
    {
        pNode->m_Text = aOldText;
    }
    if( pNode->GetWrong() )
    {
        if( bFresh )
            pNode->GetWrong()->Fresh( nChgStart, nChgEnd,
                                      nEnd, 0, nInsertPos, nActPos );

        // Calculate repaint area:

        if( nChgStart < nChgEnd )
        {
            aRect = lcl_CalculateRepaintRect(*this, rNode, nChgStart, nChgEnd);

            // fdo#71558 notify misspelled word to accessibility
#if !ENABLE_WASM_STRIP_ACCESSIBILITY
            SwViewShell* pViewSh = getRootFrame() ? getRootFrame()->GetCurrShell() : nullptr;
            if( pViewSh )
                pViewSh->InvalidateAccessibleParaAttrs( *this );
#endif
        }

        pNode->GetWrong()->SetInvalid( nInvStart, nInvEnd );
        pNode->SetWrongDirty(
            (COMPLETE_STRING != pNode->GetWrong()->GetBeginInv())
                ? (bPending
                    ? sw::WrongState::PENDING
                    : sw::WrongState::TODO)
                : sw::WrongState::DONE);
        if( !pNode->GetWrong()->Count() && ! pNode->IsWrongDirty() )
            pNode->ClearWrong();

        if (bPending && getRootFrame())
        {
            if (SwViewShell* pViewSh = getRootFrame()->GetCurrShell())
            {
                pViewSh->OnSpellWrongStatePending();
            }
        }
    }
    else
        pNode->SetWrongDirty(sw::WrongState::DONE);

    if( bAddAutoCmpl )
        pNode->SetAutoCompleteWordDirty( false );

    return aRect;
}

/** Function: SmartTagScan

    Function scans words in current text and checks them in the
    smarttag libraries. If the check returns true to bounds of the
    recognized words are stored into a list that is used later for drawing
    the underline.

    @return SwRect Repaint area
*/
SwRect SwTextFrame::SmartTagScan(SwTextNode & rNode)
{
    SwRect aRet;

    assert(sw::FrameContainsNode(*this, rNode.GetIndex()));
    SwTextNode *const pNode = &rNode;
    const OUString& rText = pNode->GetText();

    // Iterate over language portions
    SmartTagMgr& rSmartTagMgr = SwSmartTagMgr::Get();

    SwWrongList* pSmartTagList = pNode->GetSmartTags();

    sal_Int32 nBegin = 0;
    sal_Int32 nEnd = rText.getLength();

    if ( pSmartTagList )
    {
        if ( pSmartTagList->GetBeginInv() != COMPLETE_STRING )
        {
            nBegin = pSmartTagList->GetBeginInv();
            nEnd = std::min( pSmartTagList->GetEndInv(), rText.getLength() );

            if ( nBegin < nEnd )
            {
                const LanguageType aCurrLang = pNode->GetLang( nBegin );
                const css::lang::Locale aCurrLocale = g_pBreakIt->GetLocale( aCurrLang );
                nBegin = g_pBreakIt->GetBreakIter()->beginOfSentence( rText, nBegin, aCurrLocale );
                nEnd = g_pBreakIt->GetBreakIter()->endOfSentence(rText, nEnd, aCurrLocale);
                if (nEnd > rText.getLength() || nEnd < 0)
                    nEnd = rText.getLength();
            }
        }
    }

    const sal_uInt16 nNumberOfEntries = pSmartTagList ? pSmartTagList->Count() : 0;
    sal_uInt16 nNumberOfRemovedEntries = 0;
    sal_uInt16 nNumberOfInsertedEntries = 0;

    // clear smart tag list between nBegin and nEnd:
    if ( 0 != nNumberOfEntries )
    {
        sal_Int32 nChgStart = COMPLETE_STRING;
        sal_Int32 nChgEnd = 0;
        const sal_uInt16 nCurrentIndex = pSmartTagList->GetWrongPos( nBegin );
        pSmartTagList->Fresh( nChgStart, nChgEnd, nBegin, nEnd - nBegin, nCurrentIndex, COMPLETE_STRING );
        nNumberOfRemovedEntries = nNumberOfEntries - pSmartTagList->Count();
    }

    if ( nBegin < nEnd )
    {
        // Expand the string:
        const ModelToViewHelper aConversionMap(*pNode, getRootFrame() /*TODO - replace or expand fields for smart tags?*/);
        const OUString& aExpandText = aConversionMap.getViewText();

        // Ownership ov ConversionMap is passed to SwXTextMarkup object!
        uno::Reference<text::XTextMarkup> const xTextMarkup =
             new SwXTextMarkup(pNode, aConversionMap);

        css::uno::Reference< css::frame::XController > xController = pNode->GetDoc().GetDocShell()->GetController();

        SwPosition start(*pNode, nBegin);
        SwPosition end  (*pNode, nEnd);
        rtl::Reference<SwXTextRange> xRange = SwXTextRange::CreateXTextRange(pNode->GetDoc(), start, &end);

        rSmartTagMgr.RecognizeTextRange(xRange, xTextMarkup, xController);

        sal_Int32 nLangBegin = nBegin;
        sal_Int32 nLangEnd;

        // smart tag recognition has to be done for each language portion:
        SwLanguageIterator aIter( *pNode, nLangBegin );

        do
        {
            const LanguageType nLang = aIter.GetLanguage();
            const css::lang::Locale aLocale = g_pBreakIt->GetLocale( nLang );
            nLangEnd = std::min<sal_Int32>( nEnd, aIter.GetChgPos() );

            const sal_Int32 nExpandBegin = aConversionMap.ConvertToViewPosition( nLangBegin );
            const sal_Int32 nExpandEnd   = aConversionMap.ConvertToViewPosition( nLangEnd );

            rSmartTagMgr.RecognizeString(aExpandText, xTextMarkup, xController, aLocale, nExpandBegin, nExpandEnd - nExpandBegin );

            nLangBegin = nLangEnd;
        }
        while ( aIter.Next() && nLangEnd < nEnd );

        pSmartTagList = pNode->GetSmartTags();

        const sal_uInt16 nNumberOfEntriesAfterRecognize = pSmartTagList ? pSmartTagList->Count() : 0;
        nNumberOfInsertedEntries = nNumberOfEntriesAfterRecognize - ( nNumberOfEntries - nNumberOfRemovedEntries );
    }

    if( pSmartTagList )
    {
        // Update WrongList stuff
        pSmartTagList->SetInvalid( COMPLETE_STRING, 0 );
        pNode->SetSmartTagDirty( COMPLETE_STRING != pSmartTagList->GetBeginInv() );

        if( !pSmartTagList->Count() && !pNode->IsSmartTagDirty() )
            pNode->ClearSmartTags();

        // Calculate repaint area:
        if ( nBegin < nEnd && ( 0 != nNumberOfRemovedEntries ||
                                0 != nNumberOfInsertedEntries ) )
        {
            aRet = lcl_CalculateRepaintRect(*this, rNode, nBegin, nEnd);
        }
    }
    else
        pNode->SetSmartTagDirty( false );

    return aRet;
}

void SwTextFrame::CollectAutoCmplWrds(SwTextNode & rNode, sal_Int32 nActPos)
{
    assert(sw::FrameContainsNode(*this, rNode.GetIndex())); (void) this;
    SwTextNode *const pNode(&rNode);
    if (!nActPos)
        nActPos = COMPLETE_STRING;

    SwDoc& rDoc = pNode->GetDoc();
    SwAutoCompleteWord& rACW = SwDoc::GetAutoCompleteWords();

    sal_Int32  nBegin = 0;
    sal_Int32  nEnd = pNode->GetText().getLength();
    sal_Int32  nLen;
    bool bACWDirty = false;

    if( nBegin < nEnd )
    {
        int nCnt = 200;
        SwScanner aScanner( *pNode, pNode->GetText(), nullptr, ModelToViewHelper(),
                            WordType::DICTIONARY_WORD, nBegin, nEnd );
        while( aScanner.NextWord() )
        {
            nBegin = aScanner.GetBegin();
            nLen = aScanner.GetLen();
            if( rACW.GetMinWordLen() <= nLen )
            {
                const OUString& rWord = aScanner.GetWord();

                if( nActPos < nBegin || ( nBegin + nLen ) < nActPos )
                {
                    if( rACW.GetMinWordLen() <= rWord.getLength() )
                        rACW.InsertWord( rWord, rDoc );
                }
                else
                    bACWDirty = true;
            }
            if( !--nCnt )
            {
                // don't wait for TIMER here, so we can finish big paragraphs
                if (Application::AnyInput(VCL_INPUT_ANY & VclInputFlags(~VclInputFlags::TIMER)))
                    return;
                nCnt = 100;
            }
        }
    }

    if (!bACWDirty)
        pNode->SetAutoCompleteWordDirty( false );
}

SwInterHyphInfoTextFrame::SwInterHyphInfoTextFrame(
        SwTextFrame const& rFrame, SwTextNode const& rNode,
        SwInterHyphInfo const& rHyphInfo)
    : m_nStart(rFrame.MapModelToView(&rNode, rHyphInfo.m_nStart))
    , m_nEnd(rFrame.MapModelToView(&rNode, rHyphInfo.m_nEnd))
    , m_nWordStart(0)
    , m_nWordLen(0)
{
}

void SwInterHyphInfoTextFrame::UpdateTextNodeHyphInfo(SwTextFrame const& rFrame,
        SwTextNode const& rNode, SwInterHyphInfo & o_rHyphInfo)
{
    std::pair<SwTextNode const*, sal_Int32> const wordStart(rFrame.MapViewToModel(m_nWordStart));
    std::pair<SwTextNode const*, sal_Int32> const wordEnd(rFrame.MapViewToModel(m_nWordStart+m_nWordLen));
    if (wordStart.first != &rNode || wordEnd.first != &rNode)
    {   // not sure if this can happen since nStart/nEnd are in rNode
        SAL_WARN("sw.core", "UpdateTextNodeHyphInfo: outside of node");
        return;
    }
    o_rHyphInfo.m_nWordStart = wordStart.second;
    o_rHyphInfo.m_nWordLen = wordEnd.second - wordStart.second;
    o_rHyphInfo.SetHyphWord(m_xHyphWord);
}

/// Find the SwTextFrame and call its Hyphenate
bool SwTextNode::Hyphenate( SwInterHyphInfo &rHyphInf )
{
    // shortcut: paragraph doesn't have a language set:
    if ( LANGUAGE_NONE == GetSwAttrSet().GetLanguage().GetLanguage()
         && LanguageType(USHRT_MAX) == GetLang(0, m_Text.getLength()))
    {
        return false;
    }

    SwTextFrame *pFrame = ::sw::SwHyphIterCacheLastTextFrame(this,
        [&rHyphInf, this]() {
            std::pair<Point, bool> tmp;
            Point const*const pPoint = rHyphInf.GetCursorPos();
            if (pPoint)
            {
                tmp.first = *pPoint;
                tmp.second = true;
            }
            return static_cast<SwTextFrame*>(this->getLayoutFrame(
                this->GetDoc().getIDocumentLayoutAccess().GetCurrentLayout(),
                nullptr, pPoint ? &tmp : nullptr));
        });
    if (!pFrame)
    {
        // There was a comment here that claimed that the following assertion
        // shouldn't exist as it's triggered by "Trennung ueber Sonderbereiche",
        // (hyphenation across special sections?), whatever that means.
        OSL_ENSURE( pFrame, "!SwTextNode::Hyphenate: can't find any frame" );
        return false;
    }
    SwInterHyphInfoTextFrame aHyphInfo(*pFrame, *this, rHyphInf);

    pFrame = &(pFrame->GetFrameAtOfst( aHyphInfo.m_nStart ));

    while( pFrame )
    {
        if (pFrame->Hyphenate(aHyphInfo))
        {
            // The layout is not robust wrt. "direct formatting"
            // cf. layact.cxx, SwLayAction::TurboAction_(), if( !pCnt->IsValid() ...
            pFrame->SetCompletePaint();
            aHyphInfo.UpdateTextNodeHyphInfo(*pFrame, *this, rHyphInf);
            return true;
        }
        pFrame = pFrame->GetFollow();
        if( pFrame )
        {
            aHyphInfo.m_nEnd = aHyphInfo.m_nEnd - (pFrame->GetOffset() - aHyphInfo.m_nStart);
            aHyphInfo.m_nStart = pFrame->GetOffset();
        }
    }
    return false;
}

namespace
{
    struct swTransliterationChgData
    {
        sal_Int32               nStart;
        sal_Int32               nLen;
        OUString                sChanged;
        Sequence< sal_Int32 >   aOffsets;
    };
}

// change text to Upper/Lower/Hiragana/Katakana/...
void SwTextNode::TransliterateText(
    utl::TransliterationWrapper& rTrans,
    sal_Int32 nStt, sal_Int32 nEnd,
    SwUndoTransliterate* pUndo, bool bUseRedlining )
{
    if (nStt >= nEnd)
        return;

    const sal_Int32 selStart = nStt;
    const sal_Int32 selEnd = nEnd;

    // since we don't use Hiragana/Katakana or half-width/full-width transliterations here
    // it is fine to use ANYWORD_IGNOREWHITESPACES. (ANY_WORD btw is broken and will
    // occasionally miss words in consecutive sentences). Also with ANYWORD_IGNOREWHITESPACES
    // text like 'just-in-time' will be converted to 'Just-In-Time' which seems to be the
    // proper thing to do.
    const sal_Int16 nWordType = WordType::ANYWORD_IGNOREWHITESPACES;

    // In order to have less trouble with changing text size, e.g. because
    // of ligatures or German small sz being resolved, we need to process
    // the text replacements from end to start.
    // This way the offsets for the yet to be changed words will be
    // left unchanged by the already replaced text.
    // For this we temporarily save the changes to be done in this vector
    std::vector< swTransliterationChgData >   aChanges;
    swTransliterationChgData                  aChgData;

    if (rTrans.getType() == TransliterationFlags::TITLE_CASE)
    {
        // for 'capitalize every word' we need to iterate over each word

        Boundary aSttBndry;
        Boundary aEndBndry;
        aSttBndry = g_pBreakIt->GetBreakIter()->getWordBoundary(
                    GetText(), nStt,
                    g_pBreakIt->GetLocale( GetLang( nStt ) ),
                    nWordType,
                    true /*prefer forward direction*/);
        aEndBndry = g_pBreakIt->GetBreakIter()->getWordBoundary(
                    GetText(), nEnd,
                    g_pBreakIt->GetLocale( GetLang( nEnd ) ),
                    nWordType,
                    false /*prefer backward direction*/);

        // prevent backtracking to the previous word if selection is at word boundary
        if (aSttBndry.endPos <= nStt)
        {
            aSttBndry = g_pBreakIt->GetBreakIter()->nextWord(
                    GetText(), aSttBndry.endPos,
                    g_pBreakIt->GetLocale( GetLang( aSttBndry.endPos ) ),
                    nWordType);
        }
        // prevent advancing to the next word if selection is at word boundary
        if (aEndBndry.startPos >= nEnd)
        {
            aEndBndry = g_pBreakIt->GetBreakIter()->previousWord(
                    GetText(), aEndBndry.startPos,
                    g_pBreakIt->GetLocale( GetLang( aEndBndry.startPos ) ),
                    nWordType);
        }

        /* Nothing to do if user selection lies entirely outside of word start and end boundary computed above.
         * Skip this node, because otherwise the below logic for constraining to the selection will fail */
        if (aSttBndry.startPos >= selEnd || aEndBndry.endPos <= selStart) {
            return;
        }

        // prevent going outside of the user's selection, which may
        // start in the middle of a word
        aSttBndry.startPos = std::max(aSttBndry.startPos, selStart);
        aEndBndry.startPos = std::max(aSttBndry.startPos, aEndBndry.startPos);

        Boundary aCurWordBndry( aSttBndry );
        while (aCurWordBndry.startPos <= aEndBndry.startPos)
        {
            nStt = aCurWordBndry.startPos;
            nEnd = aCurWordBndry.endPos;
            const sal_Int32 nLen = nEnd - nStt;
            OSL_ENSURE( nLen > 0, "invalid word length of 0" );

            Sequence <sal_Int32> aOffsets;
            OUString const sChgd( rTrans.transliterate(
                        GetText(), GetLang(nStt), nStt, nLen, &aOffsets) );

            assert(nStt < m_Text.getLength());
            if (0 != rtl_ustr_shortenedCompare_WithLength(
                        m_Text.getStr() + nStt, m_Text.getLength() - nStt,
                        sChgd.getStr(), sChgd.getLength(), nLen))
            {
                aChgData.nStart     = nStt;
                aChgData.nLen       = nLen;
                aChgData.sChanged   = sChgd;
                aChgData.aOffsets   = std::move(aOffsets);
                aChanges.push_back( aChgData );
            }

            aCurWordBndry = g_pBreakIt->GetBreakIter()->nextWord(
                    GetText(), nStt,
                    g_pBreakIt->GetLocale(GetLang(nStt, 1)),
                    nWordType);
        }
    }
    else if (rTrans.getType() == TransliterationFlags::SENTENCE_CASE)
    {
        // For 'sentence case' we need to iterate sentence by sentence.
        // nLastStart and nLastEnd are the boundaries of the last sentence in
        // the user's selection.
        sal_Int32 nLastStart = g_pBreakIt->GetBreakIter()->beginOfSentence(
                GetText(), nEnd,
                g_pBreakIt->GetLocale( GetLang( nEnd ) ) );
        sal_Int32 nLastEnd = g_pBreakIt->GetBreakIter()->endOfSentence(
                GetText(), nLastStart,
                g_pBreakIt->GetLocale( GetLang( nLastStart ) ) );

        // Begin with the starting point of the user's selection (it may not be
        // the beginning of a sentence)...
        sal_Int32 nCurrentStart = nStt;
        // ...And extend to the end of the first sentence
        sal_Int32 nCurrentEnd = g_pBreakIt->GetBreakIter()->endOfSentence(
                GetText(), nCurrentStart,
                g_pBreakIt->GetLocale( GetLang( nCurrentStart ) ) );

        // prevent backtracking to the previous sentence if selection starts at end of a sentence
        if (nCurrentEnd <= nStt)
        {
            // now nCurrentStart is probably located on a non-letter word. (unless we
            // are in Asian text with no spaces...)
            // Thus to get the real sentence start we should locate the next real word,
            // that is one found by DICTIONARY_WORD
            i18n::Boundary aBndry = g_pBreakIt->GetBreakIter()->nextWord(
                    GetText(), nCurrentEnd,
                    g_pBreakIt->GetLocale( GetLang( nCurrentEnd ) ),
                    i18n::WordType::DICTIONARY_WORD);

            // now get new current sentence boundaries
            nCurrentStart = g_pBreakIt->GetBreakIter()->beginOfSentence(
                    GetText(), aBndry.startPos,
                    g_pBreakIt->GetLocale( GetLang( aBndry.startPos) ) );
            nCurrentEnd = g_pBreakIt->GetBreakIter()->endOfSentence(
                    GetText(), nCurrentStart,
                    g_pBreakIt->GetLocale( GetLang( nCurrentStart) ) );
        }
        // prevent advancing to the next sentence if selection ends at start of a sentence
        if (nLastStart >= nEnd)
        {
            // now nCurrentStart is probably located on a non-letter word. (unless we
            // are in Asian text with no spaces...)
            // Thus to get the real sentence start we should locate the previous real word,
            // that is one found by DICTIONARY_WORD
            i18n::Boundary aBndry = g_pBreakIt->GetBreakIter()->previousWord(
                    GetText(), nLastStart,
                    g_pBreakIt->GetLocale( GetLang( nLastStart) ),
                    i18n::WordType::DICTIONARY_WORD);
            nLastEnd = g_pBreakIt->GetBreakIter()->endOfSentence(
                    GetText(), aBndry.startPos,
                    g_pBreakIt->GetLocale( GetLang( aBndry.startPos) ) );
            if (nCurrentEnd > nLastEnd)
                nCurrentEnd = nLastEnd;
        }

        // Prevent going outside of the user's selection
        nCurrentStart = std::max(selStart, nCurrentStart);
        nCurrentEnd = std::min(selEnd, nCurrentEnd);
        nLastEnd = std::min(selEnd, nLastEnd);

        while (nCurrentStart < nLastEnd)
        {
            sal_Int32 nLen = nCurrentEnd - nCurrentStart;
            OSL_ENSURE( nLen > 0, "invalid word length of 0" );

            Sequence <sal_Int32> aOffsets;
            OUString const sChgd( rTrans.transliterate(GetText(),
                GetLang(nCurrentStart), nCurrentStart, nLen, &aOffsets) );

            assert(nStt < m_Text.getLength());
            if (0 != rtl_ustr_shortenedCompare_WithLength(
                        m_Text.getStr() + nStt, m_Text.getLength() - nStt,
                        sChgd.getStr(), sChgd.getLength(), nLen))
            {
                aChgData.nStart     = nCurrentStart;
                aChgData.nLen       = nLen;
                aChgData.sChanged   = sChgd;
                aChgData.aOffsets   = std::move(aOffsets);
                aChanges.push_back( aChgData );
            }

            Boundary aFirstWordBndry = g_pBreakIt->GetBreakIter()->nextWord(
                    GetText(), nCurrentEnd,
                    g_pBreakIt->GetLocale( GetLang( nCurrentEnd ) ),
                    nWordType);
            nCurrentStart = aFirstWordBndry.startPos;
            nCurrentEnd = g_pBreakIt->GetBreakIter()->endOfSentence(
                    GetText(), nCurrentStart,
                    g_pBreakIt->GetLocale( GetLang( nCurrentStart ) ) );
        }
    }
    else
    {
        // here we may transliterate over complete language portions...

        std::unique_ptr<SwLanguageIterator> pIter;
        if( rTrans.needLanguageForTheMode() )
            pIter.reset(new SwLanguageIterator( *this, nStt ));

        sal_Int32 nEndPos = 0;
        LanguageType nLang = LANGUAGE_NONE;
        sal_Int32 nLoopControlRuns = 0;
        do {
            if( pIter )
            {
                nLang = pIter->GetLanguage();
                nEndPos = pIter->GetChgPos();
                if( nEndPos > nEnd )
                    nEndPos = nEnd;
            }
            else
            {
                nLang = LANGUAGE_SYSTEM;
                nEndPos = nEnd;
            }
            const sal_Int32 nLen = nEndPos - nStt;

            Sequence <sal_Int32> aOffsets;
            OUString const sChgd( rTrans.transliterate(
                        m_Text, nLang, nStt, nLen, &aOffsets) );

            assert(nStt < m_Text.getLength());
            if (0 != rtl_ustr_shortenedCompare_WithLength(
                        m_Text.getStr() + nStt, m_Text.getLength() - nStt,
                        sChgd.getStr(), sChgd.getLength(), nLen))
            {
                aChgData.nStart     = nStt;
                aChgData.nLen       = nLen;
                aChgData.sChanged   = sChgd;
                aChgData.aOffsets   = std::move(aOffsets);
                aChanges.push_back( aChgData );
            }

            nStt = nEndPos;

            // tdf#157937 selection containing tracked changes needs loop control:
            // stop looping, if there are too much empty transliterations
            if ( sChgd.isEmpty() )
                ++nLoopControlRuns;

        } while( nEndPos < nEnd && pIter && pIter->Next() && nLoopControlRuns < 100 );
    }

    if (aChanges.empty())
        return;

    // now apply the changes from end to start to leave the offsets of the
    // yet unchanged text parts remain the same.
    size_t nSum(0);

    for (size_t i = 0; i < aChanges.size(); ++i)
    {   // check this here since AddChanges cannot be moved below
        // call to ReplaceTextOnly
        swTransliterationChgData & rData =
            aChanges[ aChanges.size() - 1 - i ];

        nSum += rData.sChanged.getLength() - rData.nLen;
        if (nSum > o3tl::make_unsigned(GetSpaceLeft()))
        {
            SAL_WARN("sw.core", "SwTextNode::ReplaceTextOnly: "
                    "node text with insertion > node capacity.");
            return;
        }

        if ( bUseRedlining )
        {
            // create SwPaM with mark & point spanning the attributed text
            //SwPaM aCurPaM( *this, *this, nBegin, nBegin + nLen ); <-- wrong c-tor, does sth different
            SwPaM aCurPaM( *this, rData.nStart );
            aCurPaM.SetMark();
            aCurPaM.GetPoint()->SetContent( rData.nStart + rData.nLen );
            // replace the changed words
            if ( aCurPaM.GetText() != rData.sChanged )
                GetDoc().getIDocumentContentOperations().ReplaceRange( aCurPaM, rData.sChanged, false );
        }
        else
        {
            if (pUndo)
                pUndo->AddChanges( *this, rData.nStart, rData.nLen, rData.aOffsets );
            ReplaceTextOnly( rData.nStart, rData.nLen, rData.sChanged, rData.aOffsets );
        }
    }
}

void SwTextNode::ReplaceTextOnly( sal_Int32 nPos, sal_Int32 nLen,
                                std::u16string_view aText,
                                const Sequence<sal_Int32>& rOffsets )
{
    assert(sal_Int32(aText.size()) - nLen <= GetSpaceLeft());

    m_Text = m_Text.replaceAt(nPos, nLen, aText);

    sal_Int32 nTLen = aText.size();
    const sal_Int32* pOffsets = rOffsets.getConstArray();
    // now look for no 1-1 mapping -> move the indices!
    sal_Int32 nMyOff = nPos;
    for( sal_Int32 nI = 0; nI < nTLen; ++nI )
    {
        const sal_Int32 nOff = pOffsets[ nI ];
        if( nOff < nMyOff )
        {
            // something is inserted
            sal_Int32 nCnt = 1;
            while( nI + nCnt < nTLen && nOff == pOffsets[ nI + nCnt ] )
                ++nCnt;

            Update(SwContentIndex(this, nMyOff), nCnt, UpdateMode::Default);
            nMyOff = nOff;
            //nMyOff -= nCnt;
            nI += nCnt - 1;
        }
        else if( nOff > nMyOff )
        {
            // something is deleted
            Update(SwContentIndex(this, nMyOff + 1), nOff - nMyOff, UpdateMode::Negative);
            nMyOff = nOff;
        }
        ++nMyOff;
    }
    if( nMyOff < nLen )
        // something is deleted at the end
        Update(SwContentIndex(this, nMyOff), nLen - nMyOff, UpdateMode::Negative);

    // notify the layout!
    const auto aDelHint = sw::DeleteText(nPos, nTLen);
    CallSwClientNotify(aDelHint);

    const auto aInsHint = sw::MakeInsertText(*this, nPos, nTLen);
    CallSwClientNotify(aInsHint);
}

// the return values allows us to see if we did the heavy-
// lifting required to actually break and count the words.
bool SwTextNode::CountWords( SwDocStat& rStat,
                            sal_Int32 nStt, sal_Int32 nEnd ) const
{
    if( nStt > nEnd )
    {   // bad call
        return false;
    }
    if (IsInRedlines())
    {   //not counting txtnodes used to hold deleted redline content
        return false;
    }
    bool bCountAll = ( (0 == nStt) && (GetText().getLength() == nEnd) );
    ++rStat.nAllPara; // #i93174#: count _all_ paragraphs
    if ( IsHidden() )
    {   // not counting hidden paras
        return false;
    }
    // count words in numbering string if started at beginning of para:
    bool bCountNumbering = nStt == 0;
    bool bHasBullet = false, bHasNumbering = false;
    OUString sNumString;
    if (bCountNumbering)
    {
        sNumString = GetNumString();
        bHasNumbering = !sNumString.isEmpty();
        if (!bHasNumbering)
            bHasBullet = HasBullet();
        bCountNumbering = bHasNumbering || bHasBullet;
    }

    if( nStt == nEnd && !bCountNumbering)
    {   // unnumbered empty node or empty selection
        if (bCountAll)
        {
            SetWordCountDirty( false ); // reset flag to speed up DoIdleJob
        }
        return false;
    }

    // count of non-empty paras
    ++rStat.nPara;

    // Shortcut when counting whole paragraph and current count is clean
    if ( bCountAll && !IsWordCountDirty() )
    {
        // accumulate into DocStat record to return the values

        rStat.nWord += m_aParagraphIdleData.nNumberOfWords;
        rStat.nAsianWord += m_aParagraphIdleData.nNumberOfAsianWords;
        rStat.nChar += m_aParagraphIdleData.nNumberOfChars;
        rStat.nCharExcludingSpaces += m_aParagraphIdleData.nNumberOfCharsExcludingSpaces;
        return false;
    }

    // ConversionMap to expand fields, remove invisible and redline deleted text for scanner
    const ModelToViewHelper aConversionMap(*this,
        getIDocumentLayoutAccess().GetCurrentLayout(),
        ExpandMode::ExpandFields | ExpandMode::ExpandFootnote | ExpandMode::HideInvisible | ExpandMode::HideDeletions | ExpandMode::HideFieldmarkCommands);
    const OUString& aExpandText = aConversionMap.getViewText();

    if (aExpandText.isEmpty() && !bCountNumbering)
    {
        if (bCountAll)
        {
            SetWordCountDirty( false ); // reset flag to speed up DoIdleJob
        }
        return false;
    }

    // map start and end points onto the ConversionMap
    const sal_Int32 nExpandBegin = aConversionMap.ConvertToViewPosition( nStt );
    const sal_Int32 nExpandEnd   = aConversionMap.ConvertToViewPosition( nEnd );

    //do the count
    // all counts exclude hidden paras and hidden+redlined within para
    // definition of space/white chars in SwScanner (and BreakIter!)
    // uses both u_isspace and BreakIter getWordBoundary in SwScanner
    sal_uInt32 nTmpWords = 0;        // count of all words
    sal_uInt32 nTmpAsianWords = 0;   //count of all Asian codepoints
    sal_uInt32 nTmpChars = 0;        // count of all chars
    sal_uInt32 nTmpCharsExcludingSpaces = 0;  // all non-white chars

    // count words in masked and expanded text:
    if (!aExpandText.isEmpty())
    {
        assert(g_pBreakIt && g_pBreakIt->GetBreakIter().is());

        // zero is NULL for pLanguage -----------v               last param = true for clipping
        SwScanner aScanner( *this, aExpandText, nullptr, aConversionMap, i18n::WordType::WORD_COUNT,
                            nExpandBegin, nExpandEnd, true );

        // used to filter out scanner returning almost empty strings (len=1; unichar=0x0001)
        const OUString aBreakWord( CH_TXTATR_BREAKWORD );

        while ( aScanner.NextWord() )
        {
            if( !aExpandText.match(aBreakWord, aScanner.GetBegin() ))
            {
                ++nTmpWords;
                const OUString &rWord = aScanner.GetWord();
                if (g_pBreakIt->GetBreakIter()->getScriptType(rWord, 0) == i18n::ScriptType::ASIAN)
                    ++nTmpAsianWords;
                nTmpCharsExcludingSpaces += g_pBreakIt->getGraphemeCount(rWord);
            }
        }

        nTmpCharsExcludingSpaces += aScanner.getOverriddenDashCount();

        nTmpChars = g_pBreakIt->getGraphemeCount(aExpandText, nExpandBegin, nExpandEnd);
    }

    // no nTmpCharsExcludingSpaces adjust needed neither for blanked out MaskedChars
    // nor for mid-word selection - set scanner bClip = true at creation

    // count outline number label - ? no expansion into map
    // always counts all of number-ish label
    if (bHasNumbering) // count words in numbering string
    {
        LanguageType aLanguage = GetLang( 0 );

        SwScanner aScanner( *this, sNumString, &aLanguage, ModelToViewHelper(),
                            i18n::WordType::WORD_COUNT, 0, sNumString.getLength(), true );

        while ( aScanner.NextWord() )
        {
            ++nTmpWords;
            const OUString &rWord = aScanner.GetWord();
            if (g_pBreakIt->GetBreakIter()->getScriptType(rWord, 0) == i18n::ScriptType::ASIAN)
                ++nTmpAsianWords;
            nTmpCharsExcludingSpaces += g_pBreakIt->getGraphemeCount(rWord);
        }

        nTmpCharsExcludingSpaces += aScanner.getOverriddenDashCount();
        nTmpChars += g_pBreakIt->getGraphemeCount(sNumString);
    }
    else if ( bHasBullet )
    {
        ++nTmpWords;
        ++nTmpChars;
        ++nTmpCharsExcludingSpaces;
    }

    // If counting the whole para then update cached values and mark clean
    if ( bCountAll )
    {
        m_aParagraphIdleData.nNumberOfWords = nTmpWords;
        m_aParagraphIdleData.nNumberOfAsianWords = nTmpAsianWords;
        m_aParagraphIdleData.nNumberOfChars = nTmpChars;
        m_aParagraphIdleData.nNumberOfCharsExcludingSpaces = nTmpCharsExcludingSpaces;
        SetWordCountDirty( false );
    }
    // accumulate into DocStat record to return the values
    rStat.nWord += nTmpWords;
    rStat.nAsianWord += nTmpAsianWords;
    rStat.nChar += nTmpChars;
    rStat.nCharExcludingSpaces += nTmpCharsExcludingSpaces;

    return true;
}

void SwTextNode::SetWrong( std::unique_ptr<SwWrongList> pNew )
{
    m_aParagraphIdleData.pWrong = std::move(pNew);
}

void SwTextNode::ClearWrong()
{
    m_aParagraphIdleData.pWrong.reset();
}

std::unique_ptr<SwWrongList> SwTextNode::ReleaseWrong()
{
    return std::move(m_aParagraphIdleData.pWrong);
}

SwWrongList* SwTextNode::GetWrong()
{
    return m_aParagraphIdleData.pWrong.get();
}

// #i71360#
const SwWrongList* SwTextNode::GetWrong() const
{
    return m_aParagraphIdleData.pWrong.get();
}

void SwTextNode::SetGrammarCheck( std::unique_ptr<SwGrammarMarkUp> pNew )
{
    m_aParagraphIdleData.pGrammarCheck = std::move(pNew);
}

void SwTextNode::ClearGrammarCheck()
{
    m_aParagraphIdleData.pGrammarCheck.reset();
}

std::unique_ptr<SwGrammarMarkUp> SwTextNode::ReleaseGrammarCheck()
{
    return std::move(m_aParagraphIdleData.pGrammarCheck);
}

SwGrammarMarkUp* SwTextNode::GetGrammarCheck()
{
    return m_aParagraphIdleData.pGrammarCheck.get();
}

SwWrongList const* SwTextNode::GetGrammarCheck() const
{
    return static_cast<SwWrongList const*>(const_cast<SwTextNode*>(this)->GetGrammarCheck());
}

void SwTextNode::SetSmartTags( std::unique_ptr<SwWrongList> pNew )
{
    OSL_ENSURE( !pNew || SwSmartTagMgr::Get().IsSmartTagsEnabled(),
            "Weird - we have a smart tag list without any recognizers?" );

    m_aParagraphIdleData.pSmartTags = std::move(pNew);
}

void SwTextNode::ClearSmartTags()
{
    m_aParagraphIdleData.pSmartTags.reset();
}

std::unique_ptr<SwWrongList> SwTextNode::ReleaseSmartTags()
{
    return std::move(m_aParagraphIdleData.pSmartTags);
}

SwWrongList* SwTextNode::GetSmartTags()
{
    return m_aParagraphIdleData.pSmartTags.get();
}

SwWrongList const* SwTextNode::GetSmartTags() const
{
    return const_cast<SwWrongList const*>(const_cast<SwTextNode*>(this)->GetSmartTags());
}

void SwTextNode::SetWordCountDirty( bool bNew ) const
{
    m_aParagraphIdleData.bWordCountDirty = bNew;
}

bool SwTextNode::IsWordCountDirty() const
{
    return m_aParagraphIdleData.bWordCountDirty;
}

void SwTextNode::SetWrongDirty(sw::WrongState eNew) const
{
    m_aParagraphIdleData.eWrongDirty = eNew;
}

sw::WrongState SwTextNode::GetWrongDirty() const
{
    return m_aParagraphIdleData.eWrongDirty;
}

bool SwTextNode::IsWrongDirty() const
{
    return m_aParagraphIdleData.eWrongDirty != sw::WrongState::DONE;
}

void SwTextNode::SetGrammarCheckDirty( bool bNew ) const
{
    m_aParagraphIdleData.bGrammarCheckDirty = bNew;
}

bool SwTextNode::IsGrammarCheckDirty() const
{
    return m_aParagraphIdleData.bGrammarCheckDirty;
}

void SwTextNode::SetSmartTagDirty( bool bNew ) const
{
    m_aParagraphIdleData.bSmartTagDirty = bNew;
}

bool SwTextNode::IsSmartTagDirty() const
{
    return m_aParagraphIdleData.bSmartTagDirty;
}

void SwTextNode::SetAutoCompleteWordDirty( bool bNew ) const
{
    m_aParagraphIdleData.bAutoComplDirty = bNew;
}

bool SwTextNode::IsAutoCompleteWordDirty() const
{
    return m_aParagraphIdleData.bAutoComplDirty;
}

// <-- Paragraph statistics end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

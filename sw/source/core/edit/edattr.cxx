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
#include <hintids.hxx>
#include <editeng/tstpitem.hxx>
#include <editeng/lrspitem.hxx>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <osl/diagnose.h>
#include <txatbase.hxx>
#include <txtftn.hxx>
#include <fmtftn.hxx>
#include <editsh.hxx>
#include <edimp.hxx>
#include <doc.hxx>
#include <swundo.hxx>
#include <ndtxt.hxx>
#include <ftnidx.hxx>
#include <expfld.hxx>
#include <rootfrm.hxx>
#include <cntfrm.hxx>
#include <breakit.hxx>
#include <fmtfld.hxx>
#include <txtfrm.hxx>
#include <scriptinfo.hxx>
#include <svl/itemiter.hxx>
#include <svl/languageoptions.hxx>
#include <charfmt.hxx>
#include <numrule.hxx>

/*
 * hard Formatting (Attributes)
 */

// if selection is bigger as max nodes or more than max selections
// => no attributes
static sal_uInt16 getMaxLookup()
{
    return 10000;
}

bool SwEditShell::GetPaMAttr( SwPaM* pPaM, SfxItemSet& rSet,
                              const bool bMergeIndentValuesOfNumRule ) const
{
    // ??? pPaM can be different from the Cursor ???
    if( GetCursorCnt() > getMaxLookup() )
    {
        rSet.ClearItem();
        return false;
    }

    SfxItemSet aSet( *rSet.GetPool(), rSet.GetRanges() );
    SfxItemSet *pSet = &rSet;

    for(SwPaM& rCurrentPaM : pPaM->GetRingContainer())
    {
        // #i27615# if the cursor is in front of the numbering label
        // the attributes to get are those from the numbering format.
        if (rCurrentPaM.IsInFrontOfLabel())
        {
            SwTextNode const*const pTextNd = sw::GetParaPropsNode(*GetLayout(),
                    rCurrentPaM.GetPoint()->GetNode());

            if (pTextNd)
            {
                SwNumRule * pNumRule = pTextNd->GetNumRule();

                if (pNumRule)
                {
                    int nListLevel = pTextNd->GetActualListLevel();

                    if (nListLevel < 0)
                        nListLevel = 0;

                    if (nListLevel >= MAXLEVEL)
                        nListLevel = MAXLEVEL - 1;

                    const UIName aCharFormatName(
                        pNumRule->Get(o3tl::narrowing<sal_uInt16>(nListLevel)).GetCharFormatName() );
                    SwCharFormat * pCharFormat =
                        GetDoc()->FindCharFormatByName(aCharFormatName);

                    if (pCharFormat)
                        rSet.Put(pCharFormat->GetAttrSet());
                }
            }

            continue;
        }

        SwNodeOffset nSttNd = rCurrentPaM.Start()->GetNodeIndex(),
                     nEndNd = rCurrentPaM.End()->GetNodeIndex();
        sal_Int32 nSttCnt = rCurrentPaM.Start()->GetContentIndex();
        sal_Int32 nEndCnt = rCurrentPaM.End()->GetContentIndex();

        if( sal_Int32(nEndNd - nSttNd) >= getMaxLookup() )
        {
            rSet.ClearItem();
            return false;
        }

        // at first node the node enter his values into the GetSet (Initial)
        // all additional nodes are additional merged to GetSet
        for( SwNodeOffset n = nSttNd; n <= nEndNd; ++n )
        {
            SwNode* pNd = GetDoc()->GetNodes()[ n ];
            switch( pNd->GetNodeType() )
            {
            case SwNodeType::Text:
                {
                    const sal_Int32 nStt = (n == nSttNd) ? nSttCnt : 0;
                    const sal_Int32 nEnd = (n == nEndNd)
                        ? nEndCnt
                        : pNd->GetTextNode()->GetText().getLength();

                    static_cast<SwTextNode*>(pNd)->GetParaAttr(*pSet, nStt, nEnd,
                                                false, true,
                                                bMergeIndentValuesOfNumRule,
                                                GetLayout());
                }
                break;
            case SwNodeType::Grf:
            case SwNodeType::Ole:
                static_cast<SwContentNode*>(pNd)->GetAttr( *pSet );
                break;

            default:
                pNd = nullptr;
            }

            if( pNd )
            {
                if( pSet != &rSet )
                {
                    if (!GetLayout()->HasMergedParas()
                        || pNd->GetRedlineMergeFlag() != SwNode::Merge::Hidden)
                    {
                        rSet.MergeValues( aSet );
                    }
                }

                if( aSet.Count() )
                    aSet.ClearItem();
            }
            pSet = &aSet;
        }

    }

    return true;
}

bool SwEditShell::GetCurAttr( SfxItemSet& rSet,
                              const bool bMergeIndentValuesOfNumRule ) const
{
    return GetPaMAttr( GetCursor(), rSet, bMergeIndentValuesOfNumRule );
}

void SwEditShell::GetCurParAttr( SfxItemSet& rSet) const
{
    GetPaMParAttr( GetCursor(), rSet );
}

void SwEditShell::GetPaMParAttr( SwPaM* pPaM, SfxItemSet& rSet ) const
{
    // number of nodes the function has explored so far
    sal_uInt16 numberOfLookup = 0;

    SfxItemSet aSet( *rSet.GetPool(), rSet.GetRanges() );
    SfxItemSet* pSet = &rSet;

    for(SwPaM& rCurrentPaM : pPaM->GetRingContainer())
    { // for all the point and mark (selections)

        // get the start and the end node of the current selection
        SwNodeOffset nSttNd = rCurrentPaM.GetMark()->GetNodeIndex(),
              nEndNd = rCurrentPaM.GetPoint()->GetNodeIndex();

        // reverse start and end if there number aren't sorted correctly
        if( nSttNd > nEndNd )
            std::swap(nSttNd, nEndNd);

        // for all the nodes in the current selection
        // get the node (paragraph) attributes
        // and merge them in rSet
        for( SwNodeOffset n = nSttNd; n <= nEndNd; ++n )
        {
            // get the node
            SwNode* pNd = GetDoc()->GetNodes()[ n ];

            if (GetLayout()->HasMergedParas()
                && pNd->GetRedlineMergeFlag() == SwNode::Merge::Hidden)
            {
                continue;
            }

            if( pNd->IsTextNode() )
            {
                // get the node (paragraph) attributes
                sw::GetAttrMerged(*pSet, *pNd->GetTextNode(), GetLayout());

                if( pSet != &rSet && aSet.Count() )
                {
                    rSet.MergeValues( aSet );
                    aSet.ClearItem();
                }

                pSet = &aSet;
            }

            ++numberOfLookup;

            // if the maximum number of node that can be inspected has been reached
            if (numberOfLookup >= getMaxLookup())
                return;
        }
    }
}

SwTextFormatColl* SwEditShell::GetCurTextFormatColl( ) const
{
    return GetPaMTextFormatColl( GetCursor() );
}

SwTextFormatColl* SwEditShell::GetPaMTextFormatColl( SwPaM* pPaM ) const
{
    // number of nodes the function have explored so far
    sal_uInt16 numberOfLookup = 0;

    for(SwPaM& rCurrentPaM : pPaM->GetRingContainer())
    { // for all the point and mark (selections)

        // get the start and the end node of the current selection
        SwNodeOffset nSttNd = rCurrentPaM.Start()->GetNodeIndex(),
                     nEndNd = rCurrentPaM.End()->GetNodeIndex();

        // for all the nodes in the current Point and Mark
        for( SwNodeOffset n = nSttNd; n <= nEndNd; ++n )
        {
            // get the node
            SwNode* pNd = GetDoc()->GetNodes()[ n ];

            ++numberOfLookup;

            // if the maximum number of node that can be inspected has been reached
            if (numberOfLookup >= getMaxLookup())
                return nullptr;

            if( pNd->IsTextNode() )
            {
                SwTextNode *const pTextNode(sw::GetParaPropsNode(*GetLayout(), *pNd));
                // if it's a text node get its named paragraph format
                SwTextFormatColl *const pFormat = pTextNode->GetTextColl();

                // if the paragraph format exist stop here and return it
                if( pFormat != nullptr )
                    return pFormat;
            }
        }
    }

    // if none of the selected node contain a named paragraph format
    return nullptr;
}

std::vector<std::pair< const SfxPoolItem*, std::unique_ptr<SwPaM> >> SwEditShell::GetItemWithPaM( sal_uInt16 nWhich )
{
    assert(isCHRATR(nWhich)); // sw_redlinehide: only thing that works
    std::vector<std::pair< const SfxPoolItem*, std::unique_ptr<SwPaM> >> vItem;
    for(SwPaM& rCurrentPaM : GetCursor()->GetRingContainer())
    { // for all the point and mark (selections)

        // get the start and the end node of the current selection
        SwNodeOffset nSttNd = rCurrentPaM.Start()->GetNodeIndex(),
              nEndNd = rCurrentPaM.End()->GetNodeIndex();
        sal_Int32 nSttCnt = rCurrentPaM.Start()->GetContentIndex();
        sal_Int32 nEndCnt = rCurrentPaM.End()->GetContentIndex();

        const SfxPoolItem* pItem = nullptr;

        // for all the nodes in the current selection
        for( SwNodeOffset n = nSttNd; n <= nEndNd; ++n )
        {
            SwNode* pNd = GetDoc()->GetNodes()[ n ];
            if( pNd->IsTextNode() )
            {
                SwTextNode* pTextNd = static_cast< SwTextNode* >( pNd );
                const sal_Int32 nStt = (n == nSttNd) ? nSttCnt : 0;
                const sal_Int32 nEnd = (n == nEndNd)
                    ? nEndCnt : pTextNd->GetText().getLength();
                SwTextFrame const* pFrame;
                const SwScriptInfo *const pScriptInfo =
                    SwScriptInfo::GetScriptInfo(*pTextNd, &pFrame);
                TextFrameIndex const iStt(pScriptInfo
                        ? pFrame->MapModelToView(pTextNd, nStt)
                        : TextFrameIndex(-1/*invalid, do not use*/));
                sal_uInt8 nScript = pScriptInfo
                    ? pScriptInfo->ScriptType(iStt)
                    : css::i18n::ScriptType::WEAK;
                nWhich = GetWhichOfScript( nWhich, nScript );

                // item from attribute set
                if( pTextNd->HasSwAttrSet() )
                {
                    pItem = pTextNd->GetSwAttrSet().GetItem( nWhich );
                    vItem.emplace_back( pItem, std::make_unique<SwPaM>(*pNd, nStt, *pNd, nEnd) );
                }

                if( !pTextNd->HasHints() )
                    continue;

                // items with limited range
                const size_t nSize = pTextNd->GetpSwpHints()->Count();
                for( size_t m = 0; m < nSize; m++ )
                {
                    const SwTextAttr* pHt = pTextNd->GetpSwpHints()->Get(m);
                    if( pHt->Which() == RES_TXTATR_AUTOFMT ||
                        pHt->Which() == RES_TXTATR_CHARFMT ||
                        pHt->Which() == RES_TXTATR_INETFMT )
                    {
                        const sal_Int32 nAttrStart = pHt->GetStart();
                        const sal_Int32* pAttrEnd = pHt->End();

                        // Ignore items not in selection
                        if( nAttrStart > nEnd )
                            break;
                        if( *pAttrEnd <= nStt )
                            continue;

                        nScript = pScriptInfo
                            ? pScriptInfo->ScriptType(iStt)
                            : css::i18n::ScriptType::WEAK;
                        nWhich = GetWhichOfScript( nWhich, nScript );
                        const SfxItemSet* pAutoSet = CharFormat::GetItemSet( pHt->GetAttr() );
                        if( pAutoSet )
                        {
                            SfxItemIter aItemIter( *pAutoSet );
                            pItem = aItemIter.GetCurItem();
                            while( pItem )
                            {
                                if( pItem->Which() == nWhich )
                                {
                                    sal_Int32 nStart = 0, nStop = 0;
                                    if( nAttrStart < nStt ) // Attribute starts before selection
                                        nStart = nStt;
                                    else
                                        nStart = nAttrStart;
                                    if( *pAttrEnd > nEnd ) // Attribute ends after selection
                                        nStop = nEnd;
                                    else
                                        nStop = *pAttrEnd;
                                    vItem.emplace_back( pItem, std::make_unique<SwPaM>(*pNd, nStart, *pNd, nStop) );
                                    break;
                                }
                                pItem = aItemIter.NextItem();
                            }
                            // default item
                            if( !pItem && !pTextNd->HasSwAttrSet() )
                            {
                                pItem = pAutoSet->GetPool()->GetUserDefaultItem( nWhich );
                                vItem.emplace_back( pItem,  std::make_unique<SwPaM>(*pNd, nStt, *pNd, nEnd) );
                            }
                        }
                    }
                }
            }
        }
    }
    return vItem;
}

bool SwEditShell::GetCurFootnote( SwFormatFootnote* pFillFootnote )
{
    // The cursor must be positioned on the current footnotes anchor:
    SwPaM* pCursor = GetCursor();
    SwTextNode* pTextNd = pCursor->GetPointNode().GetTextNode();
    if( !pTextNd )
        return false;

    SwTextAttr *const pFootnote = pTextNd->GetTextAttrForCharAt(
        pCursor->GetPoint()->GetContentIndex(), RES_TXTATR_FTN);
    if( pFootnote && pFillFootnote )
    {
        // Transfer data from the attribute
        const SwFormatFootnote &rFootnote = static_cast<SwTextFootnote*>(pFootnote)->GetFootnote();
        pFillFootnote->SetNumber( rFootnote );
        pFillFootnote->SetEndNote( rFootnote.IsEndNote() );
    }
    return nullptr != pFootnote;
}

bool SwEditShell::SetCurFootnote( const SwFormatFootnote& rFillFootnote )
{
    bool bChgd = false;
    StartAllAction();

    for(const SwPaM& rCursor : GetCursor()->GetRingContainer())
    {
        bChgd |=
            mxDoc->SetCurFootnote(rCursor, rFillFootnote.GetNumStr(), rFillFootnote.IsEndNote());

    }

    EndAllAction();
    return bChgd;
}

bool SwEditShell::HasFootnotes( bool bEndNotes ) const
{
    const SwFootnoteIdxs &rIdxs = mxDoc->GetFootnoteIdxs();
    for ( auto pIdx : rIdxs )
    {
        const SwFormatFootnote &rFootnote = pIdx->GetFootnote();
        if ( bEndNotes == rFootnote.IsEndNote() )
            return true;
    }
    return false;
}

/// Give a List of all footnotes and their beginning texts
size_t SwEditShell::GetSeqFootnoteList( SwSeqFieldList& rList, bool bEndNotes )
{
    rList.Clear();

    IDocumentRedlineAccess & rIDRA(mxDoc->getIDocumentRedlineAccess());

    const size_t nFootnoteCnt = mxDoc->GetFootnoteIdxs().size();
    SwTextFootnote* pTextFootnote;
    for( size_t n = 0; n < nFootnoteCnt; ++n )
    {
        pTextFootnote = mxDoc->GetFootnoteIdxs()[ n ];
        const SwFormatFootnote& rFootnote = pTextFootnote->GetFootnote();
        if ( rFootnote.IsEndNote() != bEndNotes )
            continue;

        const SwNodeIndex* pIdx = pTextFootnote->GetStartNode();
        if( pIdx )
        {
            SwNodeIndex aIdx( *pIdx, 1 );
            SwTextNode* pTextNd = aIdx.GetNode().GetTextNode();
            if( !pTextNd )
                pTextNd = static_cast<SwTextNode*>(SwNodes::GoNext(&aIdx));

            if( pTextNd )
            {
                if (GetLayout()->IsHideRedlines()
                    && sw::IsFootnoteDeleted(rIDRA, *pTextFootnote))
                {
                    continue;
                }

                OUString sText(rFootnote.GetViewNumStr(*mxDoc, GetLayout()));
                if( !sText.isEmpty() )
                    sText += " ";
                sText += pTextNd->GetExpandText(GetLayout());

                SeqFieldLstElem aNew( sText, pTextFootnote->GetSeqRefNo() );
                while( rList.InsertSort( aNew ) )
                    aNew.sDlgEntry += " ";
            }
        }
    }

    return rList.Count();
}

/// Adjust left margin via object bar (similar to adjustment of numerations).
bool SwEditShell::IsMoveLeftMargin( bool bRight, bool bModulus ) const
{
    bool bRet = true;

    constexpr sal_uInt16 constTwips_2cm = o3tl::toTwips(20, o3tl::Length::mm);
    constexpr tools::Long constTwips_5mm = o3tl::toTwips(5, o3tl::Length::mm);

    const SvxTabStopItem& rTabItem = GetDoc()->GetDefault( RES_PARATR_TABSTOP );
    sal_uInt16 nDefDist = o3tl::narrowing<sal_uInt16>(
        rTabItem.Count() ? rTabItem[0].GetTabPos() : constTwips_2cm);

    if( !nDefDist )
        return false;

    for(SwPaM& rPaM : GetCursor()->GetRingContainer())
    {
        SwNodeOffset nSttNd = rPaM.Start()->GetNodeIndex(),
                     nEndNd = rPaM.End()->GetNodeIndex();

        SwContentNode* pCNd;
        for( SwNodeOffset n = nSttNd; bRet && n <= nEndNd; ++n )
        {
            pCNd = GetDoc()->GetNodes()[ n ]->GetTextNode();
            if( nullptr != pCNd )
            {
                pCNd = sw::GetParaPropsNode(*GetLayout(), *pCNd);
                const SvxLRSpaceItem& rLS = pCNd->GetAttr( RES_LR_SPACE );
                if( bRight )
                {
                    tools::Long nNext = rLS.ResolveTextLeft({}) + nDefDist;
                    if( bModulus )
                        nNext = ( nNext / nDefDist ) * nDefDist;
                    SwFrame* pFrame = pCNd->getLayoutFrame( GetLayout() );
                    if ( pFrame )
                    {
                        const SwTwips nFrameWidth = pFrame->IsVertical() ?
                                                 pFrame->getFrameArea().Height() :
                                                 pFrame->getFrameArea().Width();
                        bRet = nFrameWidth > (nNext + constTwips_5mm);
                    }
                    else
                        bRet = false;
                }
            }
        }

        if( !bRet )
            break;

    }
    return bRet;
}

void SwEditShell::MoveLeftMargin( bool bRight, bool bModulus )
{
    StartAllAction();
    StartUndo( SwUndoId::START );

    SwPaM* pCursor = GetCursor();
    if( pCursor->GetNext() != pCursor )         // Multiple selection ?
    {
        SwPamRanges aRangeArr( *pCursor );
        SwPaM aPam( *pCursor->GetPoint() );
        for( size_t n = 0; n < aRangeArr.Count(); ++n )
            GetDoc()->MoveLeftMargin( aRangeArr.SetPam( n, aPam ),
                                        bRight, bModulus, GetLayout() );
    }
    else
        GetDoc()->MoveLeftMargin( *pCursor, bRight, bModulus, GetLayout() );

    EndUndo( SwUndoId::END );
    EndAllAction();
}

static SvtScriptType lcl_SetScriptFlags( sal_uInt16 nType )
{
    switch( nType )
    {
        case css::i18n::ScriptType::LATIN:
            return SvtScriptType::LATIN;
        case css::i18n::ScriptType::ASIAN:
            return SvtScriptType::ASIAN;
        case css::i18n::ScriptType::COMPLEX:
            return SvtScriptType::COMPLEX;
        default:
            return SvtScriptType::NONE;
    }
}

static bool lcl_IsNoEndTextAttrAtPos(SwRootFrame const& rLayout,
        const SwTextNode& rTNd, sal_Int32 const nPos,
                            SvtScriptType &rScrpt, bool bInSelection, bool bNum )
{
    bool bRet = false;
    OUString sExp;

    // consider numbering
    if ( bNum )
    {
        bRet = false;
        SwTextNode const*const pPropsNode(sw::GetParaPropsNode(rLayout, rTNd));
        if (pPropsNode->IsInList())
        {
            OSL_ENSURE( pPropsNode->GetNumRule(),
                    "<lcl_IsNoEndTextAttrAtPos(..)> - no list style found at text node. Serious defect." );
            const SwNumRule* pNumRule = pPropsNode->GetNumRule();
            if(pNumRule)
            {
                int nListLevel = pPropsNode->GetActualListLevel();

                if (nListLevel < 0)
                    nListLevel = 0;

                if (nListLevel >= MAXLEVEL)
                    nListLevel = MAXLEVEL - 1;

                const SwNumFormat &rNumFormat = pNumRule->Get( o3tl::narrowing<sal_uInt16>(nListLevel) );
                if( SVX_NUM_BITMAP != rNumFormat.GetNumberingType() )
                {
                    if ( SVX_NUM_CHAR_SPECIAL == rNumFormat.GetNumberingType() )
                    {
                        sal_UCS4 cBullet = rNumFormat.GetBulletChar();
                        sExp = OUString(&cBullet, 1);
                    }
                    else
                        sExp = pPropsNode->GetNumString(true, MAXLEVEL, &rLayout);
                }
            }
        }
    }

    // and fields
    if (nPos < rTNd.GetText().getLength() && CH_TXTATR_BREAKWORD == rTNd.GetText()[nPos])
    {
        const SwTextAttr* const pAttr = rTNd.GetTextAttrForCharAt( nPos );
        if (pAttr)
        {
            bRet = true; // all other than fields can be
                         // defined as weak-script ?
            if ( RES_TXTATR_FIELD == pAttr->Which() )
            {
                const SwField* const pField = pAttr->GetFormatField().GetField();
                if (pField)
                {
                    sExp += pField->ExpandField(true, &rLayout);
                }
            }
        }
    }

    const sal_Int32 nEnd = sExp.getLength();
    if ( nEnd )
    {
        if( bInSelection )
        {
            sal_uInt16 nScript;
            for( sal_Int32 n = 0; n < nEnd;
                 n = g_pBreakIt->GetBreakIter()->endOfScript( sExp, n, nScript ))
            {
                nScript = g_pBreakIt->GetBreakIter()->getScriptType( sExp, n );
                rScrpt |= lcl_SetScriptFlags( nScript );
            }
        }
        else
            rScrpt |= lcl_SetScriptFlags( g_pBreakIt->GetBreakIter()->
                                        getScriptType( sExp, nEnd-1 ));
    }

    return bRet;
}

/// returns the script type of the selection
SvtScriptType SwEditShell::GetScriptType() const
{
    SvtScriptType nRet = SvtScriptType::NONE;

    {
        for(SwPaM& rPaM : GetCursor()->GetRingContainer())
        {
            auto [pStart, pEnd] = rPaM.StartEnd(); // SwPosition*
            if( pStart == pEnd || *pStart == *pEnd )
            {
                const SwTextNode* pTNd = pStart->GetNode().GetTextNode();
                if( pTNd )
                {
                    // try to get SwScriptInfo
                    SwTextFrame const* pFrame;
                    const SwScriptInfo *const pScriptInfo =
                        SwScriptInfo::GetScriptInfo(*pTNd, &pFrame);

                    sal_Int32 nPos = pStart->GetContentIndex();
                    //Task 90448: we need the scripttype of the previous
                    //              position, if no selection exist!
                    if( nPos )
                    {
                        SwContentIndex aIdx( pStart->GetContentNode(), pStart->GetContentIndex() );
                        if( pTNd->GoPrevious( aIdx, SwCursorSkipMode::Chars ) )
                            nPos = aIdx.GetIndex();
                    }

                    sal_uInt16 nScript;

                    if (!pTNd->GetText().isEmpty())
                    {
                        nScript = pScriptInfo
                            ? pScriptInfo->ScriptType(pFrame->MapModelToView(pTNd, nPos))
                            : g_pBreakIt->GetBreakIter()->getScriptType( pTNd->GetText(), nPos );
                    }
                    else
                        nScript = SvtLanguageOptions::GetI18NScriptTypeOfLanguage( GetAppLanguage() );

                    if (!lcl_IsNoEndTextAttrAtPos(*GetLayout(), *pTNd, nPos, nRet, false, false))
                        nRet |= lcl_SetScriptFlags( nScript );
                }
            }
            else
            {
                SwNodeOffset nEndIdx = pEnd->GetNodeIndex();
                SwNodeIndex aIdx( pStart->GetNode() );
                for( ; aIdx.GetIndex() <= nEndIdx; ++aIdx )
                    if( aIdx.GetNode().IsTextNode() )
                    {
                        const SwTextNode* pTNd = aIdx.GetNode().GetTextNode();
                        const OUString& rText = pTNd->GetText();

                        // try to get SwScriptInfo
                        SwTextFrame const* pFrame;
                        const SwScriptInfo *const pScriptInfo =
                            SwScriptInfo::GetScriptInfo(*pTNd, &pFrame);

                        sal_Int32 nChg = aIdx == pStart->GetNode()
                                                ? pStart->GetContentIndex()
                                                : 0;
                        sal_Int32 nEndPos = aIdx == nEndIdx
                                                ? pEnd->GetContentIndex()
                                                : rText.getLength();

                        OSL_ENSURE( nEndPos <= rText.getLength(),
                                "Index outside the range - endless loop!" );
                        if (nEndPos > rText.getLength())
                            nEndPos = rText.getLength();

                        bool const isUntilEnd(pScriptInfo
                            ? pFrame->MapViewToModelPos(TextFrameIndex(pFrame->GetText().getLength())) <= *pEnd
                            : rText.getLength() == nEndPos);
                        sal_uInt16 nScript;
                        while( nChg < nEndPos )
                        {
                            TextFrameIndex iChg(pScriptInfo
                                    ? pFrame->MapModelToView(pTNd, nChg)
                                    : TextFrameIndex(-1/*invalid, do not use*/));
                            nScript = pScriptInfo ?
                                      pScriptInfo->ScriptType( iChg ) :
                                      g_pBreakIt->GetBreakIter()->getScriptType(
                                                                rText, nChg );

                            if (!lcl_IsNoEndTextAttrAtPos(*GetLayout(), *pTNd, nChg, nRet, true,
                                      TextFrameIndex(0) == iChg && isUntilEnd))
                            {
                                nRet |= lcl_SetScriptFlags( nScript );
                            }

                            if( (SvtScriptType::LATIN | SvtScriptType::ASIAN |
                                SvtScriptType::COMPLEX) == nRet )
                                break;

                            sal_Int32 nFieldPos = nChg+1;

                            if (pScriptInfo)
                            {
                                iChg = pScriptInfo->NextScriptChg(iChg);
                                if (iChg == TextFrameIndex(COMPLETE_STRING))
                                {
                                    nChg = pTNd->Len();
                                }
                                else
                                {
                                    std::pair<SwTextNode*, sal_Int32> const tmp(
                                        pFrame->MapViewToModel(iChg));
                                    nChg = (tmp.first == pTNd)
                                        ? tmp.second
                                        : pTNd->Len();
                                }
                            }
                            else
                            {
                                nChg = g_pBreakIt->GetBreakIter()->endOfScript(
                                                    rText, nChg, nScript );
                            }

                            nFieldPos = rText.indexOf(
                                            CH_TXTATR_BREAKWORD, nFieldPos);
                            if ((-1 != nFieldPos) && (nFieldPos < nChg))
                                nChg = nFieldPos;
                        }
                        if( (SvtScriptType::LATIN | SvtScriptType::ASIAN |
                                SvtScriptType::COMPLEX) == nRet )
                            break;
                    }
            }
            if( (SvtScriptType::LATIN | SvtScriptType::ASIAN |
                                SvtScriptType::COMPLEX) == nRet )
                break;

        }
    }
    if( nRet == SvtScriptType::NONE )
        nRet = SvtLanguageOptions::GetScriptTypeOfLanguage( LANGUAGE_SYSTEM );
    return nRet;
}

LanguageType SwEditShell::GetCurLang() const
{
    const SwPaM* pCursor = GetCursor();
    const SwPosition& rPos = *pCursor->GetPoint();
    const SwTextNode* pTNd = rPos.GetNode().GetTextNode();
    LanguageType nLang;
    if( pTNd )
    {
        //JP 24.9.2001: if exist no selection, then get the language before
        //              the current character!
        sal_Int32 nPos = rPos.GetContentIndex();
        if( nPos && !pCursor->HasMark() )
            --nPos;
        nLang = pTNd->GetLang( nPos );
    }
    else
        nLang = LANGUAGE_DONTKNOW;
    return nLang;
}

sal_uInt16 SwEditShell::GetScalingOfSelectedText() const
{
    const SwPaM* pCursor = GetCursor();
    const SwPosition* pStart = pCursor->Start();
    const SwTextNode* pTNd = pStart->GetNode().GetTextNode();
    OSL_ENSURE( pTNd, "no textnode available" );

    sal_uInt16 nScaleWidth;
    if( pTNd )
    {
        SwTextFrame *const pFrame(static_cast<SwTextFrame *>(
                    pTNd->getLayoutFrame(GetLayout(), pStart)));
        assert(pFrame); // shell cursor must be positioned in node with frame
        TextFrameIndex const nStart(pFrame->MapModelToViewPos(*pStart));
        TextFrameIndex const nEnd(
            sw::FrameContainsNode(*pFrame, pCursor->End()->GetNodeIndex())
                ? pFrame->MapModelToViewPos(*pCursor->End())
                : TextFrameIndex(pFrame->GetText().getLength()));
        nScaleWidth = pFrame->GetScalingOfSelectedText(nStart, nEnd);
    }
    else
        nScaleWidth = 100;              // default are no scaling -> 100%
    return nScaleWidth;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

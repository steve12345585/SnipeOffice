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

#include <algorithm>

#include <com/sun/star/i18n/ScriptType.hpp>

#include <editeng/langitem.hxx>
#include <osl/diagnose.h>
#include <svl/languageoptions.hxx>
#include <vcl/commandevent.hxx>

#include <hintids.hxx>
#include <extinput.hxx>
#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <contentindex.hxx>
#include <ndtxt.hxx>
#include <swundo.hxx>

using namespace ::com::sun::star;

SwExtTextInput::SwExtTextInput( const SwPaM& rPam, Ring* pRing )
    : SwPaM( *rPam.GetPoint(), static_cast<SwPaM*>(pRing) ),
    m_eInputLanguage(LANGUAGE_DONTKNOW)
{
    m_bIsOverwriteCursor = false;
    m_bInsText = true;
}

SwExtTextInput::~SwExtTextInput()
{
    SwDoc& rDoc = GetDoc();
    if (rDoc.IsInDtor()) { return; /* #i58606# */ }

    SwTextNode* pTNd = GetPoint()->GetNode().GetTextNode();
    if( !pTNd )
        return;

    SwPosition& rPtPos = *GetPoint();
    sal_Int32 nSttCnt = rPtPos.GetContentIndex();
    sal_Int32 nEndCnt = GetMark()->GetContentIndex();
    if( nEndCnt == nSttCnt )
        return;

    // Prevent IME edited text being grouped with non-IME edited text.
    bool bKeepGroupUndo = rDoc.GetIDocumentUndoRedo().DoesGroupUndo();
    bool bWasIME = rDoc.GetIDocumentUndoRedo().GetUndoActionCount() == 0 || rDoc.getIDocumentContentOperations().GetIME();
    if (!bWasIME)
    {
        rDoc.GetIDocumentUndoRedo().DoGroupUndo(false);
    }
    rDoc.getIDocumentContentOperations().SetIME(true);
    if( nEndCnt < nSttCnt )
    {
        std::swap(nSttCnt, nEndCnt);
    }

    // In order to get Undo/Redlining etc. working correctly,
    // we need to go through the Doc interface
    rPtPos.SetContent(nSttCnt);
    const OUString sText( pTNd->GetText().copy(nSttCnt, nEndCnt - nSttCnt));
    if( m_bIsOverwriteCursor && !m_sOverwriteText.isEmpty() )
    {
        const sal_Int32 nLen = sText.getLength();
        const sal_Int32 nOWLen = m_sOverwriteText.getLength();
        if( nLen > nOWLen )
        {
            rPtPos.AdjustContent(+nOWLen);
            pTNd->EraseText( rPtPos, nLen - nOWLen );
            rPtPos.SetContent(nSttCnt);
            pTNd->ReplaceText( rPtPos, nOWLen, m_sOverwriteText );
            if( m_bInsText )
            {
                rPtPos.SetContent(nSttCnt);
                rDoc.GetIDocumentUndoRedo().StartUndo( SwUndoId::OVERWRITE, nullptr );
                rDoc.getIDocumentContentOperations().Overwrite( *this, sText.copy( 0, nOWLen ) );
                rDoc.getIDocumentContentOperations().InsertString( *this, sText.copy( nOWLen ) );
                rDoc.GetIDocumentUndoRedo().EndUndo( SwUndoId::OVERWRITE, nullptr );
            }
        }
        else
        {
            pTNd->ReplaceText( rPtPos, nLen, m_sOverwriteText.copy( 0, nLen ));
            if( m_bInsText )
            {
                rPtPos.SetContent(nSttCnt);
                rDoc.getIDocumentContentOperations().Overwrite( *this, sText );
            }
        }
    }
    else
    {
        pTNd->EraseText( rPtPos, nEndCnt - nSttCnt );

        if( m_bInsText )
        {
            rDoc.getIDocumentContentOperations().InsertString(*this, sText);
        }
    }
    if (!bWasIME)
    {
        rDoc.GetIDocumentUndoRedo().DoGroupUndo(bKeepGroupUndo);
    }
    if (m_eInputLanguage == LANGUAGE_DONTKNOW)
        return;

    sal_uInt16 nWhich = RES_CHRATR_LANGUAGE;
    sal_Int16 nScriptType = SvtLanguageOptions::GetI18NScriptTypeOfLanguage(m_eInputLanguage);
    switch(nScriptType)
    {
        case  i18n::ScriptType::ASIAN:
            nWhich = RES_CHRATR_CJK_LANGUAGE; break;
        case  i18n::ScriptType::COMPLEX:
            nWhich = RES_CHRATR_CTL_LANGUAGE; break;
    }
    // #i41974# Only set language attribute for CJK/CTL scripts.
    if (RES_CHRATR_LANGUAGE != nWhich && pTNd->GetLang( nSttCnt, nEndCnt-nSttCnt, nScriptType) != m_eInputLanguage)
    {
        SvxLanguageItem aLangItem( m_eInputLanguage, nWhich );
        rPtPos.SetContent(nSttCnt);
        GetMark()->SetContent(nEndCnt);
        rDoc.getIDocumentContentOperations().InsertPoolItem(*this, aLangItem );
    }
}

void SwExtTextInput::SetInputData( const CommandExtTextInputData& rData )
{
    SwTextNode* pTNd = GetPoint()->GetNode().GetTextNode();
    if( !pTNd )
        return;

    sal_Int32 nSttCnt = Start()->GetContentIndex();
    sal_Int32 nEndCnt = End()->GetContentIndex();

    SwContentIndex aIdx( pTNd, nSttCnt );
    const OUString& rNewStr = rData.GetText();

    if( m_bIsOverwriteCursor && !m_sOverwriteText.isEmpty() )
    {
        sal_Int32 nReplace = nEndCnt - nSttCnt;
        const sal_Int32 nNewLen = rNewStr.getLength();
        if( nNewLen < nReplace )
        {
            // We have to insert some characters from the saved original text
            nReplace -= nNewLen;
            aIdx += nNewLen;
            pTNd->ReplaceText( aIdx, nReplace,
                        m_sOverwriteText.copy( nNewLen, nReplace ));
            aIdx = nSttCnt;
            nReplace = nNewLen;
        }
        else
        {
            const sal_Int32 nOWLen = m_sOverwriteText.getLength();
            if( nOWLen < nReplace )
            {
                aIdx += nOWLen;
                pTNd->EraseText( aIdx, nReplace-nOWLen );
                aIdx = nSttCnt;
                nReplace = nOWLen;
            }
            else
            {
                nReplace = std::min(nOWLen, nNewLen);
            }
        }

        pTNd->ReplaceText( aIdx, nReplace, rNewStr );
        if( !HasMark() )
            SetMark();
        GetMark()->Assign(*aIdx.GetContentNode(), aIdx.GetIndex());
    }
    else
    {
        if( nSttCnt < nEndCnt )
        {
            pTNd->EraseText( aIdx, nEndCnt - nSttCnt );
        }

        pTNd->InsertText(rNewStr, aIdx);
        if( !HasMark() )
            SetMark();
    }

    GetPoint()->SetContent(nSttCnt);

    m_aAttrs.clear();
    if( rData.GetTextAttr() )
    {
        const ExtTextInputAttr *pAttrs = rData.GetTextAttr();
        m_aAttrs.insert( m_aAttrs.begin(), pAttrs, pAttrs + rData.GetText().getLength() );
    }
}

void SwExtTextInput::SetOverwriteCursor( bool bFlag )
{
    m_bIsOverwriteCursor = bFlag;
    if (!m_bIsOverwriteCursor)
        return;

    const SwTextNode *const pTNd = GetPoint()->GetNode().GetTextNode();
    if (!pTNd)
        return;

    const sal_Int32 nSttCnt = GetPoint()->GetContentIndex();
    const sal_Int32 nEndCnt = GetMark()->GetContentIndex();
    m_sOverwriteText = pTNd->GetText().copy( std::min(nSttCnt, nEndCnt) );
    if( m_sOverwriteText.isEmpty() )
        return;

    const sal_Int32 nInPos = m_sOverwriteText.indexOf( CH_TXTATR_INWORD );
    const sal_Int32 nBrkPos = m_sOverwriteText.indexOf( CH_TXTATR_BREAKWORD );

    // Find the first attr found, if any.
    sal_Int32 nPos = std::min(nInPos, nBrkPos);
    if (nPos<0)
    {
        nPos = std::max(nInPos, nBrkPos);
    }
    if (nPos>=0)
    {
        m_sOverwriteText = m_sOverwriteText.copy( 0, nPos );
    }
}

// The Doc interfaces

SwExtTextInput* SwDoc::CreateExtTextInput( const SwPaM& rPam )
{
    SwExtTextInput* pNew = new SwExtTextInput( rPam, mpExtInputRing );
    if( !mpExtInputRing )
        mpExtInputRing = pNew;
    pNew->SetMark();
    return pNew;
}

void SwDoc::DeleteExtTextInput( SwExtTextInput* pDel )
{
    if( pDel == mpExtInputRing )
    {
        if( pDel->GetNext() != mpExtInputRing )
            mpExtInputRing = pDel->GetNext();
        else
            mpExtInputRing = nullptr;
    }
    delete pDel;
}

SwExtTextInput* SwDoc::GetExtTextInput( const SwNode& rNd,
                                        sal_Int32 nContentPos ) const
{
    SwExtTextInput* pRet = nullptr;
    if( mpExtInputRing )
    {
        SwNodeOffset nNdIdx = rNd.GetIndex();
        SwExtTextInput* pTmp = mpExtInputRing;
        do {
            SwNodeOffset nStartNode = pTmp->Start()->GetNodeIndex(),
                         nEndNode = pTmp->End()->GetNodeIndex();
            sal_Int32 nStartCnt = pTmp->Start()->GetContentIndex();
            sal_Int32 nEndCnt = pTmp->End()->GetContentIndex();

            if( nStartNode <= nNdIdx && nNdIdx <= nEndNode &&
                ( nContentPos<0 ||
                    ( nStartCnt <= nContentPos && nContentPos <= nEndCnt )))
            {
                pRet = pTmp;
                break;
            }
            pTmp = pTmp->GetNext();
        } while ( pTmp!=mpExtInputRing );
    }
    return pRet;
}

SwExtTextInput* SwDoc::GetExtTextInput() const
{
    OSL_ENSURE( !mpExtInputRing || !mpExtInputRing->IsMultiSelection(),
            "more than one InputEngine available" );
    return mpExtInputRing;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

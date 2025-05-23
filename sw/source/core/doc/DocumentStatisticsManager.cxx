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
#include <DocumentStatisticsManager.hxx>
#include <doc.hxx>
#include <fldbas.hxx>
#include <docsh.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <IDocumentState.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <view.hxx>
#include <ndtxt.hxx>
#include <fmtfld.hxx>
#include <rootfrm.hxx>
#include <docufld.hxx>
#include <docstat.hxx>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/frame/XModel.hpp>

using namespace ::com::sun::star;

namespace sw
{

DocumentStatisticsManager::DocumentStatisticsManager( SwDoc& i_rSwdoc )
    : m_rDoc( i_rSwdoc ),
    mpDocStat( new SwDocStat ),
    mbInitialized( false ),
    maStatsUpdateIdle( i_rSwdoc, "sw::DocumentStatisticsManager maStatsUpdateIdle" )
{
    maStatsUpdateIdle.SetPriority( TaskPriority::LOWEST );
    maStatsUpdateIdle.SetInvokeHandler( LINK( this, DocumentStatisticsManager, DoIdleStatsUpdate ) );
}

void DocumentStatisticsManager::DocInfoChgd(bool const isEnableSetModified)
{
    m_rDoc.getIDocumentFieldsAccess().GetSysFieldType( SwFieldIds::DocInfo )->UpdateFields();
    m_rDoc.getIDocumentFieldsAccess().GetSysFieldType( SwFieldIds::TemplateName )->UpdateFields();
    if (isEnableSetModified)
    {
        m_rDoc.getIDocumentState().SetModified();
    }
}

const SwDocStat& DocumentStatisticsManager::GetDocStat() const
{
    return *mpDocStat;
}

void DocumentStatisticsManager::SetDocStatModified(bool bSet)
{
    mpDocStat->bModified = bSet;
}

const SwDocStat& DocumentStatisticsManager::GetUpdatedDocStat( bool bCompleteAsync, bool bFields )
{
    if( mpDocStat->bModified || !mbInitialized)
    {
        UpdateDocStat( bCompleteAsync, bFields );
    }
    return *mpDocStat;
}

void DocumentStatisticsManager::SetDocStat( const SwDocStat& rStat )
{
    *mpDocStat = rStat;
    mbInitialized = true;
}

void DocumentStatisticsManager::UpdateDocStat( bool bCompleteAsync, bool bFields )
{
    if( !mpDocStat->bModified && mbInitialized)
        return;

    if (!bCompleteAsync)
    {
        maStatsUpdateIdle.Stop();
        while (IncrementalDocStatCalculate(
                    std::numeric_limits<tools::Long>::max(), bFields)) {}
    }
    else if (IncrementalDocStatCalculate(5000, bFields))
        maStatsUpdateIdle.Start();
    else
        maStatsUpdateIdle.Stop();
}

// returns true while there is more to do
bool DocumentStatisticsManager::IncrementalDocStatCalculate(tools::Long nChars, bool bFields)
{
    mbInitialized = true;
    mpDocStat->Reset();
    mpDocStat->nPara = 0; // default is 1!

    // This is the inner loop - at least while the paras are dirty.
    for( SwNodeOffset i = m_rDoc.GetNodes().Count(); i > SwNodeOffset(0) && nChars > 0; )
    {
        SwNode* pNd = m_rDoc.GetNodes()[ --i ];
        switch( pNd->GetNodeType() )
        {
        case SwNodeType::Text:
        {
            tools::Long const nOldChars(mpDocStat->nChar);
            SwTextNode *pText = static_cast< SwTextNode * >( pNd );
            if (pText->CountWords(*mpDocStat, 0, pText->GetText().getLength()))
            {
                nChars -= (mpDocStat->nChar - nOldChars);
            }
            break;
        }
        case SwNodeType::Table:      ++mpDocStat->nTable;   break;
        case SwNodeType::Grf:        ++mpDocStat->nGrf;   break;
        case SwNodeType::Ole:        ++mpDocStat->nOLE;   break;
        case SwNodeType::Section:    break;
        default: break;
        }
    }

    // #i93174#: notes contain paragraphs that are not nodes
    {
        SwFieldType * const pPostits( m_rDoc.getIDocumentFieldsAccess().GetSysFieldType(SwFieldIds::Postit) );
        std::vector<SwFormatField*> vFields;
        pPostits->GatherFields(vFields);
        for(auto pFormatField : vFields)
        {
            const auto pField = static_cast<SwPostItField const*>(pFormatField->GetField());
            mpDocStat->nAllPara += pField->GetNumberOfParagraphs();
        }
    }

    mpDocStat->nPage     = m_rDoc.getIDocumentLayoutAccess().GetCurrentLayout() ? m_rDoc.getIDocumentLayoutAccess().GetCurrentLayout()->GetPageNum() : 0;
    SetDocStatModified( false );

    css::uno::Sequence < css::beans::NamedValue > aStat( mpDocStat->nPage ? 8 : 7);
    auto pStat = aStat.getArray();
    sal_Int32 n=0;
    pStat[n].Name = "TableCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nTable);
    pStat[n].Name = "ImageCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nGrf);
    pStat[n].Name = "ObjectCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nOLE);
    if ( mpDocStat->nPage )
    {
        pStat[n].Name = "PageCount";
        pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nPage);
    }
    pStat[n].Name = "ParagraphCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nPara);
    pStat[n].Name = "WordCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nWord);
    pStat[n].Name = "CharacterCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nChar);
    pStat[n].Name = "NonWhitespaceCharacterCount";
    pStat[n++].Value <<= static_cast<sal_Int32>(mpDocStat->nCharExcludingSpaces);

    // For e.g. autotext documents there is no pSwgInfo (#i79945)
    SwDocShell* pObjShell(m_rDoc.GetDocShell());
    if (pObjShell)
    {
        const uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                pObjShell->GetModel(), uno::UNO_QUERY_THROW);
        const uno::Reference<document::XDocumentProperties> xDocProps(
                xDPS->getDocumentProperties());
        // #i96786#: do not set modified flag when updating statistics
        const bool bDocWasModified( m_rDoc.getIDocumentState().IsModified() );
        const ModifyBlocker_Impl b(pObjShell);
        // rhbz#1081176: don't jump to cursor pos because of (temporary)
        // activation of modified flag triggering move to input position
        auto aViewGuard(pObjShell->LockAllViews());
        xDocProps->setDocumentStatistics(aStat);
        if (!bDocWasModified)
        {
            m_rDoc.getIDocumentState().ResetModified();
        }
    }

    // optionally update stat. fields
    if (bFields)
    {
        SwFieldType *pType = m_rDoc.getIDocumentFieldsAccess().GetSysFieldType(SwFieldIds::DocStat);
        pType->UpdateFields();
    }

    return nChars < 0;
}

IMPL_LINK( DocumentStatisticsManager, DoIdleStatsUpdate, Timer *, pIdle, void )
{
    if (IncrementalDocStatCalculate(32000))
        pIdle->Start();
    SwView* pView = m_rDoc.GetDocShell() ? m_rDoc.GetDocShell()->GetView() : nullptr;
    if( pView )
        pView->UpdateDocStats();
}

DocumentStatisticsManager::~DocumentStatisticsManager()
{
    maStatsUpdateIdle.Stop();
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

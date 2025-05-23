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

#include <fesh.hxx>
#include <hintids.hxx>
#include <hints.hxx>

#include <swwait.hxx>
#include <editsh.hxx>
#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentChartDataProviderAccess.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <IDocumentState.hxx>
#include <cntfrm.hxx>
#include <pam.hxx>
#include <ndtxt.hxx>
#include <swtable.hxx>
#include <swundo.hxx>
#include <tblsel.hxx>
#include <cellfrm.hxx>
#include <cellatr.hxx>
#include <swtblfmt.hxx>
#include <swddetbl.hxx>
#include <mdiexp.hxx>
#include <itabenum.hxx>
#include <svl/numformat.hxx>
#include <vcl/uitest/logger.hxx>
#include <vcl/uitest/eventdescription.hxx>

using namespace ::com::sun::star;
namespace {

void collectUIInformation(const OUString& rAction, const OUString& aParameters)
{
    EventDescription aDescription;
    aDescription.aAction = rAction;
    aDescription.aParameters = {{"parameters", aParameters}};
    aDescription.aID = "writer_edit";
    aDescription.aKeyWord = "SwEditWinUIObject";
    aDescription.aParent = "MainWindow";
    UITestLogger::getInstance().logEvent(aDescription);
}

}

//Added for bug #i119954# Application crashed if undo/redo convert nest table to text
static bool ConvertTableToText( const SwTableNode *pTableNode, sal_Unicode cCh );

static void    ConvertNestedTablesToText( const SwTableLines &rTableLines, sal_Unicode cCh )
{
    for (size_t n = 0; n < rTableLines.size(); ++n)
    {
        SwTableLine* pTableLine = rTableLines[ n ];
        for (size_t i = 0; i < pTableLine->GetTabBoxes().size(); ++i)
        {
            SwTableBox* pTableBox = pTableLine->GetTabBoxes()[ i ];
            if (pTableBox->GetTabLines().empty())
            {
                SwNodeIndex nodeIndex( *pTableBox->GetSttNd(), 1 );
                SwNodeIndex endNodeIndex( *pTableBox->GetSttNd()->EndOfSectionNode() );
                for( ; nodeIndex < endNodeIndex ; ++nodeIndex )
                {
                    if ( SwTableNode* pTableNode = nodeIndex.GetNode().GetTableNode() )
                        ConvertTableToText( pTableNode, cCh );
                }
            }
            else
            {
                ConvertNestedTablesToText( pTableBox->GetTabLines(), cCh );
            }
        }
    }
}

bool ConvertTableToText( const SwTableNode *pConstTableNode, sal_Unicode cCh )
{
    SwTableNode *pTableNode = const_cast< SwTableNode* >( pConstTableNode );
    ConvertNestedTablesToText( pTableNode->GetTable().GetTabLines(), cCh );
    return pTableNode->GetDoc().TableToText( pTableNode, cCh );
}
//End for bug #i119954#

const SwTable& SwEditShell::InsertTable( const SwInsertTableOptions& rInsTableOpts,
                                         sal_uInt16 nRows, sal_uInt16 nCols,
                                         const SwTableAutoFormat* pTAFormat )
{
    StartAllAction();
    SwPosition* pPos = GetCursor()->GetPoint();

    bool bEndUndo = 0 != pPos->GetContentIndex();
    if( bEndUndo )
    {
        StartUndo( SwUndoId::START );
        GetDoc()->getIDocumentContentOperations().SplitNode( *pPos, false );
    }

    // If called from a shell the adjust item is propagated
    // from pPos to the new content nodes in the table.
    const SwTable *pTable = GetDoc()->InsertTable( rInsTableOpts, *pPos,
                                                   nRows, nCols,
                                                   css::text::HoriOrientation::FULL, pTAFormat,
                                                   nullptr, true );
    if( bEndUndo )
        EndUndo( SwUndoId::END );

    EndAllAction();

    OUString parameter = " Columns : " + OUString::number( nCols ) + " , Rows : " + OUString::number( nRows ) + " ";
    collectUIInformation(u"CREATE_TABLE"_ustr, parameter);

    return *pTable;
}

bool SwEditShell::TextToTable( const SwInsertTableOptions& rInsTableOpts,
                               sal_Unicode cCh,
                               const SwTableAutoFormat* pTAFormat )
{
    SwWait aWait( *GetDoc()->GetDocShell(), true );
    bool bRet = false;
    StartAllAction();
    for(const SwPaM& rPaM : GetCursor()->GetRingContainer())
    {
        if( rPaM.HasMark() )
            bRet |= nullptr != GetDoc()->TextToTable( rInsTableOpts, rPaM, cCh,
                                                css::text::HoriOrientation::FULL, pTAFormat );
    }
    EndAllAction();
    return bRet;
}

bool SwEditShell::TableToText( sal_Unicode cCh )
{
    SwWait aWait( *GetDoc()->GetDocShell(), true );
    SwPaM* pCursor = GetCursor();
    const SwTableNode* pTableNd =
            SwDoc::IsInTable( pCursor->GetPoint()->GetNode() );
    if (!pTableNd)
        return false;

    if( IsTableMode() )
    {
        ClearMark();
        pCursor = GetCursor();
    }
    else if (pCursor->GetNext() != pCursor)
        return false;

    // TL_CHART2:
    // tell the charts about the table to be deleted and have them use their own data
    GetDoc()->getIDocumentChartDataProviderAccess().CreateChartInternalDataProviders( &pTableNd->GetTable() );

    StartAllAction();

    // move current Cursor out of the listing area
    SwNodeIndex aTabIdx( *pTableNd );
    pCursor->DeleteMark();
    pCursor->GetPoint()->Assign(*pTableNd->EndOfSectionNode());
    // move sPoint and Mark out of the area!
    pCursor->SetMark();
    pCursor->DeleteMark();

    //Modified for bug #i119954# Application crashed if undo/redo convert nest table to text
    StartUndo();
    bool bRet = ConvertTableToText( pTableNd, cCh );
    EndUndo();
    //End  for bug #i119954#
    pCursor->GetPoint()->Assign(aTabIdx);

    SwContentNode* pCNd = pCursor->GetPointContentNode();
    if( !pCNd )
        pCursor->Move( fnMoveForward, GoInContent );

    EndAllAction();
    return bRet;
}

bool SwEditShell::IsTextToTableAvailable() const
{
    bool bOnlyText = false;
    for(SwPaM& rPaM : GetCursor()->GetRingContainer())
    {
        if( rPaM.HasMark() && *rPaM.GetPoint() != *rPaM.GetMark() )
        {
            bOnlyText = true;

            // check if selection is in listing
            SwNodeOffset nStt = rPaM.Start()->GetNodeIndex(),
                         nEnd = rPaM.End()->GetNodeIndex();

            for( ; nStt <= nEnd; ++nStt )
                if( !GetDoc()->GetNodes()[ nStt ]->IsTextNode() )
                {
                    bOnlyText = false;
                    break;
                }

            if( !bOnlyText )
                break;
        }
    }

    return bOnlyText;
}

void SwEditShell::InsertDDETable( const SwInsertTableOptions& rInsTableOpts,
                                  SwDDEFieldType* pDDEType,
                                  sal_uInt16 nRows, sal_uInt16 nCols )
{
    SwPosition* pPos = GetCursor()->GetPoint();
    // Do not try to insert table into Footnotes/Endnotes! tdf#76007 prevents that.
    if (pPos->GetNode() < pPos->GetNodes().GetEndOfInserts()
        && pPos->GetNode().GetIndex() >= pPos->GetNodes().GetEndOfInserts().StartOfSectionIndex())
        return;

    StartAllAction();

    bool bEndUndo = 0 != pPos->GetContentIndex();
    if( bEndUndo )
    {
        StartUndo( SwUndoId::START );
        GetDoc()->getIDocumentContentOperations().SplitNode( *pPos, false );
    }

    const SwInsertTableOptions aInsTableOpts( rInsTableOpts.mnInsMode | SwInsertTableFlags::DefaultBorder,
                                            rInsTableOpts.mnRowsToRepeat );
    SwTable* pTable = const_cast<SwTable*>(GetDoc()->InsertTable( aInsTableOpts, *pPos,
                                                     nRows, nCols, css::text::HoriOrientation::FULL ));

    SwTableNode* pTableNode = const_cast<SwTableNode*>(pTable->GetTabSortBoxes()[ 0 ]->
                                                GetSttNd()->FindTableNode());
    std::unique_ptr<SwDDETable> pDDETable(new SwDDETable( *pTable, pDDEType ));
    pTableNode->SetNewTable( std::move(pDDETable) );   // set the DDE table

    if( bEndUndo )
        EndUndo( SwUndoId::END );

    EndAllAction();
}

/** update fields of a listing */
void SwEditShell::UpdateTable()
{
    const SwTableNode* pTableNd = IsCursorInTable();

    if( pTableNd )
    {
        StartAllAction();
        if( DoesUndo() )
            StartUndo();
        EndAllTableBoxEdit();
        GetDoc()->getIDocumentFieldsAccess().UpdateTableFields(&pTableNd->GetTable());
        if( DoesUndo() )
            EndUndo();
        EndAllAction();
    }
}

// get/set Change Mode

TableChgMode SwEditShell::GetTableChgMode() const
{
    TableChgMode eMode;
    const SwTableNode* pTableNd = IsCursorInTable();
    if( pTableNd )
        eMode = pTableNd->GetTable().GetTableChgMode();
    else
        eMode = GetTableChgDefaultMode();
    return eMode;
}

void SwEditShell::SetTableChgMode( TableChgMode eMode )
{
    const SwTableNode* pTableNd = IsCursorInTable();

    if( pTableNd )
    {
        const_cast<SwTable&>(pTableNd->GetTable()).SetTableChgMode( eMode );
        if( !GetDoc()->getIDocumentState().IsModified() )   // Bug 57028
        {
            GetDoc()->GetIDocumentUndoRedo().SetUndoNoResetModified();
        }
        GetDoc()->getIDocumentState().SetModified();
    }
}

bool SwEditShell::GetTableBoxFormulaAttrs( SfxItemSet& rSet ) const
{
    SwSelBoxes aBoxes;
    if( IsTableMode() )
        ::GetTableSelCrs( *this, aBoxes );
    else
    {
        SwFrame* pFrame = GetCurrFrame()->GetUpper();
        while (pFrame && !pFrame->IsCellFrame())
            pFrame = pFrame->GetUpper();

        if (pFrame)
        {
            auto pBox = const_cast<SwTableBox*>(static_cast<SwCellFrame*>(pFrame)->GetTabBox());
            aBoxes.insert(pBox);
        }
    }

    for (size_t n = 0; n < aBoxes.size(); ++n)
    {
        const SwTableBox* pSelBox = aBoxes[ n ];
        const SwTableBoxFormat* pTableFormat = pSelBox->GetFrameFormat();
        if( !n )
        {
            // Convert formulae into external presentation
            const SwTable& rTable = pSelBox->GetSttNd()->FindTableNode()->GetTable();

            const_cast<SwTable*>(&rTable)->SwitchFormulasToExternalRepresentation();
            rSet.Put( pTableFormat->GetAttrSet() );
        }
        else
            rSet.MergeValues( pTableFormat->GetAttrSet() );
    }
    return 0 != rSet.Count();
}

void SwEditShell::SetTableBoxFormulaAttrs( const SfxItemSet& rSet )
{
    CurrShell aCurr( this );
    SwSelBoxes aBoxes;
    if( IsTableMode() )
        ::GetTableSelCrs( *this, aBoxes );
    else
    {
        do {
            SwFrame *pFrame = GetCurrFrame();
            do {
                pFrame = pFrame->GetUpper();
            } while ( pFrame && !pFrame->IsCellFrame() );
            if ( pFrame )
            {
                SwTableBox *pBox = const_cast<SwTableBox*>(static_cast<SwCellFrame*>(pFrame)->GetTabBox());
                aBoxes.insert( pBox );
            }
        } while( false );
    }

    // When setting a formula, do not check further!
    if( SfxItemState::SET == rSet.GetItemState( RES_BOXATR_FORMULA ))
        ClearTableBoxContent();

    StartAllAction();
    GetDoc()->GetIDocumentUndoRedo().StartUndo( SwUndoId::START, nullptr );
    for (size_t n = 0; n < aBoxes.size(); ++n)
    {
        GetDoc()->SetTableBoxFormulaAttrs( *aBoxes[ n ], rSet );
    }
    GetDoc()->GetIDocumentUndoRedo().EndUndo( SwUndoId::END, nullptr );
    EndAllAction();
}

bool SwEditShell::IsTableBoxTextFormat() const
{
    if( IsTableMode() )
        return false;

    const SwTableBox *pBox = nullptr;
    {
        SwFrame *pFrame = GetCurrFrame();
        do {
            pFrame = pFrame->GetUpper();
        } while ( pFrame && !pFrame->IsCellFrame() );
        if ( pFrame )
            pBox = static_cast<SwCellFrame*>(pFrame)->GetTabBox();
    }

    if( !pBox )
        return false;

    sal_uInt32 nFormat = 0;
    if( const SwTableBoxNumFormat* pItem = pBox->GetFrameFormat()->GetAttrSet().GetItemIfSet(
        RES_BOXATR_FORMAT ))
    {
        nFormat = pItem->GetValue();
        return GetDoc()->GetNumberFormatter()->IsTextFormat( nFormat );
    }

    SwNodeOffset nNd = pBox->IsValidNumTextNd();
    if( NODE_OFFSET_MAX == nNd )
        return true;

    const OUString& rText = GetDoc()->GetNodes()[ nNd ]->GetTextNode()->GetText();
    if( rText.isEmpty() )
        return false;

    double fVal;
    return !GetDoc()->IsNumberFormat( rText, nFormat, fVal );
}

OUString SwEditShell::GetTableBoxText() const
{
    OUString sRet;
    if( !IsTableMode() )
    {
        const SwTableBox *pBox = nullptr;
        {
            SwFrame *pFrame = GetCurrFrame();
            do {
                pFrame = pFrame->GetUpper();
            } while ( pFrame && !pFrame->IsCellFrame() );
            if ( pFrame )
                pBox = static_cast<SwCellFrame*>(pFrame)->GetTabBox();
        }

        SwNodeOffset nNd;
        if( pBox && NODE_OFFSET_MAX != ( nNd = pBox->IsValidNumTextNd() ) )
            sRet = GetDoc()->GetNodes()[ nNd ]->GetTextNode()->GetText();
    }
    return sRet;
}

void SwEditShell::SplitTable( SplitTable_HeadlineOption eMode )
{
    SwPaM *pCursor = GetCursor();
    if( pCursor->GetPointNode().FindTableNode() )
    {
        StartAllAction();
        GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::EMPTY, nullptr);

        GetDoc()->SplitTable( *pCursor->GetPoint(), eMode, true );

        GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::EMPTY, nullptr);
        ClearFEShellTabCols(*GetDoc(), nullptr);
        EndAllAction();
    }
}

bool SwEditShell::MergeTable( bool bWithPrev )
{
    bool bRet = false;
    SwPaM *pCursor = GetCursor();
    if( pCursor->GetPointNode().FindTableNode() )
    {
        StartAllAction();
        GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::EMPTY, nullptr);

        bRet = GetDoc()->MergeTable( *pCursor->GetPoint(), bWithPrev );

        GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::EMPTY, nullptr);
        ClearFEShellTabCols(*GetDoc(), nullptr);
        EndAllAction();
    }
    return bRet;
}

bool SwEditShell::CanMergeTable( bool bWithPrev, bool* pChkNxtPrv ) const
{
    bool bRet = false;
    const SwPaM *pCursor = GetCursor();
    const SwTableNode* pTableNd = pCursor->GetPointNode().FindTableNode();
    if( pTableNd && dynamic_cast< const SwDDETable* >(&pTableNd->GetTable()) ==  nullptr)
    {
        bool bNew = pTableNd->GetTable().IsNewModel();
        const SwNodes& rNds = GetDoc()->GetNodes();
        if( pChkNxtPrv )
        {
            const SwTableNode* pChkNd = rNds[ pTableNd->GetIndex() - 1 ]->FindTableNode();
            if( pChkNd && dynamic_cast< const SwDDETable* >(&pChkNd->GetTable()) ==  nullptr &&
                bNew == pChkNd->GetTable().IsNewModel() &&
                // Consider table in table case
                pChkNd->EndOfSectionIndex() == pTableNd->GetIndex() - 1 )
            {
                *pChkNxtPrv = true;
                bRet = true;        // using Prev is possible
            }
            else
            {
                pChkNd = rNds[ pTableNd->EndOfSectionIndex() + 1 ]->GetTableNode();
                if( pChkNd && dynamic_cast< const SwDDETable* >(&pChkNd->GetTable()) ==  nullptr &&
                    bNew == pChkNd->GetTable().IsNewModel() )
                {
                    *pChkNxtPrv = false;
                    bRet = true;   // using Next is possible
                }
            }
        }
        else
        {
            const SwTableNode* pTmpTableNd = nullptr;

            if( bWithPrev )
            {
                pTmpTableNd = rNds[ pTableNd->GetIndex() - 1 ]->FindTableNode();
                // Consider table in table case
                if ( pTmpTableNd && pTmpTableNd->EndOfSectionIndex() != pTableNd->GetIndex() - 1 )
                    pTmpTableNd = nullptr;
            }
            else
                pTmpTableNd = rNds[ pTableNd->EndOfSectionIndex() + 1 ]->GetTableNode();

            bRet = pTmpTableNd && dynamic_cast< const SwDDETable* >(&pTmpTableNd->GetTable()) ==  nullptr &&
                   bNew == pTmpTableNd->GetTable().IsNewModel();
        }
    }
    return bRet;
}

/** create InsertDB as table Undo */
void SwEditShell::AppendUndoForInsertFromDB( bool bIsTable )
{
    GetDoc()->AppendUndoForInsertFromDB( *GetCursor(), bIsTable );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

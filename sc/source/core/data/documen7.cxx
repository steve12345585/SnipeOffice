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

#include <sal/log.hxx>
#include <osl/diagnose.h>

#include <document.hxx>
#include <brdcst.hxx>
#include <bcaslot.hxx>
#include <formulacell.hxx>
#include <table.hxx>
#include <progress.hxx>
#include <scmod.hxx>
#include <inputopt.hxx>
#include <sheetevents.hxx>
#include <tokenarray.hxx>
#include <listenercontext.hxx>

void ScDocument::StartListeningArea(
    const ScRange& rRange, bool bGroupListening, SvtListener* pListener )
{
    if (!pBASM)
        return;

    // Ensure sane ranges for the slots, specifically don't attempt to listen
    // to more sheets than the document has. The slot machine handles it but
    // with memory waste. Binary import filters can set out-of-bounds ranges
    // in formula expressions' references, so all middle layers would have to
    // check it, rather have this central point here.
    ScRange aLimitedRange( ScAddress::UNINITIALIZED );
    bool bEntirelyOut;
    if (!LimitRangeToAvailableSheets( rRange, aLimitedRange, bEntirelyOut))
    {
        pBASM->StartListeningArea(rRange, bGroupListening, pListener);
        return;
    }

    // If both sheets are out-of-bounds in the same direction then just bail out.
    if (bEntirelyOut)
        return;

    pBASM->StartListeningArea( aLimitedRange, bGroupListening, pListener);
}

void ScDocument::EndListeningArea( const ScRange& rRange, bool bGroupListening, SvtListener* pListener )
{
    if (!pBASM)
        return;

    // End listening has to limit the range exactly the same as in
    // StartListeningArea(), otherwise the range would not be found.
    ScRange aLimitedRange( ScAddress::UNINITIALIZED );
    bool bEntirelyOut;
    if (!LimitRangeToAvailableSheets( rRange, aLimitedRange, bEntirelyOut))
    {
        pBASM->EndListeningArea(rRange, bGroupListening, pListener);
        return;
    }

    // If both sheets are out-of-bounds in the same direction then just bail out.
    if (bEntirelyOut)
        return;

    pBASM->EndListeningArea( aLimitedRange, bGroupListening, pListener);
}

bool ScDocument::LimitRangeToAvailableSheets( const ScRange& rRange, ScRange& o_rRange,
        bool& o_bEntirelyOutOfBounds ) const
{
    const SCTAB nMaxTab = GetTableCount() - 1;
    if (ValidTab( rRange.aStart.Tab(), nMaxTab) && ValidTab( rRange.aEnd.Tab(), nMaxTab))
        return false;

    // Originally BCA_LISTEN_ALWAYS uses an implicit tab 0 and should had been
    // valid already, but in case that would change...
    if (rRange == BCA_LISTEN_ALWAYS)
        return false;

    SCTAB nTab1 = rRange.aStart.Tab();
    SCTAB nTab2 = rRange.aEnd.Tab();
    SAL_WARN("sc.core","ScDocument::LimitRangeToAvailableSheets - bad sheet range: " << nTab1 << ".." << nTab2 <<
            ", sheets: 0.." << nMaxTab);

    // Both sheets are out-of-bounds in the same direction.
    if ((nTab1 < 0 && nTab2 < 0) || (nMaxTab < nTab1 && nMaxTab < nTab2))
    {
        o_bEntirelyOutOfBounds = true;
        return true;
    }

    // Limit the sheet range to bounds.
    o_bEntirelyOutOfBounds = false;
    nTab1 = std::clamp<SCTAB>( nTab1, 0, nMaxTab);
    nTab2 = std::clamp<SCTAB>( nTab2, 0, nMaxTab);
    o_rRange = rRange;
    o_rRange.aStart.SetTab(nTab1);
    o_rRange.aEnd.SetTab(nTab2);
    return true;
}

void ScDocument::Broadcast( const ScHint& rHint )
{
    if ( !pBASM )
        return ;    // Clipboard or Undo
    if ( eHardRecalcState == HardRecalcState::OFF )
    {
        ScBulkBroadcast aBulkBroadcast( pBASM.get(), rHint.GetId());     // scoped bulk broadcast
        bool bIsBroadcasted = BroadcastHintInternal(rHint);
        if ( pBASM->AreaBroadcast( rHint ) || bIsBroadcasted )
            TrackFormulas( rHint.GetId() );
    }

    if ( rHint.GetStartAddress() != BCA_BRDCST_ALWAYS )
    {
        SCTAB nTab = rHint.GetStartAddress().Tab();
        if (nTab < GetTableCount() && maTabs[nTab])
            maTabs[nTab]->SetStreamValid(false);
    }
}

bool ScDocument::BroadcastHintInternal( const ScHint& rHint )
{
    bool bIsBroadcasted = false;
    const ScAddress& address(rHint.GetStartAddress());
    SvtBroadcaster* pLastBC = nullptr;
    // Process all broadcasters for the given row range.
    for( SCROW nRow = 0; nRow < rHint.GetRowCount(); ++nRow )
    {
        ScAddress a(address);
        a.SetRow(address.Row() + nRow);
        SvtBroadcaster* pBC = GetBroadcaster(a);
        if ( pBC && pBC != pLastBC )
        {
            pBC->Broadcast( rHint );
            bIsBroadcasted = true;
            pLastBC = pBC;
        }
    }
    return bIsBroadcasted;
}

void ScDocument::BroadcastCells( const ScRange& rRange, SfxHintId nHint, bool bBroadcastSingleBroadcasters )
{
    PrepareFormulaCalc();

    if (!pBASM)
        return;    // Clipboard or Undo

    SCTAB nTab1 = rRange.aStart.Tab();
    SCTAB nTab2 = rRange.aEnd.Tab();
    SCROW nRow1 = rRange.aStart.Row();
    SCROW nRow2 = rRange.aEnd.Row();
    SCCOL nCol1 = rRange.aStart.Col();
    SCCOL nCol2 = rRange.aEnd.Col();

    if (eHardRecalcState == HardRecalcState::OFF)
    {
        ScBulkBroadcast aBulkBroadcast( pBASM.get(), nHint);     // scoped bulk broadcast
        bool bIsBroadcasted = false;

        if (bBroadcastSingleBroadcasters)
        {
            for (SCTAB nTab = nTab1; nTab <= nTab2; ++nTab)
            {
                ScTable* pTab = FetchTable(nTab);
                if (!pTab)
                    continue;

                bIsBroadcasted |= pTab->BroadcastBroadcasters( nCol1, nRow1, nCol2, nRow2, nHint);
            }
        }

        if (pBASM->AreaBroadcast(rRange, nHint) || bIsBroadcasted)
            TrackFormulas(nHint);
    }

    for (SCTAB nTab = nTab1; nTab <= nTab2; ++nTab)
    {
        ScTable* pTab = FetchTable(nTab);
        if (pTab)
            pTab->SetStreamValid(false);
    }

    BroadcastUno(SfxHint(SfxHintId::ScDataChanged));
}

void ScDocument::AreaBroadcast( const ScHint& rHint )
{
    if ( !pBASM )
        return ;    // Clipboard or Undo
    if (eHardRecalcState == HardRecalcState::OFF)
    {
        ScBulkBroadcast aBulkBroadcast( pBASM.get(), rHint.GetId());     // scoped bulk broadcast
        if ( pBASM->AreaBroadcast( rHint ) )
            TrackFormulas( rHint.GetId() );
    }
}

void ScDocument::DelBroadcastAreasInRange( const ScRange& rRange )
{
    if ( pBASM )
        pBASM->DelBroadcastAreasInRange( rRange );
}

void ScDocument::StartListeningCell( const ScAddress& rAddress,
                                            SvtListener* pListener )
{
    OSL_ENSURE(pListener, "StartListeningCell: pListener Null");
    SCTAB nTab = rAddress.Tab();
    if (ScTable* pTable = FetchTable(nTab))
        pTable->StartListening(rAddress, pListener);
}

void ScDocument::EndListeningCell( const ScAddress& rAddress,
                                            SvtListener* pListener )
{
    OSL_ENSURE(pListener, "EndListeningCell: pListener Null");
    SCTAB nTab = rAddress.Tab();
    if (ScTable* pTable = FetchTable(nTab))
        pTable->EndListening( rAddress, pListener );
}

void ScDocument::StartListeningCell(
    sc::StartListeningContext& rCxt, const ScAddress& rPos, SvtListener& rListener )
{
    if (ScTable* pTable = FetchTable(rPos.Tab()))
        pTable->StartListening(rCxt, rPos, rListener);
}

void ScDocument::EndListeningCell(
    sc::EndListeningContext& rCxt, const ScAddress& rPos, SvtListener& rListener )
{
    if (ScTable* pTable = FetchTable(rPos.Tab()))
        pTable->EndListening(rCxt, rPos, rListener);
}

void ScDocument::EndListeningFormulaCells( std::vector<ScFormulaCell*>& rCells )
{
    if (rCells.empty())
        return;

    sc::EndListeningContext aCxt(*this);
    for (auto& pCell : rCells)
        pCell->EndListeningTo(aCxt);

    aCxt.purgeEmptyBroadcasters();
}

void ScDocument::PutInFormulaTree( ScFormulaCell* pCell )
{
    OSL_ENSURE( pCell, "PutInFormulaTree: pCell Null" );
    RemoveFromFormulaTree( pCell );
    // append
    ScMutationGuard aGuard(*this, ScMutationGuardFlags::CORE);
    if ( pEOFormulaTree )
        pEOFormulaTree->SetNext( pCell );
    else
        pFormulaTree = pCell;               // No end, no beginning...
    pCell->SetPrevious( pEOFormulaTree );
    pCell->SetNext( nullptr );
    pEOFormulaTree = pCell;
    nFormulaCodeInTree += pCell->GetCode()->GetCodeLen();
}

void ScDocument::RemoveFromFormulaTree( ScFormulaCell* pCell )
{
    ScMutationGuard aGuard(*this, ScMutationGuardFlags::CORE);
    assert(pCell && "RemoveFromFormulaTree: pCell Null");
    ScFormulaCell* pPrev = pCell->GetPrevious();
    assert(pPrev != pCell);                 // pointing to itself?!?
    // if the cell is first or somewhere in chain
    if ( pPrev || pFormulaTree == pCell )
    {
        ScFormulaCell* pNext = pCell->GetNext();
        assert(pNext != pCell);             // pointing to itself?!?
        if ( pPrev )
        {
            assert(pFormulaTree != pCell);  // if this cell is also head something's wrong
            pPrev->SetNext( pNext );        // predecessor exists, set successor
        }
        else
        {
            pFormulaTree = pNext;           // this cell was first cell
        }
        if ( pNext )
        {
            assert(pEOFormulaTree != pCell); // if this cell is also tail something's wrong
            pNext->SetPrevious( pPrev );    // successor exists, set predecessor
        }
        else
        {
            pEOFormulaTree = pPrev;         // this cell was last cell
        }
        pCell->SetPrevious( nullptr );
        pCell->SetNext( nullptr );
        sal_uInt16 nRPN = pCell->GetCode()->GetCodeLen();
        if ( nFormulaCodeInTree >= nRPN )
            nFormulaCodeInTree -= nRPN;
        else
        {
            OSL_FAIL( "RemoveFromFormulaTree: nFormulaCodeInTree < nRPN" );
            nFormulaCodeInTree = 0;
        }
    }
    else if ( !pFormulaTree && nFormulaCodeInTree )
    {
        OSL_FAIL( "!pFormulaTree && nFormulaCodeInTree != 0" );
        nFormulaCodeInTree = 0;
    }
}

void ScDocument::CalcFormulaTree( bool bOnlyForced, bool bProgressBar, bool bSetAllDirty )
{
    OSL_ENSURE( !IsCalculatingFormulaTree(), "CalcFormulaTree recursion" );
    // never ever recurse into this, might end up lost in infinity
    if ( IsCalculatingFormulaTree() )
        return ;

    ScMutationGuard aGuard(*this, ScMutationGuardFlags::CORE);
    mpFormulaGroupCxt.reset();
    bCalculatingFormulaTree = true;

    SetForcedFormulaPending( false );
    bool bOldIdleEnabled = IsIdleEnabled();
    EnableIdle(false);
    bool bOldAutoCalc = GetAutoCalc();
    //ATTENTION: _not_ SetAutoCalc( true ) because this might call CalcFormulaTree( true )
    //ATTENTION: if it was disabled before and bHasForcedFormulas is set
    bAutoCalc = true;
    if (eHardRecalcState == HardRecalcState::ETERNAL)
        CalcAll();
    else
    {
        ::std::vector<ScFormulaCell*> vAlwaysDirty;
        ScFormulaCell* pCell = pFormulaTree;
        while ( pCell )
        {
            if ( pCell->GetDirty() )
                ;   // nothing to do
            else if ( pCell->GetCode()->IsRecalcModeAlways() )
            {
                // pCell and dependents are to be set dirty again, collect
                // them first and broadcast afterwards to not break the
                // FormulaTree chain here.
                vAlwaysDirty.push_back( pCell);
            }
            else if ( bSetAllDirty )
            {
                // Force calculating all in tree, without broadcasting.
                pCell->SetDirtyVar();
            }
            pCell = pCell->GetNext();
        }
        for (const auto& rpCell : vAlwaysDirty)
        {
            pCell = rpCell;
            if (!pCell->GetDirty())
                pCell->SetDirty();
        }

        bool bProgress = !bOnlyForced && nFormulaCodeInTree && bProgressBar;
        if ( bProgress )
            ScProgress::CreateInterpretProgress( this );

        pCell = pFormulaTree;
        ScFormulaCell* pLastNoGood = nullptr;
        while ( pCell )
        {
            // Interpret resets bDirty and calls Remove, also the referenced!
            // the Cell remains when ScRecalcMode::ALWAYS.
            if ( bOnlyForced )
            {
                if ( pCell->GetCode()->IsRecalcModeForced() )
                    pCell->Interpret();
            }
            else
            {
                pCell->Interpret();
            }
            if ( pCell->GetPrevious() || pCell == pFormulaTree )
            {   // (IsInFormulaTree(pCell)) no Remove was called => next
                pLastNoGood = pCell;
                pCell = pCell->GetNext();
            }
            else
            {
                if ( pFormulaTree )
                {
                    if ( pFormulaTree->GetDirty() && !bOnlyForced )
                    {
                        pCell = pFormulaTree;
                        pLastNoGood = nullptr;
                    }
                    else
                    {
                        // IsInFormulaTree(pLastNoGood)
                        if ( pLastNoGood && (pLastNoGood->GetPrevious() ||
                                pLastNoGood == pFormulaTree) )
                            pCell = pLastNoGood->GetNext();
                        else
                        {
                            pCell = pFormulaTree;
                            while ( pCell && !pCell->GetDirty() )
                                pCell = pCell->GetNext();
                            if ( pCell )
                                pLastNoGood = pCell->GetPrevious();
                        }
                    }
                }
                else
                    pCell = nullptr;
            }
        }
        if ( bProgress )
            ScProgress::DeleteInterpretProgress();
    }
    bAutoCalc = bOldAutoCalc;
    EnableIdle(bOldIdleEnabled);
    bCalculatingFormulaTree = false;

    mpFormulaGroupCxt.reset();
}

void ScDocument::ClearFormulaTree()
{
    ScFormulaCell* pCell;
    ScFormulaCell* pTree = pFormulaTree;
    while ( pTree )
    {
        pCell = pTree;
        pTree = pCell->GetNext();
        if ( !pCell->GetCode()->IsRecalcModeAlways() )
            RemoveFromFormulaTree( pCell );
    }
}

void ScDocument::AppendToFormulaTrack( ScFormulaCell* pCell )
{
    OSL_ENSURE( pCell, "AppendToFormulaTrack: pCell Null" );
    // The cell can not be in both lists at the same time
    RemoveFromFormulaTrack( pCell );
    RemoveFromFormulaTree( pCell );
    if ( pEOFormulaTrack )
        pEOFormulaTrack->SetNextTrack( pCell );
    else
        pFormulaTrack = pCell;              // No end, no beginning...
    pCell->SetPreviousTrack( pEOFormulaTrack );
    pCell->SetNextTrack( nullptr );
    pEOFormulaTrack = pCell;
    ++nFormulaTrackCount;
}

void ScDocument::RemoveFromFormulaTrack( ScFormulaCell* pCell )
{
    assert(pCell && "RemoveFromFormulaTrack: pCell Null");
    ScFormulaCell* pPrev = pCell->GetPreviousTrack();
    assert(pPrev != pCell);                     // pointing to itself?!?
    // if the cell is first or somewhere in chain
    if ( !(pPrev || pFormulaTrack == pCell) )
        return;

    ScFormulaCell* pNext = pCell->GetNextTrack();
    assert(pNext != pCell);                 // pointing to itself?!?
    if ( pPrev )
    {
        assert(pFormulaTrack != pCell);     // if this cell is also head something's wrong
        pPrev->SetNextTrack( pNext );       // predecessor exists, set successor
    }
    else
    {
        pFormulaTrack = pNext;              // this cell was first cell
    }
    if ( pNext )
    {
        assert(pEOFormulaTrack != pCell);   // if this cell is also tail something's wrong
        pNext->SetPreviousTrack( pPrev );   // successor exists, set predecessor
    }
    else
    {
        pEOFormulaTrack = pPrev;            // this cell was last cell
    }
    pCell->SetPreviousTrack( nullptr );
    pCell->SetNextTrack( nullptr );
    --nFormulaTrackCount;
}

void ScDocument::FinalTrackFormulas( SfxHintId nHintId )
{
    mbTrackFormulasPending = false;
    mbFinalTrackFormulas = true;
    {
        ScBulkBroadcast aBulk( GetBASM(), nHintId);
        // Collect all pending formula cells in bulk.
        TrackFormulas( nHintId );
    }
    // A final round not in bulk to track all remaining formula cells and their
    // dependents that were collected during ScBulkBroadcast dtor.
    TrackFormulas( nHintId );
    mbFinalTrackFormulas = false;
}

/*
    The first is broadcasted,
    the ones that are created through this are appended to the Track by Notify.
    The next is broadcasted again, and so on.
    View initiates Interpret.
 */
void ScDocument::TrackFormulas( SfxHintId nHintId )
{
    if (!pBASM)
        return;

    if (pBASM->IsInBulkBroadcast() && !IsFinalTrackFormulas() &&
            (nHintId == SfxHintId::ScDataChanged || nHintId == SfxHintId::ScHiddenRowsChanged))
    {
        SetTrackFormulasPending();
        return;
    }

    if ( pFormulaTrack )
    {
        // outside the loop, check if any sheet has a "calculate" event script
        bool bCalcEvent = HasAnySheetEventScript( ScSheetEventId::CALCULATE, true );
        for( ScFormulaCell* pTrack = pFormulaTrack; pTrack != nullptr; pTrack = pTrack->GetNextTrack())
        {
            SCROW rowCount = 1;
            ScAddress address = pTrack->aPos;
            // Compress to include all adjacent cells in the same column.
            for(ScFormulaCell* pNext = pTrack->GetNextTrack(); pNext != nullptr; pNext = pNext->GetNextTrack())
            {
                if(pNext->aPos != ScAddress(address.Col(), address.Row() + rowCount, address.Tab()))
                    break;
                ++rowCount;
                pTrack = pNext;
            }
            ScHint aHint( nHintId, address, rowCount );
            BroadcastHintInternal( aHint );
            pBASM->AreaBroadcast( aHint );
            // for "calculate" event, keep track of which sheets are affected by tracked formulas
            if ( bCalcEvent )
                SetCalcNotification( address.Tab() );
        }
        bool bHaveForced = false;
        for( ScFormulaCell* pTrack = pFormulaTrack; pTrack != nullptr;)
        {
            ScFormulaCell* pNext = pTrack->GetNextTrack();
            RemoveFromFormulaTrack( pTrack );
            PutInFormulaTree( pTrack );
            if ( pTrack->GetCode()->IsRecalcModeForced() )
                bHaveForced = true;
            pTrack = pNext;
        }
        if ( bHaveForced )
        {
            SetForcedFormulas( true );
            if ( bAutoCalc && !IsAutoCalcShellDisabled() && !IsInInterpreter()
                    && !IsCalculatingFormulaTree() )
                CalcFormulaTree( true );
            else
                SetForcedFormulaPending( true );
        }
    }
    OSL_ENSURE( nFormulaTrackCount==0, "TrackFormulas: nFormulaTrackCount!=0" );
}

void ScDocument::StartAllListeners()
{
    sc::StartListeningContext aCxt(*this);
    for ( auto const & i: maTabs )
        if ( i )
            i->StartListeners(aCxt, true);
}

void ScDocument::UpdateBroadcastAreas( UpdateRefMode eUpdateRefMode,
        const ScRange& rRange, SCCOL nDx, SCROW nDy, SCTAB nDz
    )
{
    bool bExpandRefsOld = IsExpandRefs();
    if ( eUpdateRefMode == URM_INSDEL && (nDx > 0 || nDy > 0 || nDz > 0) )
        SetExpandRefs(ScModule::get()->GetInputOptions().GetExpandRefs());
    if ( pBASM )
        pBASM->UpdateBroadcastAreas( eUpdateRefMode, rRange, nDx, nDy, nDz );
    SetExpandRefs( bExpandRefsOld );
}

void ScDocument::SetAutoCalc( bool bNewAutoCalc )
{
    bool bOld = bAutoCalc;
    bAutoCalc = bNewAutoCalc;
    if ( !bOld && bNewAutoCalc && bHasForcedFormulas )
    {
        if ( IsAutoCalcShellDisabled() )
            SetForcedFormulaPending( true );
        else if ( !IsInInterpreter() )
            CalcFormulaTree( true );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

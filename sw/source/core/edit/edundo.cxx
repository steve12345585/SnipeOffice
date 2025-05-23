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

#include <svx/svdmark.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <editsh.hxx>
#include <fesh.hxx>
#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <UndoCore.hxx>
#include <swundo.hxx>
#include <flyfrm.hxx>
#include <frmfmt.hxx>
#include <docsh.hxx>
#include <pagefrm.hxx>
#include <textboxhelper.hxx>
#include <fmtanchr.hxx>

#include <wrtsh.hxx>

/** helper function to select all objects in an SdrMarkList;
 * implementation: see below */
static void lcl_SelectSdrMarkList( SwEditShell* pShell,
                            const SdrMarkList* pSdrMarkList );

bool SwEditShell::CursorsLocked() const
{
    SwDocShell* pShell = GetDoc()->GetDocShell();
    if (!pShell)
        return false;
    return pShell->GetModel()->hasControllersLocked();
}

void SwEditShell::HandleUndoRedoContext(::sw::UndoRedoContext & rContext)
{
    // do nothing if somebody has locked controllers!
    if (CursorsLocked())
    {
        return;
    }

    SwFrameFormat * pSelFormat(nullptr);
    SdrMarkList * pMarkList(nullptr);
    rContext.GetSelections(pSelFormat, pMarkList);

    if (pSelFormat) // select frame
    {
        if (RES_DRAWFRMFMT == pSelFormat->Which())
        {
            SdrObject* pSObj = pSelFormat->FindSdrObject();

            // Before layout calc, inline anchored textboxes have to be synced unless crash.
            if (pSelFormat->GetAnchor().GetAnchorId() == RndStdIds::FLY_AS_CHAR
                && pSelFormat->GetOtherTextBoxFormats())
                SwTextBoxHelper::synchronizeGroupTextBoxProperty(SwTextBoxHelper::changeAnchor,
                                                                 pSelFormat, pSObj);

            static_cast<SwFEShell*>(this)->SelectObj(
                    pSObj->GetCurrentBoundRect().Center() );
        }
        else
        {
            Point aPt;
            SwFlyFrame *const pFly =
                static_cast<SwFlyFrameFormat*>(pSelFormat)->GetFrame(& aPt);
            if (pFly)
            {
                // fdo#36681: Invalidate the content and layout to refresh
                // the picture anchoring properly
                SwPageFrame* pPageFrame = pFly->FindPageFrameOfAnchor();
                pPageFrame->InvalidateFlyLayout();
                pPageFrame->InvalidateContent();

                static_cast<SwFEShell*>(this)->SelectFlyFrame(*pFly);
            }
        }
    }
    else if (pMarkList)
    {
        lcl_SelectSdrMarkList( this, pMarkList );
    }
    else if (GetCursor()->GetNext() != GetCursor())
    {
        // current cursor is the last one:
        // go around the ring, to the first cursor
        GoNextCursor();
    }
}

void SwEditShell::Undo(sal_uInt16 const nCount, sal_uInt16 nOffset)
{
    MakeAllOutlineContentTemporarilyVisible a(GetDoc(), true);

    CurrShell aCurr( this );

    // current undo state was not saved
    ::sw::UndoGuard const undoGuard(GetDoc()->GetIDocumentUndoRedo());

    StartAllAction();
    {
        // Actually it should be enough to just work on the current Cursor, i.e. if there is a cycle
        // cancel the latter temporarily, so that an insert during Undo is not done in all areas.
        KillPams();
        SetMark();          // Bound1 and Bound2 in the same Node
        ClearMark();

        // Keep Cursor - so that we're able to set it at
        // the same position for autoformat or autocorrection
        SwUndoId nLastUndoId(SwUndoId::EMPTY);
        GetLastUndoInfo(nullptr, & nLastUndoId);
        const bool bRestoreCursor = nCount == 1
                                  && ( SwUndoId::AUTOFORMAT == nLastUndoId
                                       || SwUndoId::AUTOCORRECT == nLastUndoId
                                       || SwUndoId::SETDEFTATTR == nLastUndoId );
        Push();

        // Destroy stored TableBoxPtr. A detection is only permitted for the new "Box"!
        ClearTableBoxContent();

        const RedlineFlags eOld = GetDoc()->getIDocumentRedlineAccess().GetRedlineFlags();

        try {
            for (sal_uInt16 i = 0; i < nCount; ++i)
            {
                GetDoc()->GetIDocumentUndoRedo().UndoWithOffset(nOffset);
            }
        } catch (const css::uno::Exception &) {
            TOOLS_WARN_EXCEPTION("sw.core", "SwEditShell::Undo()");
        }

        if (bRestoreCursor)
        {   // fdo#39003 Pop does not touch the rest of the cursor ring
            KillPams(); // so call this first to get rid of unwanted cursors
        }
        Pop(bRestoreCursor ? PopMode::DeleteCurrent : PopMode::DeleteStack);

        GetDoc()->getIDocumentRedlineAccess().SetRedlineFlags( eOld );
        GetDoc()->getIDocumentRedlineAccess().CompressRedlines();

        // automatic detection of the new "Box"
        SaveTableBoxContent();
    }
    EndAllAction();
}

void SwEditShell::Redo(sal_uInt16 const nCount)
{
    MakeAllOutlineContentTemporarilyVisible a(GetDoc(), true);

    CurrShell aCurr( this );

    // undo state was not saved
    ::sw::UndoGuard const undoGuard(GetDoc()->GetIDocumentUndoRedo());

    StartAllAction();

    {
        // Actually it should be enough to just work on the current Cursor, i.e. if there is a cycle
        // cancel the latter temporarily, so that an insert during Undo is not done in all areas.
        KillPams();
        SetMark();          // Bound1 and Bound2 in the same Node
        ClearMark();

        SwUndoId nFirstRedoId(SwUndoId::EMPTY);
        GetDoc()->GetIDocumentUndoRedo().GetFirstRedoInfo(nullptr, & nFirstRedoId);
        const bool bRestoreCursor = nCount == 1 && SwUndoId::SETDEFTATTR == nFirstRedoId;
        Push();

        // Destroy stored TableBoxPtr. A detection is only permitted for the new "Box"!
        ClearTableBoxContent();

        RedlineFlags eOld = GetDoc()->getIDocumentRedlineAccess().GetRedlineFlags();

        try {
            for (sal_uInt16 i = 0; i < nCount; ++i)
            {
                GetDoc()->GetIDocumentUndoRedo().Redo();
            }
        } catch (const css::uno::Exception &) {
            TOOLS_WARN_EXCEPTION("sw.core", "SwEditShell::Redo()");
        }

        Pop(bRestoreCursor ? PopMode::DeleteCurrent : PopMode::DeleteStack);

        GetDoc()->getIDocumentRedlineAccess().SetRedlineFlags( eOld );
        GetDoc()->getIDocumentRedlineAccess().CompressRedlines();

        // automatic detection of the new "Box"
        SaveTableBoxContent();
    }

    EndAllAction();
}

void SwEditShell::Repeat(sal_uInt16 const nCount)
{
    CurrShell aCurr( this );

    StartAllAction();

    try {
        ::sw::RepeatContext context(*GetDoc(), *GetCursor());
        GetDoc()->GetIDocumentUndoRedo().Repeat( context, nCount );
    } catch (const css::uno::Exception &) {
        TOOLS_WARN_EXCEPTION("sw.core", "SwEditShell::Repeat()");
    }

    EndAllAction();
}

static void lcl_SelectSdrMarkList( SwEditShell* pShell,
                            const SdrMarkList* pSdrMarkList )
{
    OSL_ENSURE( pShell != nullptr, "need shell!" );

    SwFEShell* pFEShell = dynamic_cast<SwFEShell*>( pShell );
    if( !pFEShell )
        return;

    assert(pSdrMarkList != nullptr && "need mark list");

    bool bFirst = true;
    for( size_t i = 0; i < pSdrMarkList->GetMarkCount(); ++i )
    {
        SdrObject *pObj = pSdrMarkList->GetMark( i )->GetMarkedSdrObj();
        if( pObj )
        {
            pFEShell->SelectObj( Point(), bFirst ? 0 : SW_ADD_SELECT, pObj );
            bFirst = false;
        }
    }

    // the old implementation would always unselect
    // objects, even if no new ones were selected. If this
    // is a problem, we need to re-work this a little.
    OSL_ENSURE( pSdrMarkList->GetMarkCount() != 0, "empty mark list" );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <svx/sdrundomanager.hxx>
#include <svx/svdundo.hxx>
#include <sfx2/objsh.hxx>
#include <svl/hint.hxx>

SdrUndoManager::SdrUndoManager()
    : EditUndoManager(20 /*nMaxUndoActionCount*/)
    , mpLastUndoActionBeforeTextEdit(nullptr)
    , mnRedoActionCountBeforeTextEdit(0)
    , mbEndTextEditTriggeredFromUndo(false)
    , m_pDocSh(nullptr)
{
}

SdrUndoManager::~SdrUndoManager() {}

bool SdrUndoManager::Undo()
{
    if (isTextEditActive())
    {
        bool bRetval(false);

        // we are in text edit mode
        if (GetUndoActionCount() && mpLastUndoActionBeforeTextEdit != GetUndoAction())
        {
            // there is an undo action for text edit, trigger it
            bRetval = EditUndoManager::Undo();
        }
        else
        {
            // no more text edit undo, end text edit
            mbEndTextEditTriggeredFromUndo = true;
            maEndTextEditHdl.Call(this);
            mbEndTextEditTriggeredFromUndo = false;
        }

        return bRetval;
    }
    else
    {
        // no undo triggered up to now, trigger local one
        return SfxUndoManager::Undo();
    }
}

bool SdrUndoManager::Redo()
{
    bool bRetval(false);
    bool bClearRedoStack(false);

    if (isTextEditActive())
    {
        // we are in text edit mode
        bRetval = EditUndoManager::Redo();
    }

    if (!bRetval)
    {
        // Check if the current and thus to-be undone UndoAction is a SdrUndoDiagramModelData action
        const bool bCurrentIsDiagramChange(
            GetRedoActionCount()
            && nullptr != dynamic_cast<SdrUndoDiagramModelData*>(GetRedoAction()));

        // no redo triggered up to now, trigger local one
        bRetval = SfxUndoManager::Redo();

        // it was a SdrUndoDiagramModelData action and we have more Redo actions
        if (bCurrentIsDiagramChange && GetRedoActionCount())
        {
            const bool bNextIsDiagramChange(
                nullptr != dynamic_cast<SdrUndoDiagramModelData*>(GetRedoAction()));

            // We have more Redo-actions and the 'next' one to be executed is *not* a
            // SdrUndoDiagramModelData-action. This means that the already executed
            // one had done a re-Layout/Re-create of the Diagram XShape/SdrObject
            // representation based on the restored Diagram ModelData. When the next
            // Redo action is something else (and thus will not itself re-create
            // XShapes/SdrShapes) it may be that it is an UnGroup/Delete where a former
            // as-content-of-Diagram created XShape/SdrShape is referenced, an action
            // that references a XShape/SdrShape by pointer/reference. That
            // pointer/reference *cannot* be valid anymore (now).

            // The problem here is that Undo/Redo actions historically reference
            // XShapes/SdrShapes by pointer/reference, e.g. deleting means: remove
            // from an SdrObjList and add to an Undo action. I is *not*
            // address/incarnation-invariant in the sense to remember e.g. to
            // remove the Nth object in the list (that would work).

            // It might be possible to solve/correct this better, but since it's
            // a rare corner case just avoid the possible crash when continuing Redos
            // by clearing the Redo-Stack here as a consequence
            bClearRedoStack = !bNextIsDiagramChange;
        }
    }

    if (bClearRedoStack)
    {
        // clear Redo-Stack (explanation see above)
        ClearRedo();
    }

    return bRetval;
}

void SdrUndoManager::Clear()
{
    if (isTextEditActive())
    {
        while (GetUndoActionCount() && mpLastUndoActionBeforeTextEdit != GetUndoAction())
        {
            RemoveLastUndoAction();
        }

        // urgently needed: RemoveLastUndoAction does NOT correct the Redo stack by itself (!)
        ClearRedo();
    }
    else
    {
        // call parent
        EditUndoManager::Clear();
    }
}

void SdrUndoManager::SetEndTextEditHdl(const Link<SdrUndoManager*, void>& rLink)
{
    maEndTextEditHdl = rLink;

    if (isTextEditActive())
    {
        // text edit start, remember last non-textedit action for later cleanup
        mpLastUndoActionBeforeTextEdit = GetUndoActionCount() ? GetUndoAction() : nullptr;
        mnRedoActionCountBeforeTextEdit = GetRedoActionCount();
    }
    else
    {
        // text edit ends, pop all textedit actions up to the remembered non-textedit action from the start
        // to set back the UndoManager to the state before text edit started. If that action is already gone
        // (due to being removed from the undo stack in the meantime), all need to be removed anyways
        while (GetUndoActionCount() && mpLastUndoActionBeforeTextEdit != GetUndoAction())
        {
            RemoveLastUndoAction();
        }

        // urgently needed: RemoveLastUndoAction does NOT correct the Redo stack by itself (!)
        ClearRedo();

        // forget marker again
        mpLastUndoActionBeforeTextEdit = nullptr;
        mnRedoActionCountBeforeTextEdit = 0;
    }
}

bool SdrUndoManager::isTextEditActive() const { return maEndTextEditHdl.IsSet(); }

void SdrUndoManager::SetDocShell(SfxObjectShell* pDocShell) { m_pDocSh = pDocShell; }

void SdrUndoManager::EmptyActionsChanged()
{
    if (m_pDocSh)
    {
        m_pDocSh->Broadcast(SfxHint(SfxHintId::DocumentRepair));
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

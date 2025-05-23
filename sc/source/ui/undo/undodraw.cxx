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

#include <undodraw.hxx>
#include <docsh.hxx>
#include <tabvwsh.hxx>


ScUndoDraw::ScUndoDraw( std::unique_ptr<SfxUndoAction> pUndo, ScDocShell* pDocSh ) :
    pDrawUndo( std::move(pUndo) ),
    pDocShell( pDocSh ),
    mnViewShellId( -1 )
{
    if (ScTabViewShell* pViewShell = ScTabViewShell::GetActiveViewShell())
        mnViewShellId = pViewShell->GetViewShellId();
}

ScUndoDraw::~ScUndoDraw()
{
}

OUString ScUndoDraw::GetComment() const
{
    if (pDrawUndo)
        return pDrawUndo->GetComment();
    return OUString();
}

ViewShellId ScUndoDraw::GetViewShellId() const
{
    return mnViewShellId;
}

OUString ScUndoDraw::GetRepeatComment(SfxRepeatTarget& rTarget) const
{
    if (pDrawUndo)
        return pDrawUndo->GetRepeatComment(rTarget);
    return OUString();
}

bool  ScUndoDraw::Merge( SfxUndoAction* pNextAction )
{
    if (pDrawUndo)
        return pDrawUndo->Merge(pNextAction);
    else
        return false;
}

void ScUndoDraw::UpdateSubShell()
{
    // #i26822# remove the draw shell if the selected object has been removed
    ScTabViewShell* pViewShell = pDocShell->GetBestViewShell();
    if (pViewShell)
        pViewShell->UpdateDrawShell();
}

void ScUndoDraw::Undo()
{
    if (pDrawUndo)
    {
        pDrawUndo->Undo();
        pDocShell->SetDrawModified();
        UpdateSubShell();
    }
}

void ScUndoDraw::Redo()
{
    if (pDrawUndo)
    {
        pDrawUndo->Redo();
        pDocShell->SetDrawModified();
        UpdateSubShell();
    }
}

void ScUndoDraw::Repeat(SfxRepeatTarget& rTarget)
{
    if (pDrawUndo)
        pDrawUndo->Repeat(rTarget);
}

bool ScUndoDraw::CanRepeat(SfxRepeatTarget& rTarget) const
{
    if (pDrawUndo)
        return pDrawUndo->CanRepeat(rTarget);
    else
        return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

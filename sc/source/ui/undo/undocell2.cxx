/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <undocell.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <cellvalues.hxx>
#include <formulacell.hxx>

namespace sc {

UndoSetCells::UndoSetCells( ScDocShell* pDocSh, const ScAddress& rTopPos ) :
    ScSimpleUndo(pDocSh), maTopPos(rTopPos) {}

UndoSetCells::~UndoSetCells() {}

void UndoSetCells::DoChange( const CellValues& rValues )
{
    ScDocument& rDoc = pDocShell->GetDocument();
    rDoc.CopyCellValuesFrom(maTopPos, rValues);

    ScRange aRange(maTopPos);
    aRange.aEnd.IncRow(rValues.size());
    BroadcastChanges(aRange);
    pDocShell->PostPaintGridAll();
}

void UndoSetCells::Undo()
{
    BeginUndo();
    DoChange(maOldValues);
    EndUndo();
}

void UndoSetCells::Redo()
{
    BeginRedo();
    DoChange(maNewValues);
    EndRedo();
}

bool UndoSetCells::CanRepeat( SfxRepeatTarget& ) const
{
    return false;
}

OUString UndoSetCells::GetComment() const
{
    // "Input"
    return ScResId(STR_UNDO_ENTERDATA);
}

void UndoSetCells::SetNewValues( const std::vector<double>& rVals )
{
    maNewValues.assign(rVals);
}

void UndoSetCells::SetNewValues( const std::vector<ScFormulaCell*>& rVals )
{
    maNewValues.assign(rVals);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

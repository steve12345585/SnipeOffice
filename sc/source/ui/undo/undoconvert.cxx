/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <undoconvert.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <undoutil.hxx>

namespace sc {

UndoFormulaToValue::UndoFormulaToValue( ScDocShell* pDocSh, TableValues& rUndoValues ) :
    ScSimpleUndo(pDocSh)
{
    maUndoValues.swap(rUndoValues);
}

OUString UndoFormulaToValue::GetComment() const
{
    return ScResId(STR_UNDO_FORMULA_TO_VALUE);
}

void UndoFormulaToValue::Undo()
{
    Execute();
}

void UndoFormulaToValue::Redo()
{
    Execute();
}

void UndoFormulaToValue::Execute()
{
    ScDocument& rDoc = pDocShell->GetDocument();
    rDoc.SwapNonEmpty(maUndoValues);

    ScUndoUtil::MarkSimpleBlock(pDocShell, maUndoValues.getRange());

    pDocShell->PostPaint(maUndoValues.getRange(), PaintPartFlags::Grid);
    pDocShell->PostDataChanged();
    rDoc.BroadcastCells(maUndoValues.getRange(), SfxHintId::ScDataChanged);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

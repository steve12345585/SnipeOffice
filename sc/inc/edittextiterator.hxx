/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "types.hxx"
#include "mtvelements.hxx"

class ScTable;
class ScDocument;
class EditTextObject;

namespace sc
{
/**
 * Iterate through all edit text cells in a given sheet.  The caller must
 * check the validity of the sheet index passed to its constructor.
 *
 * It iterates from top to bottom, and then left to right order.
 */
class EditTextIterator
{
    const ScTable& mrTable;
    SCCOL mnCol;
    const CellStoreType* mpCells;
    CellStoreType::const_position_type maPos;
    CellStoreType::const_iterator miEnd;

    /**
     * Move to the next edit text cell position if the current position is not
     * an edit text.
     */
    const EditTextObject* seek();

    void incBlock();
    /**
     * Initialize members w.r.t the dynamic column container in the given table.
     */
    void init();

public:
    EditTextIterator(const ScDocument& rDoc, SCTAB nTab);

    const EditTextObject* first();
    const EditTextObject* next();
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

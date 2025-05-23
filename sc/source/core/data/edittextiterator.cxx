/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <edittextiterator.hxx>
#include <document.hxx>
#include <table.hxx>
#include <column.hxx>

namespace sc {

EditTextIterator::EditTextIterator( const ScDocument& rDoc, SCTAB nTab ) :
    mrTable(*rDoc.maTabs.at(nTab)),
    mnCol(0),
    mpCells(nullptr),
    miEnd(maPos.first)
{
    init();
}

void EditTextIterator::init()
{
    mnCol = 0;
    if (mnCol >= mrTable.aCol.size())
        mnCol = -1;

    if (mnCol != -1)
    {
        mpCells = &mrTable.aCol[mnCol].maCells;
        maPos = mpCells->position(0);
        miEnd = mpCells->end();
    }
}

const EditTextObject* EditTextIterator::seek()
{
    while (maPos.first->type != sc::element_type_edittext)
    {
        incBlock();
        if (maPos.first == miEnd)
        {
            // Move to the next column.
            ++mnCol;
            if (mnCol >= mrTable.aCol.size())
                // No more columns.
                return nullptr;

            mpCells = &mrTable.aCol[mnCol].maCells;
            maPos = mpCells->position(0);
            miEnd = mpCells->end();
        }
    }

    // We are on the right block type.
    return sc::edittext_block::at(*maPos.first->data, maPos.second);
}

void EditTextIterator::incBlock()
{
    ++maPos.first;
    maPos.second = 0;
}

const EditTextObject* EditTextIterator::first()
{
    init();
    if (mnCol == -1)
        return nullptr;
    return seek();
}

const EditTextObject* EditTextIterator::next()
{
    if (mnCol == -1)
        return nullptr;

    if (maPos.first == miEnd)
        return nullptr;

    // increment position by one
    if (maPos.second + 1 < maPos.first->size)
        // Increment within the block.
        ++maPos.second;
    else
        incBlock();

    return seek();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

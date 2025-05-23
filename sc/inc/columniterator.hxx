/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stddef.h>
#include "address.hxx"
#include <mdds/multi_type_vector/types.hpp>
#include "mtvelements.hxx"
#include <sal/types.h>
#include "types.hxx"
class ScColumn;
class ScDocument;
struct ScRefCellValue;

class ScColumnTextWidthIterator
{
    const size_t mnEnd;
    size_t mnCurPos;
    sc::CellTextAttrStoreType::iterator miBlockCur;
    sc::CellTextAttrStoreType::iterator miBlockEnd;
    sc::celltextattr_block::iterator miDataCur;
    sc::celltextattr_block::iterator miDataEnd;

public:
    ScColumnTextWidthIterator(const ScColumnTextWidthIterator&) = delete;
    const ScColumnTextWidthIterator& operator=(const ScColumnTextWidthIterator&) = delete;
    ScColumnTextWidthIterator(const ScDocument& rDoc, ScColumn& rCol, SCROW nStartRow,
                              SCROW nEndRow);

    /**
     * @param rDoc document instance.
     * @param rStartPos position of the first cell from which to start
     *                  iteration. Note that the caller must ensure that this
     *                  position is valid; the constructor does not check its
     *                  validity.
     * @param nEndRow end row position.
     */
    ScColumnTextWidthIterator(const ScDocument& rDoc, const ScAddress& rStartPos, SCROW nEndRow);

    void next();
    bool hasCell() const;
    SCROW getPos() const;
    sal_uInt16 getValue() const;
    void setValue(sal_uInt16 nVal);

private:
    void init(const ScDocument& rDoc, SCROW nStartRow, SCROW nEndRow);
    void getDataIterators(size_t nOffsetInBlock);
    void checkEndRow();
};

namespace sc
{
/**
 * This iterator lets you iterate over cells over specified range in a
 * single column.  It does not modify the state of the cells, and therefore
 * is thread safe.
 */
class ColumnIterator
{
    CellStoreType::const_position_type maPos;
    CellStoreType::const_position_type maPosEnd;
    bool mbComplete;

public:
    ColumnIterator(const CellStoreType& rCells, SCROW nRow1, SCROW nRow2);

    void next();

    SCROW getRow() const;

    bool hasCell() const;

    mdds::mtv::element_t getType() const;

    ScRefCellValue getCell() const;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

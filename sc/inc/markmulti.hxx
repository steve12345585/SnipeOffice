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

#pragma once

#include "segmenttree.hxx"
#include "markarr.hxx"

#include <vector>

class ScRangeList;
struct ScSheetLimits;

class SC_DLLPUBLIC ScMultiSel
{

private:
    std::vector<ScMarkArray> aMultiSelContainer;
    ScMarkArray aRowSel;
    const ScSheetLimits& mrSheetLimits;

friend class ScMultiSelIter;

public:
    ScMultiSel(ScSheetLimits const &);
    ScMultiSel(const ScMultiSel& rMultiSel) = default;
    ScMultiSel(ScMultiSel&& rMultiSel) = default;

    ScMultiSel& operator=(const ScMultiSel& rMultiSel);
    ScMultiSel& operator=(ScMultiSel&& rMultiSel);

    SCCOL GetMultiSelectionCount() const;
    bool HasMarks( SCCOL nCol ) const;
    bool HasOneMark( SCCOL nCol, SCROW& rStartRow, SCROW& rEndRow ) const;
    bool GetMark( SCCOL nCol, SCROW nRow ) const;
    bool IsAllMarked( SCCOL nCol, SCROW nStartRow, SCROW nEndRow ) const;
    bool HasEqualRowsMarked( SCCOL nCol1, SCCOL nCol2 ) const;
    SCROW GetNextMarked( SCCOL nCol, SCROW nRow, bool bUp ) const;
    // Returns the first column of the range [column,nLastCol] for which
    // all those columns have equal marks. Value returned is not less than nMinCol.
    SCCOL GetStartOfEqualColumns( SCCOL nLastCol, SCCOL nMinCol = 0 ) const;
    void SetMarkArea( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCROW nEndRow, bool bMark );
    void Set( ScRangeList const & );
    bool IsRowMarked( SCROW nRow ) const;
    bool IsRowRangeMarked( SCROW nStartRow, SCROW nEndRow ) const;
    bool IsEmpty() const { return ( aMultiSelContainer.empty() && !aRowSel.HasMarks() ); }
    ScMarkArray GetMarkArray( SCCOL nCol ) const;
    void Clear();
    void MarkAllCols( SCROW nStartRow, SCROW nEndRow );
    bool HasAnyMarks() const;
    void ShiftCols(SCCOL nStartCol, sal_Int32 nColOffset);
    void ShiftRows(SCROW nStartRow, sal_Int32 nRowOffset);

    // For faster access from within ScMarkData, instead of creating
    // ScMultiSelIter with ScFlatBoolRowSegments bottleneck.
    const ScMarkArray& GetRowSelArray() const { return aRowSel; }
    const ScMarkArray* GetMultiSelArray( SCCOL nCol ) const;
};

class ScMultiSelIter
{

private:
    std::unique_ptr<ScFlatBoolRowSegments>  pRowSegs;
    ScMarkArrayIter                         aMarkArrayIter;
    SCROW nNextSegmentStart;
public:
    ScMultiSelIter( const ScMultiSel& rMultiSel, SCCOL nCol );

    bool Next( SCROW& rTop, SCROW& rBottom );
    /** Only to be used by ScMultiSel::IsAllMarked() or otherwise sure that a
        segment tree is actually used. */
    bool GetRangeData( SCROW nRow, ScFlatBoolRowSegments::RangeData& rRowRange ) const;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

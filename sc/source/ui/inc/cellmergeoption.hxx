/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <address.hxx>

#include <set>

struct ScCellMergeOption
{
    ::std::set<SCTAB> maTabs;
    SCCOL mnStartCol;
    SCROW mnStartRow;
    SCCOL mnEndCol;
    SCROW mnEndRow;
    bool mbCenter;

    explicit ScCellMergeOption(const ScRange& rRange);
    SC_DLLPUBLIC explicit ScCellMergeOption(SCCOL nStartCol, SCROW nStartRow,
                                            SCCOL nEndCol, SCROW nEndRow,
                                            bool bCenter = false);

    ScRange getSingleRange(SCTAB nTab) const;
    ScRange getFirstSingleRange() const;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

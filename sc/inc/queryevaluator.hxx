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

#include <memory>
#include <vector>
#include <unordered_map>

#include "queryentry.hxx"

class ScDocument;
class ScTable;
struct ScQueryParam;
class CollatorWrapper;
struct ScRefCellValue;
struct ScInterpreterContext;

namespace sc
{
class TableColumnBlockPositionSet;
}
namespace svl
{
class SharedStringPool;
}
namespace utl
{
class TransliterationWrapper;
}

class ScQueryEvaluator
{
    ScDocument& mrDoc;
    svl::SharedStringPool& mrStrPool;
    const ScTable& mrTab;
    const ScQueryParam& mrParam;
    bool* mpTestEqualCondition;
    utl::TransliterationWrapper* mpTransliteration;
    CollatorWrapper* mpCollator;
    const bool mbMatchWholeCell;
    const bool mbCaseSensitive;
    ScInterpreterContext* mpContext;

    const SCSIZE mnEntryCount;
    bool* mpPasst;
    bool* mpTest;
    static constexpr SCSIZE nFixedBools = 32;
    bool maBool[nFixedBools];
    bool maTest[nFixedBools];
    std::unique_ptr<bool[]> mpBoolDynamic;
    std::unique_ptr<bool[]> mpTestDynamic;

    std::unordered_map<FormulaError, svl::SharedString> mCachedSharedErrorStrings;
    // The "outside" index in these two is the index of ScQueryEntry in ScQueryParam.
    std::vector<std::vector<double>> mCachedSortedItemValues;
    std::vector<std::vector<const rtl_uString*>> mCachedSortedItemStrings;

    static bool isPartialTextMatchOp(ScQueryOp eOp);
    static bool isTextMatchOp(ScQueryOp eOp);
    static bool isMatchWholeCellHelper(bool docMatchWholeCell, ScQueryOp eOp);
    bool isMatchWholeCell(ScQueryOp eOp) const;
    void setupTransliteratorIfNeeded();
    void setupCollatorIfNeeded();

    bool isRealWildOrRegExp(const ScQueryEntry& rEntry) const;
    bool isTestWildOrRegExp(const ScQueryEntry& rEntry) const;
    static bool isQueryByValueForCell(const ScRefCellValue& rCell);

    sal_uInt32 getNumFmt(SCCOL nCol, SCROW nRow);

    std::pair<bool, bool> compareByValue(const ScRefCellValue& rCell, SCCOL nCol, SCROW nRow,
                                         const ScQueryEntry& rEntry,
                                         const ScQueryEntry::Item& rItem);

    bool isFastCompareByString(const ScQueryEntry& rEntry) const;
    template <bool bFast = false>
    std::pair<bool, bool> compareByString(const ScQueryEntry& rEntry,
                                          const ScQueryEntry::Item& rItem,
                                          const ScRefCellValue& rCell, SCROW nRow);
    std::pair<bool, bool> compareByTextColor(SCCOL nCol, SCROW nRow,
                                             const ScQueryEntry::Item& rItem);
    std::pair<bool, bool> compareByBackgroundColor(SCCOL nCol, SCROW nRow,
                                                   const ScQueryEntry::Item& rItem);

    static std::pair<bool, bool> compareByRangeLookup(const ScRefCellValue& rCell,
                                                      const ScQueryEntry& rEntry,
                                                      const ScQueryEntry::Item& rItem);

    std::pair<bool, bool> processEntry(SCROW nRow, SCCOL nCol, const ScRefCellValue& aCell,
                                       const ScQueryEntry& rEntry, size_t nEntryIndex);

    bool equalCellSharedString(const ScRefCellValue& rCell, SCROW nRow, SCCOLROW nField,
                               bool bCaseSens, const svl::SharedString& rString);

    template <typename TFunctor>
    auto visitCellSharedString(const ScRefCellValue& rCell, SCROW nRow, SCCOL nCol,
                               const TFunctor& rOper);

public:
    ScQueryEvaluator(ScDocument& rDoc, const ScTable& rTab, const ScQueryParam& rParam,
                     ScInterpreterContext* pContext = nullptr, bool* pTestEqualCondition = nullptr,
                     bool bNewSearchFunction = false);

    bool ValidQuery(SCROW nRow, const ScRefCellValue* pCell = nullptr,
                    sc::TableColumnBlockPositionSet* pBlockPos = nullptr);

    static bool isQueryByValue(ScQueryOp eOp, ScQueryEntry::QueryType eType,
                               const ScRefCellValue& rCell);
    static bool isQueryByString(ScQueryOp eOp, ScQueryEntry::QueryType eType,
                                const ScRefCellValue& rCell);
    OUString getCellString(const ScRefCellValue& rCell, SCROW nRow, SCCOL nCol);
    svl::SharedString getCellSharedString(const ScRefCellValue& rCell, SCROW nRow, SCCOL nCol);
    static bool isMatchWholeCell(const ScDocument& rDoc, ScQueryOp eOp);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <memory>
#include <interpretercontext.hxx>
#include <queryparam.hxx>
#include <queryentry.hxx>
#include <scmatrix.hxx>

#include <svl/sharedstringpool.hxx>
#include <svl/numformat.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>

#include <algorithm>

namespace {

const size_t MAXQUERY = 8;

class FindByField
{
    SCCOLROW mnField;
public:
    explicit FindByField(SCCOLROW nField) : mnField(nField) {}
    bool operator() (const ScQueryEntry& rpEntry) const
    {
        return rpEntry.bDoQuery && rpEntry.nField == mnField;
    }
};

struct FindUnused
{
    bool operator() (const ScQueryEntry& rpEntry) const
    {
        return !rpEntry.bDoQuery;
    }
};

}

ScQueryParamBase::const_iterator ScQueryParamBase::begin() const
{
    return m_Entries.begin();
}

ScQueryParamBase::const_iterator ScQueryParamBase::end() const
{
    return m_Entries.end();
}

ScQueryParamBase::ScQueryParamBase() :
    eSearchType(utl::SearchParam::SearchType::Normal),
    bHasHeader(true),
    bHasTotals(false),
    bByRow(true),
    bInplace(true),
    bCaseSens(false),
    bDuplicate(false),
    mbRangeLookup(false)
{
    m_Entries.resize(MAXQUERY);
}

ScQueryParamBase::ScQueryParamBase(const ScQueryParamBase& r) :
    eSearchType(r.eSearchType), bHasHeader(r.bHasHeader), bHasTotals(r.bHasTotals), bByRow(r.bByRow),
    bInplace(r.bInplace), bCaseSens(r.bCaseSens), bDuplicate(r.bDuplicate),
    mbRangeLookup(r.mbRangeLookup), m_Entries(r.m_Entries)
{
}

ScQueryParamBase& ScQueryParamBase::operator=(const ScQueryParamBase& r)
{
    if (this != &r)
    {
        eSearchType = r.eSearchType;
        bHasHeader  = r.bHasHeader;
        bHasTotals = r.bHasTotals;
        bByRow = r.bByRow;
        bInplace = r.bInplace;
        bCaseSens = r.bCaseSens;
        bDuplicate = r.bDuplicate;
        mbRangeLookup = r.mbRangeLookup;
        m_Entries = r.m_Entries;
    }
    return *this;
}

ScQueryParamBase::~ScQueryParamBase()
{
}

bool ScQueryParamBase::IsValidFieldIndex() const
{
    return true;
}

SCSIZE ScQueryParamBase::GetEntryCount() const
{
    return m_Entries.size();
}

const ScQueryEntry& ScQueryParamBase::GetEntry(SCSIZE n) const
{
    return m_Entries[n];
}

ScQueryEntry& ScQueryParamBase::GetEntry(SCSIZE n)
{
    return m_Entries[n];
}

ScQueryEntry& ScQueryParamBase::AppendEntry()
{
    // Find the first unused entry.
    EntriesType::iterator itr = std::find_if(
        m_Entries.begin(), m_Entries.end(), FindUnused());

    if (itr != m_Entries.end())
        // Found!
        return *itr;

    // Add a new entry to the end.
    m_Entries.push_back(ScQueryEntry());
    return m_Entries.back();
}

ScQueryEntry* ScQueryParamBase::FindEntryByField(SCCOLROW nField, bool bNew)
{
    EntriesType::iterator itr = std::find_if(
        m_Entries.begin(), m_Entries.end(), FindByField(nField));

    if (itr != m_Entries.end())
    {
        // existing entry found!
        return &*itr;
    }

    if (!bNew)
        // no existing entry found, and we are not creating a new one.
        return nullptr;

    return &AppendEntry();
}

std::vector<ScQueryEntry*> ScQueryParamBase::FindAllEntriesByField(SCCOLROW nField)
{
    std::vector<ScQueryEntry*> aEntries;

    auto fFind = FindByField(nField);

    for (auto& rxEntry : m_Entries)
        if (fFind(rxEntry))
            aEntries.push_back(&rxEntry);

    return aEntries;
}

bool ScQueryParamBase::RemoveEntryByField(SCCOLROW nField)
{
    EntriesType::iterator itr = std::find_if(
        m_Entries.begin(), m_Entries.end(), FindByField(nField));
    bool bRet = false;

    if (itr != m_Entries.end())
    {
        m_Entries.erase(itr);
        if (m_Entries.size() < MAXQUERY)
            // Make sure that we have at least MAXQUERY number of entries at
            // all times.
            m_Entries.resize(MAXQUERY);
        bRet = true;
    }

    return bRet;
}

void ScQueryParamBase::RemoveAllEntriesByField(SCCOLROW nField)
{
    while( RemoveEntryByField( nField ) ) {}
}

void ScQueryParamBase::Resize(size_t nNew)
{
    if (nNew < MAXQUERY)
        nNew = MAXQUERY;                // never less than MAXQUERY

    m_Entries.resize(nNew);
}

void ScQueryParamBase::FillInExcelSyntax(
    svl::SharedStringPool& rPool, const OUString& rCellStr, SCSIZE nIndex, ScInterpreterContext* pContext )
{
    if (nIndex >= m_Entries.size())
        Resize(nIndex+1);

    ScQueryEntry& rEntry = GetEntry(nIndex);
    ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
    bool bByEmpty = false;
    bool bByNonEmpty = false;

    if (rCellStr.isEmpty())
        rItem.maString = svl::SharedString::getEmptyString();
    else
    {
        rEntry.bDoQuery = true;
        // Operatoren herausfiltern
        if (rCellStr[0] == '<')
        {
            if (rCellStr.getLength() > 1 && rCellStr[1] == '>')
            {
                rItem.maString = rPool.intern(rCellStr.copy(2));
                rEntry.eOp   = SC_NOT_EQUAL;
                if (rCellStr.getLength() == 2)
                    bByNonEmpty = true;
            }
            else if (rCellStr.getLength() > 1 && rCellStr[1] == '=')
            {
                rItem.maString = rPool.intern(rCellStr.copy(2));
                rEntry.eOp   = SC_LESS_EQUAL;
            }
            else
            {
                rItem.maString = rPool.intern(rCellStr.copy(1));
                rEntry.eOp   = SC_LESS;
            }
        }
        else if (rCellStr[0]== '>')
        {
            if (rCellStr.getLength() > 1 && rCellStr[1] == '=')
            {
                rItem.maString = rPool.intern(rCellStr.copy(2));
                rEntry.eOp   = SC_GREATER_EQUAL;
            }
            else
            {
                rItem.maString = rPool.intern(rCellStr.copy(1));
                rEntry.eOp   = SC_GREATER;
            }
        }
        else
        {
            if (rCellStr[0] == '=')
            {
                rItem.maString = rPool.intern(rCellStr.copy(1));
                if (rCellStr.getLength() == 1)
                    bByEmpty = true;
            }
            else
                rItem.maString = rPool.intern(rCellStr);
            rEntry.eOp = SC_EQUAL;
        }
    }

    if (!pContext)
        return;

    /* TODO: pContext currently is also used as a flag whether matching
     * empty cells with an empty string is triggered from the interpreter.
     * This could be handled independently if all queries should support
     * it, needs to be evaluated if that actually is desired. */

    // Interpreter queries have only one query, also QueryByEmpty and
    // QueryByNonEmpty rely on that.
    if (nIndex != 0)
        return;

    // (empty = empty) is a match, and (empty <> not-empty) also is a
    // match. (empty = 0) is not a match.
    rItem.mbMatchEmpty = ((rEntry.eOp == SC_EQUAL && rItem.maString.isEmpty())
            || (rEntry.eOp == SC_NOT_EQUAL && !rItem.maString.isEmpty()));

    // SetQueryBy override item members with special values, so do this last.
    if (bByEmpty)
        rEntry.SetQueryByEmpty();
    else if (bByNonEmpty)
        rEntry.SetQueryByNonEmpty();
    else
    {
        sal_uInt32 nFormat = 0;
        bool bNumber = pContext->NFIsNumberFormat( rItem.maString.getString(), nFormat, rItem.mfVal);
        rItem.meType = bNumber ? ScQueryEntry::ByValue : ScQueryEntry::ByString;
    }
}

ScQueryParamTable::ScQueryParamTable() :
    nCol1(0),nRow1(0),nCol2(0),nRow2(0),nTab(0)
{
}

ScQueryParamTable::~ScQueryParamTable()
{
}

ScQueryParam::ScQueryParam() :
    bDestPers(true),
    nDestTab(0),
    nDestCol(0),
    nDestRow(0)
{
    Clear();
}

ScQueryParam::ScQueryParam( const ScQueryParam& ) = default;

ScQueryParam::ScQueryParam( const ScDBQueryParamInternal& r ) :
    ScQueryParamBase(r),
    ScQueryParamTable(r),
    bDestPers(true),
    nDestTab(0),
    nDestCol(0),
    nDestRow(0)
{
}

ScQueryParam::~ScQueryParam()
{
}

void ScQueryParam::Clear()
{
    nCol1=nCol2 = 0;
    nRow1=nRow2 = 0;
    nTab = SCTAB_MAX;
    eSearchType = utl::SearchParam::SearchType::Normal;
    bHasHeader = bHasTotals = bCaseSens = false;
    bInplace = bByRow = bDuplicate = true;

    for (auto & itr : m_Entries)
    {
        itr.Clear();
    }

    ClearDestParams();
}

void ScQueryParam::ClearDestParams()
{
    bDestPers = true;
    nDestTab = 0;
    nDestCol = 0;
    nDestRow = 0;
}

ScQueryParam& ScQueryParam::operator=( const ScQueryParam& ) = default;

bool ScQueryParam::operator==( const ScQueryParam& rOther ) const
{
    bool bEqual = false;

    // Are the number of queries equal?
    SCSIZE nUsed      = 0;
    SCSIZE nOtherUsed = 0;
    SCSIZE nEntryCount = GetEntryCount();
    SCSIZE nOtherEntryCount = rOther.GetEntryCount();

    while (nUsed<nEntryCount && m_Entries[nUsed].bDoQuery) ++nUsed;
    while (nOtherUsed<nOtherEntryCount && rOther.m_Entries[nOtherUsed].bDoQuery)
        ++nOtherUsed;

    if (   (nUsed       == nOtherUsed)
        && (nCol1       == rOther.nCol1)
        && (nRow1       == rOther.nRow1)
        && (nCol2       == rOther.nCol2)
        && (nRow2       == rOther.nRow2)
        && (nTab        == rOther.nTab)
        && (bHasHeader  == rOther.bHasHeader)
        && (bHasTotals  == rOther.bHasTotals)
        && (bByRow      == rOther.bByRow)
        && (bInplace    == rOther.bInplace)
        && (bCaseSens   == rOther.bCaseSens)
        && (eSearchType == rOther.eSearchType)
        && (bDuplicate  == rOther.bDuplicate)
        && (bDestPers   == rOther.bDestPers)
        && (nDestTab    == rOther.nDestTab)
        && (nDestCol    == rOther.nDestCol)
        && (nDestRow    == rOther.nDestRow) )
    {
        bEqual = true;
        for ( SCSIZE i=0; i<nUsed && bEqual; i++ )
            bEqual = m_Entries[i] == rOther.m_Entries[i];
    }
    return bEqual;
}

void ScQueryParam::MoveToDest()
{
    if (!bInplace)
    {
        SCCOL nDifX = nDestCol - nCol1;
        SCROW nDifY = nDestRow - nRow1;
        SCTAB nDifZ = nDestTab - nTab;

        nCol1 = sal::static_int_cast<SCCOL>( nCol1 + nDifX );
        nRow1 = sal::static_int_cast<SCROW>( nRow1 + nDifY );
        nCol2 = sal::static_int_cast<SCCOL>( nCol2 + nDifX );
        nRow2 = sal::static_int_cast<SCROW>( nRow2 + nDifY );
        nTab  = sal::static_int_cast<SCTAB>( nTab  + nDifZ );
        size_t n = m_Entries.size();
        for (size_t i=0; i<n; i++)
            m_Entries[i].nField += nDifX;

        bInplace = true;
    }
    else
    {
        OSL_FAIL("MoveToDest, bInplace == TRUE");
    }
}

ScDBQueryParamBase::ScDBQueryParamBase(DataType eType) :
    mnField(-1),
    mbSkipString(true),
    meType(eType)
{
}

ScDBQueryParamBase::~ScDBQueryParamBase()
{
}

ScDBQueryParamInternal::ScDBQueryParamInternal() :
    ScDBQueryParamBase(ScDBQueryParamBase::INTERNAL)
{
}

ScDBQueryParamInternal::~ScDBQueryParamInternal()
{
}

bool ScDBQueryParamInternal::IsValidFieldIndex() const
{
    return nCol1 <= mnField && mnField <= nCol2;
}

ScDBQueryParamMatrix::ScDBQueryParamMatrix() :
    ScDBQueryParamBase(ScDBQueryParamBase::MATRIX)
{
}

bool ScDBQueryParamMatrix::IsValidFieldIndex() const
{
    SCSIZE nC, nR;
    mpMatrix->GetDimensions(nC, nR);
    return 0 <= mnField && o3tl::make_unsigned(mnField) <= nC;
}

ScDBQueryParamMatrix::~ScDBQueryParamMatrix()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

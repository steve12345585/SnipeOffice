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

#include <dpsdbtab.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <dpfilteredcache.hxx>
#include <document.hxx>
#include <dpobject.hxx>

#include <com/sun/star/sdb/CommandType.hpp>

using namespace com::sun::star;

using ::std::vector;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Any;

sal_Int32 ScImportSourceDesc::GetCommandType() const
{
    sal_Int32 nSdbType = -1;

    switch ( nType )
    {
        case sheet::DataImportMode_SQL:   nSdbType = sdb::CommandType::COMMAND; break;
        case sheet::DataImportMode_TABLE: nSdbType = sdb::CommandType::TABLE;   break;
        case sheet::DataImportMode_QUERY: nSdbType = sdb::CommandType::QUERY;   break;
        default:
            ;
    }
    return nSdbType;
}

const ScDPCache* ScImportSourceDesc::CreateCache(const ScDPDimensionSaveData* pDimData) const
{
    if (!mpDoc)
        return nullptr;

    sal_Int32 nSdbType = GetCommandType();
    if (nSdbType < 0)
        return nullptr;

    ScDPCollection::DBCaches& rCaches = mpDoc->GetDPCollection()->GetDBCaches();
    return rCaches.getCache(nSdbType, aDBName, aObject, pDimData);
}

ScDatabaseDPData::ScDatabaseDPData(
    const ScDocument* pDoc, const ScDPCache& rCache) :
    ScDPTableData(pDoc),
    aCacheTable(rCache)
{
}

ScDatabaseDPData::~ScDatabaseDPData()
{
}

void ScDatabaseDPData::DisposeData()
{
    //TODO: use OpenDatabase here?
    aCacheTable.clear();
}

sal_Int32 ScDatabaseDPData::GetColumnCount()
{
    CreateCacheTable();
    return GetCacheTable().getColSize();
}

OUString ScDatabaseDPData::getDimensionName(sal_Int32 nColumn)
{
    if (getIsDataLayoutDimension(nColumn))
    {
        //TODO: different internal and display names?
        //return "Data";
        return ScResId(STR_PIVOT_DATA);
    }

    CreateCacheTable();
    return aCacheTable.getFieldName(static_cast<SCCOL>(nColumn));
}

bool ScDatabaseDPData::getIsDataLayoutDimension(sal_Int32 nColumn)
{
    return ( nColumn == GetCacheTable().getColSize());
}

bool ScDatabaseDPData::IsDateDimension(sal_Int32 /* nDim */)
{
    //TODO: later...
    return false;
}

void ScDatabaseDPData::SetEmptyFlags( bool /* bIgnoreEmptyRows */, bool /* bRepeatIfEmpty */ )
{
    //  not used for database data
    //TODO: disable flags
}

void ScDatabaseDPData::CreateCacheTable()
{
    if (!aCacheTable.empty())
        // cache table already created.
        return;

    aCacheTable.fillTable();
}

void ScDatabaseDPData::FilterCacheTable(std::vector<ScDPFilteredCache::Criterion>&& rCriteria, std::unordered_set<sal_Int32>&& rCatDims)
{
    CreateCacheTable();
    aCacheTable.filterByPageDimension(
        rCriteria, (IsRepeatIfEmpty() ? std::move(rCatDims) : std::unordered_set<sal_Int32>()));
}

void ScDatabaseDPData::GetDrillDownData(std::vector<ScDPFilteredCache::Criterion>&& rCriteria, std::unordered_set<sal_Int32>&& rCatDims, Sequence< Sequence<Any> >& rData)
{
    CreateCacheTable();
    sal_Int32 nRowSize = aCacheTable.getRowSize();
    if (!nRowSize)
        return;

    aCacheTable.filterTable(
        rCriteria, rData, IsRepeatIfEmpty() ? std::move(rCatDims) : std::unordered_set<sal_Int32>());
}

void ScDatabaseDPData::CalcResults(CalcInfo& rInfo, bool bAutoShow)
{
    CreateCacheTable();
    CalcResultsFromCacheTable( aCacheTable, rInfo, bAutoShow);
}

const ScDPFilteredCache& ScDatabaseDPData::GetCacheTable() const
{
    return aCacheTable;
}

void ScDatabaseDPData::ReloadCacheTable()
{
    aCacheTable.clear();
    CreateCacheTable();
}

#if DUMP_PIVOT_TABLE

void ScDatabaseDPData::Dump() const
{
    // TODO : Implement this.
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

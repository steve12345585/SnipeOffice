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

#include <TableConnectionData.hxx>
#include <utility>
#include <osl/diagnose.h>

using namespace dbaui;

OTableConnectionData::OTableConnectionData()
{
    Init();
}

OTableConnectionData::OTableConnectionData(TTableWindowData::value_type _pReferencingTable
                                          ,TTableWindowData::value_type _pReferencedTable )
 :m_pReferencingTable(std::move(_pReferencingTable))
 ,m_pReferencedTable(std::move(_pReferencedTable))
{
    Init();
}

void OTableConnectionData::Init()
{
    // initialise linedatalist with defaults
    OSL_ENSURE(m_vConnLineData.empty(), "OTableConnectionData::Init() : call only with empty line list!");
    ResetConnLines();
        // this creates the defaults
}

OTableConnectionData::OTableConnectionData( const OTableConnectionData& rConnData )
{
    *this = rConnData;
}

void OTableConnectionData::CopyFrom(const OTableConnectionData& rSource)
{
    *this = rSource;
    // here I revert to the (non-virtual) operator =, which only copies my members
}

OTableConnectionData::~OTableConnectionData()
{
    // delete LineDataList
    OConnectionLineDataVec().swap(m_vConnLineData);
}

OTableConnectionData& OTableConnectionData::operator=( const OTableConnectionData& rConnData )
{
    if (&rConnData == this)
        return *this;

    m_pReferencingTable = rConnData.m_pReferencingTable;
    m_pReferencedTable = rConnData.m_pReferencedTable;
    m_aConnName = rConnData.m_aConnName;

    // clear line list
    ResetConnLines();

    // and copy
    for (auto const& elem : rConnData.GetConnLineDataList())
        m_vConnLineData.push_back(new OConnectionLineData(*elem));

    return *this;
}

void OTableConnectionData::SetConnLine( sal_uInt16 nIndex, const OUString& rSourceFieldName, const OUString& rDestFieldName )
{
    if (sal_uInt16(m_vConnLineData.size()) < nIndex)
        return;

        // == still allowed, this corresponds to an Append

    if (m_vConnLineData.size() == nIndex)
    {
        AppendConnLine(rSourceFieldName, rDestFieldName);
        return;
    }

    OConnectionLineDataRef pConnLineData = m_vConnLineData[nIndex];
    OSL_ENSURE(pConnLineData != nullptr, "OTableConnectionData::SetConnLine : have invalid LineData object");

    pConnLineData->SetSourceFieldName( rSourceFieldName );
    pConnLineData->SetDestFieldName( rDestFieldName );
}

bool OTableConnectionData::AppendConnLine( const OUString& rSourceFieldName, const OUString& rDestFieldName )
{
    for (auto const& elem : m_vConnLineData)
    {
        if(elem->GetDestFieldName() == rDestFieldName && elem->GetSourceFieldName() == rSourceFieldName)
            return true;
    }
    OConnectionLineDataRef pNew = new OConnectionLineData(rSourceFieldName, rDestFieldName);
    if (!pNew.is())
        return false;

    m_vConnLineData.push_back(pNew);
    return true;
}

void OTableConnectionData::ResetConnLines()
{
    OConnectionLineDataVec().swap(m_vConnLineData);
}

std::shared_ptr<OTableConnectionData> OTableConnectionData::NewInstance() const
{
    return std::make_shared<OTableConnectionData>();
}

OConnectionLineDataVec::size_type OTableConnectionData::normalizeLines()
{
    // remove empty lines
    OConnectionLineDataVec::size_type nCount = m_vConnLineData.size();
    OConnectionLineDataVec::size_type nRet = nCount;
    for(OConnectionLineDataVec::size_type i = 0; i < nCount;)
    {
        if(m_vConnLineData[i]->GetSourceFieldName().isEmpty() && m_vConnLineData[i]->GetDestFieldName().isEmpty())
        {
            m_vConnLineData.erase(m_vConnLineData.begin()+i);
            --nCount;
            if (i < nRet)
                nRet=i;
        }
        else
            ++i;
    }
    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

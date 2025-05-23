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

#include <string.h>

#include <pagedata.hxx>

#include <osl/diagnose.h>

ScPrintRangeData::ScPrintRangeData()
{
    bTopDown = bAutomatic = true;
    nFirstPage = 1;
}

ScPrintRangeData::~ScPrintRangeData()
{
}

void ScPrintRangeData::SetPagesX( size_t nCount, const SCCOL* pData )
{
    mvPageEndX.resize( nCount );
    memcpy( mvPageEndX.data(), pData, nCount * sizeof(SCCOL) );
}

void ScPrintRangeData::SetPagesY( size_t nCount, const SCROW* pData )
{
    mvPageEndY.resize(nCount);
    memcpy( mvPageEndY.data(), pData, nCount * sizeof(SCROW) );
}

ScPageBreakData::ScPageBreakData(size_t nMax)
{
    nUsed = 0;
    if (nMax)
        pData.reset( new ScPrintRangeData[nMax] );
    nAlloc = nMax;
}

ScPageBreakData::~ScPageBreakData()
{
}

ScPrintRangeData& ScPageBreakData::GetData(size_t nPos)
{
    OSL_ENSURE(nPos < nAlloc, "ScPageBreakData::GetData bumm");

    if ( nPos >= nUsed )
    {
        OSL_ENSURE(nPos == nUsed, "ScPageBreakData::GetData wrong order");
        nUsed = nPos+1;
    }

    return pData[nPos];
}

bool ScPageBreakData::operator==( const ScPageBreakData& rOther ) const
{
    if ( nUsed != rOther.nUsed )
        return false;

    for (size_t i=0; i<nUsed; i++)
        if ( pData[i].GetPrintRange() != rOther.pData[i].GetPrintRange() )
            return false;

    //! compare ScPrintRangeData completely ??

    return true;
}

void ScPageBreakData::AddPages()
{
    if ( nUsed > 1 )
    {
        tools::Long nPage = pData[0].GetFirstPage();
        for (size_t i=0; i+1<nUsed; i++)
        {
            nPage += static_cast<tools::Long>(pData[i].GetPagesX())*pData[i].GetPagesY();
            pData[i+1].SetFirstPage( nPage );
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

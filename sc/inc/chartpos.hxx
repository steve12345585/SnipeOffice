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

#include "rangelst.hxx"
#include <memory>
#include <map>

// map of row number to ScAddress*
typedef std::map<SCROW, std::unique_ptr<ScAddress>> RowMap;
// map of column number to RowMap
typedef std::map<SCCOL, RowMap>    ColumnMap;

class ScChartPositionMap
{
    friend class ScChartPositioner;

            std::unique_ptr<std::unique_ptr<ScAddress>[]> ppData;
            std::unique_ptr<std::unique_ptr<ScAddress>[]> ppColHeader;
            std::unique_ptr<std::unique_ptr<ScAddress>[]> ppRowHeader;
            sal_uInt64          nCount;
            SCCOL               nColCount;
            SCROW               nRowCount;

                                ScChartPositionMap( SCCOL nChartCols, SCROW nChartRows,
                                    SCCOL nColAdd,      // header columns
                                    SCROW nRowAdd,      // header rows
                                    ColumnMap& rCols        // table with col tables with address*
                                    );

                                ScChartPositionMap( const ScChartPositionMap& ) = delete;
            ScChartPositionMap& operator=( const ScChartPositionMap& ) = delete;

public:
                                ~ScChartPositionMap();  //! deletes all ScAddress*

            SCCOL               GetColCount() const { return nColCount; }
            SCROW               GetRowCount() const { return nRowCount; }

            bool                IsValid( SCCOL nCol, SCROW nRow ) const
                                    { return nCol < nColCount && nRow < nRowCount; }
                                // data column by column
            sal_uInt64          GetIndex( SCCOL nCol, SCROW nRow ) const
                                    { return static_cast<sal_uInt64>(nCol) * nRowCount + nRow; }

            const ScAddress*    GetPosition( sal_uInt64 nIndex ) const
                                    {
                                        if ( nIndex < nCount )
                                            return ppData[ nIndex ].get();
                                        return nullptr;
                                    }

                                    //! might be NULL indicating "no value"
            const ScAddress*    GetPosition( SCCOL nChartCol, SCROW nChartRow ) const
                                    {
                                        if ( IsValid( nChartCol, nChartRow ) )
                                            return ppData[ GetIndex( nChartCol, nChartRow ) ].get();
                                        return nullptr;
                                    }
            const ScAddress*    GetColHeaderPosition( SCCOL nChartCol ) const
                                    {
                                        if ( nChartCol < nColCount )
                                            return ppColHeader[ nChartCol ].get();
                                        return nullptr;
                                    }
            const ScAddress*    GetRowHeaderPosition( SCROW nChartRow ) const
                                    {
                                        if ( nChartRow < nRowCount )
                                            return ppRowHeader[ nChartRow ].get();
                                        return nullptr;
                                    }
};

enum class ScChartGlue {
    NA,
    NONE,      // old mimic
    Cols,      // old mimic
    Rows,
    Both
};

class ScDocument;

class ScChartPositioner final             // only parameter struct
{
    ScRangeListRef  aRangeListRef;
    ScDocument& rDocument;
    std::unique_ptr<ScChartPositionMap> pPositionMap;
    ScChartGlue eGlue;
    SCCOL       nStartCol;
    SCROW       nStartRow;
    bool        bColHeaders;
    bool        bRowHeaders;
    bool        bDummyUpperLeft;

    void        CheckColRowHeaders();

    void        GlueState();        // summarised areas
    void        CreatePositionMap();

public:
    ScChartPositioner( ScDocument& rDoc, SCTAB nTab,
                    SCCOL nStartColP, SCROW nStartRowP,
                    SCCOL nEndColP, SCROW nEndRowP );
    ScChartPositioner( ScDocument& rDoc, ScRangeListRef xRangeList );
    ScChartPositioner( const ScChartPositioner& rPositioner );

    ~ScChartPositioner();

    const ScRangeListRef&   GetRangeList() const { return aRangeListRef; }
    void    SetRangeList( const ScRange& rNew );

    void    SetHeaders(bool bCol, bool bRow) { bColHeaders=bCol; bRowHeaders=bRow; }
    bool    HasColHeaders() const            { return bColHeaders; }
    bool    HasRowHeaders() const            { return bRowHeaders; }

    void                        InvalidateGlue();
    const ScChartPositionMap*   GetPositionMap();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

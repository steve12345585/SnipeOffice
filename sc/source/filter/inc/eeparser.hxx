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

#include <tools/gen.hxx>
#include <utility>
#include <comphelper/errcode.hxx>
#include <vcl/graph.hxx>
#include <svl/itemset.hxx>
#include <svl/itempool.hxx>
#include <editeng/editdata.hxx>
#include <optional>
#include <address.hxx>
#include <memory>
#include <vector>
#include <map>

const char nHorizontal = 1;
const char nVertical = 2;

struct ScHTMLImage
{
    OUString            aURL;
    Size                aSize;
    Point               aSpace;
    OUString            aFilterName;
    std::optional<Graphic>
                        oGraphic;       // is taken over by WriteToDocument
    char                nDir;           // 1==hori, 2==verti, 3==both

    ScHTMLImage() :
        aSize( 0, 0 ), aSpace( 0, 0 ), nDir( nHorizontal )
        {}
};

struct ScEEParseEntry
{
    SfxItemSet          aItemSet;
    ESelection          aSel;           // Selection in EditEngine
    std::optional<OUString>
                        pValStr;        // HTML possibly SDVAL string
    std::optional<OUString> moFormulaStr;
    std::optional<formula::FormulaGrammar::Grammar> moFormulaGrammar;
    std::optional<OUString>
                        pNumStr;        // HTML possibly SDNUM string
    std::optional<OUString>
                        pName;          // HTML possibly anchor/RangeName
    OUString            aAltText;       // HTML IMG ALT Text
    std::vector< std::unique_ptr<ScHTMLImage> > maImageList;       // graphics in this cell
    SCCOL               nCol;           // relative to the beginning of the parse
    SCROW               nRow;
    sal_uInt16          nTab;           // HTML TableInTable
    sal_uInt16          nTwips;         // RTF ColAdjust etc.
    SCCOL               nColOverlap;    // merged cells if >1
    SCROW               nRowOverlap;    // merged cells if >1
    sal_uInt16          nOffset;        // HTML PixelOffset
    sal_uInt16          nWidth;         // HTML PixelWidth
    bool                bHasGraphic:1;  // HTML any image loaded
    bool                bEntirePara:1;  // true = use entire paragraph, false = use selection

    ScEEParseEntry( SfxItemPool* pPool ) :
        aItemSet( *pPool ),
        nCol(SCCOL_MAX), nRow(SCROW_MAX), nTab(0),
        nTwips(0), nColOverlap(1), nRowOverlap(1),
        nOffset(0), nWidth(0), bHasGraphic(false), bEntirePara(true)
        {}

    ScEEParseEntry( SfxItemSet _aItemSet ) :
        aItemSet(std::move( _aItemSet )),
        nCol(SCCOL_MAX), nRow(SCROW_MAX), nTab(0),
        nTwips(0), nColOverlap(1), nRowOverlap(1),
        nOffset(0), nWidth(0), bHasGraphic(false), bEntirePara(true)
        {}

    ~ScEEParseEntry()
    {
        maImageList.clear();
    }
};

class EditEngine;

typedef std::map<SCCOL, sal_uInt16> ColWidthsMap;

class ScEEParser
{
protected:
    EditEngine*         pEdit;
    rtl::Reference<SfxItemPool>  pPool;
    rtl::Reference<SfxItemPool>  pDocPool;
    std::vector<std::shared_ptr<ScEEParseEntry>> maList;
    std::shared_ptr<ScEEParseEntry> mxActEntry;
    ColWidthsMap        maColWidths;
    int                 nRtfLastToken;
    SCCOL               nColCnt;
    SCROW               nRowCnt;
    SCCOL               nColMax;
    SCROW               nRowMax;

    void                NewActEntry( const ScEEParseEntry* );

public:
                        ScEEParser( EditEngine* );
    virtual             ~ScEEParser();

    virtual ErrCode         Read( SvStream&, const OUString& rBaseURL ) = 0;

    const ColWidthsMap&     GetColWidths() const { return maColWidths; }
    ColWidthsMap&           GetColWidths() { return maColWidths; }
    void                    GetDimensions( SCCOL& nCols, SCROW& nRows ) const
                                { nCols = nColMax; nRows = nRowMax; }

    size_t                  ListSize() const{ return maList.size(); }
    ScEEParseEntry*         ListEntry( size_t index ) { return maList[index].get(); }
    const ScEEParseEntry*   ListEntry( size_t index ) const { return maList[index].get(); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

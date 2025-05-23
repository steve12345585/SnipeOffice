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

/*
    IMPORTANT:
    Strictly adhere to the following sequence when creating a pivot table:

    pPivot->SetColFields(aColArr, aColCount)
    pPivot->SetRowFields(aRowArr, aRowCount)
    pPivot->SetDataFields(aDataArr, aDataCount)
    if (pPivot->CreateData())
    {
        pPivotDrawData();
        pPivotReleaseData();
    }

    Make sure that either ColArr or RowArr contains a PivotDataField entry.
*/

#pragma once

#include <rtl/ustring.hxx>
#include <tools/long.hxx>
#include "types.hxx"
#include "scdllapi.h"
#include "dpglobal.hxx"
#include "calcmacros.hxx"

#include <vector>
#include <memory>

#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/sheet/DataPilotFieldReference.hpp>
#include <com/sun/star/sheet/DataPilotFieldSortInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldLayoutInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldAutoShowInfo.hpp>

#define PIVOT_DATA_FIELD        (SCCOL(-1))

struct SC_DLLPUBLIC ScDPName
{
    OUString     maName;         ///< Original name of the dimension.
    OUString     maLayoutName;   ///< Layout name (display name)
    sal_uInt8    mnDupCount;

    ScDPName();
    explicit ScDPName(OUString aName, OUString aLayoutName, sal_uInt8 nDupCount);
};

struct ScDPLabelData
{
    OUString   maName;         ///< Original name of the dimension.
    OUString   maLayoutName;   ///< Layout name (display name)
    OUString   maSubtotalName;
    SCCOL      mnCol;          ///< 0-based field index (not the source column index)
    tools::Long       mnOriginalDim;  ///< original dimension index (>= 0 for duplicated dimension)
    PivotFunc  mnFuncMask;     ///< Page/Column/Row subtotal function.
    sal_Int32  mnUsedHier;     ///< Used hierarchy.
    sal_Int32  mnFlags;        ///< Flags from the DataPilotSource dimension
    sal_uInt8  mnDupCount;
    bool       mbShowAll:1;    ///< true = Show all (also empty) results.
    bool       mbIsValue:1;    ///< true = Sum or count in data field.
    bool       mbDataLayout:1;
    bool       mbRepeatItemLabels:1;

    struct Member
    {
        OUString maName;
        OUString maLayoutName;
        bool mbVisible;
        bool mbShowDetails;

        Member();

        /**
         * @return the name that should be displayed in the dp dialogs i.e.
         * when the layout name is present, use it, or else use the original
         * name.
         */
        SC_DLLPUBLIC OUString const & getDisplayName() const;
    };
    std::vector<Member>                    maMembers;
    css::uno::Sequence<OUString>           maHiers;        ///< Hierarchies.
    css::sheet::DataPilotFieldSortInfo     maSortInfo;     ///< Sorting info.
    css::sheet::DataPilotFieldLayoutInfo   maLayoutInfo;   ///< Layout info.
    css::sheet::DataPilotFieldAutoShowInfo maShowInfo;     ///< AutoShow info.

    ScDPLabelData();

    /**
     * @return the name that should be displayed in the dp dialogs i.e. when
     * the layout name is present, use it, or else use the original name.
     */
    SC_DLLPUBLIC OUString const & getDisplayName() const;
};

typedef std::vector< std::unique_ptr<ScDPLabelData> > ScDPLabelDataVector;

struct ScPivotField
{
    css::sheet::DataPilotFieldReference maFieldRef;

    tools::Long        mnOriginalDim; ///< >= 0 for duplicated field.
    PivotFunc   nFuncMask;
    SCCOL       nCol;          ///< 0-based dimension index (not source column index)
    sal_uInt8   mnDupCount;

    explicit ScPivotField( SCCOL nNewCol = 0 );

    tools::Long getOriginalDim() const;
};

typedef std::vector< ScPivotField > ScPivotFieldVector;

struct ScPivotParam
{
    SCCOL nCol;           ///< Cursor Position /
    SCROW nRow;           ///< or start of destination area
    SCTAB nTab;

    ScDPLabelDataVector maLabelArray;
    ScPivotFieldVector  maPageFields;
    ScPivotFieldVector  maColFields;
    ScPivotFieldVector  maRowFields;
    ScPivotFieldVector  maDataFields;

    bool bIgnoreEmptyRows;
    bool bDetectCategories;
    bool bMakeTotalCol;
    bool bMakeTotalRow;

    ScPivotParam();
    ScPivotParam( const ScPivotParam& r );
    ~ScPivotParam();

    ScPivotParam&   operator=  ( const ScPivotParam& r );
    void SetLabelData(const ScDPLabelDataVector& r);
};

struct ScPivotFuncData
{
    css::sheet::DataPilotFieldReference maFieldRef;

    tools::Long       mnOriginalDim;
    PivotFunc  mnFuncMask;
    SCCOL      mnCol;
    sal_uInt8  mnDupCount;

    explicit ScPivotFuncData( SCCOL nCol, PivotFunc nFuncMask );

#if DEBUG_PIVOT_TABLE
    void Dump() const;
#endif
};

typedef std::vector<ScDPName> ScDPNameVec;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

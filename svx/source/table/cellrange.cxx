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

#include <sal/config.h>

#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <utility>

#include "cellrange.hxx"


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::table;


namespace sdr::table {

CellRange::CellRange( TableModelRef xTable, sal_Int32 nLeft, sal_Int32 nTop, sal_Int32 nRight, sal_Int32 nBottom )
: mxTable(std::move( xTable ))
, mnLeft(nLeft)
, mnTop(nTop)
, mnRight(nRight)
, mnBottom(nBottom)
{
}


CellRange::~CellRange()
{
}


// ICellRange


sal_Int32 CellRange::getLeft()
{
    return mnLeft;
}

sal_Int32 CellRange::getTop()
{
    return mnTop;
}

sal_Int32 CellRange::getRight()
{
    return mnRight;
}

sal_Int32 CellRange::getBottom()
{
    return mnBottom;
}

Reference< XTable > CellRange::getTable()
{
    return mxTable;
}


// XCellRange


Reference< XCell > SAL_CALL CellRange::getCellByPosition( sal_Int32 nColumn, sal_Int32 nRow )
{
    return mxTable->getCellByPosition( mnLeft + nColumn, mnTop + nRow );
}


Reference< XCellRange > SAL_CALL CellRange::getCellRangeByPosition( sal_Int32 nLeft, sal_Int32 nTop, sal_Int32 nRight, sal_Int32 nBottom )
{
    if( (nLeft >= 0 ) && (nTop >= 0) && (nRight >= nLeft) && (nBottom >= nTop)  )
    {
        nLeft += mnLeft;
        nTop += mnTop;
        nRight += mnLeft;
        nBottom += mnTop;

        const sal_Int32 nMaxColumns = (mnRight == -1) ? mxTable->getColumnCount() : mnLeft;
        const sal_Int32 nMaxRows = (mnBottom == -1) ? mxTable->getRowCount() : mnBottom;
        if( (nLeft < nMaxColumns) && (nRight < nMaxColumns) && (nTop < nMaxRows) && (nBottom < nMaxRows) )
        {
            return mxTable->getCellRangeByPosition( nLeft, nTop, nRight, nBottom );
        }
    }
    throw IndexOutOfBoundsException();
}


Reference< XCellRange > SAL_CALL CellRange::getCellRangeByName( const OUString& /*aRange*/ )
{
    return Reference< XCellRange >();
}


}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

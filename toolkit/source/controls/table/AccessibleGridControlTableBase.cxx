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

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <controls/table/AccessibleGridControlTableBase.hxx>
#include <vcl/svapp.hxx>
#include <comphelper/sequence.hxx>

using css::uno::Reference;
using css::uno::Sequence;
using css::uno::Any;

using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;
using namespace ::vcl;


namespace accessibility {


AccessibleGridControlTableBase::AccessibleGridControlTableBase(
        const Reference< XAccessible >& rxParent,
        svt::table::TableControl& rTable,
        AccessibleTableControlObjType eObjType ) :
    AccessibleGridControlTableImplHelper( rxParent, rTable, eObjType )
{
}

// XAccessibleContext ---------------------------------------------------------

sal_Int64 SAL_CALL AccessibleGridControlTableBase::getAccessibleChildCount()
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    sal_Int64 nChildren = 0;
    if (m_eObjType == AccessibleTableControlObjType::ROWHEADERBAR)
        nChildren = m_aTable.GetRowCount();
    else if (m_eObjType == AccessibleTableControlObjType::TABLE)
        nChildren = static_cast<sal_Int64>(m_aTable.GetRowCount()) * static_cast<sal_Int64>(m_aTable.GetColumnCount());
    else if (m_eObjType == AccessibleTableControlObjType::COLUMNHEADERBAR)
        nChildren = m_aTable.GetColumnCount();
    return nChildren;
}

sal_Int16 SAL_CALL AccessibleGridControlTableBase::getAccessibleRole()
{
    SolarMutexGuard g;

    ensureAlive();
    return AccessibleRole::TABLE;
}

// XAccessibleTable -----------------------------------------------------------

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleRowCount()
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();

    if (m_eObjType == AccessibleTableControlObjType::COLUMNHEADERBAR)
        return 1;
    return  m_aTable.GetRowCount();
}

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleColumnCount()
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();

    if (m_eObjType == AccessibleTableControlObjType::ROWHEADERBAR)
        return 1;
    return m_aTable.GetColumnCount();
}

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleRowExtentAt(
        sal_Int32 nRow, sal_Int32 nColumn )
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    ensureIsValidAddress( nRow, nColumn );
    return 1;   // merged cells not supported
}

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleColumnExtentAt(
        sal_Int32 nRow, sal_Int32 nColumn )
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    ensureIsValidAddress( nRow, nColumn );
    return 1;   // merged cells not supported
}

Reference< XAccessible > SAL_CALL AccessibleGridControlTableBase::getAccessibleCaption()
{
    SolarMutexGuard g;

    ensureAlive();
    return nullptr;    // not supported
}

Reference< XAccessible > SAL_CALL AccessibleGridControlTableBase::getAccessibleSummary()
{
    SolarMutexGuard g;

    ensureAlive();
    return nullptr;    // not supported
}

sal_Int64 SAL_CALL AccessibleGridControlTableBase::getAccessibleIndex(
        sal_Int32 nRow, sal_Int32 nColumn )
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    ensureIsValidAddress( nRow, nColumn );
    return static_cast<sal_Int64>(nRow) * static_cast<sal_Int64>(m_aTable.GetColumnCount()) + nColumn;
}

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleRow( sal_Int64 nChildIndex )
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    ensureIsValidIndex( nChildIndex );
    return implGetRow( nChildIndex );
}

sal_Int32 SAL_CALL AccessibleGridControlTableBase::getAccessibleColumn( sal_Int64 nChildIndex )
{
    SolarMutexGuard aSolarGuard;

    ensureAlive();
    ensureIsValidIndex( nChildIndex );
    return implGetColumn( nChildIndex );
}

// internal helper methods ----------------------------------------------------

sal_Int32 AccessibleGridControlTableBase::implGetRow( sal_Int64 nChildIndex )
{
    sal_Int32 nColumns = getAccessibleColumnCount();
    return nColumns ? (nChildIndex / nColumns) : 0;
}

sal_Int32 AccessibleGridControlTableBase::implGetColumn( sal_Int64 nChildIndex )
{
    sal_Int32 nColumns = getAccessibleColumnCount();
    return nColumns ? (nChildIndex % nColumns) : 0;
}

void AccessibleGridControlTableBase::implGetSelectedRows( Sequence< sal_Int32 >& rSeq )
{
    sal_Int32 const selectionCount( m_aTable.GetSelectedRowCount() );
    rSeq.realloc( selectionCount );
    auto pSeq = rSeq.getArray();
    for ( sal_Int32 i=0; i<selectionCount; ++i )
        pSeq[i] = m_aTable.GetSelectedRowIndex(i);
}

void AccessibleGridControlTableBase::ensureIsValidRow( sal_Int32 nRow )
{
    if (nRow < 0 || nRow >= getAccessibleRowCount())
        throw lang::IndexOutOfBoundsException( u"row index is invalid"_ustr, *this );
}

void AccessibleGridControlTableBase::ensureIsValidColumn( sal_Int32 nColumn )
{
    if (nColumn < 0 || nColumn >= getAccessibleColumnCount())
        throw lang::IndexOutOfBoundsException( u"column index is invalid"_ustr, *this );
}

void AccessibleGridControlTableBase::ensureIsValidAddress(
        sal_Int32 nRow, sal_Int32 nColumn )
{
    ensureIsValidRow( nRow );
    ensureIsValidColumn( nColumn );
}

void AccessibleGridControlTableBase::ensureIsValidIndex( sal_Int64 nChildIndex )
{
    if (nChildIndex < 0 || nChildIndex >= static_cast<sal_Int64>(m_aTable.GetRowCount()) * static_cast<sal_Int64>(m_aTable.GetColumnCount()))
        throw lang::IndexOutOfBoundsException( u"child index is invalid"_ustr, *this );
}


} // namespace accessibility


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

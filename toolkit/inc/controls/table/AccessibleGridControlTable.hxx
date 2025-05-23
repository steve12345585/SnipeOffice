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

#include <controls/table/AccessibleGridControlTableBase.hxx>
#include <controls/table/AccessibleGridControlTableCell.hxx>

#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/accessibility/XAccessibleSelection.hpp>


namespace accessibility {

/** This class represents the accessible object of the data table of a
    Grid control. */
class AccessibleGridControlTable final
    : public cppu::ImplInheritanceHelper<AccessibleGridControlTableBase,
                                         css::accessibility::XAccessibleSelection>
{
public:
    AccessibleGridControlTable(
        const css::uno::Reference< css::accessibility::XAccessible >& rxParent,
            svt::table::TableControl& rTable);

private:
    virtual ~AccessibleGridControlTable() override = default;
    std::vector< rtl::Reference<AccessibleGridControlTableCell> > m_aCellVector;
public:
    // XAccessibleContext

    /** @return  The XAccessible interface of the specified child. */
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL
    getAccessibleChild( sal_Int64 nChildIndex ) override;

    /** @return  The index of this object among the parent's children. */
    virtual sal_Int64 SAL_CALL getAccessibleIndexInParent() override;

    // XAccessibleComponent

    /** @return  The accessible child rendered under the given point. */
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL
    getAccessibleAtPoint( const css::awt::Point& rPoint ) override;

    /** Grabs the focus to (the current cell of) the data table. */
    virtual void SAL_CALL grabFocus() override;

    // XAccessibleTable

    /** @return  The description text of the specified row. */
    virtual OUString SAL_CALL getAccessibleRowDescription( sal_Int32 nRow ) override;

    /** @return  The description text of the specified column. */
    virtual OUString SAL_CALL getAccessibleColumnDescription( sal_Int32 nColumn ) override;

    /** @return  The XAccessibleTable interface of the row header bar. */
    virtual css::uno::Reference< css::accessibility::XAccessibleTable > SAL_CALL
    getAccessibleRowHeaders() override;

    /** @return  The XAccessibleTable interface of the column header bar. */
    virtual css::uno::Reference< css::accessibility::XAccessibleTable > SAL_CALL
    getAccessibleColumnHeaders() override;

    /** @return  An index list of completely selected rows. */
    virtual css::uno::Sequence< sal_Int32 > SAL_CALL
    getSelectedAccessibleRows() override;

    /** @return  An index list of completely selected columns. */
    virtual css::uno::Sequence< sal_Int32 > SAL_CALL
    getSelectedAccessibleColumns() override;

    /** @return  TRUE, if the specified row is completely selected. */
    virtual sal_Bool SAL_CALL isAccessibleRowSelected( sal_Int32 nRow ) override;

    /** @return  TRUE, if the specified column is completely selected. */
    virtual sal_Bool SAL_CALL isAccessibleColumnSelected( sal_Int32 nColumn ) override;

    /** @return The XAccessible interface of the cell object at the specified
                cell position. */
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL
    getAccessibleCellAt( sal_Int32 nRow, sal_Int32 nColumn ) override;

    /** @return  TRUE, if the specified cell is selected. */
    virtual sal_Bool SAL_CALL isAccessibleSelected( sal_Int32 nRow, sal_Int32 nColumn ) override;

    // XAccessibleSelection

    /** Selects the specified child (row or column of the table). */
    virtual void SAL_CALL selectAccessibleChild( sal_Int64 nChildIndex ) override;

    /** @return  TRUE, if the specified child (row/column) is selected. */
    virtual sal_Bool SAL_CALL isAccessibleChildSelected( sal_Int64 nChildIndex ) override;

    /** Clears the complete selection. */
    virtual void SAL_CALL clearAccessibleSelection() override;

    /** Selects all children or first, if multiselection is not supported. */
    virtual void SAL_CALL selectAllAccessibleChildren() override;

    /** @return  The number of selected rows/columns. */
    virtual sal_Int64 SAL_CALL getSelectedAccessibleChildCount() override;

    /** @return  The specified selected row/column. */
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL
    getSelectedAccessibleChild( sal_Int64 nSelectedChildIndex ) override;

    /** Removes the specified row/column from the selection. */
    virtual void SAL_CALL deselectAccessibleChild( sal_Int64 nSelectedChildIndex ) override;

    // XServiceInfo

    /** @return  The name of this class. */
    virtual OUString SAL_CALL getImplementationName() override;

    // XComponent
    virtual void SAL_CALL dispose() override;

    virtual void commitEvent(sal_Int16 nEventId, const css::uno::Any& rNewValue,
                             const css::uno::Any& rOldValue) override;

private:
    // internal virtual methods

    ///** @attention  This method requires locked mutex's and a living object.
    //    @return  The bounding box (VCL rect.) in screen coordinates. */
    virtual AbsoluteScreenPixelRectangle implGetBoundingBoxOnScreen() override;

    //// internal helper methods
    ///** @attention  This method requires a locked mutex.
    //    @return  The XAccessibleTable interface of the specified header bar. */
    /// @throws css::uno::RuntimeException
    css::uno::Reference< css::accessibility::XAccessibleTable >
    implGetHeaderBar( sal_Int32 nChildIndex );
};


} // namespace accessibility



/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

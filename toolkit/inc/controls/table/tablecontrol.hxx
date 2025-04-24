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

#include <controls/table/tablecontrolinterface.hxx>
#include <controls/table/tablemodel.hxx>

#include <vcl/ctrl.hxx>
#include <vcl/seleng.hxx>

#include <memory>

enum class AccessibleTableControlObjType
{
    GRIDCONTROL,         /// The GridControl itself.
    TABLE,               /// The data table.
    ROWHEADERBAR,        /// The row header bar.
    COLUMNHEADERBAR,     /// The horizontal column header bar.
    TABLECELL,           /// A cell of the data table.
    ROWHEADERCELL,       /// A cell of the row header bar.
    COLUMNHEADERCELL,    /// A cell of the column header bar.
};

namespace svt::table
{
    class TableControl_Impl;

    //= TableControl

    /** a basic control which manages table-like data, i.e. a number of cells
        organized in <code>m</code> rows and <code>n</code> columns.

        The control itself does not do any assumptions about the concrete data
        it displays, this is encapsulated in an instance supporting the
        ->ITableModel interface.

        Also, the control does not do any assumptions about how the model's
        content is rendered. This is the responsibility of a component
        supporting the ->ITableRenderer interface (the renderer is obtained from
        the model).

        The control supports the concept of a <em>current</em> (or <em>active</em>
        cell).
    */
    class TableControl final : public Control
    {
    private:
        std::shared_ptr<TableControl_Impl>            m_pImpl;


    public:
        TableControl( vcl::Window* _pParent, WinBits _nStyle );
        virtual ~TableControl() override;
        virtual void dispose() override;

        /// sets a new table model
        void        SetModel( const PTableModel& _pModel );
        /// retrieves the current table model
        PTableModel GetModel() const;

        /** retrieves the current row

            The current row is the one which contains the active cell.

            @return
                the row index of the active cell, or ->ROW_INVALID
                if there is no active cell, e.g. because the table does
                not contain any rows or columns.
        */
        sal_Int32 GetCurrentRow() const;

        /** retrieves the current column

            The current col is the one which contains the active cell.

            @return
                the column index of the active cell, or ->COL_INVALID
                if there is no active cell, e.g. because the table does
                not contain any rows or columns.
        */
        sal_Int32 GetCurrentColumn() const;

        /** activates the cell at the given position
        */
        void    GoTo( ColPos _nColumnPos, RowPos _nRow);

        virtual void Resize() override;
        void    Select();

        /**after removing a row, updates the vector which contains the selected rows
            if the row, which should be removed, is selected, it will be erased from the vector
        */
        SelectionEngine*    getSelEngine();
        vcl::Window&             getDataWindow();

        // Window overridables
        virtual void        GetFocus() override;
        virtual void        LoseFocus() override;
        virtual void        KeyInput( const KeyEvent& rKEvt ) override;
        virtual void        StateChanged( StateChangedType i_nStateChange ) override;

        /** Creates and returns the accessible object of the whole GridControl. */
        virtual css::uno::Reference< css::accessibility::XAccessible > CreateAccessible() override;
        OUString GetAccessibleObjectName(AccessibleTableControlObjType eObjType,
                                         sal_Int32 _nRow, sal_Int32 _nCol) const;
        void GoToCell(sal_Int32 _nColumnPos, sal_Int32 _nRow);
        OUString
        GetAccessibleObjectDescription(AccessibleTableControlObjType eObjType) const;
        void FillAccessibleStateSet(sal_Int64& rStateSet,
                                    AccessibleTableControlObjType eObjType) const;

        // temporary methods
        // Those do not really belong into the public API - they're intended for firing A11Y-related events. However,
        // firing those events should be an implementation internal to the TableControl resp. TableControl_Impl,
        // instead of something triggered externally.
        void commitCellEvent(sal_Int16 const i_eventID, const css::uno::Any& i_newValue, const css::uno::Any& i_oldValue);
        void commitTableEvent(sal_Int16 const i_eventID, const css::uno::Any& i_newValue, const css::uno::Any& i_oldValue);

        sal_Int32 GetAccessibleControlCount() const;
        sal_Int32 GetRowCount() const;
        sal_Int32 GetColumnCount() const;
        bool ConvertPointToCellAddress(sal_Int32& _rnRow, sal_Int32& _rnColPos,
                                       const Point& _rPoint);
        tools::Rectangle calcHeaderRect(bool _bIsColumnBar);
        tools::Rectangle calcHeaderCellRect(bool _bIsColumnBar, sal_Int32 nPos);
        tools::Rectangle calcTableRect();
        tools::Rectangle calcCellRect(sal_Int32 _nRowPos, sal_Int32 _nColPos);
        void FillAccessibleStateSetForCell(sal_Int64& _rStateSet, sal_Int32 _nRow,
                                           sal_uInt16 _nColumnPos) const;
        OUString GetRowName(sal_Int32 _nIndex) const;
        OUString GetColumnName(sal_Int32 _nIndex) const;
        bool HasRowHeader();
        bool HasColHeader();
        OUString GetAccessibleCellText(sal_Int32 _nRowPos, sal_Int32 _nColPos) const;

        sal_Int32 GetSelectedRowCount() const;
        sal_Int32 GetSelectedRowIndex(sal_Int32 const i_selectionIndex) const;
        bool IsRowSelected(sal_Int32 const i_rowIndex) const;
        void SelectRow(sal_Int32 const i_rowIndex, bool const i_select);
        void SelectAllRows(bool const i_select);

        TableCell hitTest(const Point& rPoint) const;
        void invalidate(const TableArea aArea);

    private:
        DECL_LINK( ImplSelectHdl, LinkParamNone*, void );

    private:
        TableControl( const TableControl& ) = delete;
        TableControl& operator=( const TableControl& ) = delete;
    };


} // namespace svt::table



/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

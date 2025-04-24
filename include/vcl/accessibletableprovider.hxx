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

#ifndef INCLUDED_VCL_ACCESSIBLETABLEPROVIDER_HXX
#define INCLUDED_VCL_ACCESSIBLETABLEPROVIDER_HXX

#include <vcl/accessibility/AccessibleBrowseBoxObjType.hxx>
#include <vcl/window.hxx>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/accessibility/XAccessible.hpp>

namespace vcl
{

enum AccessibleTableChildIndex
{
    /** Child index of the column header bar (first row). Exists always. */
    BBINDEX_COLUMNHEADERBAR = 0,
    /** Child index of the row header bar ("handle column"). Exists always. */
    BBINDEX_ROWHEADERBAR    = 1,
    /** Child index of the data table. */
    BBINDEX_TABLE           = 2,
    /** Child index of the first additional control. */
    BBINDEX_FIRSTCONTROL    = 3
};


/** This abstract class provides methods to implement an accessible table object.
*/
class IAccessibleTableProvider
{
public:
    /** @return  The count of the rows. */
    virtual sal_Int32               GetRowCount() const = 0;
    /** @return  The count of the columns. */
    virtual sal_uInt16              GetColumnCount() const = 0;

    /** @return  The position of the current row. */
    virtual sal_Int32               GetCurrRow() const = 0;
    /** @return  The position of the current column. */
    virtual sal_uInt16              GetCurrColumn() const = 0;

    /** @return  The description of a row.
        @param _nRow The row which description is in demand. */
    virtual OUString         GetRowDescription( sal_Int32 _nRow ) const = 0;
    /** @return  The description of a column.
        @param _nColumn The column which description is in demand. */
    virtual OUString         GetColumnDescription( sal_uInt16 _nColumnPos ) const = 0;

    /** @return  <TRUE/>, if the object has a row header. */
    virtual bool                    HasRowHeader() const = 0; //GetColumnId
    virtual bool                    GoToCell( sal_Int32 _nRow, sal_uInt16 _nColumnPos ) = 0;

    virtual void                    SetNoSelection() = 0;
    virtual void                    SelectAll() = 0;
    virtual void                    SelectRow( sal_Int32 _nRow, bool _bSelect = true, bool bExpand = true ) = 0;
    virtual void                    SelectColumn( sal_uInt16 _nColumnPos, bool _bSelect = true ) = 0;
    virtual sal_Int32               GetSelectedRowCount() const = 0;
    virtual sal_Int32               GetSelectedColumnCount() const = 0;
    /** @return  <TRUE/>, if the row is selected. */
    virtual bool                    IsRowSelected( sal_Int32 _nRow ) const = 0;
    virtual bool                    IsColumnSelected( sal_Int32 _nColumnPos ) const = 0;
    virtual void                    GetAllSelectedRows( css::uno::Sequence< sal_Int32 >& _rRows ) const = 0;
    virtual void                    GetAllSelectedColumns( css::uno::Sequence< sal_Int32 >& _rColumns ) const = 0;

    /** @return  <TRUE/>, if the cell is visible. */
    virtual bool                    IsCellVisible( sal_Int32 _nRow, sal_uInt16 _nColumnPos ) const = 0;
    virtual OUString                GetAccessibleCellText( sal_Int32 _nRow, sal_uInt16 _nColumnPos ) const = 0;

    virtual tools::Rectangle calcHeaderRect(bool _bIsColumnBar) = 0;
    virtual tools::Rectangle calcTableRect() = 0;
    virtual tools::Rectangle calcFieldRectPixel(sal_Int32 _nRow, sal_uInt16 _nColumnPos, bool _bIsHeader) = 0;

    virtual css::uno::Reference< css::accessibility::XAccessible > CreateAccessibleCell( sal_Int32 _nRow, sal_uInt16 _nColumnPos ) = 0;
    virtual css::uno::Reference< css::accessibility::XAccessible > CreateAccessibleRowHeader( sal_Int32 _nRow ) = 0;
    virtual css::uno::Reference< css::accessibility::XAccessible > CreateAccessibleColumnHeader( sal_uInt16 _nColumnPos ) = 0;

    virtual sal_Int32               GetAccessibleControlCount() const = 0;
    virtual css::uno::Reference< css::accessibility::XAccessible >                    CreateAccessibleControl( sal_Int32 _nIndex ) = 0;
    virtual bool                    ConvertPointToControlIndex( sal_Int32& _rnIndex, const Point& _rPoint ) = 0;

    virtual bool                    ConvertPointToCellAddress( sal_Int32& _rnRow, sal_uInt16& _rnColPos, const Point& _rPoint ) = 0;
    virtual bool                    ConvertPointToRowHeader( sal_Int32& _rnRow, const Point& _rPoint ) = 0;
    virtual bool                    ConvertPointToColumnHeader( sal_uInt16& _rnColPos, const Point& _rPoint ) = 0;

    virtual OUString                GetAccessibleObjectName( AccessibleBrowseBoxObjType _eType, sal_Int32 _nPos = -1 ) const = 0;
    virtual OUString                GetAccessibleObjectDescription( AccessibleBrowseBoxObjType _eType, sal_Int32 _nPos = -1 ) const = 0;

    virtual void                    FillAccessibleStateSet( sal_Int64& _rStateSet, AccessibleBrowseBoxObjType _eType ) const = 0;
    virtual void                    FillAccessibleStateSetForCell( sal_Int64& _rStateSet, sal_Int32 _nRow, sal_uInt16 _nColumnPos ) const = 0;
    virtual void                    GrabTableFocus() = 0;

    // OutputDevice
    virtual bool                    GetGlyphBoundRects( const Point& rOrigin, const OUString& rStr, int nIndex, int nLen, std::vector< tools::Rectangle >& rVector ) = 0;

    // Window
    virtual tools::Rectangle        GetWindowExtentsRelative(const vcl::Window& rRelativeWindow) const = 0;
    virtual void                    GrabFocus() = 0;
    virtual css::uno::Reference< css::accessibility::XAccessible > GetAccessible() = 0;
    virtual vcl::Window*                 GetAccessibleParentWindow() const = 0;
    virtual vcl::Window*                 GetWindowInstance() = 0;

    virtual tools::Rectangle               GetFieldCharacterBounds(sal_Int32 _nRow,sal_Int32 _nColumnPos,sal_Int32 nIndex) = 0;
    virtual sal_Int32               GetFieldIndexAtPoint(sal_Int32 _nRow,sal_Int32 _nColumnPos,const Point& _rPoint) = 0;

protected:
    ~IAccessibleTableProvider() {}
};

} // namespace vcl

#endif // INCLUDED_VCL_ACCESSIBLETABLEPROVIDER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

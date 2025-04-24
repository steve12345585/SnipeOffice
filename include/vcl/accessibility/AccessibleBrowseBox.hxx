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

#include <cppuhelper/weakref.hxx>
#include <vcl/accessibility/AccessibleBrowseBoxBase.hxx>
#include <vcl/accessibletableprovider.hxx>

class AccessibleBrowseBoxHeaderBar;
class AccessibleBrowseBoxTable;


/** This class represents the complete accessible BrowseBox object. */
class VCL_DLLPUBLIC AccessibleBrowseBox : public AccessibleBrowseBoxBase
{

public:
    AccessibleBrowseBox(
        const css::uno::Reference< css::accessibility::XAccessible >& _rxParent,
        ::vcl::IAccessibleTableProvider& _rBrowseBox
    );

protected:
    virtual ~AccessibleBrowseBox() override;

    AccessibleBrowseBox(const AccessibleBrowseBox&) = delete;
    AccessibleBrowseBox& operator=(const AccessibleBrowseBox&) = delete;

    /** Cleans up members. */
    using AccessibleBrowseBoxBase::disposing;
    virtual void SAL_CALL disposing() override;

    // XAccessibleContext

    /** @return  The count of visible children. */
    virtual sal_Int64 SAL_CALL getAccessibleChildCount() override;

    /** @return  The XAccessible interface of the specified child. */
    virtual css::uno::Reference<
        css::accessibility::XAccessible > SAL_CALL
    getAccessibleChild( sal_Int64 nChildIndex ) override;

    // XAccessibleComponent

    /** @return
            The accessible child rendered under the given point.
    */
    virtual css::uno::Reference<
        css::accessibility::XAccessible > SAL_CALL
    getAccessibleAtPoint( const css::awt::Point& rPoint ) override;

    /** Grabs the focus to the BrowseBox. */
    virtual void SAL_CALL grabFocus() override;

    // XServiceInfo

    /** @return
            The name of this class.
    */
    virtual OUString SAL_CALL getImplementationName() override;

public:
    // helper functions
    /** commitHeaderBarEvent commit the event at all listeners of the column/row header bar
        @param nEventId
            the event id
        @param rNewValue
            the new value
        @param rOldValue
            the old value
        @param _bColumnHeaderBar
            true if a column based header bar, false if a row based header bar
    */
    void commitHeaderBarEvent(sal_Int16 nEventId,
            const css::uno::Any& rNewValue,
            const css::uno::Any& rOldValue, bool _bColumnHeaderBar);

    // helper functions
    /** commitTableEvent commit the event at all listeners of the table
        @param nEventId
            the event id
        @param rNewValue
            the new value
        @param rOldValue
            the old value
    */
    void commitTableEvent(sal_Int16 nEventId,
            const css::uno::Any& rNewValue,
            const css::uno::Any& rOldValue);

    /** returns the accessible object for the row or the column header bar
    */
    css::uno::Reference<
        css::accessibility::XAccessible >
        getHeaderBar( AccessibleBrowseBoxObjType _eObjType )
        {
            return implGetHeaderBar(_eObjType);
        }

    /** returns the accessible object for the table representation
    */
    css::uno::Reference<
        css::accessibility::XAccessible >
        getTable( )
        {
            return implGetTable();
        }

protected:
    // internal virtual methods

    /** @attention  This method requires locked mutex's and a living object.
        @return  The bounding box (VCL rect.) relative to the parent window. */
    virtual tools::Rectangle implGetBoundingBox() override;

    // internal helper methods

    /** This method creates (once) and returns the accessible data table child.
        @attention  This method requires locked mutex's and a living object.
        @return  The XAccessible interface of the data table. */
    css::uno::Reference<
        css::accessibility::XAccessible > implGetTable();

    /** This method creates (once) and returns the specified header bar.
        @attention  This method requires locked mutex's and a living object.
        @return  The XAccessible interface of the header bar. */
    css::uno::Reference<
        css::accessibility::XAccessible >
        implGetHeaderBar( AccessibleBrowseBoxObjType eObjType );

    /** This method returns one of the children that are always present:
        Data table, row and column header bar or corner control.
        @attention  This method requires locked mutex's and a living object.
        @return  The XAccessible interface of the specified child. */
    css::uno::Reference<
        css::accessibility::XAccessible >
    implGetFixedChild( sal_Int64 nChildIndex );

    /** This method creates and returns an accessible table.
        @return  An AccessibleBrowseBoxTable. */
    virtual rtl::Reference<AccessibleBrowseBoxTable> createAccessibleTable();

private:
    /** The data table child. */
    rtl::Reference<AccessibleBrowseBoxTable>                    mxTable;

    /** The header bar for rows ("handle column"). */
    rtl::Reference<AccessibleBrowseBoxHeaderBar>                mxRowHeaderBar;

    /** The header bar for columns (first row of the table). */
    rtl::Reference<AccessibleBrowseBoxHeaderBar>                mxColumnHeaderBar;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <accessibility/vclxaccessiblemenu.hxx>

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <comphelper/accessiblecontexthelper.hxx>
#include <vcl/menu.hxx>


using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::accessibility;
using namespace ::comphelper;


// VCLXAccessibleMenu


bool VCLXAccessibleMenu::IsFocused()
{
    bool bFocused = false;

    if ( IsHighlighted() && !IsChildHighlighted() )
        bFocused = true;

    return bFocused;
}


bool VCLXAccessibleMenu::IsPopupMenuOpen()
{
    bool bPopupMenuOpen = false;

    if ( m_pParent )
    {
        PopupMenu* pPopupMenu = m_pParent->GetPopupMenu( m_pParent->GetItemId( m_nItemPos ) );
        if ( pPopupMenu && pPopupMenu->IsMenuVisible() )
            bPopupMenuOpen = true;
    }

    return bPopupMenuOpen;
}


// XServiceInfo


OUString VCLXAccessibleMenu::getImplementationName()
{
    return u"com.sun.star.comp.toolkit.AccessibleMenu"_ustr;
}


Sequence< OUString > VCLXAccessibleMenu::getSupportedServiceNames()
{
    return { u"com.sun.star.awt.AccessibleMenu"_ustr };
}


// XAccessibleContext


sal_Int64 VCLXAccessibleMenu::getAccessibleChildCount(  )
{
    OExternalLockGuard aGuard( this );

    return GetChildCount();
}


Reference< XAccessible > VCLXAccessibleMenu::getAccessibleChild( sal_Int64 i )
{
    OExternalLockGuard aGuard( this );

    if ( i < 0 || i >= GetChildCount() )
        throw IndexOutOfBoundsException();

    return GetChild( i );
}


sal_Int16 VCLXAccessibleMenu::getAccessibleRole(  )
{
    OExternalLockGuard aGuard( this );

    return AccessibleRole::MENU;
}


// XAccessibleComponent


Reference< XAccessible > VCLXAccessibleMenu::getAccessibleAtPoint( const awt::Point& rPoint )
{
    OExternalLockGuard aGuard( this );

    return GetChildAt( rPoint );
}


// XAccessibleSelection


void VCLXAccessibleMenu::selectAccessibleChild( sal_Int64 nChildIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nChildIndex < 0 || nChildIndex >= GetChildCount() )
        throw IndexOutOfBoundsException();

    SelectChild( nChildIndex );
}


sal_Bool VCLXAccessibleMenu::isAccessibleChildSelected( sal_Int64 nChildIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nChildIndex < 0 || nChildIndex >= GetChildCount() )
        throw IndexOutOfBoundsException();

    return IsChildSelected( nChildIndex );
}


void VCLXAccessibleMenu::clearAccessibleSelection(  )
{
    OExternalLockGuard aGuard( this );

    DeSelectAll();
}


void VCLXAccessibleMenu::selectAllAccessibleChildren(  )
{
    // This method makes no sense in a menu, and so does nothing.
}


sal_Int64 VCLXAccessibleMenu::getSelectedAccessibleChildCount(  )
{
    OExternalLockGuard aGuard( this );

    return implGetSelectedAccessibleChildCount();
}

sal_Int64 VCLXAccessibleMenu::implGetSelectedAccessibleChildCount(  )
{
    sal_Int64 nRet = 0;

    for ( sal_Int64 i = 0, nCount = GetChildCount(); i < nCount; i++ )
    {
        if ( IsChildSelected( i ) )
            ++nRet;
    }

    return nRet;
}

Reference< XAccessible > VCLXAccessibleMenu::getSelectedAccessibleChild( sal_Int64 nSelectedChildIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nSelectedChildIndex < 0 || nSelectedChildIndex >= getSelectedAccessibleChildCount() )
        throw IndexOutOfBoundsException();

    Reference< XAccessible > xChild;

    for ( sal_Int64 i = 0, j = 0, nCount = GetChildCount(); i < nCount; i++ )
    {
        if ( IsChildSelected( i ) && ( j++ == nSelectedChildIndex ) )
        {
            xChild = GetChild( i );
            break;
        }
    }

    return xChild;
}


void VCLXAccessibleMenu::deselectAccessibleChild( sal_Int64 nChildIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nChildIndex < 0 || nChildIndex >= GetChildCount() )
        throw IndexOutOfBoundsException();

    DeSelectAll();
}


OUString VCLXAccessibleMenu::getAccessibleActionDescription ( sal_Int32 nIndex )
{
    OExternalLockGuard aGuard( this );

    if ( nIndex < 0 || nIndex >= getAccessibleActionCount() )
        throw IndexOutOfBoundsException();

    return OUString(  );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

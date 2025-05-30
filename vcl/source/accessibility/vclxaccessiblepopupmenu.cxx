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

#include <accessibility/vclxaccessiblepopupmenu.hxx>

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <comphelper/accessiblecontexthelper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>

using namespace ::com::sun::star::accessibility;
using namespace ::com::sun::star::uno;
using namespace ::comphelper;




bool VCLXAccessiblePopupMenu::IsFocused()
{
    return !IsChildHighlighted();
}


// XServiceInfo


OUString VCLXAccessiblePopupMenu::getImplementationName()
{
    return u"com.sun.star.comp.toolkit.AccessiblePopupMenu"_ustr;
}


Sequence< OUString > VCLXAccessiblePopupMenu::getSupportedServiceNames()
{
    return { u"com.sun.star.awt.AccessiblePopupMenu"_ustr };
}


// XAccessibleContext


sal_Int64 VCLXAccessiblePopupMenu::getAccessibleIndexInParent(  )
{
    OExternalLockGuard aGuard( this );

    return 0;
}


sal_Int16 VCLXAccessiblePopupMenu::getAccessibleRole(  )
{
    OExternalLockGuard aGuard( this );

    return AccessibleRole::POPUP_MENU;
}


// XAccessibleExtendedComponent


sal_Int32 VCLXAccessiblePopupMenu::getBackground(  )
{
    OExternalLockGuard aGuard( this );

    return sal_Int32(Application::GetSettings().GetStyleSettings().GetMenuColor());
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

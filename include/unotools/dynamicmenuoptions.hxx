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
#ifndef INCLUDED_UNOTOOLS_DYNAMICMENUOPTIONS_HXX
#define INCLUDED_UNOTOOLS_DYNAMICMENUOPTIONS_HXX

#include <unotools/unotoolsdllapi.h>
#include <rtl/ustring.hxx>
#include <vector>

/*-****************************************************************************************************************
    @descr  struct to hold information about one menu entry.
****************************************************************************************************************-*/
struct SvtDynMenuEntry
{
    OUString    sURL;
    OUString    sTitle;
    OUString    sImageIdentifier;
    OUString    sTargetName;
};

/*-************************************************************************************************************
    @descr          You can use these enum values to specify right menu if you call our interface methods.
*//*-*************************************************************************************************************/
enum class EDynamicMenuType
{
    NewMenu       =   0,
    WizardMenu    =   1
};


/*-************************************************************************************************************
    @short          collect information about dynamic menus
    @descr          Make it possible to configure dynamic menu structures of menus like "new" or "wizard".
    @devstatus      ready to use
*//*-*************************************************************************************************************/

namespace SvtDynamicMenuOptions
{

    /*-****************************************************************************************************
        @short      return complete specified list
        @descr      Call it to get all entries of an dynamic menu.
                    We return a list of all nodes with its names and properties.
        @param      "eMenu" select right menu.
        @return     A list of menu items is returned.

        @onerror    We return an empty list.
    *//*-*****************************************************************************************************/

    UNOTOOLS_DLLPUBLIC std::vector< SvtDynMenuEntry > GetMenu( EDynamicMenuType eMenu );


};      // namespace SvtDynamicMenuOptions

#endif // INCLUDED_UNOTOOLS_DYNAMICMENUOPTIONS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

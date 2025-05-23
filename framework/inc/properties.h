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

#include <com/sun/star/uno/Any.hxx>

namespace framework{

/** properties for "Frame" class */

inline constexpr OUString FRAME_PROPNAME_ASCII_DISPATCHRECORDERSUPPLIER = u"DispatchRecorderSupplier"_ustr;
inline constexpr OUString FRAME_PROPNAME_ASCII_ISHIDDEN = u"IsHidden"_ustr;
inline constexpr OUString FRAME_PROPNAME_ASCII_LAYOUTMANAGER = u"LayoutManager"_ustr;
inline constexpr OUString FRAME_PROPNAME_ASCII_TITLE = u"Title"_ustr;
inline constexpr OUString FRAME_PROPNAME_ASCII_INDICATORINTERCEPTION = u"IndicatorInterception"_ustr;
inline constexpr OUString FRAME_PROPNAME_ASCII_URL = u"URL"_ustr;

// Please add new entries alphabetical sorted and correct all other handles!
// Start counting with 0, so it can be used as direct index into an array too.
#define FRAME_PROPHANDLE_DISPATCHRECORDERSUPPLIER       0
#define FRAME_PROPHANDLE_ISHIDDEN                       1
#define FRAME_PROPHANDLE_LAYOUTMANAGER                  2
#define FRAME_PROPHANDLE_TITLE                          3
#define FRAME_PROPHANDLE_INDICATORINTERCEPTION          4
#define FRAME_PROPHANDLE_URL 5

/** properties for "LayoutManager" class */

inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_MENUBARCLOSER = u"MenuBarCloser"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_AUTOMATICTOOLBARS = u"AutomaticToolbars"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_REFRESHVISIBILITY = u"RefreshContextToolbarVisibility"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_HIDECURRENTUI = u"HideCurrentUI"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_LOCKCOUNT = u"LockCount"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_PRESERVE_CONTENT_SIZE = u"PreserveContentSize"_ustr;
inline constexpr OUString LAYOUTMANAGER_PROPNAME_ASCII_REFRESHTOOLTIP = u"RefreshContextToolbarToolTip"_ustr;

#define LAYOUTMANAGER_PROPNAME_MENUBARCLOSER            LAYOUTMANAGER_PROPNAME_ASCII_MENUBARCLOSER

#define LAYOUTMANAGER_PROPHANDLE_MENUBARCLOSER          0
#define LAYOUTMANAGER_PROPHANDLE_AUTOMATICTOOLBARS      1
#define LAYOUTMANAGER_PROPHANDLE_REFRESHVISIBILITY      2
#define LAYOUTMANAGER_PROPHANDLE_HIDECURRENTUI          3
#define LAYOUTMANAGER_PROPHANDLE_LOCKCOUNT              4
#define LAYOUTMANAGER_PROPHANDLE_PRESERVE_CONTENT_SIZE  5
#define LAYOUTMANAGER_PROPHANDLE_REFRESHTOOLTIP         6

/** properties for "UICommandDescription" class */
inline constexpr OUString UICOMMANDDESCRIPTION_NAMEACCESS_COMMANDIMAGELIST = u"private:resource/image/commandimagelist"_ustr;
inline constexpr OUString UICOMMANDDESCRIPTION_NAMEACCESS_COMMANDROTATEIMAGELIST = u"private:resource/image/commandrotateimagelist"_ustr;
inline constexpr OUString UICOMMANDDESCRIPTION_NAMEACCESS_COMMANDMIRRORIMAGELIST = u"private:resource/image/commandmirrorimagelist"_ustr;


#define UICOMMANDDESCRIPTION_PROPERTIES_TOGGLEBUTTON            8

/** properties for "AutoRecovery" class */

inline constexpr OUString AUTORECOVERY_PROPNAME_ASCII_EXISTS_RECOVERYDATA = u"ExistsRecoveryData"_ustr;
inline constexpr OUString AUTORECOVERY_PROPNAME_ASCII_EXISTS_SESSIONDATA = u"ExistsSessionData"_ustr;
inline constexpr OUString AUTORECOVERY_PROPNAME_ASCII_CRASHED = u"Crashed"_ustr;

#define AUTORECOVERY_PROPNAME_EXISTS_RECOVERYDATA       AUTORECOVERY_PROPNAME_ASCII_EXISTS_RECOVERYDATA
#define AUTORECOVERY_PROPNAME_EXISTS_SESSIONDATA        AUTORECOVERY_PROPNAME_ASCII_EXISTS_SESSIONDATA
#define AUTORECOVERY_PROPNAME_CRASHED                   AUTORECOVERY_PROPNAME_ASCII_CRASHED

#define AUTORECOVERY_PROPHANDLE_EXISTS_RECOVERYDATA     0
#define AUTORECOVERY_PROPHANDLE_EXISTS_SESSIONDATA      1
#define AUTORECOVERY_PROPHANDLE_CRASHED                 2

/** properties for Filter config */

inline constexpr OUString FILTER_PROPNAME_ASCII_DOCUMENTSERVICE = u"DocumentService"_ustr;

/** properties for office module config (Setup.xcu) */

inline constexpr OUString OFFICEFACTORY_PROPNAME_ASCII_UINAME = u"ooSetupFactoryUIName"_ustr;
inline constexpr OUString OFFICEFACTORY_PROPNAME_ASCII_ICON = u"ooSetupFactoryIcon"_ustr;

/** provides some helper methods to implement property sets. */

class PropHelper
{
    public:

    /** checks if given property will be changed by this settings.
     *  We compare the content of the given any values. If they are different we return sal_True - sal_False otherwise.
     *
     *  @param  aCurrentValue   contains the current value for this property
     *  @param  aNewValue       contains the new value for this property
     *  @param  aOldValue       returns the current value, in case something will be changed
     *  @param  aChangedValue   returns the new value, in case something will be changed
     *
     *  @return <True/> if value of this property will be changed; <False/> otherwise.
     */

    static bool willPropertyBeChanged( const css::uno::Any& aCurrentValue ,
                                           const css::uno::Any& aNewValue     ,
                                                 css::uno::Any& aOldValue     ,
                                                 css::uno::Any& aChangedValue )
    {
        bool bChanged = false;

        // clear return parameter to be sure, to put out only valid values ...
        aOldValue.clear();
        aChangedValue.clear();

        // if value change ...
        bChanged = aCurrentValue != aNewValue;
        if (bChanged)
        {
            // ... set information of change.
            aOldValue     = aCurrentValue;
            aChangedValue = aNewValue;
        }

        return bChanged;
    }
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

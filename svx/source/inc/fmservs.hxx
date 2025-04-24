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
#ifndef INCLUDED_SVX_SOURCE_INC_FMSERVS_HXX
#define INCLUDED_SVX_SOURCE_INC_FMSERVS_HXX

#include <sal/config.h>

#include <com/sun/star/uno/Reference.hxx>
#include <svx/svxdllapi.h>

namespace com::sun::star::lang { class XMultiServiceFactory; }

inline constexpr OUString FM_COMPONENT_EDIT = u"stardiv.one.form.component.Edit"_ustr;
inline constexpr OUString FM_COMPONENT_TEXTFIELD = u"stardiv.one.form.component.TextField"_ustr;
inline constexpr OUString FM_COMPONENT_LISTBOX = u"stardiv.one.form.component.ListBox"_ustr;
inline constexpr OUString FM_COMPONENT_COMBOBOX = u"stardiv.one.form.component.ComboBox"_ustr;
inline constexpr OUString FM_COMPONENT_RADIOBUTTON = u"stardiv.one.form.component.RadioButton"_ustr;
inline constexpr OUString FM_COMPONENT_GROUPBOX = u"stardiv.one.form.component.GroupBox"_ustr;
inline constexpr OUString FM_COMPONENT_FIXEDTEXT = u"stardiv.one.form.component.FixedText"_ustr;
inline constexpr OUString FM_COMPONENT_COMMANDBUTTON = u"stardiv.one.form.component.CommandButton"_ustr;
inline constexpr OUString FM_COMPONENT_CHECKBOX = u"stardiv.one.form.component.CheckBox"_ustr;
inline constexpr OUString FM_COMPONENT_GRID = u"stardiv.one.form.component.Grid"_ustr;
inline constexpr OUString FM_COMPONENT_GRIDCONTROL = u"stardiv.one.form.component.GridControl"_ustr;
inline constexpr OUString FM_COMPONENT_IMAGEBUTTON = u"stardiv.one.form.component.ImageButton"_ustr;
inline constexpr OUString FM_COMPONENT_FILECONTROL = u"stardiv.one.form.component.FileControl"_ustr;
inline constexpr OUString FM_COMPONENT_TIMEFIELD = u"stardiv.one.form.component.TimeField"_ustr;
inline constexpr OUString FM_COMPONENT_DATEFIELD = u"stardiv.one.form.component.DateField"_ustr;
inline constexpr OUString FM_COMPONENT_NUMERICFIELD = u"stardiv.one.form.component.NumericField"_ustr;
inline constexpr OUString FM_COMPONENT_CURRENCYFIELD = u"stardiv.one.form.component.CurrencyField"_ustr;
inline constexpr OUString FM_COMPONENT_PATTERNFIELD = u"stardiv.one.form.component.PatternField"_ustr;
inline constexpr OUString FM_COMPONENT_FORMATTEDFIELD = u"stardiv.one.form.component.FormattedField"_ustr;
inline constexpr OUString FM_COMPONENT_HIDDEN = u"stardiv.one.form.component.Hidden"_ustr;
inline constexpr OUString FM_COMPONENT_HIDDENCONTROL = u"stardiv.one.form.component.HiddenControl"_ustr;
inline constexpr OUString FM_COMPONENT_IMAGECONTROL = u"stardiv.one.form.component.ImageControl"_ustr;
inline constexpr OUString FM_CONTROL_GRID = u"stardiv.one.form.control.Grid"_ustr;
inline constexpr OUString FM_CONTROL_GRIDCONTROL = u"stardiv.one.form.control.GridControl"_ustr;
inline constexpr OUString SRV_SDB_CONNECTION = u"com.sun.star.sdb.Connection"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_FORM = u"com.sun.star.form.component.Form"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_TEXTFIELD = u"com.sun.star.form.component.TextField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_LISTBOX = u"com.sun.star.form.component.ListBox"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_COMBOBOX = u"com.sun.star.form.component.ComboBox"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_RADIOBUTTON = u"com.sun.star.form.component.RadioButton"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_GROUPBOX = u"com.sun.star.form.component.GroupBox"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_FIXEDTEXT = u"com.sun.star.form.component.FixedText"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_COMMANDBUTTON = u"com.sun.star.form.component.CommandButton"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_CHECKBOX = u"com.sun.star.form.component.CheckBox"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_GRIDCONTROL = u"com.sun.star.form.component.GridControl"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_IMAGEBUTTON = u"com.sun.star.form.component.ImageButton"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_FILECONTROL = u"com.sun.star.form.component.FileControl"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_TIMEFIELD = u"com.sun.star.form.component.TimeField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_DATEFIELD = u"com.sun.star.form.component.DateField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_NUMERICFIELD = u"com.sun.star.form.component.NumericField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_CURRENCYFIELD = u"com.sun.star.form.component.CurrencyField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_PATTERNFIELD = u"com.sun.star.form.component.PatternField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_HIDDENCONTROL = u"com.sun.star.form.component.HiddenControl"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_IMAGECONTROL = u"com.sun.star.form.component.DatabaseImageControl"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_FORMATTEDFIELD = u"com.sun.star.form.component.FormattedField"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_SCROLLBAR = u"com.sun.star.form.component.ScrollBar"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_SPINBUTTON = u"com.sun.star.form.component.SpinButton"_ustr;
inline constexpr OUString FM_SUN_COMPONENT_NAVIGATIONBAR = u"com.sun.star.form.component.NavigationToolBar"_ustr;
inline constexpr OUString FM_SUN_CONTROL_GRIDCONTROL = u"com.sun.star.form.control.GridControl"_ustr;

namespace svxform
{
    SVXCORE_DLLPUBLIC void ImplSmartRegisterUnoServices();

    css::uno::Reference<css::uno::XInterface>
    OAddConditionDialog_Create(
        css::uno::Reference<css::lang::XMultiServiceFactory> const &);

    OUString OAddConditionDialog_GetImplementationName();

    css::uno::Sequence<OUString>
    OAddConditionDialog_GetSupportedServiceNames();
}

/// @throws css::uno::Exception
css::uno::Reference<css::uno::XInterface>
FmXGridControl_NewInstance_Impl(
    css::uno::Reference<css::lang::XMultiServiceFactory> const &);

/// @throws css::uno::Exception
css::uno::Reference<css::uno::XInterface>
FormController_NewInstance_Impl(
    css::uno::Reference<css::lang::XMultiServiceFactory> const &);

/// @throws css::uno::Exception
css::uno::Reference<css::uno::XInterface>
LegacyFormController_NewInstance_Impl(
    css::uno::Reference<css::lang::XMultiServiceFactory> const &);

#endif // INCLUDED_SVX_SOURCE_INC_FMSERVS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

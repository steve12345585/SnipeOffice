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

#include <com/sun/star/uno/Sequence.hxx>

#include <sal/log.hxx>

#include <global.hxx>
#include <addincol.hxx>
#include <addincfg.hxx>
#include <scmod.hxx>
#include <sc.hrc>

using namespace com::sun::star;

constexpr OUStringLiteral CFGPATH_ADDINS = u"Office.CalcAddIns/AddInInfo";

ScAddInCfg::ScAddInCfg()
    : ConfigItem(CFGPATH_ADDINS)
{
    EnableNotification({ {} });
}

void ScAddInCfg::ImplCommit() { SAL_WARN("sc", "ScAddInCfg shouldn't be modified"); }

void ScAddInCfg::Notify(const uno::Sequence<OUString>&)
{
    // forget all add-in information, re-initialize when needed next time
    ScGlobal::GetAddInCollection()->Clear();

    // function list must also be rebuilt, but can't be modified while function
    // autopilot is open (function list for autopilot is then still old)
    if (ScModule::get()->GetCurRefDlgId() != SID_OPENDLG_FUNCTION)
        ScGlobal::ResetFunctionList();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

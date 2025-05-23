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

#include <SwSmartTagMgr.hxx>

#include <docsh.hxx>
#include <swmodule.hxx>
#include <comphelper/configuration.hxx>
#include <vcl/svapp.hxx>

using namespace com::sun::star;

rtl::Reference<SwSmartTagMgr> SwSmartTagMgr::spTheSwSmartTagMgr;

SwSmartTagMgr& SwSmartTagMgr::Get()
{
    if (!spTheSwSmartTagMgr)
    {
        OUString sModuleName
            = !comphelper::IsFuzzing() ? SwDocShell::Factory().GetModuleName() : u"Writer"_ustr;
        spTheSwSmartTagMgr = new SwSmartTagMgr(sModuleName);
        spTheSwSmartTagMgr->Init(u"Writer");
    }
    return *spTheSwSmartTagMgr;
}

SwSmartTagMgr::SwSmartTagMgr(const OUString& rModuleName)
    : SmartTagMgr(rModuleName)
{
}

SwSmartTagMgr::~SwSmartTagMgr() {}

void SwSmartTagMgr::modified(const lang::EventObject& rEO)
{
    SolarMutexGuard aGuard;

    // Installed recognizers have changed. We remove all existing smart tags:
    SwModule::CheckSpellChanges(false, true, true, true);

    SmartTagMgr::modified(rEO);
}

void SwSmartTagMgr::changesOccurred(const util::ChangesEvent& rEvent)
{
    SolarMutexGuard aGuard;

    // Configuration has changed. We remove all existing smart tags:
    SwModule::CheckSpellChanges(false, true, true, true);

    SmartTagMgr::changesOccurred(rEvent);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

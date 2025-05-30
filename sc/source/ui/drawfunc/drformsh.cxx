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

#include <sfx2/objface.hxx>
#include <sfx2/msg.hxx>
#include <sfx2/toolbarids.hxx>
#include <sfx2/shell.hxx>

#include <drawsh.hxx>
#include <drformsh.hxx>
#include <vcl/EnumContext.hxx>

#define ShellClass_ScDrawFormShell
#include <scslots.hxx>

SFX_IMPL_INTERFACE(ScDrawFormShell, ScDrawShell)

void ScDrawFormShell::InitInterface_Impl()
{
    GetStaticInterface()->RegisterObjectBar(SFX_OBJECTBAR_OBJECT,
                                            SfxVisibilityFlags::Standard | SfxVisibilityFlags::Server,
                                            ToolbarId::Objectbar_Format);

    GetStaticInterface()->RegisterPopupMenu(u"form"_ustr);
}


ScDrawFormShell::ScDrawFormShell(ScViewData& rData) :
    ScDrawShell(rData)
{
    SetName(u"DrawForm"_ustr);
    SfxShell::SetContextName(vcl::EnumContext::GetContextName(vcl::EnumContext::Context::Form));
}

ScDrawFormShell::~ScDrawFormShell()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

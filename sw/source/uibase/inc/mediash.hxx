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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_MEDIASH_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_MEDIASH_HXX

#include <sfx2/module.hxx>
#include <sfx2/shell.hxx>

#include "basesh.hxx"
#include <shellid.hxx>

class SwMediaShell final : public SwBaseShell
{
public:
    SFX_DECL_INTERFACE(SW_MEDIASHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
    void ExecMedia(SfxRequest const&);
    void GetMediaState(SfxItemSet&);

    SwMediaShell(SwView& rView);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

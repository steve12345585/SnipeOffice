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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_SYNCBTN_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_SYNCBTN_HXX

#include <sfx2/basedlgs.hxx>
#include <sfx2/childwin.hxx>

class SwSyncBtnDlg final : public SfxModelessDialogController
{
    std::unique_ptr<weld::Button> m_xSyncBtn;

    DECL_STATIC_LINK( SwSyncBtnDlg, BtnHdl, weld::Button&, void );

public:
    SwSyncBtnDlg(SfxBindings*, SfxChildWindow*, weld::Window *pParent);
    virtual ~SwSyncBtnDlg() override;
};

class SwSyncChildWin final : public SfxChildWindow
{
public:
    SwSyncChildWin(vcl::Window*,
                   sal_uInt16 nId,
                   SfxBindings*,
                   SfxChildWinInfo*);

    SFX_DECL_CHILDWINDOW(SwSyncChildWin);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

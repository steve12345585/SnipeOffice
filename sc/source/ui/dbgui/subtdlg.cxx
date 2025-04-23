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

#undef SC_DLLIMPLEMENTATION

#include <tpsubt.hxx>
#include <subtdlg.hxx>
#include <scui_def.hxx>

ScSubTotalDlg::ScSubTotalDlg(weld::Window* pParent, const SfxItemSet& rArgSet)
    : SfxTabDialogController(pParent, u"modules/scalc/ui/subtotaldialog.ui"_ustr, u"SubTotalDialog"_ustr, &rArgSet)
    , m_xBtnRemove(m_xBuilder->weld_button(u"remove"_ustr))
{
    AddTabPage(u"1stgroup"_ustr,  ScTpSubTotalGroup1::Create, nullptr);
    AddTabPage(u"2ndgroup"_ustr,  ScTpSubTotalGroup2::Create, nullptr);
    AddTabPage(u"3rdgroup"_ustr,  ScTpSubTotalGroup3::Create, nullptr);
    AddTabPage(u"options"_ustr, ScTpSubTotalOptions::Create, nullptr);
    m_xBtnRemove->connect_clicked( LINK( this, ScSubTotalDlg, RemoveHdl ) );
}

ScSubTotalDlg::~ScSubTotalDlg()
{
}

IMPL_LINK_NOARG(ScSubTotalDlg, RemoveHdl, weld::Button&, void)
{
    m_xDialog->response(SCRET_REMOVE);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

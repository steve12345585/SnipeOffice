/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <sal/config.h>

#include <officecfg/Office/Common.hxx>
#include <vcl/fileregistration.hxx>

#include <fileextcheckdlg.hxx>

FileExtCheckDialog::FileExtCheckDialog(weld::Window* pParent, const OUString& sTitle,
                                       const OUString& sMsg)
    : GenericDialogController(pParent, "cui/ui/fileextcheckdialog.ui", "FileExtCheckDialog")
    , m_pText(m_xBuilder->weld_label("lbText"))
    , m_pPerformCheck(m_xBuilder->weld_check_button("cbPerformCheck"))
    , m_pOk(m_xBuilder->weld_button("btnOk"))
{
    m_pPerformCheck->set_active(true);
    m_pOk->connect_clicked(LINK(this, FileExtCheckDialog, OnOkClick));
    m_xDialog->set_title(sTitle);
    m_pText->set_label(sMsg);
}

FileExtCheckDialog::~FileExtCheckDialog()
{
    std::shared_ptr<comphelper::ConfigurationChanges> xChanges(
        comphelper::ConfigurationChanges::create());
    officecfg::Office::Common::Misc::PerformFileExtCheck::set(m_pPerformCheck->get_active(),
                                                              xChanges);
    xChanges->commit();
}

IMPL_LINK_NOARG(FileExtCheckDialog, OnOkClick, weld::Button&, void)
{
    vcl::fileregistration::LaunchRegistrationUI();
    FileExtCheckDialog::response(RET_OK);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

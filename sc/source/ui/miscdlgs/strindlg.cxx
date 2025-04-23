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

#include <strindlg.hxx>

ScStringInputDlg::ScStringInputDlg(weld::Window* pParent,
                                   const OUString& rTitle,
                                   const OUString& rEditTitle,
                                   const OUString& rDefault,
                                   const OUString& rHelpId, const OUString& rEditHelpId)
    : GenericDialogController(pParent, u"modules/scalc/ui/inputstringdialog.ui"_ustr,
            u"InputStringDialog"_ustr)
    , m_xLabel(m_xBuilder->weld_label(u"description_label"_ustr))
    , m_xEdInput(m_xBuilder->weld_entry(u"name_entry"_ustr))
{
    m_xLabel->set_label(rEditTitle);
    m_xDialog->set_title(rTitle);
    m_xDialog->set_help_id(rHelpId);
    m_xEdInput->set_text(rDefault);
    m_xEdInput->set_help_id(rEditHelpId);
    m_xEdInput->select_region(0, -1);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

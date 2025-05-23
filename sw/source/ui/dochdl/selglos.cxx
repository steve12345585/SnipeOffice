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

#include <selglos.hxx>

SwSelGlossaryDlg::SwSelGlossaryDlg(weld::Window * pParent, std::u16string_view rShortName)
    : GenericDialogController(pParent, u"modules/swriter/ui/insertautotextdialog.ui"_ustr, u"InsertAutoTextDialog"_ustr)
    , m_xFrame(m_xBuilder->weld_frame(u"frame"_ustr))
    , m_xGlosBox(m_xBuilder->weld_tree_view(u"treeview"_ustr))
{
    m_xFrame->set_label(m_xFrame->get_label() + rShortName);
    m_xGlosBox->set_size_request(-1, m_xGlosBox->get_height_rows(10));
    m_xGlosBox->connect_row_activated(LINK(this, SwSelGlossaryDlg, DoubleClickHdl));
}

SwSelGlossaryDlg::~SwSelGlossaryDlg()
{
}

IMPL_LINK_NOARG(SwSelGlossaryDlg, DoubleClickHdl, weld::TreeView&, bool)
{
    m_xDialog->response(RET_OK);
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

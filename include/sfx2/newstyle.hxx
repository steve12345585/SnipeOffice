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
#ifndef INCLUDED_SFX2_NEWSTYLE_HXX
#define INCLUDED_SFX2_NEWSTYLE_HXX

#include <comphelper/string.hxx>
#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <svl/style.hxx>
#include <vcl/weld.hxx>

class SFX2_DLLPUBLIC SfxNewStyleDlg final : public weld::GenericDialogController
{
private:
    SfxStyleSheetBasePool& m_rPool;
    SfxStyleFamily m_eSearchFamily;

    std::unique_ptr<weld::EntryTreeView> m_xColBox;
    std::unique_ptr<weld::Button> m_xOKBtn;

    std::unique_ptr<weld::MessageDialog> m_xQueryOverwriteBox;

    DECL_DLLPRIVATE_LINK(OKHdl, weld::TreeView&, bool);
    DECL_DLLPRIVATE_LINK(OKClickHdl, weld::Button&, void);
    DECL_DLLPRIVATE_LINK(ModifyHdl, weld::ComboBox&, void);

public:
    SfxNewStyleDlg(weld::Widget* pParent, SfxStyleSheetBasePool& rPool, SfxStyleFamily eFam);
    virtual ~SfxNewStyleDlg() override;

    OUString GetName() const
    {
        return comphelper::string::stripStart(m_xColBox->get_active_text(), ' ');
    }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#pragma once

#include <vcl/layout.hxx>
#include "IPrioritable.hxx"
#include "NotebookbarPopup.hxx"

class Button;

class DropdownBox final : public VclHBox, public vcl::IPrioritable
{
private:
    bool m_bInFullView;
    VclPtr<PushButton> m_pButton;
    VclPtr<NotebookbarPopup> m_pPopup;

public:
    explicit DropdownBox(vcl::Window* pParent);
    virtual ~DropdownBox() override;
    virtual void dispose() override;

    void HideContent() override;
    void ShowContent() override;

private:
    DECL_LINK(PBClickHdl, Button*, void);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

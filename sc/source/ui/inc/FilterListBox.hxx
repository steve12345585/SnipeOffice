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

#include <types.hxx>

#include <tools/solar.h>
#include <vcl/weld.hxx>

class ScGridWindow;
struct ImplSVEvent;

enum class ScFilterBoxMode
{
    DataSelect,
    Scenario
};

class ScFilterListBox final : public std::enable_shared_from_this<ScFilterListBox>
{
private:
    std::unique_ptr<weld::Builder> xBuilder;
    std::unique_ptr<weld::Popover> xPopover;
    std::unique_ptr<weld::TreeView> xTreeView;
    VclPtr<ScGridWindow> pGridWin;
    SCCOL nCol;
    SCROW nRow;
    bool bInit;
    bool bCancelled;
    bool bGridHadMouseCaptured;
    sal_uLong nSel;
    ScFilterBoxMode eMode;
    ImplSVEvent* nAsyncSelectHdl;

    DECL_LINK(SelectHdl, weld::TreeView&, bool);
    DECL_LINK(KeyInputHdl, const KeyEvent&, bool);
    DECL_LINK(AsyncSelectHdl, void*, void);

public:
    ScFilterListBox(weld::Window* pParent, ScGridWindow* pGrid, SCCOL nNewCol, SCROW nNewRow,
                    ScFilterBoxMode eNewMode);
    void popup_at_rect(weld::Widget* pParent, const tools::Rectangle& rRect)
    {
        xPopover->popup_at_rect(pParent, rRect);
    }
    void connect_closed(const Link<weld::Popover&, void>& rLink)
    {
        xPopover->connect_closed(rLink);
    }
    void popdown() { xPopover->popdown(); }
    ~ScFilterListBox();

    weld::TreeView& get_widget() { return *xTreeView; }

    SCCOL GetCol() const { return nCol; }
    SCROW GetRow() const { return nRow; }
    ScFilterBoxMode GetMode() const { return eMode; }
    void EndInit();
    bool IsInInit() const { return bInit; }
    bool MouseWasCaptured() const { return bGridHadMouseCaptured; }
    void SetCancelled() { bCancelled = true; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

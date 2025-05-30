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

#ifndef INCLUDED_SVX_CLIPBOARDCTL_HXX
#define INCLUDED_SVX_CLIPBOARDCTL_HXX

#include <sal/types.h>
#include <sfx2/tbxctrl.hxx>
#include <svl/poolitem.hxx>
#include <svx/svxdllapi.h>
#include <memory>

class SfxModule;
class ToolBox;
class SvxClipboardFormatItem;

class SAL_WARN_UNUSED SVX_DLLPUBLIC SvxClipBoardControl final : public SfxToolBoxControl
{
    std::unique_ptr<SvxClipboardFormatItem> pClipboardFmtItem;
    bool                    bDisabled;

public:
    SFX_DECL_TOOLBOX_CONTROL();

    SvxClipBoardControl( sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rTbx );
    virtual ~SvxClipBoardControl() override;

    void CreatePopupWindow() override;
    virtual void                StateChangedAtToolBoxControl( sal_uInt16 nSID,
                                              SfxItemState eState,
                                              const SfxPoolItem* pState ) override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

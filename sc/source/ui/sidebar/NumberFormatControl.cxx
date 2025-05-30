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

#include <NumberFormatControl.hxx>
#include <cbnumberformat.hxx>
#include <svl/intitem.hxx>
#include <vcl/toolbox.hxx>

using namespace sc;

SFX_IMPL_TOOLBOX_CONTROL(ScNumberFormatControl, SfxUInt16Item);

ScNumberFormatControl::ScNumberFormatControl(sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rTbx)
    : SfxToolBoxControl(nSlotId, nId, rTbx)
{
}

ScNumberFormatControl::~ScNumberFormatControl()
{
}

void ScNumberFormatControl::StateChangedAtToolBoxControl(sal_uInt16, SfxItemState eState,
                                         const SfxPoolItem* pState)
{
    ToolBoxItemId nId = GetId();
    ToolBox& rTbx = GetToolBox();
    ScNumberFormat* pComboBox = static_cast<ScNumberFormat*>(rTbx.GetItemWindow(nId));

    DBG_ASSERT( pComboBox, "Control not found!" );

    if(SfxItemState::DISABLED == eState)
        pComboBox->Disable();
    else
        pComboBox->Enable();

    rTbx.EnableItem(nId, SfxItemState::DISABLED != eState);

    switch(eState)
    {
        case SfxItemState::DEFAULT:
        {
            const SfxUInt16Item* pItem = static_cast<const SfxUInt16Item*>(pState);
            sal_uInt16 nVal = pItem->GetValue();
            pComboBox->set_active(nVal);
            break;
        }

        default:
            break;
    }
}

VclPtr<InterimItemWindow> ScNumberFormatControl::CreateItemWindow( vcl::Window *pParent )
{
    VclPtr<ScNumberFormat> pControl = VclPtr<ScNumberFormat>::Create(pParent);
    pControl->Show();

    return pControl;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swtypes.hxx>
#include <strings.hrc>
#include <wordcountctrl.hxx>
#include <svl/stritem.hxx>
#include <vcl/status.hxx>

SFX_IMPL_STATUSBAR_CONTROL(SwWordCountStatusBarControl, SfxStringItem);

SwWordCountStatusBarControl::SwWordCountStatusBarControl(
        sal_uInt16 _nSlotId,
        sal_uInt16 _nId,
        StatusBar& rStb) :
    SfxStatusBarControl(_nSlotId, _nId, rStb)
{
}

SwWordCountStatusBarControl::~SwWordCountStatusBarControl()
{
}

void SwWordCountStatusBarControl::StateChangedAtStatusBarControl(
    sal_uInt16 /*nSID*/, SfxItemState eState, const SfxPoolItem* pState )
{
    if (eState == SfxItemState::DEFAULT) // Can access pState
    {
        GetStatusBar().SetItemText( GetId(), static_cast<const SfxStringItem*>(pState)->GetValue() );
        GetStatusBar().SetQuickHelpText(GetId(), SwResId(STR_WORDCOUNT_HINT));
    }
    else
    {
        GetStatusBar().SetItemText(GetId(), u""_ustr);
        GetStatusBar().SetQuickHelpText(GetId(), u""_ustr);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

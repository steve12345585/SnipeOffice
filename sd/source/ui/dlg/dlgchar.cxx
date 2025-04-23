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

#include <svx/dialogs.hrc>
#include <editeng/flstitem.hxx>
#include <svx/flagsdef.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/sfxdlg.hxx>

#include <dlg_char.hxx>
#include <svx/svxids.hrc>
#include <svl/intitem.hxx>

/**
 * Constructor of tab dialog: append pages to dialog
 */
SdCharDlg::SdCharDlg(weld::Window* pParent, const SfxItemSet* pAttr,
                    const SfxObjectShell* pDocShell)
    : SfxTabDialogController(pParent, u"modules/sdraw/ui/drawchardialog.ui"_ustr,
                             u"DrawCharDialog"_ustr, pAttr)
    , rDocShell(*pDocShell)
{
    SfxAbstractDialogFactory* pFact = SfxAbstractDialogFactory::Create();

    AddTabPage(u"RID_SVXPAGE_CHAR_NAME"_ustr, pFact->GetTabPageCreatorFunc(RID_SVXPAGE_CHAR_NAME), nullptr);
    AddTabPage(u"RID_SVXPAGE_CHAR_EFFECTS"_ustr, pFact->GetTabPageCreatorFunc(RID_SVXPAGE_CHAR_EFFECTS), nullptr);
    AddTabPage(u"RID_SVXPAGE_CHAR_POSITION"_ustr, pFact->GetTabPageCreatorFunc(RID_SVXPAGE_CHAR_POSITION), nullptr);
    AddTabPage(u"RID_SVXPAGE_BKG"_ustr, pFact->GetTabPageCreatorFunc(RID_SVXPAGE_BKG), nullptr);
}

void SdCharDlg::PageCreated(const OUString& rId, SfxTabPage &rPage)
{
    SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
    if (rId == "RID_SVXPAGE_CHAR_NAME")
    {
        SvxFontListItem aItem(* static_cast<const SvxFontListItem*>( rDocShell.GetItem( SID_ATTR_CHAR_FONTLIST) ) );

        aSet.Put (SvxFontListItem( aItem.GetFontList(), SID_ATTR_CHAR_FONTLIST));
        rPage.PageCreated(aSet);
    }
    else if (rId == "RID_SVXPAGE_CHAR_EFFECTS")
    {
        // Opt in for character transparency.
        aSet.Put(SfxUInt32Item(SID_FLAG_TYPE, SVX_ENABLE_CHAR_TRANSPARENCY));
        rPage.PageCreated(aSet);
    }
    else if (rId == "RID_SVXPAGE_BKG")
    {
        aSet.Put(SfxUInt32Item(SID_FLAG_TYPE,static_cast<sal_uInt32>(SvxBackgroundTabFlags::SHOW_CHAR_BKGCOLOR)));
        rPage.PageCreated(aSet);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

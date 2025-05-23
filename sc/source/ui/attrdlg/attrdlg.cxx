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

#include <sfx2/objsh.hxx>
#include <sfx2/tabdlg.hxx>
#include <sfx2/sfxdlg.hxx>
#include <svl/cjkoptions.hxx>

#include <tabpages.hxx>
#include <attrdlg.hxx>
#include <svx/dialogs.hrc>
#include <editeng/editids.hrc>
#include <editeng/flstitem.hxx>
#include <osl/diagnose.h>

ScAttrDlg::ScAttrDlg(weld::Window* pParent, const SfxItemSet* pCellAttrs)
    : SfxTabDialogController(pParent, u"modules/scalc/ui/formatcellsdialog.ui"_ustr,
                             u"FormatCellsDialog"_ustr, pCellAttrs)
{
    SfxAbstractDialogFactory* pFact = SfxAbstractDialogFactory::Create();

    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_NUMBERFORMAT ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"numbers"_ustr, pFact->GetTabPageCreatorFunc( RID_SVXPAGE_NUMBERFORMAT ), nullptr );
    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_CHAR_NAME ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"font"_ustr, pFact->GetTabPageCreatorFunc( RID_SVXPAGE_CHAR_NAME ), nullptr );
    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_CHAR_EFFECTS ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"fonteffects"_ustr, pFact->GetTabPageCreatorFunc( RID_SVXPAGE_CHAR_EFFECTS ), nullptr );
    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_ALIGNMENT ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"alignment"_ustr, pFact->GetTabPageCreatorFunc( RID_SVXPAGE_ALIGNMENT ),    nullptr );

    if (SvtCJKOptions::IsAsianTypographyEnabled())
    {
        OSL_ENSURE(pFact->GetTabPageCreatorFunc(RID_SVXPAGE_PARA_ASIAN), "GetTabPageCreatorFunc fail!");
        AddTabPage( u"asiantypography"_ustr,   pFact->GetTabPageCreatorFunc(RID_SVXPAGE_PARA_ASIAN),       nullptr );
    }
    else
        RemoveTabPage( u"asiantypography"_ustr );
    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_BORDER ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"borders"_ustr,      pFact->GetTabPageCreatorFunc( RID_SVXPAGE_BORDER ),     nullptr );
    OSL_ENSURE(pFact->GetTabPageCreatorFunc( RID_SVXPAGE_BKG ), "GetTabPageCreatorFunc fail!");
    AddTabPage( u"background"_ustr,  pFact->GetTabPageCreatorFunc( RID_SVXPAGE_BKG ), nullptr );
    AddTabPage( u"cellprotection"_ustr ,  ScTabPageProtection::Create,    nullptr );
}

ScAttrDlg::~ScAttrDlg()
{
}

void ScAttrDlg::PageCreated(const OUString& rPageId, SfxTabPage& rTabPage)
{
    SfxObjectShell* pDocSh = SfxObjectShell::Current();
    SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
    if (rPageId == "numbers")
    {
        rTabPage.PageCreated(aSet);
    }
    else if (rPageId == "font" && pDocSh)
    {
        const SfxPoolItem* pInfoItem = pDocSh->GetItem( SID_ATTR_CHAR_FONTLIST );
        SAL_WARN_IF(!pInfoItem, "sc.ui", "we should have a FontListItem normally here");
        if (pInfoItem)
        {
            aSet.Put (SvxFontListItem(static_cast<const SvxFontListItem*>(pInfoItem)->GetFontList(), SID_ATTR_CHAR_FONTLIST ));
            rTabPage.PageCreated(aSet);
        }
    }
    else if (rPageId == "background")
    {
        rTabPage.PageCreated(aSet);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <dlgedpage.hxx>
#include <dlged.hxx>
#include <dlgedmod.hxx>
#include <dlgedobj.hxx>

namespace basctl
{


DlgEdPage::DlgEdPage(DlgEdModel& rModel, bool bMasterPage)
:   SdrPage(rModel, bMasterPage)
    ,pDlgEdForm(nullptr)
{
}

DlgEdPage::~DlgEdPage()
{
    // clear SdrObjects with broadcasting
    ClearSdrObjList();
}

rtl::Reference<SdrPage> DlgEdPage::CloneSdrPage(SdrModel& rTargetModel) const
{
    DlgEdModel& rDlgEdModel(static_cast< DlgEdModel& >(rTargetModel));
    rtl::Reference<DlgEdPage> pClonedDlgEdPage =
        new DlgEdPage(
            rDlgEdModel,
            IsMasterPage());
    pClonedDlgEdPage->SdrPage::lateInit(*this);
    return pClonedDlgEdPage;
}


SdrObject* DlgEdPage::SetObjectOrdNum(size_t nOldObjNum, size_t nNewObjNum)
{
    SdrObject* pObj = SdrPage::SetObjectOrdNum( nOldObjNum, nNewObjNum );

    DlgEdHint aHint( DlgEdHint::OBJORDERCHANGED );
    if ( pDlgEdForm )
        pDlgEdForm->GetDlgEditor().Broadcast( aHint );

    return pObj;
}

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

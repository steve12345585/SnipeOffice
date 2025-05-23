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

#include <DrawViewShell.hxx>

#include <svx/imapdlg.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdograf.hxx>
#include <svx/svxdlg.hxx>
#include <svx/ImageMapInfo.hxx>

#include <sfx2/viewfrm.hxx>

#include <drawdoc.hxx>
#include <drawview.hxx>
#include <memory>

namespace sd {

void DrawViewShell::UpdateIMapDlg( SdrObject* pObj )
{
    if( ( dynamic_cast< SdrGrafObj *>( pObj ) == nullptr && dynamic_cast< SdrOle2Obj *>( pObj ) == nullptr )
        || mpDrawView->IsTextEdit()
        || !GetViewFrame()->HasChildWindow( SvxIMapDlgChildWindow::GetChildWindowId() ) )
        return;

    Graphic     aGraphic;
    ImageMap*   pIMap = nullptr;
    std::unique_ptr<TargetList> pTargetList;
    SvxIMapInfo* pIMapInfo = SvxIMapInfo::GetIMapInfo( pObj );

    // get graphic from shape
    SdrGrafObj* pGrafObj = dynamic_cast< SdrGrafObj* >( pObj );
    if( pGrafObj )
        aGraphic = pGrafObj->GetGraphic();

    if ( pIMapInfo )
    {
        pIMap = const_cast<ImageMap*>(&pIMapInfo->GetImageMap());
        pTargetList.reset(new TargetList);
        SfxViewFrame::GetTargetList( *pTargetList );
    }

    SvxIMapDlgChildWindow::UpdateIMapDlg( aGraphic, pIMap, pTargetList.get(), pObj );
}

IMPL_LINK( DrawViewShell, NameObjectHdl, AbstractSvxObjectNameDialog&, rDialog, bool )
{
    OUString aName = rDialog.GetName();
    return aName.isEmpty() || ( GetDoc() && !GetDoc()->GetObj( aName ) );
}

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

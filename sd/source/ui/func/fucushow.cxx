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

#include <fucushow.hxx>

#include <svx/svxids.hrc>

#include <ViewShell.hxx>
#include <Window.hxx>
#include <drawdoc.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>

#include <sdabstdlg.hxx>

namespace sd {


FuCustomShowDlg::FuCustomShowDlg (
    ViewShell* pViewSh,
    ::sd::Window*    pWin,
    ::sd::View* pView,
    SdDrawDocument* pDoc,
    SfxRequest& rReq)
    : FuPoor( pViewSh, pWin, pView, pDoc, rReq )
{
}

rtl::Reference<FuPoor> FuCustomShowDlg::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
    rtl::Reference<FuPoor> xFunc( new FuCustomShowDlg( pViewSh, pWin, pView, pDoc, rReq ) );
    xFunc->DoExecute(rReq);
    return xFunc;
}

void FuCustomShowDlg::DoExecute( SfxRequest& )
{
    SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
    vcl::Window* pWin = mpViewShell->GetActiveWindow();
    ScopedVclPtr<AbstractSdCustomShowDlg> pDlg( pFact->CreateSdCustomShowDlg(pWin ? pWin->GetFrameWeld() : nullptr, *mpDoc) );
    sal_uInt16 nRet = pDlg->Execute();
    mpDoc->SetChanged();
    sd::PresentationSettings& rSettings = mpDoc->getPresentationSettings();

    if( nRet == RET_YES )
    {
        // If the custom show is not set by default
        if (!rSettings.mbCustomShow)
        {
            rSettings.mbStartCustomShow = true;
            rSettings.mbCustomShow = pDlg->IsCustomShow();
        }

        mpViewShell->SetStartShowWithDialog(true);

        mpViewShell->GetViewFrame()->GetDispatcher()->Execute( SID_PRESENTATION,
                SfxCallMode::ASYNCHRON | SfxCallMode::RECORD );
    }
    if (nRet == RET_OK)
    {
        if (mpDoc->GetCustomShowList())
        {
            if (!pDlg->IsCustomShow())
            {
                rSettings.mbCustomShow = false;
                rSettings.mbAll = true;
            }
        }
    }
    pDlg.disposeAndClear();
}

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

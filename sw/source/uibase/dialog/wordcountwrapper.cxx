/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wordcountdialog.hxx>
#include <docstat.hxx>
#include <cmdid.h>

SFX_IMPL_CHILDWINDOW_WITHID(SwWordCountWrapper, FN_WORDCOUNT_DIALOG)

SwWordCountWrapper::SwWordCountWrapper(vcl::Window *pParentWindow,
                            sal_uInt16 nId,
                            SfxBindings* pBindings,
                            SfxChildWinInfo* pInfo )
    : SfxChildWindow(pParentWindow, nId)
{
    SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
    m_xAbstDlg.reset(pFact->CreateSwWordCountDialog(pBindings, this, pParentWindow->GetFrameWeld(), pInfo));
    SetController(m_xAbstDlg->GetController());
}

SwWordCountWrapper::~SwWordCountWrapper()
{
    m_xAbstDlg.disposeAndClear();
}

SfxChildWinInfo SwWordCountWrapper::GetInfo() const
{
    SfxChildWinInfo aInfo = SfxChildWindow::GetInfo();
    return aInfo;
}

void SwWordCountWrapper::UpdateCounts()
{
    m_xAbstDlg->UpdateCounts();
}

void SwWordCountWrapper::SetCounts(const SwDocStat &rCurrCnt, const SwDocStat &rDocStat)
{
    m_xAbstDlg->SetCounts(rCurrCnt, rDocStat);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

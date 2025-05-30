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

#include <swwait.hxx>
#include <wrtsh.hxx>
#include <view.hxx>
#include <docsh.hxx>
#include <pview.hxx>
#include <doc.hxx>
#include <docstdlg.hxx>
#include <IDocumentStatistics.hxx>

#include <unotools/localedatawrapper.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <osl/diagnose.h>

std::unique_ptr<SfxTabPage> SwDocStatPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet *rSet)
{
    return std::make_unique<SwDocStatPage>(pPage, pController, *rSet);
}

SwDocStatPage::SwDocStatPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet &rSet)
    : SfxTabPage(pPage, pController, u"modules/swriter/ui/statisticsinfopage.ui"_ustr, u"StatisticsInfoPage"_ustr, &rSet)
    , m_xPageNo(m_xBuilder->weld_label(u"nopages"_ustr))
    , m_xTableNo(m_xBuilder->weld_label(u"notables"_ustr))
    , m_xGrfNo(m_xBuilder->weld_label(u"nogrfs"_ustr))
    , m_xOLENo(m_xBuilder->weld_label(u"nooles"_ustr))
    , m_xParaNo(m_xBuilder->weld_label(u"noparas"_ustr))
    , m_xWordNo(m_xBuilder->weld_label(u"nowords"_ustr))
    , m_xCharNo(m_xBuilder->weld_label(u"nochars"_ustr))
    , m_xCharExclSpacesNo(m_xBuilder->weld_label(u"nocharsexspaces"_ustr))
    , m_xLineLbl(m_xBuilder->weld_label(u"lineft"_ustr))
    , m_xLineNo(m_xBuilder->weld_label(u"nolines"_ustr))
    , m_xUpdatePB(m_xBuilder->weld_button(u"update"_ustr))
{
    Update();
    m_xUpdatePB->connect_clicked(LINK(this, SwDocStatPage, UpdateHdl));
    //#111684# is the current view a page preview no SwFEShell can be found -> hide the update button
    SwDocShell* pDocShell = static_cast<SwDocShell*>( SfxObjectShell::Current() );
    SwFEShell* pFEShell = pDocShell ? pDocShell->GetFEShell() : nullptr;
    if(!pFEShell)
    {
        m_xUpdatePB->hide();
        m_xLineLbl->hide();
        m_xLineNo->hide();
    }
}

SwDocStatPage::~SwDocStatPage()
{
}

// Description: fill ItemSet when changed
bool  SwDocStatPage::FillItemSet(SfxItemSet * /*rSet*/)
{
    return false;
}

void  SwDocStatPage::Reset(const SfxItemSet *)
{
}

// Description: update / set data
void SwDocStatPage::SetData(const SwDocStat &rStat)
{
    const LocaleDataWrapper& rLocaleData = Application::GetSettings().GetUILocaleDataWrapper();
    m_xTableNo->set_label(rLocaleData.getNum(rStat.nTable, 0));
    m_xGrfNo->set_label(rLocaleData.getNum(rStat.nGrf, 0));
    m_xOLENo->set_label(rLocaleData.getNum(rStat.nOLE, 0));
    m_xPageNo->set_label(rLocaleData.getNum(rStat.nPage, 0));
    m_xParaNo->set_label(rLocaleData.getNum(rStat.nPara, 0));
    m_xWordNo->set_label(rLocaleData.getNum(rStat.nWord, 0));
    m_xCharNo->set_label(rLocaleData.getNum(rStat.nChar, 0));
    m_xCharExclSpacesNo->set_label(rLocaleData.getNum(rStat.nCharExcludingSpaces, 0));
}

// Description: update statistics
void SwDocStatPage::Update()
{
    SfxViewShell *pVSh = SfxViewShell::Current();
    SwViewShell *pSh = nullptr;
    if ( auto pSwView = dynamic_cast<SwView *>( pVSh ) )
        pSh = pSwView->GetWrtShellPtr();
    else if ( auto pPagePreview = dynamic_cast<SwPagePreview *>( pVSh ) )
        pSh = pPagePreview->GetViewShell();

    OSL_ENSURE( pSh, "Shell not found" );

    if (!pSh)
        return;

    SwWait aWait( *pSh->GetDoc()->GetDocShell(), true );
    pSh->StartAction();
    m_aDocStat = pSh->GetDoc()->getIDocumentStatistics().GetUpdatedDocStat( false, true );
    pSh->EndAction();

    SetData(m_aDocStat);
}

IMPL_LINK_NOARG(SwDocStatPage, UpdateHdl, weld::Button&, void)
{
    Update();
    SwDocShell* pDocShell = static_cast<SwDocShell*>( SfxObjectShell::Current());
    SwFEShell* pFEShell = pDocShell ? pDocShell->GetFEShell() : nullptr;
    if (pFEShell)
    {
        const LocaleDataWrapper& rLocaleData = Application::GetSettings().GetUILocaleDataWrapper();
        OUString sLineCount = rLocaleData.getNum(pFEShell->GetLineCount(), 0);
        m_xLineNo->set_label(sLineCount);
        m_xLineNo->set_size_request(m_xLineNo->get_approximate_digit_width() * sLineCount.getLength(), -1);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

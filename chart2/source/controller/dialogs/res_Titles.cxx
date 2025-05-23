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

#include <res_Titles.hxx>
#include <TitleDialogData.hxx>
#include <vcl/weld.hxx>

namespace chart
{
TitleResources::TitleResources(weld::Builder& rBuilder, bool bShowSecondaryAxesTitle)
    : m_xFT_Main(rBuilder.weld_label(u"labelMainTitle"_ustr))
    , m_xFT_Sub(rBuilder.weld_label(u"labelSubTitle"_ustr))
    , m_xEd_Main(rBuilder.weld_entry(u"maintitle"_ustr))
    , m_xEd_Sub(rBuilder.weld_entry(u"subtitle"_ustr))
    , m_xFT_XAxis(rBuilder.weld_label(u"labelPrimaryXaxis"_ustr))
    , m_xFT_YAxis(rBuilder.weld_label(u"labelPrimaryYaxis"_ustr))
    , m_xFT_ZAxis(rBuilder.weld_label(u"labelPrimaryZaxis"_ustr))
    , m_xEd_XAxis(rBuilder.weld_entry(u"primaryXaxis"_ustr))
    , m_xEd_YAxis(rBuilder.weld_entry(u"primaryYaxis"_ustr))
    , m_xEd_ZAxis(rBuilder.weld_entry(u"primaryZaxis"_ustr))
    , m_xFT_SecondaryXAxis(rBuilder.weld_label(u"labelSecondaryXAxis"_ustr))
    , m_xFT_SecondaryYAxis(rBuilder.weld_label(u"labelSecondaryYAxis"_ustr))
    , m_xEd_SecondaryXAxis(rBuilder.weld_entry(u"secondaryXaxis"_ustr))
    , m_xEd_SecondaryYAxis(rBuilder.weld_entry(u"secondaryYaxis"_ustr))
{
    m_xFT_SecondaryXAxis->set_visible(bShowSecondaryAxesTitle);
    m_xFT_SecondaryYAxis->set_visible(bShowSecondaryAxesTitle);
    m_xEd_SecondaryXAxis->set_visible(bShowSecondaryAxesTitle);
    m_xEd_SecondaryYAxis->set_visible(bShowSecondaryAxesTitle);
}

TitleResources::~TitleResources() {}

void TitleResources::connect_changed(const Link<weld::Entry&, void>& rLink)
{
    m_xEd_Main->connect_changed(rLink);
    m_xEd_Sub->connect_changed(rLink);
    m_xEd_XAxis->connect_changed(rLink);
    m_xEd_YAxis->connect_changed(rLink);
    m_xEd_ZAxis->connect_changed(rLink);
    m_xEd_SecondaryXAxis->connect_changed(rLink);
    m_xEd_SecondaryYAxis->connect_changed(rLink);
}

bool TitleResources::get_value_changed_from_saved() const
{
    return m_xEd_Main->get_value_changed_from_saved() || m_xEd_Sub->get_value_changed_from_saved()
           || m_xEd_XAxis->get_value_changed_from_saved()
           || m_xEd_YAxis->get_value_changed_from_saved()
           || m_xEd_ZAxis->get_value_changed_from_saved()
           || m_xEd_SecondaryXAxis->get_value_changed_from_saved()
           || m_xEd_SecondaryYAxis->get_value_changed_from_saved();
}

void TitleResources::save_value()
{
    m_xEd_Main->save_value();
    m_xEd_Sub->save_value();
    m_xEd_XAxis->save_value();
    m_xEd_YAxis->save_value();
    m_xEd_ZAxis->save_value();
    m_xEd_SecondaryXAxis->save_value();
    m_xEd_SecondaryYAxis->save_value();
}

void TitleResources::writeToResources(const TitleDialogData& rInput)
{
    m_xFT_Main->set_sensitive(rInput.aPossibilityList[0]);
    m_xFT_Sub->set_sensitive(rInput.aPossibilityList[1]);
    m_xFT_XAxis->set_sensitive(rInput.aPossibilityList[2]);
    m_xFT_YAxis->set_sensitive(rInput.aPossibilityList[3]);
    m_xFT_ZAxis->set_sensitive(rInput.aPossibilityList[4]);
    m_xFT_SecondaryXAxis->set_sensitive(rInput.aPossibilityList[5]);
    m_xFT_SecondaryYAxis->set_sensitive(rInput.aPossibilityList[6]);

    m_xEd_Main->set_sensitive(rInput.aPossibilityList[0]);
    m_xEd_Sub->set_sensitive(rInput.aPossibilityList[1]);
    m_xEd_XAxis->set_sensitive(rInput.aPossibilityList[2]);
    m_xEd_YAxis->set_sensitive(rInput.aPossibilityList[3]);
    m_xEd_ZAxis->set_sensitive(rInput.aPossibilityList[4]);
    m_xEd_SecondaryXAxis->set_sensitive(rInput.aPossibilityList[5]);
    m_xEd_SecondaryYAxis->set_sensitive(rInput.aPossibilityList[6]);

    m_xEd_Main->set_text(rInput.aTextList[0]);
    m_xEd_Sub->set_text(rInput.aTextList[1]);
    m_xEd_XAxis->set_text(rInput.aTextList[2]);
    m_xEd_YAxis->set_text(rInput.aTextList[3]);
    m_xEd_ZAxis->set_text(rInput.aTextList[4]);
    m_xEd_SecondaryXAxis->set_text(rInput.aTextList[5]);
    m_xEd_SecondaryYAxis->set_text(rInput.aTextList[6]);
}

void TitleResources::readFromResources(TitleDialogData& rOutput)
{
    sal_Bool* pExistenceList = rOutput.aExistenceList.getArray();
    pExistenceList[0] = !m_xEd_Main->get_text().isEmpty();
    pExistenceList[1] = !m_xEd_Sub->get_text().isEmpty();
    pExistenceList[2] = !m_xEd_XAxis->get_text().isEmpty();
    pExistenceList[3] = !m_xEd_YAxis->get_text().isEmpty();
    pExistenceList[4] = !m_xEd_ZAxis->get_text().isEmpty();
    pExistenceList[5] = !m_xEd_SecondaryXAxis->get_text().isEmpty();
    pExistenceList[6] = !m_xEd_SecondaryYAxis->get_text().isEmpty();

    auto pTextList = rOutput.aTextList.getArray();
    pTextList[0] = m_xEd_Main->get_text();
    pTextList[1] = m_xEd_Sub->get_text();
    pTextList[2] = m_xEd_XAxis->get_text();
    pTextList[3] = m_xEd_YAxis->get_text();
    pTextList[4] = m_xEd_ZAxis->get_text();
    pTextList[5] = m_xEd_SecondaryXAxis->get_text();
    pTextList[6] = m_xEd_SecondaryYAxis->get_text();
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

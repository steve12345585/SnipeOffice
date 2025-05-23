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

#pragma once

#include <sfx2/tabdlg.hxx>
#include <com/sun/star/container/XNameReplace.hpp>
#include <com/sun/star/configuration/XReadWriteAccess.hpp>

// class SvxPathTabPage --------------------------------------------------

class SvxOnlineUpdateTabPage : public SfxTabPage
{
private:
    bool m_showTraditionalOnlineUpdate;
    bool m_showMarOnlineUpdate;

    OUString       m_aNeverChecked;
    OUString       m_aLastCheckedTemplate;

    css::uno::Reference< css::container::XNameReplace > m_xUpdateAccess;
    css::uno::Reference<css::configuration::XReadWriteAccess> m_xReadWriteAccess;

    std::unique_ptr<weld::Label> m_xNeverChecked;
    std::unique_ptr<weld::CheckButton> m_xAutoCheckCheckBox;
    std::unique_ptr<weld::Widget> m_xAutoCheckImg;
    std::unique_ptr<weld::RadioButton> m_xEveryDayButton;
    std::unique_ptr<weld::RadioButton> m_xEveryWeekButton;
    std::unique_ptr<weld::RadioButton> m_xEveryMonthButton;
    std::unique_ptr<weld::Widget> m_xCheckIntervalImg;
    std::unique_ptr<weld::Button> m_xCheckNowButton;
    std::unique_ptr<weld::CheckButton> m_xAutoDownloadCheckBox;
    std::unique_ptr<weld::Widget> m_xAutoDownloadImg;
    std::unique_ptr<weld::Label> m_xDestPathLabel;
    std::unique_ptr<weld::Label> m_xDestPath;
    std::unique_ptr<weld::Button> m_xChangePathButton;
    std::unique_ptr<weld::Label> m_xLastChecked;
    std::unique_ptr<weld::CheckButton> m_xExtrasCheckBox;
    std::unique_ptr<weld::Widget> m_xExtrasImg;
    std::unique_ptr<weld::Label> m_xUserAgentLabel;
    std::unique_ptr<weld::LinkButton> m_xPrivacyPolicyButton;
    std::unique_ptr<weld::Box> m_xBox2;
    std::unique_ptr<weld::Frame> m_xFrameDest;
    std::unique_ptr<weld::Frame> m_xFrameAgent;
    std::unique_ptr<weld::Frame> m_xMar;
    std::unique_ptr<weld::CheckButton> m_xEnableMar;

    DECL_LINK(FileDialogHdl_Impl, weld::Button&, void);
    DECL_LINK(CheckNowHdl_Impl, weld::Button&, void);
    DECL_LINK(AutoCheckHdl_Impl, weld::Toggleable&, void);
    DECL_LINK(ExtrasCheckHdl_Impl, weld::Toggleable&, void);

    void                    UpdateLastCheckedText();
    void                    UpdateUserAgent();

public:
    SvxOnlineUpdateTabPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rSet);
    static std::unique_ptr<SfxTabPage> Create( weld::Container* pPage, weld::DialogController* pController, const SfxItemSet* rSet );
    virtual ~SvxOnlineUpdateTabPage() override;

    virtual OUString GetAllStrings() override;

    virtual bool            FillItemSet( SfxItemSet* rSet ) override;
    virtual void            Reset( const SfxItemSet* rSet ) override;
    virtual void            FillUserData() override;

    static bool isTraditionalOnlineUpdateAvailable();
    static bool isMarOnlineUpdateAvailable();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

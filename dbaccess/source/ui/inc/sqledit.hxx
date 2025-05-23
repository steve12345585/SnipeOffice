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

#include <sal/config.h>

#include <comphelper/syntaxhighlight.hxx>
#include <rtl/ref.hxx>
#include <svtools/colorcfg.hxx>
#include <svx/weldeditview.hxx>
#include <vcl/timer.hxx>
#include <mutex>

namespace com::sun::star::beans { class XMultiPropertySet; }

namespace dbaui
{
    class SQLEditView final : public WeldEditView, public utl::ConfigurationListener
    {
    private:
        class ChangesListener;
        friend class ChangesListener;

        std::unique_ptr<weld::ScrolledWindow> m_xScrolledWindow;
        Link<LinkParamNone*,void> m_aModifyLink;
        const svtools::ColorConfig m_aColorConfig;
        Timer m_aUpdateDataTimer;
        const SyntaxHighlighter m_aHighlighter;
        svtools::ColorConfig m_ColorConfig;
        rtl::Reference<SfxItemPool> m_pItemPool;

        rtl::Reference<ChangesListener> m_listener;
        std::mutex m_mutex;
        css::uno::Reference<css::beans::XMultiPropertySet> m_notifier;

        bool m_bInUpdate;
        bool m_bDisableInternalUndo;

        DECL_LINK(ModifyHdl, LinkParamNone*, void);
        DECL_LINK(ImplUpdateDataHdl, Timer*, void);
        DECL_LINK(ScrollHdl, weld::ScrolledWindow&, void);
        DECL_LINK(EditStatusHdl, EditStatus&, void);

        Color GetColorValue(TokenType aToken);

        void ImplSetFont();

        void DoBracketHilight(sal_uInt16 nKey);

        static void SetItemPoolFont(SfxItemPool* pItemPool);

        void UpdateData();

        void SetScrollBarRange();
        void DoScroll();

        virtual void EditViewScrollStateChange() override;

    public:
        SQLEditView(std::unique_ptr<weld::ScrolledWindow> xScrolledWindow);
        virtual void makeEditEngine() override;
        virtual void SetDrawingArea(weld::DrawingArea* pDrawingArea) override;
        virtual ~SQLEditView() override;

        virtual bool KeyInput(const KeyEvent& rKEvt) override;
        virtual bool Command(const CommandEvent& rCEvt) override;

        void SetTextAndUpdate(const OUString& rNewText);

        void SetModifyHdl(const Link<LinkParamNone*,void>& rLink)
        {
            m_aModifyLink = rLink;
        }

        void DisableInternalUndo();

        static Color GetSyntaxHighlightColor(const svtools::ColorConfig& rColorConfig, HighlighterLanguage eLanguage, TokenType aToken);

        virtual void ConfigurationChanged(utl::ConfigurationBroadcaster*, ConfigurationHints) override;
    };
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

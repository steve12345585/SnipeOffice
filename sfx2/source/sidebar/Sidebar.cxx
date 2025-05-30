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

#include <sfx2/sidebar/Sidebar.hxx>
#include <sfx2/sidebar/SidebarController.hxx>
#include <sfx2/sidebar/ResourceManager.hxx>
#include <sfx2/sidebar/SidebarDockingWindow.hxx>
#include <sidebar/PanelDescriptor.hxx>
#include <sidebar/Tools.hxx>
#include <sfx2/sidebar/FocusManager.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/viewsh.hxx>
#include <com/sun/star/frame/XDispatch.hpp>

using namespace css;

namespace sfx2::sidebar {

void Sidebar::ShowDeck(std::u16string_view rsDeckId, SfxViewFrame* pViewFrame, bool bToggle)
{
    if (!pViewFrame)
        return;

    SfxChildWindow* pSidebarChildWindow = pViewFrame->GetChildWindow(SID_SIDEBAR);
    bool bInitiallyVisible = pSidebarChildWindow && pSidebarChildWindow->IsVisible();
    if (!bInitiallyVisible)
        pViewFrame->ShowChildWindow(SID_SIDEBAR);

    SidebarController* pController =
            SidebarController::GetSidebarControllerForFrame(pViewFrame->GetFrame().GetFrameInterface());
    if (!pController)
        return;

    if (bToggle && bInitiallyVisible && pController->IsDeckVisible(rsDeckId))
    {
        // close the sidebar if it was already visible and showing this sidebar deck
        const util::URL aURL(Tools::GetURL(u".uno:Sidebar"_ustr));
        css::uno::Reference<frame::XDispatch> xDispatch(Tools::GetDispatch(pViewFrame->GetFrame().GetFrameInterface(), aURL));
        if (xDispatch.is())
            xDispatch->dispatch(aURL, css::uno::Sequence<beans::PropertyValue>());
    }
    else
    {
        pController->OpenThenSwitchToDeck(rsDeckId);
        pController->GetFocusManager().GrabFocusPanel();
    }
}

void Sidebar::ShowPanel (
    std::u16string_view rsPanelId,
    const css::uno::Reference<frame::XFrame>& rxFrame, bool bFocus)
{
    SidebarController* pController = SidebarController::GetSidebarControllerForFrame(rxFrame);
    if (!pController)
        return;

    std::shared_ptr<PanelDescriptor> xPanelDescriptor = pController->GetResourceManager()->GetPanelDescriptor(rsPanelId);

    if (!xPanelDescriptor)
        return;

    // This should be a lot more sophisticated:
    // - Make the deck switching asynchronous
    // - Make sure to use a context that really shows the panel

    // All that is not necessary for the current use cases so let's
    // keep it simple for the time being.
    pController->OpenThenSwitchToDeck(xPanelDescriptor->msDeckId);

    if (bFocus)
        pController->GetFocusManager().GrabFocusPanel();
}

void Sidebar::TogglePanel (
    std::u16string_view rsPanelId,
    const css::uno::Reference<frame::XFrame>& rxFrame)
{
    SidebarController* pController = SidebarController::GetSidebarControllerForFrame(rxFrame);
    if (!pController)
        return;

    std::shared_ptr<PanelDescriptor> xPanelDescriptor = pController->GetResourceManager()->GetPanelDescriptor(rsPanelId);

    if (!xPanelDescriptor)
        return;

    // This should be a lot more sophisticated:
    // - Make the deck switching asynchronous
    // - Make sure to use a context that really shows the panel

    // All that is not necessary for the current use cases so let's
    // keep it simple for the time being.
    pController->OpenThenToggleDeck(xPanelDescriptor->msDeckId);
}

bool Sidebar::IsPanelVisible(
    std::u16string_view rsPanelId,
    const css::uno::Reference<frame::XFrame>& rxFrame)
{
    SidebarController* pController = SidebarController::GetSidebarControllerForFrame(rxFrame);
    if (!pController)
        return false;

    std::shared_ptr<PanelDescriptor> xPanelDescriptor = pController->GetResourceManager()->GetPanelDescriptor(rsPanelId);
    if (!xPanelDescriptor)
        return false;

    return pController->IsDeckVisible(xPanelDescriptor->msDeckId);
}

bool Sidebar::Setup(std::u16string_view sidebarDeckId)
{
    SfxViewShell* pViewShell = SfxViewShell::Current();
    SfxViewFrame* pViewFrame = pViewShell ? &pViewShell->GetViewFrame() : nullptr;
    if (pViewFrame)
    {
        if (!pViewFrame->GetChildWindow(SID_SIDEBAR))
            pViewFrame->SetChildWindow(SID_SIDEBAR, false /* create it */, true /* focus */);

        pViewFrame->ShowChildWindow(SID_SIDEBAR, true);

        // Force synchronous population of panels
        SfxChildWindow *pChild = pViewFrame->GetChildWindow(SID_SIDEBAR);
        if (!pChild)
            return false;

        auto pDockingWin = dynamic_cast<sfx2::sidebar::SidebarDockingWindow *>(pChild->GetWindow());
        if (!pDockingWin)
            return false;

        pViewFrame->ShowChildWindow( SID_SIDEBAR );

        const rtl::Reference<sfx2::sidebar::SidebarController>& xController
            = pDockingWin->GetOrCreateSidebarController();

        xController->FadeIn();
        xController->RequestOpenDeck();

        if (!sidebarDeckId.empty())
        {
            xController->SwitchToDeck(sidebarDeckId);
        }
        else
        {
            xController->SwitchToDefaultDeck();
        }

        pDockingWin->SyncUpdate();
        return true;
    }
    else
        return false;
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

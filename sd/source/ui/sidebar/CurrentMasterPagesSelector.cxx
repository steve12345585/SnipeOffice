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

#include "CurrentMasterPagesSelector.hxx"
#include <ViewShellBase.hxx>
#include <DrawViewShell.hxx>
#include <drawdoc.hxx>
#include <sdpage.hxx>
#include "MasterPageContainer.hxx"
#include "MasterPageContainerProviders.hxx"
#include "MasterPageDescriptor.hxx"
#include <EventMultiplexer.hxx>
#include <DrawDocShell.hxx>
#include <osl/diagnose.h>

#include <helpids.h>

#include <set>

using namespace ::com::sun::star;

namespace sd::sidebar {

std::unique_ptr<PanelLayout> CurrentMasterPagesSelector::Create (
    weld::Widget* pParent,
    ViewShellBase& rViewShellBase,
    const css::uno::Reference<css::ui::XSidebar>& rxSidebar)
{
    SdDrawDocument* pDocument = rViewShellBase.GetDocument();
    if (pDocument == nullptr)
        return nullptr;

    auto pContainer = std::make_shared<MasterPageContainer>();

    auto xSelector(std::make_unique<CurrentMasterPagesSelector>(
            pParent,
            *pDocument,
            rViewShellBase,
            pContainer,
            rxSidebar));
    xSelector->LateInit();
    xSelector->SetHelpId( HID_SD_TASK_PANE_PREVIEW_CURRENT );

    return xSelector;
}

CurrentMasterPagesSelector::CurrentMasterPagesSelector(
    weld::Widget* pParent, SdDrawDocument& rDocument, ViewShellBase& rBase,
    const std::shared_ptr<MasterPageContainer>& rpContainer,
    const css::uno::Reference<css::ui::XSidebar>& rxSidebar)
    : MasterPagesSelector(pParent, rDocument, rBase, rpContainer, rxSidebar,
                          u"modules/simpress/ui/masterpagepanel.ui"_ustr,
                          u"masterpagecurrent_icons"_ustr)
{
    Link<sd::tools::EventMultiplexerEvent&, void> aLink(
        LINK(this, CurrentMasterPagesSelector, EventMultiplexerListener));
    rBase.GetEventMultiplexer()->AddEventListener(aLink);
}

CurrentMasterPagesSelector::~CurrentMasterPagesSelector()
{
    if (mrDocument.GetDocSh() != nullptr)
    {
        EndListening(*mrDocument.GetDocSh());
    }
    else
    {
        OSL_ASSERT(mrDocument.GetDocSh() != nullptr);
    }

    Link<sd::tools::EventMultiplexerEvent&,void> aLink (LINK(this,CurrentMasterPagesSelector,EventMultiplexerListener));
    mrBase.GetEventMultiplexer()->RemoveEventListener(aLink);
}

void CurrentMasterPagesSelector::LateInit()
{
    MasterPagesSelector::LateInit();
    MasterPagesSelector::Fill();
    if (mrDocument.GetDocSh() != nullptr)
    {
        StartListening(*mrDocument.GetDocSh());
    }
    else
    {
        OSL_ASSERT(mrDocument.GetDocSh() != nullptr);
    }
}

void CurrentMasterPagesSelector::Fill (ItemList& rItemList)
{
    sal_uInt16 nPageCount = mrDocument.GetMasterSdPageCount(PageKind::Standard);
    // Remember the names of the master pages that have been inserted to
    // avoid double insertion.
    ::std::set<OUString> aMasterPageNames;
    for (sal_uInt16 nIndex=0; nIndex<nPageCount; nIndex++)
    {
        SdPage* pMasterPage = mrDocument.GetMasterSdPage (nIndex, PageKind::Standard);
        if (pMasterPage == nullptr)
            continue;

        // Use the name of the master page to avoid duplicate entries.
        OUString sName (pMasterPage->GetName());
        if (!aMasterPageNames.insert(sName).second)
            continue;

        // Look up the master page in the container and, when it is not yet
        // in it, insert it.
        MasterPageContainer::Token aToken = mpContainer->GetTokenForPageObject(pMasterPage);
        if (aToken == MasterPageContainer::NIL_TOKEN)
        {
            SharedMasterPageDescriptor pDescriptor = std::make_shared<MasterPageDescriptor>(
                MasterPageContainer::MASTERPAGE,
                nIndex,
                OUString(),
                pMasterPage->GetName(),
                OUString(),
                pMasterPage->IsPrecious(),
                std::make_shared<ExistingPageProvider>(pMasterPage),
                std::make_shared<PagePreviewProvider>());
            aToken = mpContainer->PutMasterPage(pDescriptor);
        }

        rItemList.push_back(aToken);
    }
}

OUString CurrentMasterPagesSelector::GetContextMenuUIFile() const
{
    return u"modules/simpress/ui/currentmastermenu.ui"_ustr;
}

void CurrentMasterPagesSelector::UpdateSelection()
{
    // Iterate over all pages and for the selected ones put the name of
    // their master page into a set.
    sal_uInt16 nPageCount = mrDocument.GetSdPageCount(PageKind::Standard);
    ::std::set<OUString> aNames;
    sal_uInt16 nIndex;
    bool bLoop (true);
    for (nIndex=0; nIndex<nPageCount && bLoop; nIndex++)
    {
        SdPage* pPage = mrDocument.GetSdPage (nIndex, PageKind::Standard);
        if (pPage != nullptr && pPage->IsSelected())
        {
            if ( ! pPage->TRG_HasMasterPage())
            {
                // One of the pages has no master page.  This is an
                // indicator for that this method is called in the middle of
                // a document change and that the model is not in a valid
                // state.  Therefore we stop update the selection and wait
                // for another call to UpdateSelection when the model is
                // valid again.
                bLoop = false;
            }
            else
            {
                SdrPage& rMasterPage (pPage->TRG_GetMasterPage());
                assert(dynamic_cast<SdPage*>(&rMasterPage));
                aNames.insert(static_cast<SdPage&>(rMasterPage).GetName());
            }
        }
    }
}

void CurrentMasterPagesSelector::ExecuteCommand(const OUString &rIdent)
{
    if (rIdent == "delete")
    {
        // Check once again that the master page can safely be deleted,
        // i.e. is not used.
        SdPage* pMasterPage = GetSelectedMasterPage();
        if (pMasterPage != nullptr
            && mrDocument.GetMasterPageUserCount(pMasterPage) == 0)
        {
            // Removing the precious flag so that the following call to
            // RemoveUnnecessaryMasterPages() will remove this master page.
            pMasterPage->SetPrecious(false);
            mrDocument.RemoveUnnecessaryMasterPages(pMasterPage);
        }
    }
    else
        MasterPagesSelector::ExecuteCommand(rIdent);
}

void CurrentMasterPagesSelector::ProcessPopupMenu(weld::Menu& rMenu)
{
    // Disable the delete entry when there is only one master page.
    if (mrDocument.GetMasterPageUserCount(GetSelectedMasterPage()) > 0)
        rMenu.set_sensitive(u"delete"_ustr, false);

    std::shared_ptr<DrawViewShell> pDrawViewShell (
        std::dynamic_pointer_cast<DrawViewShell>(mrBase.GetMainViewShell()));
    if (pDrawViewShell && pDrawViewShell->GetEditMode() == EditMode::MasterPage)
    {
        rMenu.set_sensitive(u"edit"_ustr, false);
    }

    MasterPagesSelector::ProcessPopupMenu(rMenu);
}

IMPL_LINK(CurrentMasterPagesSelector,EventMultiplexerListener,
    sd::tools::EventMultiplexerEvent&, rEvent, void)
{
    switch (rEvent.meEventId)
    {
        case EventMultiplexerEventId::CurrentPageChanged:
        case EventMultiplexerEventId::EditModeNormal:
        case EventMultiplexerEventId::EditModeMaster:
        case EventMultiplexerEventId::SlideSortedSelection:
            UpdateSelection();
            break;

        case EventMultiplexerEventId::PageOrder:
            // This is tricky.  If a master page is removed, moved, or
            // added we have to wait until both the notes master page
            // and the standard master page have been removed, moved,
            // or added.  We do this by looking at the number of master
            // pages which has to be odd in the consistent state (the
            // handout master page is always present).  If the number is
            // even we ignore the hint.
            if (mrBase.GetDocument()->GetMasterPageCount()%2 == 1)
                MasterPagesSelector::Fill();
            break;

        case EventMultiplexerEventId::ShapeChanged:
        case EventMultiplexerEventId::ShapeInserted:
        case EventMultiplexerEventId::ShapeRemoved:
            InvalidatePreview(static_cast<const SdPage*>(rEvent.mpUserData));
            break;
        default: break;
    }
}

} // end of namespace sd::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

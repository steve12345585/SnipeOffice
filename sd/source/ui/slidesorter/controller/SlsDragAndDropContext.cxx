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

#include "SlsDragAndDropContext.hxx"

#include <SlideSorter.hxx>
#include <model/SlideSorterModel.hxx>
#include <controller/SlideSorterController.hxx>
#include <controller/SlsInsertionIndicatorHandler.hxx>
#include <controller/SlsScrollBarManager.hxx>
#include <controller/SlsProperties.hxx>
#include <controller/SlsClipboard.hxx>
#include <controller/SlsTransferableData.hxx>
#include <Window.hxx>
#include <sdtreelb.hxx>
#include <sdmod.hxx>

namespace sd::slidesorter::controller {

DragAndDropContext::DragAndDropContext (SlideSorter& rSlideSorter)
    : mpTargetSlideSorter(&rSlideSorter),
      mnInsertionIndex(-1)
{
    // No Drag-and-Drop for master pages.
    if (rSlideSorter.GetModel().GetEditMode() != EditMode::Page)
        return;

    // For properly handling transferables created by the navigator we
    // need additional information.  For this a user data object is
    // created that contains the necessary information.
    SdTransferable* pTransferable = SdModule::get()->pTransferDrag;
    SdPageObjsTLV::SdPageObjsTransferable* pTreeListBoxTransferable
        = dynamic_cast<SdPageObjsTLV::SdPageObjsTransferable*>(pTransferable);
    if (pTreeListBoxTransferable!=nullptr && !TransferableData::GetFromTransferable(pTransferable))
    {
        pTransferable->AddUserData(
            sd::slidesorter::controller::Clipboard::CreateTransferableUserData(pTransferable));
    }

    rSlideSorter.GetController().GetInsertionIndicatorHandler()->UpdateIndicatorIcon(pTransferable);
}

DragAndDropContext::~DragAndDropContext() COVERITY_NOEXCEPT_FALSE
{
    SetTargetSlideSorter();
}

void DragAndDropContext::Dispose()
{
    mnInsertionIndex = -1;
}

void DragAndDropContext::UpdatePosition (
    const Point& rMousePosition,
    const InsertionIndicatorHandler::Mode eMode,
    const bool bAllowAutoScroll)
{
    if (mpTargetSlideSorter == nullptr)
        return;

    // Convert window coordinates into model coordinates (we need the
    // window coordinates for auto-scrolling because that remains
    // constant while scrolling.)
    sd::Window *pWindow = mpTargetSlideSorter->GetContentWindow().get();
    const Point aMouseModelPosition (pWindow->PixelToLogic(rMousePosition));
    std::shared_ptr<InsertionIndicatorHandler> pInsertionIndicatorHandler (
        mpTargetSlideSorter->GetController().GetInsertionIndicatorHandler());

    bool bDoAutoScroll = bAllowAutoScroll
            && mpTargetSlideSorter->GetController().GetScrollBarManager().AutoScroll(
                rMousePosition,
                [this, eMode, rMousePosition] () {
                    return this->UpdatePosition(rMousePosition, eMode, false);
                });

    if (!bDoAutoScroll)
    {
        pInsertionIndicatorHandler->UpdatePosition(aMouseModelPosition, eMode);

        // Remember the new insertion index.
        mnInsertionIndex = pInsertionIndicatorHandler->GetInsertionPageIndex();
        if (pInsertionIndicatorHandler->IsInsertionTrivial(mnInsertionIndex, eMode))
            mnInsertionIndex = -1;
    }
}

void DragAndDropContext::SetTargetSlideSorter()
{
    if (mpTargetSlideSorter != nullptr)
    {
        mpTargetSlideSorter->GetController().GetScrollBarManager().StopAutoScroll();
        mpTargetSlideSorter->GetController().GetInsertionIndicatorHandler()->End(
            Animator::AM_Animated);
    }

    mpTargetSlideSorter = nullptr;
}

} // end of namespace ::sd::slidesorter::controller

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

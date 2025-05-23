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

#include <model/SlsSharedPageDescriptor.hxx>
#include <tools/link.hxx>
#include <rtl/ustring.hxx>

class AbstractSvxNameDialog;
class SfxItemSet;
class SfxRequest;

namespace sd::slidesorter
{
class SlideSorter;
}

namespace sd::slidesorter::controller
{
/** This manager takes over the work of handling slot calls from the
    controller of the slide sorter.
*/
class SlotManager
{
public:
    /** Create a new slot manager that handles slot calls for the controller
        of a slide sorter.
        @param rController
            The controller for which to handle the slot calls.
    */
    SlotManager(SlideSorter& rSlideSorter);

    void FuTemporary(SfxRequest& rRequest);
    void FuPermanent(SfxRequest& rRequest);
    void FuSupport(SfxRequest& rRequest);
    void GetMenuState(SfxItemSet& rSet);
    void GetClipboardState(SfxItemSet& rSet);
    void GetStatusBarState(SfxItemSet& rSet);
    void ExecCtrl(SfxRequest& rRequest);
    void GetAttrState(SfxItemSet& rSet);

    /** Exclude or include one slide or all selected slides.
        @param rpDescriptor
            When the pointer is empty then apply the new state to all
            selected pages.  Otherwise apply the new state to just the
            specified state.
    */
    void ChangeSlideExclusionState(const model::SharedPageDescriptor& rpDescriptor,
                                   const bool bExcludeSlide);

    /** Call this after a change from normal mode to master mode or back.
        The affected slots are invalidated.
    */
    void NotifyEditModeChange();

private:
    /// The controller for which we manage the slot calls.
    SlideSorter& mrSlideSorter;

    /** The implementation is a copy of the code for SID_RENAMEPAGE in
        drviews2.cxx.
    */
    void RenameSlide(const SfxRequest& rRequest);
    DECL_LINK(RenameSlideHdl, AbstractSvxNameDialog&, bool);
    DECL_STATIC_LINK(SlotManager, RenameSlideTooltipHdl, AbstractSvxNameDialog&, OUString);
    bool RenameSlideFromDrawViewShell(sal_uInt16 nPageId, const OUString& rName);

    /** Handle SID_INSERTPAGE slot calls.
    */
    void InsertSlide(SfxRequest& rRequest);

    void DuplicateSelectedSlides(SfxRequest& rRequest);

    /** Use one of several ways to determine where to insert a new page.
        This can be the current selection or the insertion indicator.
    */
    sal_Int32 GetInsertionPosition() const;
};

} // end of namespace ::sd::slidesorter::controller

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

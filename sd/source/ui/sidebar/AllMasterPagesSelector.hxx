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

#include "MasterPagesSelector.hxx"

#include <memory>

namespace sd::sidebar {

/** Show a list of all available master pages so that the user can assign
    them to the document.
*/
class AllMasterPagesSelector
    : public MasterPagesSelector
{
    friend class VclPtrInstance<AllMasterPagesSelector>;
public:
    static std::unique_ptr<PanelLayout> Create (
        weld::Widget* pParent,
        ViewShellBase& rViewShellBase,
        const css::uno::Reference<css::ui::XSidebar>& rxSidebar);

    AllMasterPagesSelector (
        weld::Widget* pParent,
        SdDrawDocument& rDocument,
        ViewShellBase& rBase,
        const std::shared_ptr<MasterPageContainer>& rpContainer,
        const css::uno::Reference<css::ui::XSidebar>& rxSidebar);
    virtual ~AllMasterPagesSelector() override;


    /** Scan the set of templates for the ones whose first master pages are
        shown by this control and store them in the MasterPageContainer.
    */
    virtual void Fill (ItemList& rItemList) override;

protected:
    virtual void NotifyContainerChangeEvent (const MasterPageContainerChangeEvent& rEvent) override;

private:
    /** The list of master pages displayed by this class.
    */
    class SortedMasterPageDescriptorList;
    ::std::unique_ptr<SortedMasterPageDescriptorList> mpSortedMasterPages;

    void AddItem (MasterPageContainer::Token aToken);

    /** Add all items in the internal master page list into the given list.
    */
    void UpdatePageSet (ItemList& rItemList);

    /** Update the internal list of master pages that are to show in the
        control.
    */
    void UpdateMasterPageList();

    using MasterPagesSelector::Fill;
};

} // end of namespace sd::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

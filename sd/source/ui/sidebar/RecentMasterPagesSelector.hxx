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

namespace sd::sidebar {

/** Show the recently used master pages (that are not currently used).
*/
class RecentMasterPagesSelector final
    : public MasterPagesSelector
{
    friend class VclPtrInstance<RecentMasterPagesSelector>;
public:
    static std::unique_ptr<PanelLayout> Create (
        weld::Widget* pParent,
        ViewShellBase& rViewShellBase,
        const css::uno::Reference<css::ui::XSidebar>& rxSidebar);

    RecentMasterPagesSelector (
        weld::Widget* pParent,
        SdDrawDocument& rDocument,
        ViewShellBase& rBase,
        const std::shared_ptr<MasterPageContainer>& rpContainer,
        const css::uno::Reference<css::ui::XSidebar>& rxSidebar);
    virtual ~RecentMasterPagesSelector() override;

private:
    DECL_LINK(MasterPageListListener, LinkParamNone*, void);
    virtual void Fill (ItemList& rItemList) override;

    using sd::sidebar::MasterPagesSelector::Fill;

    virtual void LateInit() override;
};

} // end of namespace sd::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

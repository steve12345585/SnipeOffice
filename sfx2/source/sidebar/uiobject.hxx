/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vcl/uitest/uiobject.hxx>
#include <sfx2/sidebar/TabBar.hxx>

namespace sfx2::sidebar
{
class TabBarUIObject : public WindowUIObject
{
    VclPtr<sfx2::sidebar::TabBar> mxTabBar;

    virtual OUString get_name() const override;

public:
    TabBarUIObject(const VclPtr<TabBar>& xTabBar);

    virtual void execute(const OUString& rAction, const StringMap& rParameters) override;
    virtual StringMap get_state() override;

    static std::unique_ptr<UIObject> create(vcl::Window* pWindow);
};

} // namespace sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

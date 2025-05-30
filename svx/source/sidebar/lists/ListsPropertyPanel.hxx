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
#ifndef INCLUDED_SVX_SOURCE_SIDEBAR_LISTS_LISTSPROPERTYPANEL_HXX
#define INCLUDED_SVX_SOURCE_SIDEBAR_LISTS_LISTSPROPERTYPANEL_HXX

#include <sfx2/weldutils.hxx>
#include <sfx2/sidebar/PanelLayout.hxx>
#include <com/sun/star/frame/XFrame.hpp>

namespace svx::sidebar
{
class ListsPropertyPanel : public PanelLayout
{
public:
    virtual ~ListsPropertyPanel() override;

    static std::unique_ptr<PanelLayout>
    Create(weld::Widget* pParent, const css::uno::Reference<css::frame::XFrame>& rxFrame);

    ListsPropertyPanel(weld::Widget* pParent,
                       const css::uno::Reference<css::frame::XFrame>& rxFrame);

private:
    std::unique_ptr<weld::Toolbar> mxTBxNumBullet;
    std::unique_ptr<ToolbarUnoDispatcher> mxNumBulletDispatcher;
    std::unique_ptr<weld::Toolbar> mxTBxOutline;
    std::unique_ptr<ToolbarUnoDispatcher> mxOutlineDispatcher;
};

} // end of namespace svx::sidebar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

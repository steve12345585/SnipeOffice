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
#include <TextCharacterSpacingPopup.hxx>
#include "TextCharacterSpacingControl.hxx"
#include <vcl/toolbox.hxx>

using namespace svx;

TextCharacterSpacingPopup::TextCharacterSpacingPopup(
    const css::uno::Reference<css::uno::XComponentContext>& rContext)
    : PopupWindowController(rContext, nullptr, OUString())
{
}

void TextCharacterSpacingPopup::initialize(const css::uno::Sequence<css::uno::Any>& rArguments)
{
    PopupWindowController::initialize(rArguments);

    if (m_pToolbar)
    {
        mxPopoverContainer.reset(new ToolbarPopupContainer(m_pToolbar));
        m_pToolbar->set_item_popover(m_aCommandURL, mxPopoverContainer->getTopLevel());
    }

    ToolBox* pToolBox = nullptr;
    ToolBoxItemId nId;
    if (getToolboxId(nId, &pToolBox))
        pToolBox->SetItemBits(nId, ToolBoxItemBits::DROPDOWNONLY | pToolBox->GetItemBits(nId));
}

TextCharacterSpacingPopup::~TextCharacterSpacingPopup() {}

std::unique_ptr<WeldToolbarPopup> TextCharacterSpacingPopup::weldPopupWindow()
{
    return std::make_unique<TextCharacterSpacingControl>(this, m_pToolbar);
}

VclPtr<vcl::Window> TextCharacterSpacingPopup::createVclPopupWindow(vcl::Window* pParent)
{
    mxInterimPopover = VclPtr<InterimToolbarPopup>::Create(
        getFrameInterface(), pParent,
        std::make_unique<TextCharacterSpacingControl>(this, pParent->GetFrameWeld()));

    mxInterimPopover->Show();

    return mxInterimPopover;
}

OUString TextCharacterSpacingPopup::getImplementationName()
{
    return u"com.sun.star.comp.svx.CharacterSpacingToolBoxControl"_ustr;
}

css::uno::Sequence<OUString> TextCharacterSpacingPopup::getSupportedServiceNames()
{
    return { u"com.sun.star.frame.ToolbarController"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_svx_CharacterSpacingToolBoxControl_get_implementation(
    css::uno::XComponentContext* rContext, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new TextCharacterSpacingPopup(rContext));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

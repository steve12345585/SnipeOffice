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

#include <helper/tagwindowasmodified.hxx>

#include <com/sun/star/awt/XWindow.hpp>

#include <com/sun/star/frame/FrameAction.hpp>

#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wintypes.hxx>

namespace framework{

TagWindowAsModified::TagWindowAsModified()
{
}

TagWindowAsModified::~TagWindowAsModified()
{
}

void SAL_CALL TagWindowAsModified::initialize(const css::uno::Sequence< css::uno::Any >& lArguments)
{
    css::uno::Reference< css::frame::XFrame > xFrame;

    if (lArguments.hasElements())
        lArguments[0] >>= xFrame;

    if (!xFrame)
        return;

    m_xFrame = xFrame;
    xFrame->addFrameActionListener(this);
    impl_update (xFrame);
}

void SAL_CALL TagWindowAsModified::modified(const css::lang::EventObject& aEvent)
{
    if (!m_xModel || !m_xWindow || aEvent.Source != m_xModel)
        return;

    bool bModified = m_xModel->isModified ();

    // SYNCHRONIZED ->
    SolarMutexGuard aSolarGuard;

    if (m_xWindow->isDisposed())
        return;

    if (bModified)
        m_xWindow->SetExtendedStyle(WindowExtendedStyle::DocModified);
    else
        m_xWindow->SetExtendedStyle(WindowExtendedStyle::NONE);
    // <- SYNCHRONIZED
}

void SAL_CALL TagWindowAsModified::frameAction(const css::frame::FrameActionEvent& aEvent)
{
    if (
        (aEvent.Action != css::frame::FrameAction_COMPONENT_REATTACHED) &&
        (aEvent.Action != css::frame::FrameAction_COMPONENT_ATTACHED  )
       )
        return;

    if ( aEvent.Source != m_xFrame )
        return;

    impl_update (m_xFrame);
}

void SAL_CALL TagWindowAsModified::disposing(const css::lang::EventObject& aEvent)
{
    SolarMutexGuard g;

    if (m_xFrame && aEvent.Source == m_xFrame)
    {
        m_xFrame->removeFrameActionListener(this);
        m_xFrame.clear();
        return;
    }

    if (m_xModel && aEvent.Source == m_xModel)
    {
        m_xModel->removeModifyListener(this);
        m_xModel.clear();
        return;
    }
}

void TagWindowAsModified::impl_update (const css::uno::Reference< css::frame::XFrame >& xFrame)
{
    if (!xFrame)
        return;

    css::uno::Reference< css::awt::XWindow >       xWindow     = xFrame->getContainerWindow ();
    css::uno::Reference< css::frame::XController > xController = xFrame->getController ();
    css::uno::Reference< css::util::XModifiable >  xModel;
    if (xController.is ())
        xModel = css::uno::Reference< css::util::XModifiable >(xController->getModel(), css::uno::UNO_QUERY);

    if (!xWindow || !xModel)
        return;

    {
        SolarMutexGuard g;

        VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow(xWindow);
        bool bSystemWindow = pWindow->IsSystemWindow();
        bool bWorkWindow   = (pWindow->GetType() == WindowType::WORKWINDOW);
        if (!bSystemWindow && !bWorkWindow)
            return;

        if (m_xModel)
            m_xModel->removeModifyListener (this);

        // Note: frame was set as member outside ! we have to refresh connections
        // regarding window and model only here.
        m_xWindow = std::move(pWindow);
        m_xModel  = std::move(xModel);
    }

    m_xModel->addModifyListener (this);
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include "PanelFactory.hxx"
#include <framework/Pane.hxx>
#include <ViewShellBase.hxx>
#include <DrawController.hxx>
#include "LayoutMenu.hxx"
#include "CurrentMasterPagesSelector.hxx"
#include "RecentMasterPagesSelector.hxx"
#include "AllMasterPagesSelector.hxx"
#include <CustomAnimationPane.hxx>
#include "NavigatorWrapper.hxx"
#include <SlideTransitionPane.hxx>
#include <TableDesignPane.hxx>
#include "SlideBackground.hxx"

#include <sfx2/sidebar/SidebarPanelBase.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/weldutils.hxx>

using namespace css;
using namespace css::uno;

namespace sd::sidebar {

//----- PanelFactory --------------------------------------------------------

PanelFactory::PanelFactory()
{
}

PanelFactory::~PanelFactory()
{
}

// XUIElementFactory

Reference<ui::XUIElement> SAL_CALL PanelFactory::createUIElement (
    const OUString& rsUIElementResourceURL,
    const css::uno::Sequence<css::beans::PropertyValue>& rArguments)
{
    // Process arguments.
    const ::comphelper::NamedValueCollection aArguments (rArguments);
    Reference<frame::XFrame> xFrame (aArguments.getOrDefault(u"Frame"_ustr, Reference<frame::XFrame>()));
    Reference<awt::XWindow> xParentWindow (aArguments.getOrDefault(u"ParentWindow"_ustr, Reference<awt::XWindow>()));
    Reference<ui::XSidebar> xSidebar (aArguments.getOrDefault(u"Sidebar"_ustr, Reference<ui::XSidebar>()));

    // Throw exceptions when the arguments are not as expected.
    weld::Widget* pParent(nullptr);
    if (weld::TransportAsXWindow* pTunnel = dynamic_cast<weld::TransportAsXWindow*>(xParentWindow.get()))
        pParent = pTunnel->getWidget();

    if (!pParent)
        throw RuntimeException(
            u"PanelFactory::createUIElement called without ParentWindow"_ustr);
    if ( ! xFrame.is())
        throw RuntimeException(
            u"PanelFactory::createUIElement called without XFrame"_ustr);

    // Tunnel through the controller to obtain a ViewShellBase.
    ViewShellBase* pBase = nullptr;
    rtl::Reference<sd::DrawController> pController = dynamic_cast<sd::DrawController*>(xFrame->getController().get());
    if (pController != nullptr)
        pBase = pController->GetViewShellBase();
    if (pBase == nullptr)
        throw RuntimeException(u"can not get ViewShellBase for frame"_ustr);

    // Get bindings from given arguments.
    const sal_uInt64 nBindingsValue (aArguments.getOrDefault(u"SfxBindings"_ustr, sal_uInt64(0)));
    SfxBindings* pBindings = reinterpret_cast<SfxBindings*>(nBindingsValue);

    // Create a framework view.
    std::unique_ptr<PanelLayout> xControl;
    css::ui::LayoutSize aLayoutSize (-1,-1,-1);

    /** Note that these names have to be identical to (the tail of)
        the entries in officecfg/registry/data/org/openoffice/Office/Impress.xcu
        for the TaskPanelFactory.
    */
    if (rsUIElementResourceURL.endsWith("/CustomAnimations"))
        xControl = std::make_unique<CustomAnimationPane>(pParent, *pBase);
    else if (rsUIElementResourceURL.endsWith("/Layouts"))
        xControl = std::make_unique<LayoutMenu>(pParent, *pBase, xSidebar);
    else if (rsUIElementResourceURL.endsWith("/AllMasterPages"))
        xControl = AllMasterPagesSelector::Create(pParent, *pBase, xSidebar);
    else if (rsUIElementResourceURL.endsWith("/RecentMasterPages"))
        xControl = RecentMasterPagesSelector::Create(pParent, *pBase, xSidebar);
    else if (rsUIElementResourceURL.endsWith("/UsedMasterPages"))
        xControl = CurrentMasterPagesSelector::Create(pParent, *pBase, xSidebar);
    else if (rsUIElementResourceURL.endsWith("/SlideTransitions"))
        xControl = std::make_unique<SlideTransitionPane>(pParent, *pBase);
    else if (rsUIElementResourceURL.endsWith("/TableDesign"))
        xControl = std::make_unique<TableDesignPane>(pParent, *pBase);
    else if (rsUIElementResourceURL.endsWith("/NavigatorPanel"))
        xControl = std::make_unique<NavigatorWrapper>(pParent, *pBase, pBindings);
    else if (rsUIElementResourceURL.endsWith("/SlideBackgroundPanel"))
        xControl = std::make_unique<SlideBackground>(pParent, *pBase, xFrame, pBindings);

    if (!xControl)
        throw lang::IllegalArgumentException();

    // Create a wrapper around the control that implements the
    // necessary UNO interfaces.
    return sfx2::sidebar::SidebarPanelBase::Create(
        rsUIElementResourceURL,
        xFrame,
        std::move(xControl),
        aLayoutSize);
}

OUString PanelFactory::getImplementationName() {
    return u"org.openoffice.comp.Draw.framework.PanelFactory"_ustr;
}

sal_Bool PanelFactory::supportsService(OUString const & ServiceName) {
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> PanelFactory::getSupportedServiceNames() {
    return {u"com.sun.star.drawing.framework.PanelFactory"_ustr};
}

} // end of namespace sd::sidebar


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
org_openoffice_comp_Draw_framework_PanelFactory_get_implementation(css::uno::XComponentContext* /*context*/,
                                                                   css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new sd::sidebar::PanelFactory);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

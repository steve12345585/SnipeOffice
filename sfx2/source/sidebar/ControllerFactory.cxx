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

#include <sidebar/ControllerFactory.hxx>
#include <sidebar/Tools.hxx>

#include <com/sun/star/frame/XToolbarController.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/theToolbarControllerFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <framework/sfxhelperfunctions.hxx>
#include <framework/generictoolbarcontroller.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/commandinfoprovider.hxx>
#include <vcl/weldutils.hxx>
#include <comphelper/processfactory.hxx>
#include <toolkit/helper/vclunohelper.hxx>

using namespace css;
using namespace css::uno;

namespace sfx2::sidebar {

Reference<lang::XComponent> ControllerFactory::CreateImageController(
    const Reference<frame::XFrame>& rxFrame,
    const Reference<awt::XWindow>& rxParentWindow)
{
    rtl::Reference xController(new framework::ImageOrientationController(
        comphelper::getProcessComponentContext(), rxFrame, rxParentWindow,
        vcl::CommandInfoProvider::GetModuleIdentifier(rxFrame)));

    xController->update();
    return xController;
}

Reference<frame::XToolbarController> ControllerFactory::CreateToolBoxController(
    ToolBox* pToolBox,
    const ToolBoxItemId nItemId,
    const OUString& rsCommandName,
    const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxController,
    const Reference<awt::XWindow>& rxParentWindow,
    const sal_Int32 nWidth, bool bSideBar)
{
    Reference<frame::XToolbarController> xController (
        CreateToolBarController(
            VCLUnoHelper::GetInterface(pToolBox),
            rsCommandName,
            rxFrame, rxController,
            nWidth, bSideBar));

    bool bFactoryHasController( xController.is() );

    // Create a controller for the new item.
    if ( !bFactoryHasController )
    {
        xController = ::framework::CreateToolBoxController(
                    rxFrame,
                    pToolBox,
                    nItemId,
                    rsCommandName);
    }
    if ( ! xController.is())
    {
        xController = new framework::GenericToolbarController(
                    ::comphelper::getProcessComponentContext(),
                    rxFrame,
                    pToolBox,
                    nItemId,
                    rsCommandName);
    }

    // Initialize the controller with eg a service factory.
    Reference<lang::XInitialization> xInitialization (xController, UNO_QUERY);
    if (!bFactoryHasController && xInitialization.is())
    {
        beans::PropertyValue aPropValue;
        std::vector<Any> aPropertyVector;

        aPropValue.Name = "Frame";
        aPropValue.Value <<= rxFrame;
        aPropertyVector.push_back(Any(aPropValue));

        aPropValue.Name = "ServiceManager";
        aPropValue.Value <<= ::comphelper::getProcessServiceFactory();
        aPropertyVector.push_back(Any(aPropValue));

        aPropValue.Name = "CommandURL";
        aPropValue.Value <<= rsCommandName;
        aPropertyVector.push_back(Any(aPropValue));

        Sequence<Any> aArgs (comphelper::containerToSequence(aPropertyVector));
        xInitialization->initialize(aArgs);
    }

    if (xController.is())
    {
        if (rxParentWindow.is())
        {
            Reference<awt::XWindow> xItemWindow (xController->createItemWindow(rxParentWindow));
            VclPtr<vcl::Window> pItemWindow = VCLUnoHelper::GetWindow(xItemWindow);
            if (pItemWindow != nullptr)
            {
                WindowType eType = pItemWindow->GetType();
                if (eType == WindowType::LISTBOX || eType == WindowType::MULTILISTBOX || eType == WindowType::COMBOBOX)
                    pItemWindow->SetAccessibleName(pToolBox->GetItemText(nItemId));
                if (nWidth > 0)
                    pItemWindow->SetSizePixel(Size(nWidth, pItemWindow->GetSizePixel().Height()));
                pToolBox->SetItemWindow(nItemId, pItemWindow);
            }
        }

        Reference<util::XUpdatable> xUpdatable (xController, UNO_QUERY);
        if (xUpdatable.is())
            xUpdatable->update();

        // Add tooltip.
        if (xController.is())
        {
            auto aProperties = vcl::CommandInfoProvider::GetCommandProperties(rsCommandName,
                vcl::CommandInfoProvider::GetModuleIdentifier(rxFrame));
            const OUString sTooltip (vcl::CommandInfoProvider::GetTooltipForCommand(
                    rsCommandName, aProperties, rxFrame));
            if (pToolBox->GetQuickHelpText(nItemId).isEmpty())
                pToolBox->SetQuickHelpText(nItemId, sTooltip);
            pToolBox->EnableItem(nItemId);
        }
    }

    return xController;
}

Reference<frame::XToolbarController> ControllerFactory::CreateToolBoxController(
    weld::Toolbar& rToolbar, weld::Builder& rBuilder,
    const OUString& rsCommandName,
    const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxController,
    bool bSideBar)
{
    css::uno::Reference<css::awt::XWindow> xWidget(new weld::TransportAsXWindow(&rToolbar, &rBuilder));

    Reference<frame::XToolbarController> xController(
        CreateToolBarController(
            xWidget,
            rsCommandName,
            rxFrame, rxController,
            -1, bSideBar));

    if (!xController.is())
    {
        xController = new framework::GenericToolbarController(
                    ::comphelper::getProcessComponentContext(),
                    rxFrame,
                    rToolbar,
                    rsCommandName);
    }

    if (xController.is())
    {
        xController->createItemWindow(xWidget);

        Reference<util::XUpdatable> xUpdatable(xController, UNO_QUERY);
        if (xUpdatable.is())
            xUpdatable->update();
    }

    return xController;
}


Reference<frame::XToolbarController> ControllerFactory::CreateToolBarController(
    const Reference<awt::XWindow>& rxToolbar,
    const OUString& rsCommandName,
    const Reference<frame::XFrame>& rxFrame,
    const Reference<frame::XController>& rxController,
    const sal_Int32 nWidth, bool bSideBar)
{
    try
    {
        const Reference<XComponentContext>& xContext = comphelper::getProcessComponentContext();
        Reference<frame::XUIControllerFactory> xFactory = frame::theToolbarControllerFactory::get( xContext );
        OUString sModuleName (Tools::GetModuleName(rxController));

        if (xFactory.is() && xFactory->hasController(rsCommandName,  sModuleName))
        {
            beans::PropertyValue aPropValue;
            std::vector<Any> aPropertyVector;

            aPropValue.Name = "ModuleIdentifier";
            aPropValue.Value <<= sModuleName;
            aPropertyVector.push_back( Any( aPropValue ));

            aPropValue.Name = "Frame";
            aPropValue.Value <<= rxFrame;
            aPropertyVector.push_back( Any( aPropValue ));

            aPropValue.Name = "ServiceManager";
            aPropValue.Value <<= comphelper::getProcessServiceFactory();
            aPropertyVector.push_back( Any( aPropValue ));

            aPropValue.Name = "ParentWindow";
            aPropValue.Value <<= rxToolbar;
            aPropertyVector.push_back( Any( aPropValue ));

            aPropValue.Name = "IsSidebar";
            aPropValue.Value <<= bSideBar;
            aPropertyVector.push_back( Any( aPropValue ));

            if (nWidth > 0)
            {
                aPropValue.Name = "Width";
                aPropValue.Value <<= nWidth;
                aPropertyVector.push_back( Any( aPropValue ));
            }

            Sequence<Any> aArgs (comphelper::containerToSequence(aPropertyVector));
            return Reference<frame::XToolbarController>(
                xFactory->createInstanceWithArgumentsAndContext(
                    rsCommandName,
                    aArgs,
                    xContext),
                UNO_QUERY);
        }
    }
    catch (Exception&)
    {
        // Ignore exception.
    }
    return nullptr;
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <helper/titlebarupdate.hxx>

#include <properties.h>

#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/frame/ModuleManager.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XTitle.hpp>
#include <com/sun/star/frame/XTitleChangeBroadcaster.hpp>

#include <comphelper/sequenceashashmap.hxx>
#include <unotools/configmgr.hxx>
#include <utility>
#include <vcl/window.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#include <comphelper/diagnose_ex.hxx>

namespace framework{

const ::sal_Int32 INVALID_ICON_ID = -1;
const ::sal_Int32 DEFAULT_ICON_ID =  0;

TitleBarUpdate::TitleBarUpdate(css::uno::Reference< css::uno::XComponentContext >  xContext)
    : m_xContext              (std::move(xContext                     ))
{
}

TitleBarUpdate::~TitleBarUpdate()
{
}

void SAL_CALL TitleBarUpdate::initialize(const css::uno::Sequence< css::uno::Any >& lArguments)
{
    // check arguments
    css::uno::Reference< css::frame::XFrame > xFrame;
    if (!lArguments.hasElements())
        throw css::lang::IllegalArgumentException(
                u"Empty argument list!"_ustr,
                static_cast< ::cppu::OWeakObject* >(this),
                1);

    lArguments[0] >>= xFrame;
    if (!xFrame.is())
        throw css::lang::IllegalArgumentException(
                u"No valid frame specified!"_ustr,
                static_cast< ::cppu::OWeakObject* >(this),
                1);

    {
        SolarMutexGuard g;
        // hold the frame as weak reference(!) so it can die everytimes :-)
        m_xFrame = xFrame;
    }

    // start listening
    xFrame->addFrameActionListener(this);

    css::uno::Reference< css::frame::XTitleChangeBroadcaster > xBroadcaster(xFrame, css::uno::UNO_QUERY);
    if (xBroadcaster.is ())
        xBroadcaster->addTitleChangeListener (this);
}

void SAL_CALL TitleBarUpdate::frameAction(const css::frame::FrameActionEvent& aEvent)
{
    // we are interested on events only, which must trigger a title bar update
    // because component was changed.
    if (
        (aEvent.Action == css::frame::FrameAction_COMPONENT_ATTACHED  ) ||
        (aEvent.Action == css::frame::FrameAction_COMPONENT_REATTACHED) ||
        (aEvent.Action == css::frame::FrameAction_COMPONENT_DETACHING )
       )
    {
        impl_forceUpdate ();
    }
}

void SAL_CALL TitleBarUpdate::titleChanged(const css::frame::TitleChangedEvent& /* aEvent */)
{
    impl_forceUpdate ();
}

void SAL_CALL TitleBarUpdate::disposing(const css::lang::EventObject&)
{
    css::uno::Reference< css::frame::XFrame > xFrame(m_xFrame.get(), css::uno::UNO_QUERY);
    if (xFrame.is())
        xFrame->removeFrameActionListener(this);

    // nothing todo here - because we hold the frame as weak reference only
}

//http://live.gnome.org/GnomeShell/ApplicationBased
//http://msdn.microsoft.com/en-us/library/dd378459(v=VS.85).aspx
void TitleBarUpdate::impl_updateApplicationID(const css::uno::Reference< css::frame::XFrame >& xFrame)
{
    css::uno::Reference< css::awt::XWindow > xWindow = xFrame->getContainerWindow ();
    if ( ! xWindow.is() )
        return;

#if !defined(MACOSX)
    OUString sApplicationID;
    try
    {
        css::uno::Reference< css::frame::XModuleManager2 > xModuleManager =
            css::frame::ModuleManager::create( m_xContext );

        OUString sDesktopName;
        OUString aModuleId = xModuleManager->identify(xFrame);
        if ( aModuleId.startsWith("com.sun.star.text.") || aModuleId.startsWith("com.sun.star.xforms.") )
            sDesktopName = "Writer";
        else if ( aModuleId.startsWith("com.sun.star.sheet.") )
            sDesktopName = "Calc";
        else if ( aModuleId.startsWith("com.sun.star.presentation.") )
            sDesktopName = "Impress";
        else if ( aModuleId.startsWith("com.sun.star.drawing." ) )
            sDesktopName = "Draw";
        else if ( aModuleId.startsWith("com.sun.star.formula." ) )
            sDesktopName = "Math";
        else if ( aModuleId.startsWith("com.sun.star.sdb.") )
            sDesktopName = "Base";
        else
            sDesktopName = "Startcenter";
#if defined(_WIN32)
        // We use a hardcoded product name matching the registry keys so applications can be associated with file types
        sApplicationID = "TheDocumentFoundation.LibreOffice." + sDesktopName;
#else
        sApplicationID = utl::ConfigManager::getProductName().toAsciiLowerCase() + "-" + sDesktopName.toAsciiLowerCase();
#endif
    }
    catch(const css::uno::Exception&)
    {
    }
#else
    OUString const sApplicationID;
#endif

    // VCL SYNCHRONIZED ->
    SolarMutexGuard aSolarGuard;

    VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xWindow );
    if ( pWindow && pWindow->GetType() == WindowType::WORKWINDOW )
    {
        WorkWindow* pWorkWindow = static_cast<WorkWindow*>(pWindow.get());
        pWorkWindow->SetApplicationID( sApplicationID );
    }
    // <- VCL SYNCHRONIZED
}

bool TitleBarUpdate::implst_getModuleInfo(const css::uno::Reference< css::frame::XFrame >& xFrame,
                                                TModuleInfo&                               rInfo )
{
    if ( ! xFrame.is ())
        return false;

    try
    {
        css::uno::Reference< css::frame::XModuleManager2 > xModuleManager =
            css::frame::ModuleManager::create( m_xContext );

        rInfo.sID = xModuleManager->identify(xFrame);
        ::comphelper::SequenceAsHashMap lProps    = xModuleManager->getByName (rInfo.sID);

        rInfo.nIcon   = lProps.getUnpackedValueOrDefault (OFFICEFACTORY_PROPNAME_ASCII_ICON  , INVALID_ICON_ID  );

        // Note: If we could retrieve a module id ... everything is OK.
        // UIName and Icon ID are optional values !
        bool bSuccess = !rInfo.sID.isEmpty();
        return bSuccess;
    }
    catch(const css::uno::Exception&)
        {}

    return false;
}

void TitleBarUpdate::impl_forceUpdate()
{
    css::uno::Reference< css::frame::XFrame > xFrame;
    {
        SolarMutexGuard g;
        xFrame.set(m_xFrame.get(), css::uno::UNO_QUERY);
    }

    // frame already gone ? We hold it weak only ...
    if ( ! xFrame.is())
        return;

    // no window -> no chance to set/update title and icon
    css::uno::Reference< css::awt::XWindow > xWindow = xFrame->getContainerWindow();
    if ( ! xWindow.is())
        return;

    impl_updateIcon  (xFrame);
    impl_updateTitle (xFrame);
#if !defined(MACOSX)
    impl_updateApplicationID (xFrame);
#endif
}

void TitleBarUpdate::impl_updateIcon(const css::uno::Reference< css::frame::XFrame >& xFrame)
{
    css::uno::Reference< css::frame::XController > xController = xFrame->getController      ();
    css::uno::Reference< css::awt::XWindow >       xWindow     = xFrame->getContainerWindow ();

    if (
        ( ! xController.is() ) ||
        ( ! xWindow.is()     )
       )
        return;

    // a) set default value to an invalid one. So we can start further searches for right icon id, if
    //    first steps failed!
    sal_Int32 nIcon = INVALID_ICON_ID;

    // b) try to find information on controller property set directly
    //    Don't forget to catch possible exceptions - because these property is an optional one!
    css::uno::Reference< css::beans::XPropertySet > xSet( xController, css::uno::UNO_QUERY );
    if ( xSet.is() )
    {
        try
        {
            css::uno::Reference< css::beans::XPropertySetInfo > const xPSI( xSet->getPropertySetInfo(), css::uno::UNO_SET_THROW );
            if ( xPSI->hasPropertyByName( u"IconId"_ustr ) )
                xSet->getPropertyValue( u"IconId"_ustr ) >>= nIcon;
        }
        catch(const css::uno::Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("fwk");
        }
    }

    // c) if b) failed ... identify the used module and retrieve set icon from module config.
    //    Tirck :-) Module was already specified outside and aInfo contains all needed information.
    if ( nIcon == INVALID_ICON_ID )
    {
        TModuleInfo aInfo;
        if (implst_getModuleInfo(xFrame, aInfo))
            nIcon = aInfo.nIcon;
    }

    // d) if all steps failed - use fallback :-)
    //    ... means using the global staroffice icon
    if( nIcon == INVALID_ICON_ID )
        nIcon = DEFAULT_ICON_ID;

    // e) set icon on container window now
    //    Don't forget SolarMutex! We use vcl directly :-(
    //    Check window pointer for right WorkWindow class too!!!

    // VCL SYNCHRONIZED ->
    SolarMutexGuard aSolarGuard;

    VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xWindow );
    if ( pWindow && ( pWindow->GetType() == WindowType::WORKWINDOW ) )
    {
        WorkWindow* pWorkWindow = static_cast<WorkWindow*>(pWindow.get());
        pWorkWindow->SetIcon( static_cast<sal_uInt16>(nIcon) );

        css::uno::Reference< css::frame::XModel > xModel = xController->getModel();
        OUString aURL;
        if( xModel.is() )
            aURL = xModel->getURL();
        pWorkWindow->SetRepresentedURL( aURL );
    }
    // <- VCL SYNCHRONIZED
}

// static
void TitleBarUpdate::impl_updateTitle(const css::uno::Reference< css::frame::XFrame >& xFrame)
{
    // no window ... no chance to set any title -> return
    css::uno::Reference< css::awt::XWindow > xWindow = xFrame->getContainerWindow ();
    if ( ! xWindow.is() )
        return;

    css::uno::Reference< css::frame::XTitle > xTitle(xFrame, css::uno::UNO_QUERY);
    if ( ! xTitle.is() )
        return;

    const OUString sTitle = xTitle->getTitle ();

    // VCL SYNCHRONIZED ->
    SolarMutexGuard aSolarGuard;

    VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xWindow );
    if ( pWindow && ( pWindow->GetType() == WindowType::WORKWINDOW ) )
    {
        WorkWindow* pWorkWindow = static_cast<WorkWindow*>(pWindow.get());
        pWorkWindow->SetText( sTitle );
    }
    // <- VCL SYNCHRONIZED
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

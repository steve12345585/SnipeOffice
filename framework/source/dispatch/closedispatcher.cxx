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

#include <dispatch/closedispatcher.hxx>
#include <pattern/frame.hxx>
#include <framework/framelistanalyzer.hxx>
#include <services.h>

#include <com/sun/star/bridge/BridgeFactory.hpp>
#include <com/sun/star/bridge/XBridgeFactory2.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/DispatchResultState.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/CommandGroup.hpp>
#include <com/sun/star/frame/StartModule.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/document/XActionLockable.hpp>
#include <com/sun/star/beans/XFastPropertySet.hpp>
#include <toolkit/helper/vclunohelper.hxx>

#include <osl/diagnose.h>
#include <utility>
#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vcl/syswin.hxx>
#include <unotools/moduleoptions.hxx>
#include <o3tl/string_view.hxx>

using namespace com::sun::star;

namespace framework{

#ifdef fpf
    #error "Who uses \"fpf\" as define. It will overwrite my namespace alias ..."
#endif
namespace fpf = ::framework::pattern::frame;

constexpr OUString URL_CLOSEDOC = u".uno:CloseDoc"_ustr;
constexpr OUString URL_CLOSEWIN = u".uno:CloseWin"_ustr;
const char URL_CLOSEFRAME[] = ".uno:CloseFrame";

CloseDispatcher::CloseDispatcher(css::uno::Reference< css::uno::XComponentContext >        xContext ,
                                 const css::uno::Reference< css::frame::XFrame >&          xFrame ,
                                 std::u16string_view                                       sTarget)
    : m_xContext(std::move(xContext))
    , m_aAsyncCallback(
        new vcl::EventPoster(LINK(this, CloseDispatcher, impl_asyncCallback)))
    , m_eOperation(E_CLOSE_DOC)
    , m_pSysWindow(nullptr)
{
    uno::Reference<frame::XFrame> xTarget = static_impl_searchRightTargetFrame(xFrame, sTarget);
    m_xCloseFrame = xTarget;

    // Try to retrieve the system window instance of the closing frame.
    uno::Reference<awt::XWindow> xWindow = xTarget->getContainerWindow();
    if (xWindow.is())
    {
        VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow(xWindow);
        if (pWindow->IsSystemWindow())
            m_pSysWindow = dynamic_cast<SystemWindow*>(pWindow.get());
    }
}

CloseDispatcher::~CloseDispatcher()
{
    SolarMutexGuard g;
    m_aAsyncCallback.reset();
    m_pSysWindow.reset();
}

void SAL_CALL CloseDispatcher::dispatch(const css::util::URL&                                  aURL      ,
                                        const css::uno::Sequence< css::beans::PropertyValue >& lArguments)
{
    dispatchWithNotification(aURL, lArguments, css::uno::Reference< css::frame::XDispatchResultListener >());
}

css::uno::Sequence< sal_Int16 > SAL_CALL CloseDispatcher::getSupportedCommandGroups()
{
    return  css::uno::Sequence< sal_Int16 >{css::frame::CommandGroup::VIEW, css::frame::CommandGroup::DOCUMENT};
}

css::uno::Sequence< css::frame::DispatchInformation > SAL_CALL CloseDispatcher::getConfigurableDispatchInformation(sal_Int16 nCommandGroup)
{
    if (nCommandGroup == css::frame::CommandGroup::VIEW)
    {
        /* Attention: Don't add .uno:CloseFrame here. Because it's not really
                      a configurable feature ... and further it does not have
                      a valid UIName entry inside the GenericCommands.xcu ... */
        css::uno::Sequence< css::frame::DispatchInformation > lViewInfos{
            { URL_CLOSEWIN, css::frame::CommandGroup::VIEW }
        };
        return lViewInfos;
    }
    else if (nCommandGroup == css::frame::CommandGroup::DOCUMENT)
    {
        css::uno::Sequence< css::frame::DispatchInformation > lDocInfos{
            { URL_CLOSEDOC, css::frame::CommandGroup::DOCUMENT }
        };
        return lDocInfos;
    }

    return css::uno::Sequence< css::frame::DispatchInformation >();
}

void SAL_CALL CloseDispatcher::addStatusListener(const css::uno::Reference< css::frame::XStatusListener >& /*xListener*/,
                                                 const css::util::URL&                                     /*aURL*/     )
{
}

void SAL_CALL CloseDispatcher::removeStatusListener(const css::uno::Reference< css::frame::XStatusListener >& /*xListener*/,
                                                    const css::util::URL&                                     /*aURL*/     )
{
}

void SAL_CALL CloseDispatcher::dispatchWithNotification(const css::util::URL&                                             aURL      ,
                                                        const css::uno::Sequence< css::beans::PropertyValue >&            lArguments,
                                                        const css::uno::Reference< css::frame::XDispatchResultListener >& xListener )
{
    // SAFE -> ----------------------------------
    SolarMutexClearableGuard aWriteLock;

    // This reference indicates, that we were already called before and
    // our asynchronous process was not finished yet.
    // We have to reject double calls. Otherwise we risk,
    // that we try to close an already closed resource...
    // And it is no problem to do nothing then. The UI user will try it again, if
    // non of these jobs was successful.
    if (m_xSelfHold.is())
    {
        aWriteLock.clear();
        // <- SAFE ------------------------------

        implts_notifyResultListener(
            xListener,
            css::frame::DispatchResultState::DONTKNOW,
            css::uno::Any());
        return;
    }

    // First we have to check, if this dispatcher is used right. Means if valid URLs are used.
    // If not - we have to break this operation. But an optional listener must be informed.
    // BTW: We save the information about the requested operation. Because
    // we need it later.
    if ( aURL.Complete == URL_CLOSEDOC )
        m_eOperation = E_CLOSE_DOC;
    else if ( aURL.Complete == URL_CLOSEWIN )
        m_eOperation = E_CLOSE_WIN;
    else if ( aURL.Complete == URL_CLOSEFRAME )
        m_eOperation = E_CLOSE_FRAME;
    else
    {
        aWriteLock.clear();
        // <- SAFE ------------------------------

        implts_notifyResultListener(
            xListener,
            css::frame::DispatchResultState::FAILURE,
            css::uno::Any());
        return;
    }

    if (m_pSysWindow && m_pSysWindow->GetCloseHdl().IsSet())
    {
        // The closing frame has its own close handler.  Call it instead.
        m_pSysWindow->GetCloseHdl().Call(*m_pSysWindow);

        aWriteLock.clear();
        // <- SAFE ------------------------------

        implts_notifyResultListener(
            xListener,
            css::frame::DispatchResultState::SUCCESS,
            css::uno::Any());

        return;
    }

    // OK - URLs are the right ones.
    // But we can't execute synchronously :-)
    // May we are called from a generic key-input handler,
    // which isn't aware that this call kill its own environment...
    // Do it asynchronous everytimes!

    // But don't forget to hold ourselves alive.
    // We are called back from an environment, which doesn't know a uno reference.
    // They call us back by using our c++ interface.

    m_xResultListener = xListener;
    m_xSelfHold.set(static_cast< ::cppu::OWeakObject* >(this), css::uno::UNO_QUERY);

    aWriteLock.clear();
    // <- SAFE ----------------------------------

    bool bIsSynchron = false;
    for (const css::beans::PropertyValue& rArg : lArguments )
    {
        if ( rArg.Name == "SynchronMode" )
        {
            rArg.Value >>= bIsSynchron;
            break;
        }
    }

    if ( bIsSynchron )
        impl_asyncCallback(nullptr);
    else
    {
        SolarMutexGuard g;
        m_aAsyncCallback->Post();
    }
}

/**
    @short      asynchronous callback
    @descr      We start all actions inside this object asynchronous
                (see comments there).
                Now we do the following:
                - close all views to the same document, if needed and possible
                - make the current frame empty
                  ! This step is necessary to handle errors during closing the
                    document inside the frame. May the document shows a dialog and
                    the user ignore it. Then the state of the office can be changed
                    during we try to close frame and document.
                - check the environment (means count open frames - excluding our
                  current one)
                - decide then, if we must close this frame only, establish the backing mode
                  or shutdown the whole application.
*/
IMPL_LINK_NOARG(CloseDispatcher, impl_asyncCallback, LinkParamNone*, void)
{
    try
    {

    // Allow calling of XController->suspend() everytimes.
    // Dispatch is an UI functionality. We implement such dispatch object here.
    // And further XController->suspend() was designed to bring an UI ...
    bool bControllerSuspended = false;

    bool bCloseAllViewsToo;
    EOperation                                                  eOperation;
    css::uno::Reference< css::uno::XComponentContext >          xContext;
    css::uno::Reference< css::frame::XFrame >                   xCloseFrame;
    css::uno::Reference< css::frame::XDispatchResultListener >  xListener;
    {
        SolarMutexGuard g;

        // Closing of all views, related to the same document, is allowed
        // only if the dispatched URL was ".uno:CloseDoc"!
        bCloseAllViewsToo = (m_eOperation == E_CLOSE_DOC);

        eOperation  = m_eOperation;
        xContext    = m_xContext;
        xCloseFrame.set(m_xCloseFrame.get(), css::uno::UNO_QUERY);
        xListener   = m_xResultListener;
    }

    // frame already dead ?!
    // Nothing to do !
    if (! xCloseFrame.is())
        return;

    bool bCloseFrame           = false;
    bool bEstablishBackingMode = false;
    bool bTerminateApp         = false;

    // Analyze the environment a first time.
    // If we found some special cases, we can
    // make some decisions earlier!
    css::uno::Reference< css::frame::XFramesSupplier > xDesktop( css::frame::Desktop::create(xContext), css::uno::UNO_QUERY_THROW);
    FrameListAnalyzer aCheck1(xDesktop, xCloseFrame, FrameAnalyzerFlags::Help | FrameAnalyzerFlags::BackingComponent);

    // Check for existing UNO connections.
    // NOTE: There is a race between checking this and connections being created/destroyed before
    //       we close the frame / terminate the app.
    css::uno::Reference<css::bridge::XBridgeFactory2> bridgeFac( css::bridge::BridgeFactory::create(xContext) );
    bool bHasActiveConnections = bridgeFac->getExistingBridges().hasElements();

    // a) If the current frame (where the close dispatch was requested for) does not have
    //    any parent frame ... it will close this frame only. Such frame isn't part of the
    //    global desktop tree ... and such frames are used as "implementation details" only.
    //    E.g. the live previews of our wizards doing such things. And then the owner of the frame
    //    is responsible for closing the application or accepting closing of the application
    //    by others.
    if ( ! xCloseFrame->getCreator().is())
        bCloseFrame = true;

    // b) The help window can't disagree with any request.
    //    Because it doesn't implement a controller - it uses a window only.
    //    Further it can't be the last open frame - if we do all other things
    //    right inside this CloseDispatcher implementation.
    //    => close it!
    else if (aCheck1.m_bReferenceIsHelp)
        bCloseFrame = true;

    // c) If we are already in "backing mode", we terminate the application, if no active UNO connections are found.
    //    If there is an active UNO connection, we only close the frame and leave the application alive.
    //    It doesn't matter, how many other frames (can be the help or hidden frames only) are open then.
    else if (aCheck1.m_bReferenceIsBacking) {
        if (bHasActiveConnections)
            bCloseFrame = true;
        else
            bTerminateApp = true;
    }

    // d) Otherwise we have to: close all views to the same document, close the
    //    document inside our own frame and decide then again, what has to be done!
    else
    {
        if (implts_prepareFrameForClosing(m_xCloseFrame, bCloseAllViewsToo, bControllerSuspended))
        {
            // OK; this frame is empty now.
            // Check the environment again to decide, what is the next step.
            FrameListAnalyzer aCheck2(xDesktop, xCloseFrame, FrameAnalyzerFlags::All);

            // c1) there is as minimum 1 frame open, which is visible and contains a document
            //     different from our one. And it's not the help!
            //     (tdf#30920 consider that closing a frame which is not the backing window (start center) while there is
            //      another frame that is the backing window open only closes the frame, and not terminate the app, so
            //      closing the license frame doesn't terminate the app if launched from the start center)
            //     => close our frame only - nothing else.
            if (!aCheck2.m_lOtherVisibleFrames.empty() || (!aCheck2.m_bReferenceIsBacking && aCheck2.m_xBackingComponent.is()))
                bCloseFrame = true;

            // c2) if we close the current view ... but not all other views
            //     to the same document, we must close the current frame only!
            //     Because implts_closeView() suspended this view only - does not
            //     close the frame.
            if (
                (!bCloseAllViewsToo                    ) &&
                (!aCheck2.m_lModelFrames.empty())
               )
                bCloseFrame = true;

            else
            // c3) there is no other (visible) frame open ...
            //     The help module will be ignored everytimes!
            //     But we have to decide if we must terminate the
            //     application or establish the backing mode now.
            //     And that depends from the dispatched URL ...
            {
                if (eOperation == E_CLOSE_FRAME)
                {
                    if (bHasActiveConnections)
                        bCloseFrame = true;
                    else
                        bTerminateApp = true;
                }
                else if( SvtModuleOptions().IsModuleInstalled(SvtModuleOptions::EModule::STARTMODULE) )
                    bEstablishBackingMode = true;
                else if (bHasActiveConnections)
                    bCloseFrame = true;
                else
                    bTerminateApp = true;
            }
        }
    }

    // Do it now ...
    bool bSuccess = false;
    if (bCloseFrame)
        bSuccess = implts_closeFrame();
    else if (bEstablishBackingMode)
    #if defined MACOSX
    {
        // on mac close down, quickstarter keeps the process alive
        // however if someone has shut down the quickstarter
        // behave as any other platform

        bool bQuickstarterRunning = false;
        // get quickstart service
        try
        {
            css::uno::Reference< css::beans::XFastPropertySet > xSet( xContext->getServiceManager()->createInstanceWithContext(IMPLEMENTATIONNAME_QUICKLAUNCHER, xContext), css::uno::UNO_QUERY_THROW );
            css::uno::Any aVal( xSet->getFastPropertyValue( 0 ) );
            bool bState = false;
            if( aVal >>= bState )
                bQuickstarterRunning = bState;
        }
        catch( const css::uno::Exception& )
        {
        }
        bSuccess = bQuickstarterRunning ? implts_terminateApplication() : implts_establishBackingMode();
    }
    #else
        bSuccess = implts_establishBackingMode();
    #endif
    else if (bTerminateApp)
        bSuccess = implts_terminateApplication();

    if ( ! bSuccess &&  bControllerSuspended )
    {
        css::uno::Reference< css::frame::XController > xController = xCloseFrame->getController();
        if (xController.is())
            xController->suspend(false);
    }

    // inform listener
    sal_Int16 nState = css::frame::DispatchResultState::FAILURE;
    if (bSuccess)
        nState = css::frame::DispatchResultState::SUCCESS;
    implts_notifyResultListener(xListener, nState, css::uno::Any());

    SolarMutexGuard g;
    // This method was called asynchronous from our main thread by using a pointer.
    // We reached this method only, by using a reference to ourself :-)
    // Further this member is used to detect still running and not yet finished
    // asynchronous operations. So it's time now to release this reference.
    // But hold it temp alive. Otherwise we die before we can finish this method really :-))
    css::uno::Reference< css::uno::XInterface > xTempHold = m_xSelfHold;
    m_xSelfHold.clear();
    m_xResultListener.clear();
    }
    catch(const css::lang::DisposedException&)
    {
    }
}

bool CloseDispatcher::implts_prepareFrameForClosing(const css::uno::Reference< css::frame::XFrame >& xFrame,
                                                    bool                                   bCloseAllOtherViewsToo,
                                                    bool&                                  bControllerSuspended  )
{
    // Frame already dead ... so this view is closed ... is closed ... is ... .-)
    if (! xFrame.is())
        return true;

    // Close all views to the same document ... if forced to do so.
    // But don't touch our own frame here!
    // We must do so ... because the may be following controller->suspend()
    // will show the "save/discard/cancel" dialog for the last view only!
    if (bCloseAllOtherViewsToo)
    {
        css::uno::Reference< css::uno::XComponentContext > xContext;
        {
            SolarMutexGuard g;
            xContext = m_xContext;
        }

        css::uno::Reference< css::frame::XFramesSupplier > xDesktop( css::frame::Desktop::create( xContext ), css::uno::UNO_QUERY_THROW);
        FrameListAnalyzer aCheck(xDesktop, xFrame, FrameAnalyzerFlags::All);

        size_t c = aCheck.m_lModelFrames.size();
        size_t i = 0;
        for (i=0; i<c; ++i)
        {
            if (!fpf::closeIt(aCheck.m_lModelFrames[i]))
                return false;
        }
    }

    // Inform user about modified documents or still running jobs (e.g. printing).
    {
        css::uno::Reference< css::frame::XController > xController = xFrame->getController();
        if (xController.is()) // some views don't uses a controller .-( (e.g. the help window)
        {
            bControllerSuspended = xController->suspend(true);
            if (! bControllerSuspended)
                return false;
        }
    }

    // don't remove the component really by e.g. calling setComponent(null, null).
    // It's enough to suspend the controller.
    // If we close the frame later this controller doesn't show the same dialog again.
    return true;
}

bool CloseDispatcher::implts_closeFrame()
{
    css::uno::Reference< css::frame::XFrame > xFrame;
    {
        SolarMutexGuard g;
        xFrame.set(m_xCloseFrame.get(), css::uno::UNO_QUERY);
    }

    // frame already dead ? => so it's closed ... it's closed ...
    if ( ! xFrame.is() )
        return true;

    // don't deliver ownership; our "UI user" will try it again if it failed.
    // OK - he will get an empty frame then. But normally an empty frame
    // should be closeable always :-)
    if (!fpf::closeIt(xFrame))
        return false;

    {
        SolarMutexGuard g;
        m_xCloseFrame.clear();
    }

    return true;
}

bool CloseDispatcher::implts_establishBackingMode()
{
    css::uno::Reference< css::uno::XComponentContext > xContext;
    css::uno::Reference< css::frame::XFrame >          xFrame;
    {
        SolarMutexGuard g;
        xContext  = m_xContext;
        xFrame.set(m_xCloseFrame.get(), css::uno::UNO_QUERY);
    }

    if (!xFrame.is())
        return false;

    css::uno::Reference < css::document::XActionLockable > xLock( xFrame, css::uno::UNO_QUERY );
    if ( xLock.is() && xLock->isActionLocked() )
        return false;

    css::uno::Reference< css::awt::XWindow > xContainerWindow = xFrame->getContainerWindow();

    css::uno::Reference< css::frame::XController > xStartModule = css::frame::StartModule::createWithParentWindow(
                        xContext, xContainerWindow);

    // Attention: You MUST(!) call setComponent() before you call attachFrame().
    css::uno::Reference< css::awt::XWindow > xBackingWin(xStartModule, css::uno::UNO_QUERY);
    xFrame->setComponent(xBackingWin, xStartModule);
    xStartModule->attachFrame(xFrame);
    xContainerWindow->setVisible(true);

    return true;
}

bool CloseDispatcher::implts_terminateApplication()
{
    css::uno::Reference< css::uno::XComponentContext > xContext;
    {
        SolarMutexGuard g;
        xContext = m_xContext;
    }

    css::uno::Reference< css::frame::XDesktop2 > xDesktop = css::frame::Desktop::create( xContext );

    return xDesktop->terminate();
}

void CloseDispatcher::implts_notifyResultListener(const css::uno::Reference< css::frame::XDispatchResultListener >& xListener,
                                                        sal_Int16                                                   nState   ,
                                                  const css::uno::Any&                                              aResult  )
{
    if (!xListener.is())
        return;

    css::frame::DispatchResultEvent aEvent(
        css::uno::Reference< css::uno::XInterface >(static_cast< ::cppu::OWeakObject* >(this), css::uno::UNO_QUERY),
        nState,
        aResult);

    xListener->dispatchFinished(aEvent);
}

css::uno::Reference< css::frame::XFrame > CloseDispatcher::static_impl_searchRightTargetFrame(const css::uno::Reference< css::frame::XFrame >& xFrame ,
                                                                                              std::u16string_view                           sTarget)
{
    if (o3tl::equalsIgnoreAsciiCase(sTarget, u"_self"))
        return xFrame;

    OSL_ENSURE(sTarget.empty(), "CloseDispatch used for unexpected target. Magic things will happen now .-)");

    css::uno::Reference< css::frame::XFrame > xTarget = xFrame;
    while(true)
    {
        // a) top frames will be closed
        if (xTarget->isTop())
            return xTarget;

        // b) even child frame containing top level windows (e.g. query designer of database) will be closed
        css::uno::Reference< css::awt::XWindow >    xWindow        = xTarget->getContainerWindow();
        css::uno::Reference< css::awt::XTopWindow > xTopWindowCheck(xWindow, css::uno::UNO_QUERY);
        if (xTopWindowCheck.is())
        {
            // b1) Note: Toolkit interface XTopWindow sometimes is used by real VCL-child-windows also .-)
            //     Be sure that these window is really a "top system window".
            //     Attention ! Checking Window->GetParent() isn't the right approach here.
            //     Because sometimes VCL create "implicit border windows" as parents even we created
            //     a simple XWindow using the toolkit only .-(
            SolarMutexGuard aSolarLock;
            VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xWindow );
            if ( pWindow && pWindow->IsSystemWindow() )
                return xTarget;
        }

        // c) try to find better results on parent frame
        //    If no parent frame exists (because this frame is used outside the desktop tree)
        //    the given frame must be used directly.
        css::uno::Reference< css::frame::XFrame > xParent = xTarget->getCreator();
        if ( ! xParent.is())
            return xTarget;

        // c1) check parent frame inside next loop ...
        xTarget = std::move(xParent);
    }
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

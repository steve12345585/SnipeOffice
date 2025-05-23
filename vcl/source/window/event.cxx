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

#include <vcl/event.hxx>
#include <vcl/window.hxx>
#include <vcl/dockwin.hxx>
#include <vcl/layout.hxx>
#include <sal/log.hxx>

#include <window.h>
#include <svdata.hxx>
#include <salframe.hxx>
#include <config_features.h>
#include <comphelper/scopeguard.hxx>

#include "impldockingwrapper.hxx"

namespace vcl {

void Window::DataChanged( const DataChangedEvent& )
{
}

void Window::NotifyAllChildren( DataChangedEvent& rDCEvt )
{
    CompatDataChanged( rDCEvt );

    vcl::Window* pChild = mpWindowImpl->mpFirstChild;
    while ( pChild )
    {
        pChild->NotifyAllChildren( rDCEvt );
        pChild = pChild->mpWindowImpl->mpNext;
    }
}

bool Window::PreNotify( NotifyEvent& rNEvt )
{
    bool bDone = false;
    if ( mpWindowImpl->mpParent && !ImplIsOverlapWindow() )
        bDone = mpWindowImpl->mpParent->CompatPreNotify( rNEvt );

    if ( !bDone )
    {
        if( rNEvt.GetType() == NotifyEventType::GETFOCUS )
        {
            bool bCompoundFocusChanged = false;
            if ( mpWindowImpl->mbCompoundControl && !mpWindowImpl->mbCompoundControlHasFocus && HasChildPathFocus() )
            {
                mpWindowImpl->mbCompoundControlHasFocus = true;
                bCompoundFocusChanged = true;
            }

            if ( bCompoundFocusChanged || ( rNEvt.GetWindow() == this ) )
                CallEventListeners( VclEventId::WindowGetFocus );
        }
        else if( rNEvt.GetType() == NotifyEventType::LOSEFOCUS )
        {
            bool bCompoundFocusChanged = false;
            if ( mpWindowImpl->mbCompoundControl && mpWindowImpl->mbCompoundControlHasFocus && !HasChildPathFocus() )
            {
                mpWindowImpl->mbCompoundControlHasFocus = false ;
                bCompoundFocusChanged = true;
            }

            if ( bCompoundFocusChanged || ( rNEvt.GetWindow() == this ) )
                CallEventListeners( VclEventId::WindowLoseFocus );
        }

        // #82968# mouse and key events will be notified after processing ( in ImplNotifyKeyMouseCommandEventListeners() )!
        //    see also ImplHandleMouseEvent(), ImplHandleKey()

    }

    return bDone;
}

namespace
{
    bool parentNotDialogControl(Window* pWindow)
    {
        vcl::Window* pParent = getNonLayoutParent(pWindow);
        if (!pParent)
            return true;
        return ((pParent->GetStyle() & (WB_DIALOGCONTROL | WB_NODIALOGCONTROL)) != WB_DIALOGCONTROL);
    }
}

bool Window::EventNotify( NotifyEvent& rNEvt )
{
    bool bRet = false;

    if (isDisposed())
        return false;

    // check for docking window
    // but do nothing if window is docked and locked
    ImplDockingWindowWrapper *pWrapper = ImplGetDockingManager()->GetDockingWindowWrapper( this );
    if ((GetStyle() & WB_DOCKABLE) &&
            pWrapper && ( pWrapper->IsFloatingMode() || !pWrapper->IsLocked() ))
    {
        const bool bDockingSupportCrippled = !StyleSettings::GetDockingFloatsSupported();

        if ( rNEvt.GetType() == NotifyEventType::MOUSEBUTTONDOWN )
        {
            const MouseEvent* pMEvt = rNEvt.GetMouseEvent();
            bool bHit = pWrapper->GetDragArea().Contains( pMEvt->GetPosPixel() );
            if ( pMEvt->IsLeft() )
            {
                if (!bDockingSupportCrippled && pMEvt->IsMod1() && (pMEvt->GetClicks() == 2))
                {
                    // ctrl double click toggles floating mode
                    pWrapper->SetFloatingMode( !pWrapper->IsFloatingMode() );
                    return true;
                }
                else if ( pMEvt->GetClicks() == 1 && bHit)
                {
                    // allow start docking during mouse move
                    pWrapper->ImplEnableStartDocking();
                    return true;
                }
            }
        }
        else if ( rNEvt.GetType() == NotifyEventType::MOUSEMOVE )
        {
            const MouseEvent* pMEvt = rNEvt.GetMouseEvent();
            bool bHit = pWrapper->GetDragArea().Contains( pMEvt->GetPosPixel() );
            if ( pMEvt->IsLeft() )
            {
                // check if a single click initiated this sequence ( ImplStartDockingEnabled() )
                // check if window is docked and
                if( pWrapper->ImplStartDockingEnabled() && !pWrapper->IsFloatingMode() &&
                    !pWrapper->IsDocking() && bHit )
                {
                    Point   aPos = pMEvt->GetPosPixel();
                    vcl::Window* pWindow = rNEvt.GetWindow();
                    if ( pWindow != this )
                    {
                        aPos = pWindow->OutputToScreenPixel( aPos );
                        aPos = ScreenToOutputPixel( aPos );
                    }
                    pWrapper->ImplStartDocking( aPos );
                }
                return true;
            }
        }
        else if( rNEvt.GetType() == NotifyEventType::KEYINPUT )
        {
            const vcl::KeyCode& rKey = rNEvt.GetKeyEvent()->GetKeyCode();
            if (rKey.GetCode() == KEY_F10 && rKey.GetModifier() &&
                rKey.IsShift() && rKey.IsMod1() && !bDockingSupportCrippled)
            {
                pWrapper->SetFloatingMode( !pWrapper->IsFloatingMode() );
                /* At this point the floating toolbar frame does not have the
                 * input focus since these frames don't get the focus per default
                 * To enable keyboard handling of this toolbar set the input focus
                 * to the frame. This needs to be done with ToTop since GrabFocus
                 * would not notice any change since "this" already has the focus.
                 */
                if( pWrapper->IsFloatingMode() )
                    ToTop( ToTopFlags::GrabFocusOnly );
                return true;
            }
        }
    }

    // manage the dialogs
    if ( (GetStyle() & (WB_DIALOGCONTROL | WB_NODIALOGCONTROL)) == WB_DIALOGCONTROL )
    {
        // if the parent also has dialog control activated, the parent takes over control
        if ( (rNEvt.GetType() == NotifyEventType::KEYINPUT) || (rNEvt.GetType() == NotifyEventType::KEYUP) )
        {
            // ScGridWindow has WB_DIALOGCONTROL set, so pressing tab in ScCheckListMenuControl won't
            // get processed here by the toplevel DockingWindow of ScCheckListMenuControl by
            // just checking if parentNotDialogControl is true
            bool bTopLevelFloatingWindow = (pWrapper && pWrapper->IsFloatingMode());
            if (ImplIsOverlapWindow() || parentNotDialogControl(this) || bTopLevelFloatingWindow)
            {
                bRet = ImplDlgCtrl( *rNEvt.GetKeyEvent(), rNEvt.GetType() == NotifyEventType::KEYINPUT );
            }
        }
        else if ( (rNEvt.GetType() == NotifyEventType::GETFOCUS) || (rNEvt.GetType() == NotifyEventType::LOSEFOCUS) )
        {
            ImplDlgCtrlFocusChanged( rNEvt.GetWindow(), rNEvt.GetType() == NotifyEventType::GETFOCUS );
            if ( (rNEvt.GetWindow() == this) && (rNEvt.GetType() == NotifyEventType::GETFOCUS) &&
                 !(GetStyle() & WB_TABSTOP) && !(mpWindowImpl->mnDlgCtrlFlags & DialogControlFlags::WantFocus) )
            {
                vcl::Window* pFirstChild = ImplGetDlgWindow( 0, GetDlgWindowType::First );
                if ( pFirstChild )
                    pFirstChild->ImplControlFocus();
            }
        }
    }

    if ( !bRet )
    {
        if ( mpWindowImpl->mpParent && !ImplIsOverlapWindow() )
            bRet = mpWindowImpl->mpParent->CompatNotify( rNEvt );
    }

    return bRet;
}

void Window::CallEventListeners( VclEventId nEvent, void* pData )
{
    VclWindowEvent aEvent( this, nEvent, pData );

    VclPtr<vcl::Window> xWindow = this;

    Application::ImplCallEventListeners( aEvent );

    // if we have ObjectDying, then the bIsDisposed flag has already been set,
    // but we still need to let listeners know.
    const bool bIgnoreDisposed = nEvent == VclEventId::ObjectDying;

    if ( !bIgnoreDisposed && xWindow->isDisposed() )
        return;

    // If maEventListeners is empty, the XVCLWindow has not yet been initialized.
    // Calling GetComponentInterface will do that.
    if (mpWindowImpl->maEventListeners.empty() && pData)
        xWindow->GetComponentInterface();

    if (!mpWindowImpl->maEventListeners.empty())
    {
        // Copy the list, because this can be destroyed when calling a Link...
        std::vector<Link<VclWindowEvent&,void>> aCopy( mpWindowImpl->maEventListeners );
        // we use an iterating counter/flag and a set of deleted Link's to avoid O(n^2) behaviour
        mpWindowImpl->mnEventListenersIteratingCount++;
        auto& rWindowImpl = *mpWindowImpl;
        comphelper::ScopeGuard aGuard(
            [&rWindowImpl, &xWindow, &bIgnoreDisposed]()
            {
                if (bIgnoreDisposed || !xWindow->isDisposed())
                {
                    rWindowImpl.mnEventListenersIteratingCount--;
                    if (rWindowImpl.mnEventListenersIteratingCount == 0)
                        rWindowImpl.maEventListenersDeleted.clear();
                }
            }
        );
        for ( const Link<VclWindowEvent&,void>& rLink : aCopy )
        {
            if (!bIgnoreDisposed && xWindow->isDisposed()) break;
            // check this hasn't been removed in some re-enterancy scenario fdo#47368
            if( rWindowImpl.maEventListenersDeleted.find(rLink) == rWindowImpl.maEventListenersDeleted.end() )
                rLink.Call( aEvent );
        }
    }

    while ( xWindow )
    {

        if ( !bIgnoreDisposed && xWindow->isDisposed() )
            return;

        if (!xWindow->mpWindowImpl)
            break;

        auto& rWindowImpl = *xWindow->mpWindowImpl;
        if (!rWindowImpl.maChildEventListeners.empty())
        {
            // Copy the list, because this can be destroyed when calling a Link...
            std::vector<Link<VclWindowEvent&,void>> aCopy( rWindowImpl.maChildEventListeners );
            // we use an iterating counter/flag and a set of deleted Link's to avoid O(n^2) behaviour
            rWindowImpl.mnChildEventListenersIteratingCount++;
            comphelper::ScopeGuard aGuard(
                [&rWindowImpl, &xWindow, &bIgnoreDisposed]()
                {
                    if (bIgnoreDisposed || !xWindow->isDisposed())
                    {
                        rWindowImpl.mnChildEventListenersIteratingCount--;
                        if (rWindowImpl.mnChildEventListenersIteratingCount == 0)
                            rWindowImpl.maChildEventListenersDeleted.clear();
                    }
                }
            );
            for ( const Link<VclWindowEvent&,void>& rLink : aCopy )
            {
                if (!bIgnoreDisposed && xWindow->isDisposed())
                    return;
                // Check this hasn't been removed in some re-enterancy scenario fdo#47368.
                if( rWindowImpl.maChildEventListenersDeleted.find(rLink) == rWindowImpl.maChildEventListenersDeleted.end() )
                    rLink.Call( aEvent );
            }
        }

        if ( !bIgnoreDisposed && xWindow->isDisposed() )
            return;

        xWindow = xWindow->GetParent();
    }
}

void Window::AddEventListener( const Link<VclWindowEvent&,void>& rEventListener )
{
    mpWindowImpl->maEventListeners.push_back( rEventListener );
}

void Window::RemoveEventListener( const Link<VclWindowEvent&,void>& rEventListener )
{
    if (mpWindowImpl)
    {
        auto& rListeners = mpWindowImpl->maEventListeners;
        std::erase(rListeners, rEventListener);
        if (mpWindowImpl->mnEventListenersIteratingCount)
            mpWindowImpl->maEventListenersDeleted.insert(rEventListener);
    }
}

void Window::AddChildEventListener( const Link<VclWindowEvent&,void>& rEventListener )
{
    mpWindowImpl->maChildEventListeners.push_back( rEventListener );
}

void Window::RemoveChildEventListener( const Link<VclWindowEvent&,void>& rEventListener )
{
    if (mpWindowImpl)
    {
        auto& rListeners = mpWindowImpl->maChildEventListeners;
        std::erase(rListeners, rEventListener);
        if (mpWindowImpl->mnChildEventListenersIteratingCount)
            mpWindowImpl->maChildEventListenersDeleted.insert(rEventListener);
    }
}

ImplSVEvent * Window::PostUserEvent( const Link<void*,void>& rLink, void* pCaller, bool bReferenceLink )
{
    std::unique_ptr<ImplSVEvent> pSVEvent(new ImplSVEvent);
    pSVEvent->mpData    = pCaller;
    pSVEvent->maLink    = rLink;
    pSVEvent->mpWindow  = this;
    pSVEvent->mbCall    = true;
    if (bReferenceLink)
    {
        pSVEvent->mpInstanceRef = static_cast<vcl::Window *>(rLink.GetInstance());
    }

    auto pTmpEvent = pSVEvent.get();
    if (!mpWindowImpl->mpFrame->PostEvent( std::move(pSVEvent) ))
        return nullptr;
    return pTmpEvent;
}

void Window::RemoveUserEvent( ImplSVEvent * nUserEvent )
{
    SAL_WARN_IF( nUserEvent->mpWindow.get() != this, "vcl",
                "Window::RemoveUserEvent(): Event doesn't send to this window or is already removed" );
    SAL_WARN_IF( !nUserEvent->mbCall, "vcl",
                "Window::RemoveUserEvent(): Event is already removed" );

    if ( nUserEvent->mpWindow )
    {
        nUserEvent->mpWindow = nullptr;
    }

    nUserEvent->mbCall = false;
}


static MouseEvent ImplTranslateMouseEvent( const MouseEvent& rE, vcl::Window const * pSource, vcl::Window const * pDest )
{
    // the mouse event occurred in a different window, we need to translate the coordinates of
    // the mouse cursor within that (source) window to the coordinates the mouse cursor would
    // be in the destination window
    Point aPos = pSource->OutputToScreenPixel( rE.GetPosPixel() );
    return MouseEvent( pDest->ScreenToOutputPixel( aPos ), rE.GetClicks(), rE.GetMode(), rE.GetButtons(), rE.GetModifier() );
}

void Window::ImplNotifyKeyMouseCommandEventListeners( NotifyEvent& rNEvt )
{
    if( rNEvt.GetType() == NotifyEventType::COMMAND )
    {
        const CommandEvent* pCEvt = rNEvt.GetCommandEvent();
        if ( pCEvt->GetCommand() != CommandEventId::ContextMenu )
            // non context menu events are not to be notified up the chain
            // so we return immediately
            return;

        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
        {
            // not interested: The event listeners are already called in ::Command,
            // and calling them here a second time doesn't make sense
            if ( rNEvt.GetWindow() != this )
            {
                CommandEvent aCommandEvent;

                if ( !pCEvt->IsMouseEvent() )
                {
                    aCommandEvent = *pCEvt;
                }
                else
                {
                    // the mouse event occurred in a different window, we need to translate the coordinates of
                    // the mouse cursor within that window to the coordinates the mouse cursor would be in the
                    // current window
                    vcl::Window* pSource = rNEvt.GetWindow();
                    Point aPos = pSource->OutputToScreenPixel( pCEvt->GetMousePosPixel() );
                    aCommandEvent = CommandEvent( ScreenToOutputPixel( aPos ), pCEvt->GetCommand(), pCEvt->IsMouseEvent(), pCEvt->GetEventData() );
                }

                CallEventListeners( VclEventId::WindowCommand, &aCommandEvent );
            }
        }
    }

    // #82968# notify event listeners for mouse and key events separately and
    // not in PreNotify ( as for focus listeners )
    // this allows for processing those events internally first and pass it to
    // the toolkit later

    VclPtr<vcl::Window> xWindow = this;

    if( rNEvt.GetType() == NotifyEventType::MOUSEMOVE )
    {
        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
        {
            if ( rNEvt.GetWindow() == this )
                CallEventListeners( VclEventId::WindowMouseMove, const_cast<MouseEvent *>(rNEvt.GetMouseEvent()) );
            else
            {
                MouseEvent aMouseEvent = ImplTranslateMouseEvent( *rNEvt.GetMouseEvent(), rNEvt.GetWindow(), this );
                CallEventListeners( VclEventId::WindowMouseMove, &aMouseEvent );
            }
        }
    }
    else if( rNEvt.GetType() == NotifyEventType::MOUSEBUTTONUP )
    {
        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
        {
            if ( rNEvt.GetWindow() == this )
                CallEventListeners( VclEventId::WindowMouseButtonUp, const_cast<MouseEvent *>(rNEvt.GetMouseEvent()) );
            else
            {
                MouseEvent aMouseEvent = ImplTranslateMouseEvent( *rNEvt.GetMouseEvent(), rNEvt.GetWindow(), this );
                CallEventListeners( VclEventId::WindowMouseButtonUp, &aMouseEvent );
            }
        }
    }
    else if( rNEvt.GetType() == NotifyEventType::MOUSEBUTTONDOWN )
    {
        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
        {
            if ( rNEvt.GetWindow() == this )
                CallEventListeners( VclEventId::WindowMouseButtonDown, const_cast<MouseEvent *>(rNEvt.GetMouseEvent()) );
            else
            {
                MouseEvent aMouseEvent = ImplTranslateMouseEvent( *rNEvt.GetMouseEvent(), rNEvt.GetWindow(), this );
                CallEventListeners( VclEventId::WindowMouseButtonDown, &aMouseEvent );
            }
        }
    }
    else if( rNEvt.GetType() == NotifyEventType::KEYINPUT )
    {
        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
            CallEventListeners( VclEventId::WindowKeyInput, const_cast<KeyEvent *>(rNEvt.GetKeyEvent()) );
    }
    else if( rNEvt.GetType() == NotifyEventType::KEYUP )
    {
        if ( mpWindowImpl->mbCompoundControl || ( rNEvt.GetWindow() == this ) )
            CallEventListeners( VclEventId::WindowKeyUp, const_cast<KeyEvent *>(rNEvt.GetKeyEvent()) );
    }

    if ( xWindow->isDisposed() )
        return;

    // #106721# check if we're part of a compound control and notify
    vcl::Window *pParent = ImplGetParent();
    while( pParent )
    {
        if( pParent->IsCompoundControl() )
        {
            pParent->ImplNotifyKeyMouseCommandEventListeners( rNEvt );
            break;
        }
        pParent = pParent->ImplGetParent();
    }
}

void Window::ImplCallInitShow()
{
    mpWindowImpl->mbReallyShown   = true;
    mpWindowImpl->mbInInitShow    = true;
    CompatStateChanged( StateChangedType::InitShow );
    mpWindowImpl->mbInInitShow    = false;

    vcl::Window* pWindow = mpWindowImpl->mpFirstOverlap;
    while ( pWindow )
    {
        if ( pWindow->mpWindowImpl->mbVisible )
            pWindow->ImplCallInitShow();
        pWindow = pWindow->mpWindowImpl->mpNext;
    }

    pWindow = mpWindowImpl->mpFirstChild;
    while ( pWindow )
    {
        if ( pWindow->mpWindowImpl->mbVisible )
            pWindow->ImplCallInitShow();
        pWindow = pWindow->mpWindowImpl->mpNext;
    }
}


void Window::ImplCallResize()
{
    mpWindowImpl->mbCallResize = false;

    // Normally we avoid blanking on re-size unless people might notice:
    if( GetBackground().IsGradient() )
        Invalidate();

    Resize();

    // #88419# Most classes don't call the base class in Resize() and Move(),
    // => Call ImpleResize/Move instead of Resize/Move directly...
    CallEventListeners( VclEventId::WindowResize );
}

void Window::ImplCallMove()
{
    mpWindowImpl->mbCallMove = false;

    if( mpWindowImpl->mbFrame )
    {
        // update frame position
        SalFrame *pParentFrame = nullptr;
        vcl::Window *pParent = ImplGetParent();
        while( pParent )
        {
            if( pParent->mpWindowImpl &&
                pParent->mpWindowImpl->mpFrame != mpWindowImpl->mpFrame )
            {
                pParentFrame = pParent->mpWindowImpl->mpFrame;
                break;
            }
            pParent = pParent->GetParent();
        }

        SalFrameGeometry g = mpWindowImpl->mpFrame->GetGeometry();
        mpWindowImpl->maPos = Point(g.x(), g.y());
        if( pParentFrame )
        {
            g = pParentFrame->GetGeometry();
            mpWindowImpl->maPos -= Point(g.x(), g.y());
        }
        // the client window and all its subclients have the same position as the borderframe
        // this is important for floating toolbars where the borderwindow is a floating window
        // which has another borderwindow (ie the system floating window)
        vcl::Window *pClientWin = mpWindowImpl->mpClientWindow;
        while( pClientWin )
        {
            pClientWin->mpWindowImpl->maPos = mpWindowImpl->maPos;
            pClientWin = pClientWin->mpWindowImpl->mpClientWindow;
        }
    }

    Move();

    CallEventListeners( VclEventId::WindowMove );
}

void Window::ImplCallFocusChangeActivate( vcl::Window* pNewOverlapWindow,
                                          vcl::Window* pOldOverlapWindow )
{
    ImplSVData* pSVData = ImplGetSVData();
    vcl::Window*     pNewRealWindow;
    vcl::Window*     pOldRealWindow;
    bool bCallActivate = true;
    bool bCallDeactivate = true;

    if (!pOldOverlapWindow)
    {
        return;
    }

    pOldRealWindow = pOldOverlapWindow->ImplGetWindow();
    if (!pNewOverlapWindow)
    {
        return;
    }

    pNewRealWindow = pNewOverlapWindow->ImplGetWindow();
    if ( (pOldRealWindow->GetType() != WindowType::FLOATINGWINDOW) ||
         pOldRealWindow->GetActivateMode() != ActivateModeFlags::NONE )
    {
        if ( (pNewRealWindow->GetType() == WindowType::FLOATINGWINDOW) &&
             pNewRealWindow->GetActivateMode() == ActivateModeFlags::NONE)
        {
            pSVData->mpWinData->mpLastDeacWin = pOldOverlapWindow;
            bCallDeactivate = false;
        }
    }
    else if ( (pNewRealWindow->GetType() != WindowType::FLOATINGWINDOW) ||
              pNewRealWindow->GetActivateMode() != ActivateModeFlags::NONE )
    {
        if (pSVData->mpWinData->mpLastDeacWin)
        {
            if (pSVData->mpWinData->mpLastDeacWin.get() == pNewOverlapWindow)
                bCallActivate = false;
            else
            {
                vcl::Window* pLastRealWindow = pSVData->mpWinData->mpLastDeacWin->ImplGetWindow();
                pSVData->mpWinData->mpLastDeacWin->mpWindowImpl->mbActive = false;
                pSVData->mpWinData->mpLastDeacWin->Deactivate();
                if (pLastRealWindow != pSVData->mpWinData->mpLastDeacWin.get())
                {
                    pLastRealWindow->mpWindowImpl->mbActive = true;
                    pLastRealWindow->Activate();
                }
            }
            pSVData->mpWinData->mpLastDeacWin = nullptr;
        }
    }

    if ( bCallDeactivate )
    {
        if( pOldOverlapWindow->mpWindowImpl->mbActive )
        {
            pOldOverlapWindow->mpWindowImpl->mbActive = false;
            pOldOverlapWindow->Deactivate();
        }
        if ( pOldRealWindow != pOldOverlapWindow )
        {
            if( pOldRealWindow->mpWindowImpl->mbActive )
            {
                pOldRealWindow->mpWindowImpl->mbActive = false;
                pOldRealWindow->Deactivate();
            }
        }
    }
    if ( !bCallActivate || pNewOverlapWindow->mpWindowImpl->mbActive )
        return;

    pNewOverlapWindow->mpWindowImpl->mbActive = true;
    pNewOverlapWindow->Activate();

    if ( pNewRealWindow != pNewOverlapWindow )
    {
        if( ! pNewRealWindow->mpWindowImpl->mbActive )
        {
            pNewRealWindow->mpWindowImpl->mbActive = true;
            pNewRealWindow->Activate();
        }
    }
}

} /* namespace vcl */


NotifyEvent::NotifyEvent( NotifyEventType nEventType, vcl::Window* pWindow,
                          const void* pEvent )
{
    mpWindow    = pWindow;
    mpData      = const_cast<void*>(pEvent);
    mnEventType  = nEventType;
}

NotifyEvent::~NotifyEvent() = default;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

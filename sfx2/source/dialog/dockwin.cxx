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

#include <svl/eitem.hxx>
#include <svl/solar.hrc>
#include <vcl/event.hxx>
#include <vcl/settings.hxx>

#include <vcl/svapp.hxx>
#include <vcl/timer.hxx>
#include <vcl/idle.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <osl/diagnose.h>
#include <toolkit/helper/vclunohelper.hxx>
#include <tools/debug.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <svl/itemset.hxx>

#include <sfx2/dockwin.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <workwin.hxx>
#include <splitwin.hxx>
#include <sfx2/viewsh.hxx>

#include <com/sun/star/beans/UnknownPropertyException.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/ModuleManager.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/ui/theWindowStateConfiguration.hpp>
#include <com/sun/star/ui/theWindowContentFactoryManager.hpp>

#define MAX_TOGGLEAREA_WIDTH        20
#define MAX_TOGGLEAREA_HEIGHT       20

using namespace ::com::sun::star;

// If you want to change the number you also have to:
// - Add new slot ids to sfxsids.hrc
// - Add new slots to frmslots.sdi
// - Add new slot definitions to sfx.sdi
const int NUM_OF_DOCKINGWINDOWS = 10;

namespace {

class SfxTitleDockingWindow : public SfxDockingWindow
{
    VclPtr<vcl::Window>   m_pWrappedWindow;

public:
                        SfxTitleDockingWindow(
                            SfxBindings* pBindings ,
                            SfxChildWindow* pChildWin ,
                            vcl::Window* pParent ,
                            WinBits nBits);
    virtual             ~SfxTitleDockingWindow() override;
    virtual void        dispose() override;

    vcl::Window*        GetWrappedWindow() const { return m_pWrappedWindow; }
    void                SetWrappedWindow(vcl::Window* const pWindow);

    virtual void        StateChanged( StateChangedType nType ) override;
    virtual void        Resize() override;
    virtual void        Resizing( Size& rSize ) override;
};

    struct WindowState
    {
        OUString sTitle;
    };
}

static bool lcl_getWindowState( const uno::Reference< container::XNameAccess >& xWindowStateMgr, const OUString& rResourceURL, WindowState& rWindowState )
{
    bool bResult = false;

    try
    {
        uno::Any a;
        uno::Sequence< beans::PropertyValue > aWindowState;
        a = xWindowStateMgr->getByName( rResourceURL );
        if ( a >>= aWindowState )
        {
            for (const auto& rProp : aWindowState)
            {
                if ( rProp.Name == "UIName" )
                {
                    rProp.Value >>= rWindowState.sTitle;
                }
            }
        }

        bResult = true;
    }
    catch ( container::NoSuchElementException& )
    {
        bResult = false;
    }

    return bResult;
}

SfxDockingWrapper::SfxDockingWrapper( vcl::Window* pParentWnd ,
                                      sal_uInt16 nId ,
                                      SfxBindings* pBindings ,
                                      SfxChildWinInfo* pInfo )
                    : SfxChildWindow( pParentWnd , nId )
{
    const uno::Reference< uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();

    VclPtr<SfxTitleDockingWindow> pTitleDockWindow = VclPtr<SfxTitleDockingWindow>::Create( pBindings, this, pParentWnd,
        WB_STDDOCKWIN | WB_CLIPCHILDREN | WB_SIZEABLE | WB_3DLOOK);
    SetWindow( pTitleDockWindow );

    // Use factory manager to retrieve XWindow factory. That can be used to instantiate
    // the real window factory.
    uno::Reference< lang::XSingleComponentFactory > xFactoryMgr = ui::theWindowContentFactoryManager::get(xContext);

    SfxDispatcher* pDispatcher = pBindings->GetDispatcher();
    uno::Reference< frame::XFrame > xFrame = pDispatcher->GetFrame()->GetFrame().GetFrameInterface();
    // create a resource URL from the nId provided by the sfx2
    OUString aResourceURL =  "private:resource/dockingwindow/" + OUString::number(nId);
    uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
    {
        {"Frame", uno::Any(xFrame)},
        {"ResourceURL", uno::Any(aResourceURL)},
    }));

    uno::Reference< awt::XWindow > xWindow;
    try
    {
        xWindow.set(
            xFactoryMgr->createInstanceWithArgumentsAndContext( aArgs, xContext ),
            uno::UNO_QUERY );

        static uno::WeakReference< frame::XModuleManager2 >  s_xModuleManager;

        uno::Reference< frame::XModuleManager2 > xModuleManager( s_xModuleManager );
        if ( !xModuleManager.is() )
        {
            xModuleManager = frame::ModuleManager::create(xContext);
            s_xModuleManager = xModuleManager;
        }

        static uno::WeakReference< container::XNameAccess > s_xWindowStateConfiguration;

        uno::Reference< container::XNameAccess > xWindowStateConfiguration( s_xWindowStateConfiguration );
        if ( !xWindowStateConfiguration.is() )
        {
            xWindowStateConfiguration = ui::theWindowStateConfiguration::get( xContext );
            s_xWindowStateConfiguration = xWindowStateConfiguration;
        }

        OUString sModuleIdentifier = xModuleManager->identify( xFrame );

        uno::Reference< container::XNameAccess > xModuleWindowState(
                                                    xWindowStateConfiguration->getByName( sModuleIdentifier ),
                                                    uno::UNO_QUERY );
        if ( xModuleWindowState.is() )
        {
            WindowState aDockWinState;
            if ( lcl_getWindowState( xModuleWindowState, aResourceURL, aDockWinState ))
                pTitleDockWindow->SetText( aDockWinState.sTitle );
        }
    }
    catch ( beans::UnknownPropertyException& )
    {
    }
    catch ( uno::RuntimeException& )
    {
    }
    catch ( uno::Exception& )
    {
    }

    VclPtr<vcl::Window> pContentWindow = VCLUnoHelper::GetWindow(xWindow);
    if ( pContentWindow )
        pContentWindow->SetStyle( pContentWindow->GetStyle() | WB_DIALOGCONTROL | WB_CHILDDLGCTRL );
    pTitleDockWindow->SetWrappedWindow(pContentWindow);

    GetWindow()->SetOutputSizePixel( Size( 270, 240 ) );

    static_cast<SfxDockingWindow*>( GetWindow() )->Initialize( pInfo );
    SetHideNotDelete( true );
}

std::unique_ptr<SfxChildWindow> SfxDockingWrapper::CreateImpl(vcl::Window *pParent, sal_uInt16 nId,
                                              SfxBindings *pBindings, SfxChildWinInfo* pInfo)
{
    return std::make_unique<SfxDockingWrapper>(pParent, nId, pBindings, pInfo);
}

void SfxDockingWrapper::RegisterChildWindow (bool bVis, SfxModule *pMod, SfxChildWindowFlags nFlags)
{
    // pre-register a couple of docking windows
    for (int i=0; i < NUM_OF_DOCKINGWINDOWS; i++ )
    {
        sal_uInt16 nID = sal_uInt16(SID_DOCKWIN_START+i);
        SfxChildWinFactory aFact( SfxDockingWrapper::CreateImpl, nID, 0xffff );
        aFact.aInfo.nFlags |= nFlags;
        aFact.aInfo.bVisible = bVis;
        SfxChildWindow::RegisterChildWindow(pMod, aFact);
    }
}

SfxChildWinInfo  SfxDockingWrapper::GetInfo() const
{
    SfxChildWinInfo aInfo = SfxChildWindow::GetInfo();
    static_cast<SfxDockingWindow*>(GetWindow())->FillInfo( aInfo );
    return aInfo;
};

SfxTitleDockingWindow::SfxTitleDockingWindow(SfxBindings* pBind, SfxChildWindow* pChildWin,
                                             vcl::Window* pParent, WinBits nBits)
    : SfxDockingWindow(pBind, pChildWin, pParent, nBits)
    , m_pWrappedWindow(nullptr)
{
}

SfxTitleDockingWindow::~SfxTitleDockingWindow()
{
    disposeOnce();
}

void SfxTitleDockingWindow::dispose()
{
    m_pWrappedWindow.disposeAndClear();
    SfxDockingWindow::dispose();
}

void SfxTitleDockingWindow::SetWrappedWindow( vcl::Window* const pWindow )
{
    m_pWrappedWindow = pWindow;
    if (m_pWrappedWindow)
    {
        m_pWrappedWindow->SetParent(this);
        m_pWrappedWindow->SetSizePixel( GetOutputSizePixel() );
        m_pWrappedWindow->Show();
    }
}

void SfxTitleDockingWindow::StateChanged( StateChangedType nType )
{
    if ( nType == StateChangedType::InitShow )
    {
        vcl::Window* pWindow = GetWrappedWindow();
        if ( pWindow )
        {
            pWindow->SetSizePixel( GetOutputSizePixel() );
            pWindow->Show();
        }
    }

    SfxDockingWindow::StateChanged(nType);
}

void SfxTitleDockingWindow::Resize()
{
    SfxDockingWindow::Resize();
    if (m_pWrappedWindow)
        m_pWrappedWindow->SetSizePixel( GetOutputSizePixel() );
}

void SfxTitleDockingWindow::Resizing( Size &rSize )
{
    SfxDockingWindow::Resizing( rSize );
    if (m_pWrappedWindow)
        m_pWrappedWindow->SetSizePixel( GetOutputSizePixel() );
}

static bool lcl_checkDockingWindowID( sal_uInt16 nID )
{
    return nID >= SID_DOCKWIN_START && nID < o3tl::make_unsigned(SID_DOCKWIN_START+NUM_OF_DOCKINGWINDOWS);
}

static SfxWorkWindow* lcl_getWorkWindowFromXFrame( const uno::Reference< frame::XFrame >& rFrame )
{
    // We need to find the corresponding SfxFrame of our XFrame
    SfxFrame* pFrame  = SfxFrame::GetFirst();
    SfxFrame* pXFrame = nullptr;
    while ( pFrame )
    {
        uno::Reference< frame::XFrame > xViewShellFrame( pFrame->GetFrameInterface() );
        if ( xViewShellFrame == rFrame )
        {
            pXFrame = pFrame;
            break;
        }
        else
            pFrame = SfxFrame::GetNext( *pFrame );
    }

    // If we have a SfxFrame we can retrieve the work window (Sfx layout manager for docking windows)
    if ( pXFrame )
        return pXFrame->GetWorkWindow_Impl();
    else
        return nullptr;
}

/** Factory function used by the framework layout manager to "create" a docking window with a special name.
    The string rDockingWindowName MUST BE a valid ID! The ID is pre-defined by a certain slot range located
    in sfxsids.hrc (currently SID_DOCKWIN_START = 9800).
*/
void SfxDockingWindowFactory( const uno::Reference< frame::XFrame >& rFrame, std::u16string_view rDockingWindowName )
{
    SolarMutexGuard aGuard;
    sal_uInt16 nID = sal_uInt16(o3tl::toInt32(rDockingWindowName));

    // Check the range of the provided ID otherwise nothing will happen
    if ( !lcl_checkDockingWindowID( nID ))
        return;

    SfxWorkWindow* pWorkWindow = lcl_getWorkWindowFromXFrame( rFrame );
    if ( pWorkWindow )
    {
        SfxChildWindow* pChildWindow = pWorkWindow->GetChildWindow_Impl(nID);
        if ( !pChildWindow )
        {
            // Register window at the workwindow child window list
            pWorkWindow->SetChildWindow_Impl( nID, true, false );
        }
    }
}

/** Function used by the framework layout manager to determine the visibility state of a docking window with
    a special name. The string rDockingWindowName MUST BE a valid ID! The ID is pre-defined by a certain slot
    range located in sfxsids.hrc (currently SID_DOCKWIN_START = 9800).
*/
bool IsDockingWindowVisible( const uno::Reference< frame::XFrame >& rFrame, std::u16string_view rDockingWindowName )
{
    SolarMutexGuard aGuard;

    sal_uInt16 nID = sal_uInt16(o3tl::toInt32(rDockingWindowName));

    // Check the range of the provided ID otherwise nothing will happen
    if ( lcl_checkDockingWindowID( nID ))
    {
        SfxWorkWindow* pWorkWindow = lcl_getWorkWindowFromXFrame( rFrame );
        if ( pWorkWindow )
        {
            SfxChildWindow* pChildWindow = pWorkWindow->GetChildWindow_Impl(nID);
            if ( pChildWindow )
                return true;
        }
    }

    return false;
}

class SfxDockingWindow_Impl
{
friend class SfxDockingWindow;

    SfxChildAlignment   eLastAlignment;
    SfxChildAlignment   eDockAlignment;
    bool                bConstructed;
    Size                aMinSize;
    VclPtr<SfxSplitWindow>  pSplitWin;
    Idle                aMoveIdle;

    // The following members are only valid in the time from startDocking to
    // EndDocking:
    Size                aSplitSize;
    tools::Long                nHorizontalSize;
    tools::Long                nVerticalSize;
    sal_uInt16          nLine;
    sal_uInt16          nPos;
    sal_uInt16          nDockLine;
    sal_uInt16          nDockPos;
    bool                bNewLine;
    bool                bDockingPrevented;
    OUString            aWinState;

    explicit            SfxDockingWindow_Impl(SfxDockingWindow *pBase);
    SfxChildAlignment   GetLastAlignment() const
                        { return eLastAlignment; }
    void                SetLastAlignment(SfxChildAlignment eAlign)
                        { eLastAlignment = eAlign; }
    SfxChildAlignment   GetDockAlignment() const
                        { return eDockAlignment; }
    void                SetDockAlignment(SfxChildAlignment eAlign)
                        { eDockAlignment = eAlign; }
};

SfxDockingWindow_Impl::SfxDockingWindow_Impl(SfxDockingWindow* pBase)
    :eLastAlignment(SfxChildAlignment::NOALIGNMENT)
    ,eDockAlignment(SfxChildAlignment::NOALIGNMENT)
    ,bConstructed(false)
    ,pSplitWin(nullptr)
    ,aMoveIdle( "sfx::SfxDockingWindow_Impl aMoveIdle" )
    ,nHorizontalSize(0)
    ,nVerticalSize(0)
    ,nLine(0)
    ,nPos(0)
    ,nDockLine(0)
    ,nDockPos(0)
    ,bNewLine(false)
    ,bDockingPrevented(false)
{
    aMoveIdle.SetPriority(TaskPriority::RESIZE);
    aMoveIdle.SetInvokeHandler(LINK(pBase,SfxDockingWindow,TimerHdl));
}

/*  [Description]

    This virtual method of the class FloatingWindow keeps track of changes in
    FloatingSize. If this method is overridden by a derived class,
    then the FloatingWindow: Resize() must also be called.
*/
void SfxDockingWindow::Resize()
{
    ResizableDockingWindow::Resize();
    Invalidate();
    if ( !pImpl || !pImpl->bConstructed || !pMgr )
        return;

    if ( IsFloatingMode() )
    {
        // start timer for saving window status information
        pImpl->aMoveIdle.Start();
    }
    else
    {
        Size aSize( GetSizePixel() );
        switch ( pImpl->GetDockAlignment() )
        {
            case SfxChildAlignment::LEFT:
            case SfxChildAlignment::FIRSTLEFT:
            case SfxChildAlignment::LASTLEFT:
            case SfxChildAlignment::RIGHT:
            case SfxChildAlignment::FIRSTRIGHT:
            case SfxChildAlignment::LASTRIGHT:
                pImpl->nHorizontalSize = aSize.Width();
                pImpl->aSplitSize = aSize;
                break;
            case SfxChildAlignment::TOP:
            case SfxChildAlignment::LOWESTTOP:
            case SfxChildAlignment::HIGHESTTOP:
            case SfxChildAlignment::BOTTOM:
            case SfxChildAlignment::HIGHESTBOTTOM:
            case SfxChildAlignment::LOWESTBOTTOM:
                pImpl->nVerticalSize = aSize.Height();
                pImpl->aSplitSize = aSize;
                break;
            default:
                break;
        }
    }
}

/*  [Description]

    This virtual method of the class DockingWindow makes it possible to
    intervene in the switching of the floating mode.
    If this method is overridden by a derived class,
    then the SfxDockingWindow::PrepareToggleFloatingMode() must be called
    afterwards, if not FALSE is returned.
*/
bool SfxDockingWindow::PrepareToggleFloatingMode()
{
    if (!pImpl || !pImpl->bConstructed)
        return true;

    if ( (Application::IsInModalMode() && IsFloatingMode()) || !pMgr )
        return false;

    if ( pImpl->bDockingPrevented )
        return false;

    if (!IsFloatingMode())
    {
        // Test, if FloatingMode is permitted.
        if ( CheckAlignment(GetAlignment(),SfxChildAlignment::NOALIGNMENT) != SfxChildAlignment::NOALIGNMENT )
            return false;

        if ( pImpl->pSplitWin )
        {
            // The DockingWindow is inside a SplitWindow and will be teared of.
            pImpl->pSplitWin->RemoveWindow(this/*, sal_False*/);
            pImpl->pSplitWin = nullptr;
        }
    }
    else if ( pMgr )
    {
        pImpl->aWinState = GetFloatingWindow()->GetWindowState();

        // Test if it is allowed to dock,
        if (CheckAlignment(GetAlignment(),pImpl->GetLastAlignment()) == SfxChildAlignment::NOALIGNMENT)
            return false;

        // Test, if the Workwindow allows for docking at the moment.
        SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
        if ( !pWorkWin->IsDockingAllowed() || !pWorkWin->IsInternalDockingAllowed() )
            return false;
    }

    return true;
}

/*  [Description]

    This virtual method of the DockingWindow class sets the internal data of
    the SfxDockingWindow and ensures the correct alignment on the parent window.
    Through PrepareToggleFloatMode and Initialize it is ensured that
    pImpl-> GetLastAlignment() always delivers an allowed alignment. If this
    method is overridden by a derived class, then first the
    SfxDockingWindow::ToggleFloatingMode() must be called.
*/
void SfxDockingWindow::ToggleFloatingMode()
{
    if ( !pImpl || !pImpl->bConstructed || !pMgr )
        return;                                 // No Handler call

    // Remember old alignment and then switch.
    // SV has already switched, but the alignment SfxDockingWindow is still
    // the old one. What I was before?
    SfxChildAlignment eLastAlign = GetAlignment();

    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();

    if (IsFloatingMode())
    {
        SetAlignment(SfxChildAlignment::NOALIGNMENT);
        if ( !pImpl->aWinState.isEmpty() )
            GetFloatingWindow()->SetWindowState( pImpl->aWinState );
        else
            GetFloatingWindow()->SetOutputSizePixel( GetFloatingSize() );
    }
    else
    {
        if (pImpl->GetDockAlignment() == eLastAlign)
        {
            // If ToggleFloatingMode was called, but the DockAlignment still
            // is unchanged, then this means that it must have been a toggling
            // through DClick, so use last alignment
            SetAlignment (pImpl->GetLastAlignment());
        }
        else
        {

            // Toggling was triggered by dragging
            pImpl->nLine = pImpl->nDockLine;
            pImpl->nPos = pImpl->nDockPos;
            SetAlignment (pImpl->GetDockAlignment());
        }

        // The DockingWindow is now in a SplitWindow
        pImpl->pSplitWin = pWorkWin->GetSplitWindow_Impl(GetAlignment());

        // The LastAlignment is still the last docked
        SfxSplitWindow *pSplit = pWorkWin->GetSplitWindow_Impl(pImpl->GetLastAlignment());

        DBG_ASSERT( pSplit, "LastAlignment is not correct!" );
        if ( pSplit && pSplit != pImpl->pSplitWin )
            pSplit->ReleaseWindow_Impl(this);
        if ( pImpl->GetDockAlignment() == eLastAlign )
            pImpl->pSplitWin->InsertWindow( this, pImpl->aSplitSize );
        else
            pImpl->pSplitWin->InsertWindow( this, pImpl->aSplitSize, pImpl->nLine, pImpl->nPos, pImpl->bNewLine );
        if ( !pImpl->pSplitWin->IsFadeIn() )
            pImpl->pSplitWin->FadeIn();
    }

    // Keep the old alignment for the next toggle; set it only now due to the
    // unregister SplitWindow!
    pImpl->SetLastAlignment(eLastAlign);

    // Reset DockAlignment, if EndDocking is still called
    pImpl->SetDockAlignment(GetAlignment());

    // Dock or undock SfxChildWindow correctly.
    pWorkWin->ConfigChild_Impl( SfxChildIdentifier::SPLITWINDOW, SfxDockingConfig::TOGGLEFLOATMODE, pMgr->GetType() );
}

/*  [Description]

    This virtual method of the DockingWindow class takes the inner and outer
    docking rectangle from the parent window. If this method is overridden by
    a derived class, then SfxDockingWindow:StartDocking() has to be called at
    the end.
*/
void SfxDockingWindow::StartDocking()
{
    if ( !pImpl || !pImpl->bConstructed || !pMgr )
        return;
    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
    pWorkWin->ConfigChild_Impl( SfxChildIdentifier::SPLITWINDOW, SfxDockingConfig::SETDOCKINGRECTS, pMgr->GetType() );
    pImpl->SetDockAlignment(GetAlignment());

    if ( pImpl->pSplitWin )
    {
        // Get the current docking data
        pImpl->pSplitWin->GetWindowPos(this, pImpl->nLine, pImpl->nPos);
        pImpl->nDockLine = pImpl->nLine;
        pImpl->nDockPos = pImpl->nPos;
        pImpl->bNewLine = false;
    }
}

/*  [Description]

    This virtual method of the DockingWindow class calculates the current
    tracking rectangle. For this purpose the method CalcAlignment(RPOs, rRect)
    is used, the behavior can be influenced by the derived classes (see below).
    This method should if possible not be overwritten.
*/
bool SfxDockingWindow::Docking( const Point& rPos, tools::Rectangle& rRect )
{
    if ( Application::IsInModalMode() )
        return true;

    if ( !pImpl || !pImpl->bConstructed || !pMgr )
    {
        rRect.SetSize( Size() );
        return IsFloatingMode();
    }

    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
    if ( pImpl->bDockingPrevented || !pWorkWin->IsInternalDockingAllowed() )
        return false;

    bool bFloatMode = false;

    if ( GetOuterRect().Contains( rPos ) )
    {
        // Mouse within OuterRect: calculate Alignment and Rectangle
        SfxChildAlignment eAlign = CalcAlignment(rPos, rRect);
        if (eAlign == SfxChildAlignment::NOALIGNMENT)
            bFloatMode = true;
        pImpl->SetDockAlignment(eAlign);
    }
    else
    {
        // Mouse is not within OuterRect: must be FloatingWindow
        // Is this allowed?
        if (CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::NOALIGNMENT) != SfxChildAlignment::NOALIGNMENT)
            return false;
        bFloatMode = true;
        if ( SfxChildAlignment::NOALIGNMENT != pImpl->GetDockAlignment() )
        {
            // Due to a bug the rRect may only be changed when the
            // alignment is changed!
            pImpl->SetDockAlignment(SfxChildAlignment::NOALIGNMENT);
            rRect.SetSize(CalcDockingSize(SfxChildAlignment::NOALIGNMENT));
        }
    }

    return bFloatMode;
}

/** Virtual method of the DockingWindow class ensures the correct alignment on
    the parent window. If this method is overridden by a derived class, then
    SfxDockingWindow::EndDocking() must be called first.
*/
void SfxDockingWindow::EndDocking( const tools::Rectangle& rRect, bool bFloatMode )
{
    if ( !pImpl || !pImpl->bConstructed || IsDockingCanceled() || !pMgr )
        return;

    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();

    // If the alignment changes and the window is in a docked state in a
    // SplitWindow, then it must be re-registered. If it is docked again,
    // PrepareToggleFloatingMode() and ToggleFloatingMode() perform the
    // re-registered
    bool bReArrange = !bFloatMode;

    if ( bReArrange )
    {
        if ( GetAlignment() != pImpl->GetDockAlignment() )
        {
            // before Show() is called must the reassignment have been made,
            // therefore the base class can not be called
            if ( IsFloatingMode() )
                Show( false, ShowFlags::NoFocusChange );

            // Set the size for toggling.
            pImpl->aSplitSize = rRect.GetSize();
            if ( IsFloatingMode() )
            {
                SetFloatingMode( bFloatMode );
                if ( IsFloatingMode() )
                    Show( true, ShowFlags::NoFocusChange );
            }
            else
            {
                pImpl->pSplitWin->RemoveWindow(this,false);
                pImpl->nLine = pImpl->nDockLine;
                pImpl->nPos = pImpl->nDockPos;
                pImpl->pSplitWin->ReleaseWindow_Impl(this);
                pImpl->pSplitWin = pWorkWin->GetSplitWindow_Impl(pImpl->GetDockAlignment());
                pImpl->pSplitWin->InsertWindow( this, pImpl->aSplitSize, pImpl->nDockLine, pImpl->nDockPos, pImpl->bNewLine );
                if ( !pImpl->pSplitWin->IsFadeIn() )
                    pImpl->pSplitWin->FadeIn();
            }
        }
        else if ( pImpl->nLine != pImpl->nDockLine || pImpl->nPos != pImpl->nDockPos || pImpl->bNewLine )
        {
            // Moved within Splitwindows
            if ( pImpl->nLine != pImpl->nDockLine )
                pImpl->aSplitSize = rRect.GetSize();
            pImpl->pSplitWin->MoveWindow( this, pImpl->aSplitSize, pImpl->nDockLine, pImpl->nDockPos, pImpl->bNewLine );
        }
    }
    else
    {
        ResizableDockingWindow::EndDocking(rRect, bFloatMode);
    }

    SetAlignment( IsFloatingMode() ? SfxChildAlignment::NOALIGNMENT : pImpl->GetDockAlignment() );
}

/*  [Description]

    Virtual method of the DockingWindow class. Here, the interactive resize in
    FloatingMode can be influenced, for example by only allowing for discrete
    values for width and / or height. The base implementation prevents that the
    output size is smaller than one set with SetMinOutputSizePixel().
*/
void SfxDockingWindow::Resizing( Size& /*rSize*/ )
{

}

/*  [Description]

    Constructor for the SfxDockingWindow class. A SfxChildWindow will be
    required because the docking is implemented in Sfx through SfxChildWindows.
*/
SfxDockingWindow::SfxDockingWindow( SfxBindings *pBindinx, SfxChildWindow *pCW,
    vcl::Window* pParent, WinBits nWinBits)
    : ResizableDockingWindow(pParent, nWinBits)
    , pBindings(pBindinx)
    , pMgr(pCW)
{
    pImpl.reset(new SfxDockingWindow_Impl(this));
}

/** Constructor for the SfxDockingWindow class. A SfxChildWindow will be
    required because the docking is implemented in Sfx through SfxChildWindows.
*/
SfxDockingWindow::SfxDockingWindow( SfxBindings *pBindinx, SfxChildWindow *pCW,
    vcl::Window* pParent, const OUString& rID, const OUString& rUIXMLDescription)
    : ResizableDockingWindow(pParent)
    , pBindings(pBindinx)
    , pMgr(pCW)
{
    m_xBuilder = Application::CreateInterimBuilder(m_xBox, rUIXMLDescription, true);
    m_xContainer = m_xBuilder->weld_box(rID);

    pImpl.reset(new SfxDockingWindow_Impl(this));
}

/** Initialization of the SfxDockingDialog class via a SfxChildWinInfo.
    The initialization is done only in a 2nd step after the constructor, this
    constructor should be called from the derived class or from the
    SfxChildWindows.
*/
void SfxDockingWindow::Initialize(SfxChildWinInfo *pInfo)
{
    if ( !pMgr )
    {
        pImpl->SetDockAlignment( SfxChildAlignment::NOALIGNMENT );
        pImpl->bConstructed = true;
        return;
    }

    if (pInfo && (pInfo->nFlags & SfxChildWindowFlags::FORCEDOCK))
        pImpl->bDockingPrevented = true;

    pImpl->aSplitSize = GetOutputSizePixel();
    if ( !GetFloatingSize().Width() )
    {
        Size aMinSize( GetMinOutputSizePixel() );
        SetFloatingSize( pImpl->aSplitSize );
        if ( pImpl->aSplitSize.Width() < aMinSize.Width() )
            pImpl->aSplitSize.setWidth( aMinSize.Width() );
        if ( pImpl->aSplitSize.Height() < aMinSize.Height() )
            pImpl->aSplitSize.setHeight( aMinSize.Height() );
    }

    bool bVertHorzRead( false );
    if (pInfo && !pInfo->aExtraString.isEmpty())
    {
        // get information about alignment, split size and position in SplitWindow
        OUString aStr;
        sal_Int32 nPos = pInfo->aExtraString.indexOf("AL:");
        if ( nPos != -1 )
        {
            // alignment information
            sal_Int32 n1 = pInfo->aExtraString.indexOf('(', nPos);
            if ( n1 != -1 )
            {
                sal_Int32 n2 = pInfo->aExtraString.indexOf(')', n1);
                if ( n2 != -1 )
                {
                    // extract alignment information from extrastring
                    aStr = pInfo->aExtraString.copy(nPos, n2 - nPos + 1);
                    pInfo->aExtraString = pInfo->aExtraString.replaceAt(nPos, n2 - nPos + 1, u"");
                    aStr = aStr.replaceAt(nPos, n1-nPos+1, u"");
                }
            }
        }

        if ( !aStr.isEmpty() )
        {
            // accept window state only if alignment is also set
            pImpl->aWinState = pInfo->aWinState;

            // check for valid alignment
            SfxChildAlignment eLocalAlignment = static_cast<SfxChildAlignment>(static_cast<sal_uInt16>(aStr.toInt32()));
            bool bIgnoreFloatConfig = (eLocalAlignment == SfxChildAlignment::NOALIGNMENT &&
                                       !StyleSettings::GetDockingFloatsSupported());
            if (pImpl->bDockingPrevented || bIgnoreFloatConfig)
                // docking prevented, ignore old configuration and take alignment from default
                aStr.clear();
            else
                SetAlignment( eLocalAlignment );

            SfxChildAlignment eAlign = CheckAlignment(GetAlignment(),GetAlignment());
            if ( eAlign != GetAlignment() )
            {
                OSL_FAIL("Invalid Alignment!");
                SetAlignment( eAlign );
                aStr.clear();
            }

            // get last alignment (for toggling)
            nPos = aStr.indexOf(',');
            if ( nPos != -1 )
            {
                aStr = aStr.copy(nPos+1);
                pImpl->SetLastAlignment( static_cast<SfxChildAlignment>(static_cast<sal_uInt16>(aStr.toInt32())) );
            }

            nPos = aStr.indexOf(',');
            if ( nPos != -1 )
            {
                // get split size and position in SplitWindow
                Point aPos;
                aStr = aStr.copy(nPos+1);
                if ( GetPosSizeFromString( aStr, aPos, pImpl->aSplitSize ) )
                {
                    pImpl->nLine = pImpl->nDockLine = static_cast<sal_uInt16>(aPos.X());
                    pImpl->nPos  = pImpl->nDockPos  = static_cast<sal_uInt16>(aPos.Y());
                    pImpl->nVerticalSize = pImpl->aSplitSize.Height();
                    pImpl->nHorizontalSize = pImpl->aSplitSize.Width();
                    if ( GetSplitSizeFromString( aStr, pImpl->aSplitSize ))
                        bVertHorzRead = true;
                }
            }
        }
        else {
            OSL_FAIL( "Information is missing!" );
        }
    }

    if ( !bVertHorzRead )
    {
        pImpl->nVerticalSize = pImpl->aSplitSize.Height();
        pImpl->nHorizontalSize = pImpl->aSplitSize.Width();
    }

    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
    if ( GetAlignment() != SfxChildAlignment::NOALIGNMENT )
    {
        // check if SfxWorkWindow is able to allow docking at its border
        if (
            !pWorkWin->IsDockingAllowed() ||
            !pWorkWin->IsInternalDockingAllowed() ||
            ( (GetFloatStyle() & WB_STANDALONE) && Application::IsInModalMode()) )
        {
            SetAlignment( SfxChildAlignment::NOALIGNMENT );
        }
    }

    // detect floating mode
    // toggling mode will not execute code in handlers, because pImpl->bConstructed is not set yet
    bool bFloatMode = IsFloatingMode();
    if ( bFloatMode != (GetAlignment() == SfxChildAlignment::NOALIGNMENT) )
    {
        bFloatMode = !bFloatMode;
        SetFloatingMode( bFloatMode );
        if ( bFloatMode )
        {
            if ( !pImpl->aWinState.isEmpty() )
                GetFloatingWindow()->SetWindowState( pImpl->aWinState );
            else
                GetFloatingWindow()->SetOutputSizePixel( GetFloatingSize() );
        }
    }

    if ( IsFloatingMode() )
    {
        // validate last alignment
        SfxChildAlignment eLastAlign = pImpl->GetLastAlignment();
        if ( eLastAlign == SfxChildAlignment::NOALIGNMENT)
            eLastAlign = CheckAlignment(eLastAlign, SfxChildAlignment::LEFT);
        if ( eLastAlign == SfxChildAlignment::NOALIGNMENT)
            eLastAlign = CheckAlignment(eLastAlign, SfxChildAlignment::RIGHT);
        if ( eLastAlign == SfxChildAlignment::NOALIGNMENT)
            eLastAlign = CheckAlignment(eLastAlign, SfxChildAlignment::TOP);
        if ( eLastAlign == SfxChildAlignment::NOALIGNMENT)
            eLastAlign = CheckAlignment(eLastAlign, SfxChildAlignment::BOTTOM);
        pImpl->SetLastAlignment(eLastAlign);
    }
    else
    {
        // docked window must have NOALIGNMENT as last alignment
        pImpl->SetLastAlignment(SfxChildAlignment::NOALIGNMENT);

        pImpl->pSplitWin = pWorkWin->GetSplitWindow_Impl(GetAlignment());
        pImpl->pSplitWin->InsertWindow(this, pImpl->aSplitSize);
    }

    // save alignment
    pImpl->SetDockAlignment( GetAlignment() );
}

void SfxDockingWindow::Initialize_Impl()
{
    if ( !pMgr )
    {
        pImpl->bConstructed = true;
        return;
    }

    SystemWindow* pFloatWin = GetFloatingWindow();
    bool bSet = false;
    if ( pFloatWin )
    {
        bSet = !pFloatWin->IsDefaultPos();
    }
    else
    {
        Point aPos = GetFloatingPos();
        if ( aPos != Point() )
            bSet = true;
    }

    if ( !bSet)
    {
        SfxViewFrame *pFrame = pBindings->GetDispatcher_Impl()->GetFrame();
        vcl::Window* pEditWin = pFrame->GetViewShell()->GetWindow();
        Point aPos = pEditWin->OutputToScreenPixel( pEditWin->GetPosPixel() );
        aPos = GetParent()->ScreenToOutputPixel( aPos );
        SetFloatingPos( aPos );
    }

    if ( pFloatWin )
    {
        // initialize floating window
        if ( pImpl->aWinState.isEmpty() )
            // window state never set before, get if from defaults
            pImpl->aWinState = pFloatWin->GetWindowState();

        // trick: use VCL method SetWindowState to adjust position and size
        pFloatWin->SetWindowState( pImpl->aWinState );
        Size aSize(pFloatWin->GetSizePixel());

        // remember floating size for calculating alignment and tracking rectangle
        SetFloatingSize(aSize);

    }

    // allow calling of docking handlers
    pImpl->bConstructed = true;
}

/** Fills a SfxChildWinInfo with specific data from SfxDockingWindow,
    so that it can be written in the INI file. It is assumed that rinfo
    receives all other possible relevant data in the ChildWindow class.
    Insertions are marked with size and the ZoomIn flag.
    If this method is overridden, the base implementation must be called first.
*/
void SfxDockingWindow::FillInfo(SfxChildWinInfo& rInfo) const
{
    if (!pMgr || !pImpl)
        return;

    if (GetFloatingWindow() && pImpl->bConstructed)
        pImpl->aWinState = GetFloatingWindow()->GetWindowState();

    rInfo.aWinState = pImpl->aWinState;
    rInfo.aExtraString = "AL:(";
    rInfo.aExtraString += OUString::number(static_cast<sal_uInt16>(GetAlignment()));
    rInfo.aExtraString += ",";
    rInfo.aExtraString += OUString::number (static_cast<sal_uInt16>(pImpl->GetLastAlignment()));

    Point aPos(pImpl->nLine, pImpl->nPos);
    rInfo.aExtraString += ",";
    rInfo.aExtraString += OUString::number( aPos.X() );
    rInfo.aExtraString += "/";
    rInfo.aExtraString += OUString::number( aPos.Y() );
    rInfo.aExtraString += "/";
    rInfo.aExtraString += OUString::number( pImpl->nHorizontalSize );
    rInfo.aExtraString += "/";
    rInfo.aExtraString += OUString::number( pImpl->nVerticalSize );
    rInfo.aExtraString += ",";
    rInfo.aExtraString += OUString::number( pImpl->aSplitSize.Width() );
    rInfo.aExtraString += ";";
    rInfo.aExtraString += OUString::number( pImpl->aSplitSize.Height() );

    rInfo.aExtraString += ")";
}

SfxDockingWindow::~SfxDockingWindow()
{
    disposeOnce();
}

void SfxDockingWindow::dispose()
{
    ReleaseChildWindow_Impl();
    pImpl.reset();
    m_xContainer.reset();
    m_xBuilder.reset();
    ResizableDockingWindow::dispose();
}

void SfxDockingWindow::ReleaseChildWindow_Impl()
{
    if ( pMgr && pMgr->GetFrame() == pBindings->GetActiveFrame() )
        pBindings->SetActiveFrame( nullptr );

    if ( pMgr && pImpl->pSplitWin && pImpl->pSplitWin->IsItemValid( GetType() ) )
        pImpl->pSplitWin->RemoveWindow(this);

    pMgr=nullptr;
}

/** This method calculates a resulting alignment for the given mouse position
    and tracking rectangle. When changing the alignment it can also be that
    the tracking rectangle is changed, so that an altered rectangle is
    returned. The user of this class can influence behaviour of this method,
    and thus the behavior of his DockinWindow class when docking where the
    called virtual method:

    SfxDockingWindow::CalcDockingSize (SfxChildAlignment eAlign)

    is overridden (see below).
*/
SfxChildAlignment SfxDockingWindow::CalcAlignment(const Point& rPos, tools::Rectangle& rRect)
{
    // calculate hypothetical sizes for different modes
    Size aFloatingSize(CalcDockingSize(SfxChildAlignment::NOALIGNMENT));

    // check if docking is permitted
    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
    if ( !pWorkWin->IsDockingAllowed() )
    {
        rRect.SetSize( aFloatingSize );
        return pImpl->GetDockAlignment();
    }

    // calculate borders to shrink inner area before checking for intersection with tracking rectangle
    tools::Long nLRBorder, nTBBorder;

    // take the smaller size of docked and floating mode
    Size aBorderTmp = pImpl->aSplitSize;
    if ( GetFloatingSize().Height() < aBorderTmp.Height() )
        aBorderTmp.setHeight( GetFloatingSize().Height() );
    if ( GetFloatingSize().Width() < aBorderTmp.Width() )
        aBorderTmp.setWidth( GetFloatingSize().Width() );

    nLRBorder = aBorderTmp.Width();
    nTBBorder = aBorderTmp.Height();

    // limit border to predefined constant values
    if ( nLRBorder > MAX_TOGGLEAREA_WIDTH )
        nLRBorder = MAX_TOGGLEAREA_WIDTH;
    if ( nTBBorder > MAX_TOGGLEAREA_WIDTH )
        nTBBorder = MAX_TOGGLEAREA_WIDTH;

    // shrink area for floating mode if possible
    tools::Rectangle aInRect = GetInnerRect();
    if ( aInRect.GetWidth() > nLRBorder )
        aInRect.AdjustLeft(nLRBorder/2 );
    if ( aInRect.GetWidth() > nLRBorder )
        aInRect.AdjustRight( -(nLRBorder/2) );
    if ( aInRect.GetHeight() > nTBBorder )
        aInRect.AdjustTop(nTBBorder/2 );
    if ( aInRect.GetHeight() > nTBBorder )
        aInRect.AdjustBottom( -(nTBBorder/2) );

    // calculate alignment resulting from docking rectangle
    bool bBecomesFloating = false;
    SfxChildAlignment eDockAlign = pImpl->GetDockAlignment();
    tools::Rectangle aDockingRect( rRect );
    if ( !IsFloatingMode() )
    {
        // don't use tracking rectangle for alignment check, because it will be too large
        // to get a floating mode as result - switch to floating size
        // so the calculation only depends on the position of the rectangle, not the current
        // docking state of the window
        aDockingRect.SetSize( GetFloatingSize() );

        // in this mode docking is never done by keyboard, so it's OK to use the mouse position
        aDockingRect.SetPos( pWorkWin->GetWindow()->OutputToScreenPixel( pWorkWin->GetWindow()->GetPointerPosPixel() ) );
    }

    Point aPos = aDockingRect.TopLeft();
    tools::Rectangle aIntersect = GetOuterRect().GetIntersection( aDockingRect );
    if ( aIntersect.IsEmpty() )
        // docking rectangle completely outside docking area -> floating mode
        bBecomesFloating = true;
    else
    {
        // create a small test rect around the mouse position and use this one
        // instead of the passed rRect to not dock too easily or by accident
        tools::Rectangle aSmallDockingRect;
        aSmallDockingRect.SetSize( Size( MAX_TOGGLEAREA_WIDTH, MAX_TOGGLEAREA_HEIGHT ) );
        Point aNewPos(rPos);
        aNewPos.AdjustX( -(aSmallDockingRect.GetWidth()/2) );
        aNewPos.AdjustY( -(aSmallDockingRect.GetHeight()/2) );
        aSmallDockingRect.SetPos(aNewPos);
        tools::Rectangle aIntersectRect = aInRect.GetIntersection( aSmallDockingRect );
        if ( aIntersectRect == aSmallDockingRect )
            // docking rectangle completely inside (shrunk) inner area -> floating mode
            bBecomesFloating = true;
    }

    if ( bBecomesFloating )
    {
        eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::NOALIGNMENT);
    }
    else
    {
        // docking rectangle is in the "sensible area"
        Point aInPosTL( aPos.X()-aInRect.Left(), aPos.Y()-aInRect.Top() );
        Point aInPosBR( aPos.X()-aInRect.Left() + aDockingRect.GetWidth(), aPos.Y()-aInRect.Top() + aDockingRect.GetHeight() );
        Size  aInSize = aInRect.GetSize();
        bool  bNoChange = false;

        // check if alignment is still unchanged
        switch ( GetAlignment() )
        {
            case SfxChildAlignment::LEFT:
            case SfxChildAlignment::FIRSTLEFT:
            case SfxChildAlignment::LASTLEFT:
                if (aInPosTL.X() <= 0)
                {
                    eDockAlign = GetAlignment();
                    bNoChange = true;
                }
                break;
            case SfxChildAlignment::TOP:
            case SfxChildAlignment::LOWESTTOP:
            case SfxChildAlignment::HIGHESTTOP:
                if ( aInPosTL.Y() <= 0)
                {
                    eDockAlign = GetAlignment();
                    bNoChange = true;
                }
                break;
            case SfxChildAlignment::RIGHT:
            case SfxChildAlignment::FIRSTRIGHT:
            case SfxChildAlignment::LASTRIGHT:
                if ( aInPosBR.X() >= aInSize.Width())
                {
                    eDockAlign = GetAlignment();
                    bNoChange = true;
                }
                break;
            case SfxChildAlignment::BOTTOM:
            case SfxChildAlignment::LOWESTBOTTOM:
            case SfxChildAlignment::HIGHESTBOTTOM:
                if ( aInPosBR.Y() >= aInSize.Height())
                {
                    eDockAlign = GetAlignment();
                    bNoChange = true;
                }
                break;
            default:
                break;
        }

        if ( !bNoChange )
        {
            // alignment will change, test alignment according to distance of the docking rectangles edges
            bool bForbidden = true;
            if ( aInPosTL.X() <= 0)
            {
                eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::LEFT);
                bForbidden = ( eDockAlign != SfxChildAlignment::LEFT &&
                               eDockAlign != SfxChildAlignment::FIRSTLEFT &&
                               eDockAlign != SfxChildAlignment::LASTLEFT );
            }

            if ( bForbidden && aInPosTL.Y() <= 0)
            {
                eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::TOP);
                bForbidden = ( eDockAlign != SfxChildAlignment::TOP &&
                               eDockAlign != SfxChildAlignment::HIGHESTTOP &&
                               eDockAlign != SfxChildAlignment::LOWESTTOP );
            }

            if ( bForbidden && aInPosBR.X() >= aInSize.Width())
            {
                eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::RIGHT);
                bForbidden = ( eDockAlign != SfxChildAlignment::RIGHT &&
                               eDockAlign != SfxChildAlignment::FIRSTRIGHT &&
                               eDockAlign != SfxChildAlignment::LASTRIGHT );
            }

            if ( bForbidden && aInPosBR.Y() >= aInSize.Height())
            {
                eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::BOTTOM);
                bForbidden = ( eDockAlign != SfxChildAlignment::BOTTOM &&
                               eDockAlign != SfxChildAlignment::HIGHESTBOTTOM &&
                               eDockAlign != SfxChildAlignment::LOWESTBOTTOM );
            }

            // the calculated alignment was rejected by the window -> take floating mode
            if ( bForbidden )
                eDockAlign = CheckAlignment(pImpl->GetDockAlignment(),SfxChildAlignment::NOALIGNMENT);
        }
    }

    if ( eDockAlign == SfxChildAlignment::NOALIGNMENT )
    {
        // In the FloatingMode the tracking rectangle will get the floating
        // size. Due to a bug the rRect may only be changed when the
        // alignment is changed!
        if ( eDockAlign != pImpl->GetDockAlignment() )
            aDockingRect.SetSize( aFloatingSize );
    }
    else
    {
        sal_uInt16 nLine, nPos;
        SfxSplitWindow *pSplitWin = pWorkWin->GetSplitWindow_Impl(eDockAlign);
        aPos = pSplitWin->ScreenToOutputPixel( aPos );
        if ( pSplitWin->GetWindowPos( aPos, nLine, nPos ) )
        {
            // mouse over splitwindow, get line and position
            pImpl->nDockLine = nLine;
            pImpl->nDockPos = nPos;
            pImpl->bNewLine = false;
        }
        else
        {
            // mouse touches inner border -> create new line
            if ( eDockAlign == GetAlignment() && pImpl->pSplitWin &&
                 pImpl->nLine == pImpl->pSplitWin->GetLineCount()-1 && pImpl->pSplitWin->GetWindowCount(pImpl->nLine) == 1 )
            {
                // if this window is the only one in the last line, it can't be docked as new line in the same splitwindow
                pImpl->nDockLine = pImpl->nLine;
                pImpl->nDockPos = pImpl->nPos;
                pImpl->bNewLine = false;
            }
            else
            {
                // create new line
                pImpl->nDockLine = pSplitWin->GetLineCount();
                pImpl->nDockPos = 0;
                pImpl->bNewLine = true;
            }
        }

        bool bChanged = pImpl->nLine != pImpl->nDockLine || pImpl->nPos != pImpl->nDockPos || eDockAlign != GetAlignment();
        if ( !bChanged && !IsFloatingMode() )
        {
            // window only slightly moved, no change of any property
            rRect.SetSize( pImpl->aSplitSize );
            rRect.SetPos( aDockingRect.TopLeft() );
            return eDockAlign;
        }

        // calculate new size and position
        Size aSize;
        Point aPoint = aDockingRect.TopLeft();
        Size aInnerSize = GetInnerRect().GetSize();
        if ( eDockAlign == SfxChildAlignment::LEFT || eDockAlign == SfxChildAlignment::RIGHT )
        {
            if ( pImpl->bNewLine )
            {
                // set height to height of free area
                aSize.setHeight( aInnerSize.Height() );
                aSize.setWidth( pImpl->nHorizontalSize );
                if ( eDockAlign == SfxChildAlignment::LEFT )
                {
                    aPoint = aInnerRect.TopLeft();
                }
                else
                {
                    aPoint = aInnerRect.TopRight();
                    aPoint.AdjustX( -(aSize.Width()) );
                }
            }
            else
            {
                // get width from splitwindow
                aSize.setWidth( pSplitWin->GetLineSize(nLine) );
                aSize.setHeight( pImpl->aSplitSize.Height() );
            }
        }
        else
        {
            if ( pImpl->bNewLine )
            {
                // set width to width of free area
                aSize.setWidth( aInnerSize.Width() );
                aSize.setHeight( pImpl->nVerticalSize );
                if ( eDockAlign == SfxChildAlignment::TOP )
                {
                    aPoint = aInnerRect.TopLeft();
                }
                else
                {
                    aPoint = aInnerRect.BottomLeft();
                    aPoint.AdjustY( -(aSize.Height()) );
                }
            }
            else
            {
                // get height from splitwindow
                aSize.setHeight( pSplitWin->GetLineSize(nLine) );
                aSize.setWidth( pImpl->aSplitSize.Width() );
            }
        }

        aDockingRect.SetSize( aSize );
        aDockingRect.SetPos( aPoint );
    }

    rRect = aDockingRect;
    return eDockAlign;
}

/** Virtual method of the SfxDockingWindow class. This method determines how
    the size of the DockingWindows changes depending on the alignment. The base
    implementation uses the floating mode, the size of the marked Floating
    Size. For horizontal alignment, the width will be the width of the outer
    DockingRectangle, with vertical alignment the height will be the height of
    the inner DockingRectangle (resulting from the order in which the SFX child
    windows are displayed). The other size is set to the current floating-size,
    this could changed by a to intervening derived class. The docking size must
    be the same for Left/Right and Top/Bottom.
*/
Size SfxDockingWindow::CalcDockingSize(SfxChildAlignment eAlign)
{
    // Note: if the resizing is also possible in the docked state, then the
    // Floating-size does also have to be adjusted?

    Size aSize = GetFloatingSize();
    switch (eAlign)
    {
        case SfxChildAlignment::TOP:
        case SfxChildAlignment::BOTTOM:
        case SfxChildAlignment::LOWESTTOP:
        case SfxChildAlignment::HIGHESTTOP:
        case SfxChildAlignment::LOWESTBOTTOM:
        case SfxChildAlignment::HIGHESTBOTTOM:
            aSize.setWidth( aOuterRect.Right() - aOuterRect.Left() );
            break;
        case SfxChildAlignment::LEFT:
        case SfxChildAlignment::RIGHT:
        case SfxChildAlignment::FIRSTLEFT:
        case SfxChildAlignment::LASTLEFT:
        case SfxChildAlignment::FIRSTRIGHT:
        case SfxChildAlignment::LASTRIGHT:
            aSize.setHeight( aInnerRect.Bottom() - aInnerRect.Top() );
            break;
        case SfxChildAlignment::NOALIGNMENT:
            break;
              default:
                  break;
    }

    return aSize;
}

/** Virtual method of the SfxDockingWindow class. Here a derived class can
    disallow certain alignments. The base implementation does not
    prohibit alignment.
*/
SfxChildAlignment SfxDockingWindow::CheckAlignment(SfxChildAlignment,
    SfxChildAlignment eAlign)
{
    return eAlign;
}

/** The window is closed when the ChildWindow is destroyed by running the
    ChildWindow-slots. If this is method is overridden by a derived class
    method, then the SfxDockingDialogWindow: Close() must be called afterwards
    if the Close() was not cancelled with "return sal_False".
*/
bool SfxDockingWindow::Close()
{
    // Execute with Parameters, since Toggle is ignored by some ChildWindows.
    if ( !pMgr )
        return true;

    SfxBoolItem aValue( pMgr->GetType(), false);
    pBindings->GetDispatcher_Impl()->ExecuteList(
        pMgr->GetType(), SfxCallMode::RECORD | SfxCallMode::ASYNCHRON,
        { &aValue });
    return true;
}

void SfxDockingWindow::Paint(vcl::RenderContext&, const tools::Rectangle& /*rRect*/)
{
}

/** With this method, a minimal OutputSize be can set, that is queried in
    the Resizing()-Handler.
*/
void SfxDockingWindow::SetMinOutputSizePixel( const Size& rSize )
{
    pImpl->aMinSize = rSize;
    ResizableDockingWindow::SetMinOutputSizePixel( rSize );
}

/** Set the minimum size which is returned.*/
const Size& SfxDockingWindow::GetMinOutputSizePixel() const
{
    return pImpl->aMinSize;
}

bool SfxDockingWindow::EventNotify( NotifyEvent& rEvt )
{
    if ( !pImpl )
        return ResizableDockingWindow::EventNotify( rEvt );

    if ( rEvt.GetType() == NotifyEventType::GETFOCUS )
    {
        if (pMgr != nullptr)
            pBindings->SetActiveFrame( pMgr->GetFrame() );

        if ( pImpl->pSplitWin )
            pImpl->pSplitWin->SetActiveWindow_Impl( this );
        else if (pMgr != nullptr)
            pMgr->Activate_Impl();

        // In VCL EventNotify goes first to the window itself, also call the
        // base class, otherwise the parent learns nothing
        // if ( rEvt.GetWindow() == this )  PB: #i74693# not necessary any longer
        ResizableDockingWindow::EventNotify( rEvt );
        // tdf#151112 move focus into container widget hierarchy if not already there
        if (m_xContainer && !m_xContainer->has_child_focus())
            m_xContainer->child_grab_focus();
        return true;
    }
    else if( rEvt.GetType() == NotifyEventType::KEYINPUT )
    {
        // First, allow KeyInput for Dialog functions
        if (!DockingWindow::EventNotify(rEvt) && SfxViewShell::Current())
        {
            // then also for valid global accelerators.
            return SfxViewShell::Current()->GlobalKeyInput_Impl( *rEvt.GetKeyEvent() );
        }
        return true;
    }
    else if ( rEvt.GetType() == NotifyEventType::LOSEFOCUS && !HasChildPathFocus() )
    {
        pBindings->SetActiveFrame( nullptr );
    }

    return ResizableDockingWindow::EventNotify( rEvt );
}

void SfxDockingWindow::SetItemSize_Impl( const Size& rSize )
{
    pImpl->aSplitSize = rSize;

    SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
    pWorkWin->ConfigChild_Impl( SfxChildIdentifier::SPLITWINDOW, SfxDockingConfig::ALIGNDOCKINGWINDOW, pMgr->GetType() );
}

void SfxDockingWindow::Disappear_Impl()
{
    if ( pImpl->pSplitWin && pImpl->pSplitWin->IsItemValid( GetType() ) )
        pImpl->pSplitWin->RemoveWindow(this);
}

void SfxDockingWindow::Reappear_Impl()
{
    if ( pImpl->pSplitWin && !pImpl->pSplitWin->IsItemValid( GetType() ) )
    {
        pImpl->pSplitWin->InsertWindow( this, pImpl->aSplitSize );
    }
}

bool SfxDockingWindow::IsAutoHide_Impl() const
{
    if ( pImpl->pSplitWin )
        return !pImpl->pSplitWin->IsFadeIn();
    else
        return false;
}

void SfxDockingWindow::AutoShow_Impl()
{
    if ( pImpl->pSplitWin )
    {
        pImpl->pSplitWin->FadeIn();
    }
}

void SfxDockingWindow::StateChanged( StateChangedType nStateChange )
{
    if ( nStateChange == StateChangedType::InitShow )
        Initialize_Impl();

    ResizableDockingWindow::StateChanged( nStateChange );
}

void SfxDockingWindow::Move()
{
    if ( pImpl )
        pImpl->aMoveIdle.Start();
}

IMPL_LINK_NOARG(SfxDockingWindow, TimerHdl, Timer *, void)
{
    pImpl->aMoveIdle.Stop();
    if ( IsReallyVisible() && IsFloatingMode() )
    {
        SetFloatingSize( GetOutputSizePixel() );
        pImpl->aWinState = GetFloatingWindow()->GetWindowState();
        SfxWorkWindow *pWorkWin = pBindings->GetWorkWindow_Impl();
        pWorkWin->ConfigChild_Impl( SfxChildIdentifier::SPLITWINDOW, SfxDockingConfig::ALIGNDOCKINGWINDOW, pMgr->GetType() );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

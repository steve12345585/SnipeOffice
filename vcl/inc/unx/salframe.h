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

#pragma once

#include <X11/Xlib.h>

#include <unx/saltype.h>
#include <unx/saldisp.hxx>
#include <unx/sessioninhibitor.hxx>
#include <salframe.hxx>
#include <salwtype.hxx>

#include <vcl/ptrstyle.hxx>
#include <vcl/sysdata.hxx>
#include <vcl/timer.hxx>

#include <list>

class   X11SalGraphics;
class   SalI18N_InputContext;

namespace vcl_sal { class WMAdaptor; class NetWMAdaptor; class GnomeWMAdaptor; }

// X11SalFrame
enum class X11ShowState
{
    Unknown = -1,
    Minimized = 0,
    Normal = 1,
    Hidden = 2
};

enum class WMWindowType
{
    Normal,
    ModelessDialogue,
    Utility,
    Splash,
    Toolbar,
    Dock
};

class X11SalFrame final : public SalFrame
{
    friend class vcl_sal::WMAdaptor;
    friend class vcl_sal::NetWMAdaptor;
    friend class vcl_sal::GnomeWMAdaptor;

    X11SalFrame*    mpParent;             // pointer to parent frame
                                          // which should never obscure this frame
    bool            mbTransientForRoot;
    std::list< X11SalFrame* > maChildren; // List of child frames

    SalDisplay     *pDisplay_;
    SalX11Screen    m_nXScreen;
    ::Window        mhWindow;
    cairo_surface_t* mpSurface;
    ::Window        mhShellWindow;
    ::Window        mhForeignParent;
    // window to fall back to when no longer in fullscreen mode
    ::Window        mhStackingWindow;
    // window to listen for CirculateNotify events

    Cursor          hCursor_;
    int             nCaptured_;         // is captured

    std::unique_ptr<X11SalGraphics> pGraphics_;            // current frame graphics
    std::unique_ptr<X11SalGraphics> pFreeGraphics_;        // first free frame graphics

    bool            mbSendExtKeyModChange;
    ModKeyFlags     mnExtKeyMod;

    X11ShowState    nShowState_;        // show state
    int             nWidth_;            // client width
    int             nHeight_;           // client height
    AbsoluteScreenPixelRectangle maRestorePosSize;
    SalFrameStyleFlags nStyle_;
    SalExtStyle     mnExtStyle;
    bool            bAlwaysOnTop_;
    bool            bViewable_;
    bool            bMapped_;
    bool            bDefaultPosition_;  // client is centered initially
    bool            m_bXEmbed;
    int             nVisibility_;
    int             m_nWorkArea;
    bool            m_bSetFocusOnMap;

    SessionManagerInhibitor maSessionManagerInhibitor;
    tools::Rectangle       maPaintRegion;

    Timer           maAlwaysOnTopRaiseTimer;

    // data for WMAdaptor
    WMWindowType    meWindowType;
    bool            mbMaximizedVert;
    bool            mbMaximizedHorz;
    bool            mbFullScreen;
    bool m_bIsPartialFullScreen;

    // icon id
    int             mnIconID;

    OUString        m_aTitle;

    OUString        m_sWMClass;

    SystemEnvData maSystemChildData;

    std::unique_ptr<SalI18N_InputContext> mpInputContext;
    Bool            mbInputFocus;

    std::vector<XRectangle> m_vClipRectangles;

    bool mPendingSizeEvent;

    void            GetPosSize( AbsoluteScreenPixelRectangle &rPosSize );
    void            SetSize   ( const Size      &rSize );
    void            Center();
    void            SetPosSize( const AbsoluteScreenPixelRectangle &rPosSize );
    void            Minimize();
    void            Maximize();
    void            Restore();

    void            RestackChildren( ::Window* pTopLevelWindows, int nTopLevelWindows );
    void            RestackChildren();

    bool            HandleKeyEvent      ( XKeyEvent         *pEvent );
    bool            HandleMouseEvent    ( XEvent            *pEvent );
    bool            HandleFocusEvent    ( XFocusChangeEvent const *pEvent );
    bool            HandleExposeEvent   ( XEvent const      *pEvent );
    bool            HandleSizeEvent     ( XConfigureEvent   *pEvent );
    bool            HandleStateEvent    ( XPropertyEvent const *pEvent );
    bool            HandleReparentEvent ( XReparentEvent    *pEvent );
    bool            HandleClientMessage ( XClientMessageEvent*pEvent );

    DECL_LINK( HandleAlwaysOnTopRaise, Timer*, void );

    void            createNewWindow( ::Window aParent, SalX11Screen nXScreen = SalX11Screen( -1 ) );
    void            updateScreenNumber();

    void            setXEmbedInfo();
    void            askForXEmbedFocus( sal_Int32 i_nTimeCode );

    void            updateWMClass();
public:
    X11SalFrame( SalFrame* pParent, SalFrameStyleFlags nSalFrameStyle, SystemParentData const * pSystemParent = nullptr );
    virtual ~X11SalFrame() override;

    bool            Dispatch( XEvent *pEvent );
    void            Init( SalFrameStyleFlags nSalFrameStyle, SalX11Screen nScreen,
                          SystemParentData const * pParentData, bool bUseGeometry = false );

    SalDisplay* GetDisplay() const
    {
        return pDisplay_;
    }
    Display *GetXDisplay() const
    {
        return pDisplay_->GetDisplay();
    }
    const SalX11Screen&     GetScreenNumber() const { return m_nXScreen; }
    ::Window                GetWindow() const { return mhWindow; }
    cairo_surface_t*        GetSurface() const { return mpSurface; }
    ::Window                GetShellWindow() const { return mhShellWindow; }
    ::Window                GetForeignParent() const { return mhForeignParent; }
    ::Window                GetStackingWindow() const { return mhStackingWindow; }
    void                    Close() const { CallCallback( SalEvent::Close, nullptr ); }
    SalFrameStyleFlags      GetStyle() const { return nStyle_; }

    Cursor                  GetCursor() const { return hCursor_; }
    bool                    IsCaptured() const { return nCaptured_ == 1; }
#if !defined(__synchronous_extinput__)
    void                    HandleExtTextEvent (XClientMessageEvent const *pEvent);
#endif
    bool                    IsOverrideRedirect() const;
    bool                    IsChildWindow() const { return bool(nStyle_ & (SalFrameStyleFlags::PLUG|SalFrameStyleFlags::SYSTEMCHILD)); }
    bool                    IsSysChildWindow() const { return bool(nStyle_ & SalFrameStyleFlags::SYSTEMCHILD); }
    bool                    IsFloatGrabWindow() const;
    SalI18N_InputContext* getInputContext() const { return mpInputContext.get(); }
    bool                    hasFocus() const { return mbInputFocus; }

    void                    beginUnicodeSequence();
    bool                    appendUnicodeSequence( sal_Unicode );
    bool                    endUnicodeSequence();

    virtual SalGraphics*        AcquireGraphics() override;
    virtual void                ReleaseGraphics( SalGraphics* pGraphics ) override;

    // call with true to clear graphics (setting None as drawable)
    // call with false to setup graphics with window (GetWindow())
    virtual void                updateGraphics( bool bClear );

    virtual bool                PostEvent(std::unique_ptr<ImplSVEvent> pData) override;

    virtual void                SetTitle( const OUString& rTitle ) override;
    virtual void                SetIcon( sal_uInt16 nIcon ) override;
    virtual void                SetMenu( SalMenu* pMenu ) override;

    virtual void                SetExtendedFrameStyle( SalExtStyle nExtStyle ) override;
    virtual void                Show( bool bVisible, bool bNoActivate = false ) override;
    virtual void                SetMinClientSize( tools::Long nWidth, tools::Long nHeight ) override;
    virtual void                SetMaxClientSize( tools::Long nWidth, tools::Long nHeight ) override;
    virtual void                SetPosSize( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight, sal_uInt16 nFlags ) override;
    virtual void                GetClientSize( tools::Long& rWidth, tools::Long& rHeight ) override;
    virtual void                GetWorkArea( AbsoluteScreenPixelRectangle& rRect ) override;
    virtual SalFrame*           GetParent() const override;
    virtual void SetWindowState(const vcl::WindowData*) override;
    virtual bool GetWindowState(vcl::WindowData*) override;
    virtual void                ShowFullScreen( bool bFullScreen, sal_Int32 nMonitor ) override;
    virtual void                StartPresentation( bool bStart ) override;
    virtual void                SetAlwaysOnTop( bool bOnTop ) override;
    virtual void                ToTop( SalFrameToTop nFlags ) override;
    virtual void                SetPointer( PointerStyle ePointerStyle ) override;
    virtual void                CaptureMouse( bool bMouse ) override;
    virtual void                SetPointerPos( tools::Long nX, tools::Long nY ) override;
    using SalFrame::Flush;
    virtual void                Flush() override;
    virtual void                SetInputContext( SalInputContext* pContext ) override;
    virtual void                EndExtTextInput( EndExtTextInputFlags nFlags ) override;
    virtual OUString              GetKeyName( sal_uInt16 nKeyCode ) override;
    virtual bool                MapUnicodeToKeyCode( sal_Unicode aUnicode, LanguageType aLangType, vcl::KeyCode& rKeyCode ) override;
    virtual LanguageType        GetInputLanguage() override;
    virtual void                UpdateSettings( AllSettings& rSettings ) override;
    virtual void                Beep() override;
    virtual const SystemEnvData& GetSystemData() const override;
    virtual SalPointerState     GetPointerState() override;
    virtual KeyIndicatorState   GetIndicatorState() override;
    virtual void                SimulateKeyPress( sal_uInt16 nKeyCode ) override;
    virtual void                SetParent( SalFrame* pNewParent ) override;
    virtual void                SetPluginParent( SystemParentData* pNewParent ) override;

    virtual void                SetScreenNumber( unsigned int ) override;
    virtual void                SetApplicationID( const OUString &rWMClass ) override;

    // shaped system windows
    // set clip region to none (-> rectangular windows, normal state)
    virtual void                    ResetClipRegion() override;
    // start setting the clipregion consisting of nRects rectangles
    virtual void                    BeginSetClipRegion( sal_uInt32 nRects ) override;
    // add a rectangle to the clip region
    virtual void                    UnionClipRegion( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight ) override;
    // done setting up the clipregion
    virtual void                    EndSetClipRegion() override;

    /// @internal
    void setPendingSizeEvent();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

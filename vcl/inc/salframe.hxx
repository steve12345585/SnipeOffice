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

#include "impdel.hxx"
#include "salwtype.hxx"
#include "salgeom.hxx"

#include <vcl/help.hxx>
#include <o3tl/typed_flags_set.hxx>

#include <vcl/window.hxx>
    // complete vcl::Window for SalFrame::CallCallback under -fsanitize=function

class AllSettings;
class SalGraphics;
class SalBitmap;
class SalMenu;

namespace vcl { class WindowData; }
struct SalInputContext;
struct SystemEnvData;

// SalFrame types
enum class SalFrameToTop {
    NONE             = 0x00,
    RestoreWhenMin   = 0x01,
    ForegroundTask   = 0x02,
    GrabFocus        = 0x04,
    GrabFocusOnly    = 0x08
};
namespace o3tl {
    template<> struct typed_flags<SalFrameToTop> : is_typed_flags<SalFrameToTop, 0x0f> {};
};

namespace vcl { class KeyCode; }

namespace weld
{
    class Window;
}

enum class FloatWinPopupFlags;

// SalFrame styles
enum class SalFrameStyleFlags
{
    NONE                = 0x00000000,
    DEFAULT             = 0x00000001,
    MOVEABLE            = 0x00000002,
    SIZEABLE            = 0x00000004,
    CLOSEABLE           = 0x00000008,
    // no shadow effect on Windows XP
    NOSHADOW            = 0x00000010,
    // indicate tooltip windows, so they can always be topmost
    TOOLTIP             = 0x00000020,
    // windows without windowmanager decoration, this typically only applies to floating windows
    OWNERDRAWDECORATION = 0x00000040,
    // dialogs
    DIALOG              = 0x00000080,
    // the window containing the intro bitmap, aka splashscreen
    INTRO               = 0x00000100,
    // tdf#144624: don't set icon
    NOICON              = 0x01000000,
    // system child window inside another SalFrame
    SYSTEMCHILD         = 0x08000000,
    // plugged system child window
    PLUG                = 0x10000000,
    // floating window
    FLOAT               = 0x20000000,
    // toolwindows should be painted with a smaller decoration
    TOOLWINDOW          = 0x40000000,
};

namespace o3tl {
    template<> struct typed_flags<SalFrameStyleFlags> : is_typed_flags<SalFrameStyleFlags, 0x798001ff> {};
};

// Extended frame style (sal equivalent to extended WinBits)
typedef sal_uInt64 SalExtStyle;
#define SAL_FRAME_EXT_STYLE_DOCUMENT        SalExtStyle(0x00000001)
#define SAL_FRAME_EXT_STYLE_DOCMODIFIED     SalExtStyle(0x00000002)

// Flags for SetPosSize
#define SAL_FRAME_POSSIZE_X                 (sal_uInt16(0x0001))
#define SAL_FRAME_POSSIZE_Y                 (sal_uInt16(0x0002))
#define SAL_FRAME_POSSIZE_WIDTH             (sal_uInt16(0x0004))
#define SAL_FRAME_POSSIZE_HEIGHT            (sal_uInt16(0x0008))

struct SystemParentData;
struct ImplSVEvent;

/// A SalFrame is a system window (e.g. an X11 window).
class VCL_PLUGIN_PUBLIC SalFrame
    : public vcl::DeletionNotifier
    , public SalGeometryProvider
{
private:
    // the VCL window corresponding to this frame
    VclPtr<vcl::Window>     m_pWindow;
    SALFRAMEPROC            m_pProc;
    Link<bool, void>        m_aModalHierarchyHdl;
protected:
    // subclasses need to either keep this up to date
    // or override GetUnmirroredGeometry()
    SalFrameGeometry maGeometry; ///< absolute, unmirrored values

    mutable std::unique_ptr<weld::Window> m_xFrameWeld;
public:
                            SalFrame();
    virtual                 ~SalFrame() override;

    // SalGeometryProvider
    virtual tools::Long GetWidth() const override { return GetUnmirroredGeometry().width(); }
    virtual tools::Long GetHeight() const override { return GetUnmirroredGeometry().height(); }
    virtual bool IsOffScreen() const override { return false; }

    // SalGraphics or NULL, but two Graphics for all SalFrames
    // must be returned
    virtual SalGraphics*    AcquireGraphics() = 0;
    virtual void            ReleaseGraphics( SalGraphics* pGraphics ) = 0;

    // Event must be destroyed, when Frame is destroyed
    // When Event is called, SalInstance::Yield() must be returned
    virtual bool            PostEvent(std::unique_ptr<ImplSVEvent> pData) = 0;

    virtual void            SetTitle( const OUString& rTitle ) = 0;
    virtual void            SetIcon( sal_uInt16 nIcon ) = 0;
    virtual void            SetRepresentedURL( const OUString& );
    virtual void            SetMenu( SalMenu *pSalMenu ) = 0;

    virtual void            SetExtendedFrameStyle( SalExtStyle nExtStyle ) = 0;

    // Before the window is visible, a resize event
    // must be sent with the correct size
    virtual void            Show( bool bVisible, bool bNoActivate = false ) = 0;

    // Set ClientSize and Center the Window to the desktop
    // and send/post a resize message
    virtual void            SetMinClientSize( tools::Long nWidth, tools::Long nHeight ) = 0;
    virtual void            SetMaxClientSize( tools::Long nWidth, tools::Long nHeight ) = 0;
    virtual void            SetPosSize( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight, sal_uInt16 nFlags ) = 0;
    static OUString DumpSetPosSize(tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight, sal_uInt16 nFlags);
    virtual void            GetClientSize( tools::Long& rWidth, tools::Long& rHeight ) = 0;
    virtual void            GetWorkArea( AbsoluteScreenPixelRectangle& rRect ) = 0;
    virtual SalFrame*       GetParent() const = 0;
    // Note: x will be mirrored at parent if UI mirroring is active
    SalFrameGeometry        GetGeometry() const;

    // subclasses either have to keep maGeometry up to date or override this
    // method to return an up-to-date SalFrameGeometry
    virtual SalFrameGeometry GetUnmirroredGeometry() const { return maGeometry; }

    virtual void SetWindowState(const vcl::WindowData*) = 0;
    // return the absolute, unmirrored system frame state
    // if this returns false the structure is uninitialised
    [[nodiscard]]
    virtual bool GetWindowState(vcl::WindowData*) = 0;
    virtual void            ShowFullScreen( bool bFullScreen, sal_Int32 nDisplay ) = 0;
    virtual void            PositionByToolkit( const tools::Rectangle&, FloatWinPopupFlags ) {};

    // Enable/Disable ScreenSaver, SystemAgents, ...
    virtual void            StartPresentation( bool bStart ) = 0;
    // Show Window over all other Windows
    virtual void            SetAlwaysOnTop( bool bOnTop ) = 0;

    // Window to top and grab focus
    virtual void            ToTop( SalFrameToTop nFlags ) = 0;

    // grab focus to the main widget, can be no-op if the vclplug only uses one widget
    virtual void            GrabFocus() {}

    // this function can call with the same
    // pointer style
    virtual void            SetPointer( PointerStyle ePointerStyle ) = 0;
    virtual void            CaptureMouse( bool bMouse ) = 0;
    virtual void            SetPointerPos( tools::Long nX, tools::Long nY ) = 0;

    // flush output buffer
    virtual void            Flush() = 0;
    virtual void            Flush( const tools::Rectangle& );

    virtual void            SetInputContext( SalInputContext* pContext ) = 0;
    virtual void            EndExtTextInput( EndExtTextInputFlags nFlags ) = 0;

    virtual OUString        GetKeyName( sal_uInt16 nKeyCode ) = 0;

    // returns in 'rKeyCode' the single keycode that translates to the given unicode when using a keyboard layout of language 'aLangType'
    // returns false if no mapping exists or function not supported
    // this is required for advanced menu support
    virtual bool            MapUnicodeToKeyCode( sal_Unicode aUnicode, LanguageType aLangType, vcl::KeyCode& rKeyCode ) = 0;

    // returns the input language used for the last key stroke
    // may be LANGUAGE_DONTKNOW if not supported by the OS
    virtual LanguageType    GetInputLanguage() = 0;

    virtual void            UpdateSettings( AllSettings& rSettings ) = 0;

    virtual void            Beep() = 0;

    virtual void            FlashWindow() const {};

    // returns system data (most prominent: window handle)
    virtual const SystemEnvData& GetSystemData() const = 0;

    // tdf#139609 SystemEnvData::GetWindowHandle() calls this to on-demand fill the aWindow
    // member of SystemEnvData for backends that want to defer doing that
    virtual void            ResolveWindowHandle(SystemEnvData& /*rData*/) const {};

    // get current modifier, button mask and mouse position
    struct SalPointerState
    {
        sal_Int32 mnState;
        Point     maPos;      // in frame coordinates
    };

    virtual SalPointerState GetPointerState() = 0;

    virtual KeyIndicatorState GetIndicatorState() = 0;

    virtual void            SimulateKeyPress( sal_uInt16 nKeyCode ) = 0;

    // set new parent window
    virtual void            SetParent( SalFrame* pNewParent ) = 0;
    // reparent window to act as a plugin; implementation
    // may choose to use a new system window internally
    // return false to indicate failure
    virtual void            SetPluginParent( SystemParentData* pNewParent ) = 0;

    // move the frame to a new screen
    virtual void            SetScreenNumber( unsigned int nScreen ) = 0;

    virtual void            SetApplicationID( const OUString &rApplicationID) = 0;

    // shaped system windows
    // set clip region to none (-> rectangular windows, normal state)
    virtual void            ResetClipRegion() = 0;
    // start setting the clipregion consisting of nRects rectangles
    virtual void            BeginSetClipRegion( sal_uInt32 nRects ) = 0;
    // add a rectangle to the clip region
    virtual void            UnionClipRegion( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight ) = 0;
    // done setting up the clipregion
    virtual void            EndSetClipRegion() = 0;

    virtual void            SetModal(bool /*bModal*/)
    {
    }

    // return true to indicate tooltips are shown natively, false otherwise
    virtual bool            ShowTooltip(const OUString& /*rHelpText*/, const tools::Rectangle& /*rHelpArea*/)
    {
        return false;
    }

    // return !0 to indicate popovers are shown natively, 0 otherwise
    virtual void*           ShowPopover(const OUString& /*rHelpText*/, vcl::Window* /*pParent*/, const tools::Rectangle& /*rHelpArea*/, QuickHelpFlags /*nFlags*/)
    {
        return nullptr;
    }

    // return true to indicate popovers are shown natively, false otherwise
    virtual bool            UpdatePopover(void* /*nId*/, const OUString& /*rHelpText*/, vcl::Window* /*pParent*/, const tools::Rectangle& /*rHelpArea*/)
    {
        return false;
    }

    // return true to indicate popovers are shown natively, false otherwise
    virtual bool            HidePopover(void* /*nId*/)
    {
        return false;
    }

    virtual weld::Window*   GetFrameWeld() const;

    // Callbacks (independent part in vcl/source/window/winproc.cxx)
    // for default message handling return 0
    void                    SetCallback( vcl::Window* pWindow, SALFRAMEPROC pProc );

    // returns the instance set
    vcl::Window*            GetWindow() const { return m_pWindow; }

    void SetModalHierarchyHdl(const Link<bool, void>& rLink) { m_aModalHierarchyHdl = rLink; }
    void NotifyModalHierarchy(bool bModal) { m_aModalHierarchyHdl.Call(bModal); }

    virtual void            UpdateDarkMode() {}
    virtual bool            GetUseDarkMode() const { return false; }
    virtual bool            GetUseReducedAnimation() const { return false; };

    // Call the callback set; this sometimes necessary for implementation classes
    // that should not know more than necessary about the SalFrame implementation
    // (e.g. input methods, printer update handlers).
    bool                    CallCallback( SalEvent nEvent, const void* pEvent ) const
        { return m_pProc && m_pProc( m_pWindow, nEvent, pEvent ); }

    // Helper method for input method handling: Calculate cursor index in (UTF-16) OUString,
    // starting at nCursorIndex, moving number of characters (not UTF-16 codepoints) specified
    // in nOffset, nChars.
    static Selection        CalcDeleteSurroundingSelection(std::u16string_view rSurroundingText,
                                                           sal_Int32 nCursorIndex, int nOffset, int nChars);

    virtual void  SetTaskBarProgress(int /*nCurrentProgress*/) {}
    virtual void  SetTaskBarState(VclTaskBarStates /*eTaskBarState*/) {}
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

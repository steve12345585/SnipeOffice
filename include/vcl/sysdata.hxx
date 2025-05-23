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

#ifndef INCLUDED_VCL_SYSDATA_HXX
#define INCLUDED_VCL_SYSDATA_HXX

#include <sal/types.h>
#include <vcl/dllapi.h>
#include <config_vclplug.h>

class SalFrame;

#ifdef MACOSX
// predeclare the native classes to avoid header/include problems
typedef struct CGContext *CGContextRef;
typedef struct CGLayer   *CGLayerRef;
typedef const struct __CTFont * CTFontRef;
#ifdef __OBJC__
@class NSView;
#else
class NSView;
#endif
#endif

#ifdef IOS
typedef const struct __CTFont * CTFontRef;
typedef struct CGContext *CGContextRef;
#endif

#if defined(_WIN32)
#include <prewin.h>
#include <windef.h>
#include <postwin.h>
#endif

struct VCL_DLLPUBLIC SystemEnvData
{
    enum class Toolkit { Invalid, Gen, Gtk, Qt };
    Toolkit             toolkit;        // the toolkit in use
#if defined(_WIN32)
    HWND                hWnd;           // the window hwnd
#elif defined( MACOSX )
    NSView*             mpNSView;       // the cocoa (NSView *) implementing this object
    bool                mbOpenGL;       // use an OpenGL providing NSView
#elif defined( ANDROID )
    // Nothing
#elif defined( IOS )
    // Nothing
#elif defined( UNX )
    enum class Platform { Invalid, Wayland, Xcb, WASM };

    void*               pDisplay;       // the relevant display connection
    SalFrame*           pSalFrame;      // contains a salframe, if object has one
    void*               pWidget;        // the corresponding widget
    void*               pVisual;        // the visual in use
    int                 nScreen;        // the current screen of the window
    // note: this is a "long" in Xlib *but* in the protocol it's only 32-bit
    // however, the GTK3 vclplug wants to store pointers in here!
    sal_IntPtr          aShellWindow;   // the window of the frame's shell
    Platform            platform;       // the windowing system in use
private:
    sal_uIntPtr         aWindow;        // the window of the object
public:

    void SetWindowHandle(sal_uIntPtr nWindow)
    {
        aWindow = nWindow;
    }

    // SalFrame can be any SalFrame, just needed to determine which backend to use
    // to resolve the window handle
    sal_uIntPtr GetWindowHandle(const SalFrame* pReference) const;

#endif

    SystemEnvData()
        : toolkit(Toolkit::Invalid)
#if defined(_WIN32)
        , hWnd(nullptr)
#elif defined( MACOSX )
        , mpNSView(nullptr)
        , mbOpenGL(false)
#elif defined( ANDROID )
#elif defined( IOS )
#elif defined( UNX )
        , pDisplay(nullptr)
        , pSalFrame(nullptr)
        , pWidget(nullptr)
        , pVisual(nullptr)
        , nScreen(0)
        , aShellWindow(0)
        , platform(Platform::Invalid)
        , aWindow(0)
#endif
    {
    }
};

struct SystemParentData
{
    sal_uInt32      nSize;            // size in bytes of this structure
#if defined(_WIN32)
    HWND            hWnd;             // the window hwnd
#elif defined( MACOSX )
    NSView*         pView;            // the cocoa (NSView *) implementing this object
#elif defined( ANDROID )
    // Nothing
#elif defined( IOS )
    // Nothing
#elif defined( UNX )
    sal_uIntPtr     aWindow;          // the window of the object
    bool            bXEmbedSupport:1; // decides whether the object in question
                                      // should support the XEmbed protocol
#endif
};

struct SystemMenuData
{
#if defined(_WIN32)
    HMENU           hMenu;          // the menu handle of the menu bar
#else
    // Nothing
#endif
};

struct SystemGraphicsData
{
    sal_uInt32      nSize;          // size in bytes of this structure
#if defined(_WIN32)
    HDC             hDC;            // handle to a device context
    HWND            hWnd;           // optional handle to a window
#elif defined( MACOSX )
    CGContextRef    rCGContext;     // CoreGraphics graphic context
#elif defined( ANDROID )
    // Nothing
#elif defined( IOS )
    CGContextRef    rCGContext;     // CoreGraphics graphic context
#elif defined( UNX )
    void*           pDisplay;       // the relevant display connection
    sal_uIntPtr     hDrawable;      // a drawable
    void*           pVisual;        // the visual in use
    int             nScreen;        // the current screen of the drawable
#endif
#if USE_HEADLESS_CODE
    void*           pSurface;       // the cairo surface when using svp-based backends, which includes gtk[3|4]
#endif
    SystemGraphicsData()
        : nSize( sizeof( SystemGraphicsData ) )
#if defined(_WIN32)
        , hDC( nullptr )
        , hWnd( nullptr )
#elif defined( MACOSX )
        , rCGContext( nullptr )
#elif defined( ANDROID )
    // Nothing
#elif defined( IOS )
        , rCGContext( NULL )
#elif defined( UNX )
        , pDisplay( nullptr )
        , hDrawable( 0 )
        , pVisual( nullptr )
        , nScreen( 0 )
#endif
#if USE_HEADLESS_CODE
        , pSurface( nullptr )
#endif
    { }
};

struct SystemWindowData
{
#if defined(_WIN32)                  // meaningless on Windows
#elif defined( MACOSX )
    bool            bOpenGL;        // create an OpenGL providing NSView
    bool            bLegacy;        // create a 2.1 legacy context, only valid if bOpenGL == true
#elif defined( ANDROID )
    // Nothing
#elif defined( IOS )
    // Nothing
#elif defined( UNX )
    void*           pVisual;        // the visual to be used
    bool            bClipUsingNativeWidget; // default is false, true will attempt to clip the childwindow with a native widget
#endif
};

#endif // INCLUDED_VCL_SYSDATA_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

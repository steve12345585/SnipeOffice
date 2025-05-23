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

#include <string.h>


// Adjustments for TypeChecking

inline HPEN SelectPen( HDC hDC, HPEN hPen )
{
    return static_cast<HPEN>(SelectObject( hDC, static_cast<HGDIOBJ>(hPen) ));
}

inline void DeletePen( HPEN hPen )
{
    DeleteObject( static_cast<HGDIOBJ>(hPen) );
}

inline HPEN GetStockPen( int nObject )
{
    return static_cast<HPEN>(GetStockObject( nObject ));
}

inline HBRUSH SelectBrush( HDC hDC, HBRUSH hBrush )
{
    return static_cast<HBRUSH>(SelectObject( hDC, static_cast<HGDIOBJ>(hBrush) ));
}

inline void DeleteBrush( HBRUSH hBrush )
{
    DeleteObject( static_cast<HGDIOBJ>(hBrush) );
}

inline HBRUSH GetStockBrush( int nObject )
{
    return static_cast<HBRUSH>(GetStockObject( nObject ));
}

inline HFONT SelectFont( HDC hDC, HFONT hFont )
{
    return static_cast<HFONT>(SelectObject( hDC, static_cast<HGDIOBJ>(hFont) ));
}

inline void DeleteFont( HFONT hFont )
{
    DeleteObject( static_cast<HGDIOBJ>(hFont) );
}

inline HFONT GetStockFont( int nObject )
{
    return static_cast<HFONT>(GetStockObject( nObject ));
}

inline HBITMAP SelectBitmap( HDC hDC, HBITMAP hBitmap )
{
    return static_cast<HBITMAP>(SelectObject( hDC, static_cast<HGDIOBJ>(hBitmap) ));
}

inline void DeleteBitmap( HBITMAP hBitmap )
{
    DeleteObject( static_cast<HGDIOBJ>(hBitmap) );
}

inline void DeleteRegion( HRGN hRegion )
{
    DeleteObject( static_cast<HGDIOBJ>(hRegion) );
}

inline HPALETTE GetStockPalette( int nObject )
{
    return static_cast<HPALETTE>(GetStockObject( nObject ));
}

inline void DeletePalette( HPALETTE hPalette )
{
    DeleteObject( static_cast<HGDIOBJ>(hPalette) );
}

inline void SetWindowStyle( HWND hWnd, DWORD nStyle )
{
    SetWindowLongPtrW( hWnd, GWL_STYLE, nStyle );
}

inline DWORD GetWindowStyle( HWND hWnd )
{
    return GetWindowLongPtrW( hWnd, GWL_STYLE );
}

inline void SetWindowExStyle( HWND hWnd, DWORD nStyle )
{
    SetWindowLongPtrW( hWnd, GWL_EXSTYLE, nStyle );
}

inline DWORD GetWindowExStyle( HWND hWnd )
{
    return GetWindowLongPtrW( hWnd, GWL_EXSTYLE );
}

inline BOOL IsMinimized( HWND hWnd )
{
    return IsIconic( hWnd );
}

inline BOOL IsMaximized( HWND hWnd )
{
    return IsZoomed( hWnd );
}

inline void SetWindowFont( HWND hWnd, HFONT hFont, BOOL bRedraw )
{
    SendMessageW( hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), MAKELPARAM(static_cast<UINT>(bRedraw),0) );
}

inline HFONT GetWindowFont( HWND hWnd )
{
    return reinterpret_cast<HFONT>(SendMessageW( hWnd, WM_GETFONT, 0, 0 ));
}

inline void SetClassCursor( HWND hWnd, HCURSOR hCursor )
{
    SetClassLongPtr( hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(hCursor) );
}

inline HCURSOR GetClassCursor( HWND hWnd )
{
    return reinterpret_cast<HCURSOR>(GetClassLongPtr( hWnd, GCLP_HCURSOR ));
}

inline void SetClassIcon( HWND hWnd, HICON hIcon )
{
    SetClassLongPtr( hWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hIcon) );
}

inline HICON GetClassIcon( HWND hWnd )
{
    return reinterpret_cast<HICON>(GetClassLongPtr( hWnd, GCLP_HICON ));
}

inline HBRUSH SetClassBrush( HWND hWnd, HBRUSH hBrush )
{
    return reinterpret_cast<HBRUSH>(SetClassLongPtr( hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(hBrush) ));
}

inline HBRUSH GetClassBrush( HWND hWnd )
{
    return reinterpret_cast<HBRUSH>(GetClassLongPtr( hWnd, GCLP_HBRBACKGROUND ));
}

inline HINSTANCE GetWindowInstance( HWND hWnd )
{
    return reinterpret_cast<HINSTANCE>(GetWindowLongPtrW( hWnd, GWLP_HINSTANCE ));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

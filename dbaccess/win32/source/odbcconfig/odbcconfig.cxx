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


#include <sal/config.h>

#include <systools/win32/odbccp32.hxx>

// displays the error text for the last error (GetLastError), and returns this error value
static int displayLastError()
{
    DWORD   dwError = GetLastError();

    LPVOID lpMsgBuf = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        reinterpret_cast<LPWSTR>(&lpMsgBuf),
        0,
        nullptr
    );

    // Display the string.
    MessageBoxW( nullptr, static_cast<LPCWSTR>(lpMsgBuf), nullptr, MB_OK | MB_ICONERROR );

    // Free the buffer.
    HeapFree( GetProcessHeap(), 0, lpMsgBuf );

    return dwError;
}

/** registers the window class for our application's main window
*/
static bool registerWindowClass( HINSTANCE _hAppInstance )
{
    WNDCLASSEXW wcx;

    wcx.cbSize = sizeof(wcx);                   // size of structure
    wcx.style = CS_HREDRAW | CS_VREDRAW;        // redraw if size changes
    wcx.lpfnWndProc = DefWindowProcW;           // points to window procedure
    wcx.cbClsExtra = 0;                         // no extra class memory
    wcx.cbWndExtra = 0;                         // no extra window memory
    wcx.hInstance = _hAppInstance;              // handle to instance
    wcx.hIcon = nullptr;                        // predefined app. icon
    wcx.hCursor = nullptr;                      // predefined arrow
    wcx.hbrBackground = nullptr;                // no background brush
    wcx.lpszMenuName =  nullptr;                // name of menu resource
    wcx.lpszClassName = L"ODBCConfigMainClass"; // name of window class
    wcx.hIconSm = nullptr;                      // small class icon

    return ( !!RegisterClassExW( &wcx ) );
}

/// initializes the application instances
static HWND initInstance( HINSTANCE _hAppInstance )
{
    HWND hWindow = CreateWindowW(
        L"ODBCConfigMainClass", // name of window class
        L"ODBC Config Wrapper", // title-bar string
        WS_OVERLAPPEDWINDOW,    // top-level window
        CW_USEDEFAULT,          // default horizontal position
        CW_USEDEFAULT,          // default vertical position
        CW_USEDEFAULT,          // default width
        CW_USEDEFAULT,          // default height
        nullptr,                // no owner window
        nullptr,                // use class menu
        _hAppInstance,          // handle to application instance
        nullptr);               // no window-creation data

    // don't show the window, we only need it as parent handle for the
    // SQLManageDataSources function
    return hWindow;
}

// main window function
extern "C" int APIENTRY wWinMain( HINSTANCE _hAppInstance, HINSTANCE, LPWSTR, int )
{
    if ( !registerWindowClass( _hAppInstance ) )
        return FALSE;

    HWND hAppWindow = initInstance( _hAppInstance );
    if ( !IsWindow( hAppWindow ) )
        return displayLastError();

    // Have a odbccp32 variable, to not call FreeLibrary before displayLastError
    sal::systools::odbccp32 odbccp32;
    if (!odbccp32.SQLManageDataSources(hAppWindow))
        return displayLastError();

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#if defined(_WIN32)                     // Windows
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#include <stdlib.h>
#include <fstream>
#include <string.h>


#ifdef UNX
#if defined( MACOSX )
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
# else
    extern char** environ;
# endif
#endif

#ifdef _WIN32
#   define SLEEP(t) (Sleep((t)*1000))
#else
#   define SLEEP(t) (sleep((t)))
#endif

static void wait_for_seconds(char* time)
{
    SLEEP(atoi(time));
}

#ifdef _WIN32

static void w_to_a(LPCWSTR strW, LPSTR strA, DWORD size)
{
    WideCharToMultiByte(CP_ACP, 0, strW, -1, strA, size, nullptr, nullptr);
}

    static void dump_env(char* file_path)
    {
        LPWSTR env = GetEnvironmentStringsW();
        LPWSTR p   = env;

        std::ofstream file(file_path);

        char buffer[32767];
        while (size_t l = wcslen(p))
        {
            w_to_a(p, buffer, sizeof(buffer));
            file << buffer << '\0';
            p += l + 1;
        }
        FreeEnvironmentStringsW(env);
    }
#else
    static void dump_env(char* file_path)
    {
        std::ofstream file(file_path);
        for (int i = 0; environ[i] != nullptr; ++i)
            file << environ[i] << '\0';
    }
#endif

int main(int argc, char* argv[])
{
    if (argc > 2)
    {
        if (strcmp("-join", argv[1]) == 0)
        {
            // coverity[tainted_data] - this is a build-time only test tool
            wait_for_seconds(argv[2]);
        }
        else if (strcmp("-env", argv[1]) == 0)
            dump_env(argv[2]);
    }

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

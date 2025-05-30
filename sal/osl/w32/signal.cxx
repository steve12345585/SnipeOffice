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

#include <stdlib.h>

#include <config_features.h>

#include <signalshared.hxx>

#include <systools/win32/uwinapi.h>
#include <errorrep.h>
#include <werapi.h>

namespace
{
long WINAPI signalHandlerFunction(LPEXCEPTION_POINTERS lpEP);

// Similar to SIGINT handler in sal/osl/unx/signal.cxx
BOOL WINAPI CtrlHandlerFunction(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            switch (oslSignalInfo Info{ osl_Signal_Terminate, 0, nullptr };
                    callSignalHandler(&Info))
            {
                case osl_Signal_ActCallNextHdl:
                    break; // Fall through to call the next handler

                case osl_Signal_ActAbortApp:
                    abort();
                    break;

                case osl_Signal_ActKillApp:
                    _exit(255);
                    break;

                default:
                    return TRUE; // do not call the next handler
            }
            [[fallthrough]];
        default:
            return FALSE; // call the next handler
    }
}

LPTOP_LEVEL_EXCEPTION_FILTER pPreviousHandler = nullptr;
}

bool onInitSignal()
{
    pPreviousHandler = SetUnhandledExceptionFilter(signalHandlerFunction);
    SetConsoleCtrlHandler(CtrlHandlerFunction, TRUE);

    WerAddExcludedApplication(L"SOFFICE.EXE", FALSE);

    return true;
}

bool onDeInitSignal()
{
    SetConsoleCtrlHandler(CtrlHandlerFunction, FALSE);
    SetUnhandledExceptionFilter(pPreviousHandler);

    return false;
}

namespace
{
/* magic Microsoft C++ compiler exception constant */
#define EXCEPTION_MSC_CPP_EXCEPTION 0xe06d7363

long WINAPI signalHandlerFunction(LPEXCEPTION_POINTERS lpEP)
{
#if HAVE_FEATURE_BREAKPAD
    // we should make sure to call the breakpad handler as
    // first step when we hit a problem
    if (pPreviousHandler)
        pPreviousHandler(lpEP);
#endif

    static bool bNested = false;

    oslSignalInfo info;

    info.UserSignal = lpEP->ExceptionRecord->ExceptionCode;
    info.UserData = nullptr;

    switch (lpEP->ExceptionRecord->ExceptionCode)
    {
        /* Transform unhandled exceptions into access violations.
           Microsoft C++ compiler (add more for other compilers if necessary).
         */
        case EXCEPTION_MSC_CPP_EXCEPTION:
        case EXCEPTION_ACCESS_VIOLATION:
            info.Signal = osl_Signal_AccessViolation;
            break;

        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            info.Signal = osl_Signal_IntegerDivideByZero;
            break;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            info.Signal = osl_Signal_FloatDivideByZero;
            break;

        case EXCEPTION_BREAKPOINT:
            info.Signal = osl_Signal_DebugBreak;
            break;

        default:
            info.Signal = osl_Signal_System;
            break;
    }

    oslSignalAction action;

    if (!bNested)
    {
        bNested = true;
        action = callSignalHandler(&info);
    }
    else
        action = osl_Signal_ActKillApp;

    switch (action)
    {
        case osl_Signal_ActCallNextHdl:
            return EXCEPTION_CONTINUE_SEARCH;

        case osl_Signal_ActAbortApp:
            return EXCEPTION_EXECUTE_HANDLER;

        case osl_Signal_ActKillApp:
            SetErrorMode(SEM_NOGPFAULTERRORBOX);
            exit(255);
            break;
        default:
            break;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

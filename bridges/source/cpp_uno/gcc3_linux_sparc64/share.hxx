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

#include "uno/mapping.h"
#include <typeinfo>
#include <exception>
#include <cstddef>
namespace CPPU_CURRENT_NAMESPACE
{
void dummy_can_throw_anything( char const * );
// ----- following decl from libstdc++-v3/libsupc++/unwind-cxx.h and unwind.h

struct _Unwind_Exception
{
    unsigned exception_class __attribute__((__mode__(__DI__)));
    void * exception_cleanup;
    unsigned private_1 __attribute__((__mode__(__word__)));
    unsigned private_2 __attribute__((__mode__(__word__)));
} __attribute__((__aligned__));

struct __cxa_exception
{
    std::type_info *exceptionType;
    void (*exceptionDestructor)(void *);

    void (*unexpectedHandler)(); // std::unexpected_handler dropped from C++17
    std::terminate_handler terminateHandler;

    __cxa_exception *nextException;

    int handlerCount;

    int handlerSwitchValue;
    const unsigned char *actionRecord;
    const unsigned char *languageSpecificData;
    void *catchTemp;
    void *adjustedPtr;

    _Unwind_Exception unwindHeader;
};

extern "C" void *__cxa_allocate_exception(
    std::size_t thrown_size ) throw();
extern "C" void __cxa_throw (
    void *thrown_exception, std::type_info *tinfo, void (*dest) (void *) ) __attribute__((noreturn));

struct __cxa_eh_globals
{
    __cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
};
extern "C" __cxa_eh_globals *__cxa_get_globals () throw();
extern "C" std::type_info *__cxa_current_exception_type() throw();

void raiseException(
    uno_Any * pUnoExc, uno_Mapping * pUno2Cpp );

void fillUnoException(uno_Any *, uno_Mapping * pCpp2Uno);

bool return_in_hidden_param( typelib_TypeDescriptionReference *pTypeRef );

inline char* adjustPointer( char* pIn, typelib_TypeDescription* pType )
{
    switch( pType->nSize )
    {
        case 1: return pIn + 7;
        case 2: return pIn + 6;
        case 3: return pIn + 5;
        case 4: return pIn + 4;
        case 5: return pIn + 3;
        case 6: return pIn + 2;
        case 7: return pIn + 1;
            // Huh ? perhaps a char[3] ? Though that would be a pointer
            // well, we have it anyway for symmetry
    }
    return pIn;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <typeinfo>
#include <exception>
#include <cstddef>

#include <cxxabi.h>
#ifndef _GLIBCXX_CDTOR_CALLABI // new in GCC 4.7 cxxabi.h
#define _GLIBCXX_CDTOR_CALLABI
#endif
#include <unwind.h>

#include <config_cxxabi.h>
#include <uno/any2.h>
#include <uno/mapping.h>

#if !HAVE_CXXABI_H_CLASS_TYPE_INFO
// <https://mentorembedded.github.io/cxx-abi/abi.html>,
// libstdc++-v3/libsupc++/cxxabi.h:
namespace __cxxabiv1 {
class __class_type_info: public std::type_info {
public:
    explicit __class_type_info(char const * n): type_info(n) {}
    ~__class_type_info() override;
};
}
#endif

#if !HAVE_CXXABI_H_SI_CLASS_TYPE_INFO
// <https://mentorembedded.github.io/cxx-abi/abi.html>,
// libstdc++-v3/libsupc++/cxxabi.h:
namespace __cxxabiv1 {
class __si_class_type_info: public __class_type_info {
public:
    __class_type_info const * __base_type;
    explicit __si_class_type_info(
        char const * n, __class_type_info const *base):
        __class_type_info(n), __base_type(base) {}
    ~__si_class_type_info() override;
};
}
#endif

namespace CPPU_CURRENT_NAMESPACE
{

void dummy_can_throw_anything( char const * );

// ----- following decl from libstdc++-v3/libsupc++/unwind-cxx.h

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

struct __cxa_eh_globals
{
    __cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
};

}

// __cxa_get_globals is exported from libstdc++ since GCC 3.4.0 (CXXABI_1.3),
// but it is only declared in cxxabi.h (in namespace __cxxabiv1) since
// GCC 4.7.0.  It returns a pointer to a struct __cxa_eh_globals, but that
// struct is only incompletely declared even in the GCC 4.7.0 cxxabi.h.
// Therefore, provide a declaration here for old GCC (libstdc++, really) version
// that returns a void pointer, and in the code calling it always cast to the
// above fake definition of CPPU_CURRENT_NAMESPACE::__cxa_eh_globals (which
// hopefully keeps matching the real definition in libstdc++); similarly for
// __cxa_allocate_exception and __cxa_throw, though they do not have the
// additional problem of an incompletely declared return type:

#if !HAVE_CXXABI_H_CXA_GET_GLOBALS
namespace __cxxabiv1 { extern "C" void * __cxa_get_globals() throw(); }
#endif

#if !HAVE_CXXABI_H_CXA_CURRENT_EXCEPTION_TYPE
namespace __cxxabiv1 {
extern "C" std::type_info *__cxa_current_exception_type() throw();
}
#endif

#if !HAVE_CXXABI_H_CXA_ALLOCATE_EXCEPTION
namespace __cxxabiv1 {
extern "C" void * __cxa_allocate_exception(std::size_t thrown_size) throw();
}
#endif

#if !HAVE_CXXABI_H_CXA_THROW
namespace __cxxabiv1 {
extern "C" void __cxa_throw(
    void * thrown_exception, void * tinfo, void (* dest)(void *))
    __attribute__((noreturn));
}
#endif

extern "C" void privateSnippetExecutorGeneral();
extern "C" void privateSnippetExecutorVoid();
extern "C" void privateSnippetExecutorHyper();
extern "C" void privateSnippetExecutorFloat();
extern "C" void privateSnippetExecutorDouble();
extern "C" void privateSnippetExecutorClass();

namespace CPPU_CURRENT_NAMESPACE
{

void raiseException(
    uno_Any * pUnoExc, uno_Mapping * pUno2Cpp );

void fillUnoException(uno_Any *, uno_Mapping * pCpp2Uno);

}

namespace x86
{
    bool isSimpleReturnType(typelib_TypeDescription * pTD, bool recursive = false);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

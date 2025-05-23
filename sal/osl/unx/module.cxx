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

#include <sal/log.hxx>
#include <sal/types.h>
#include <osl/module.h>
#include <osl/thread.h>
#include <osl/file.h>
#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <assert.h>
#include <dlfcn.h>
#include <limits.h>
#include "file_url.hxx"

static bool getModulePathFromAddress(void * address, rtl_String ** path)
{
    bool result = false;
#if HAVE_UNIX_DLAPI
    Dl_info dl_info;

    result = dladdr(address, &dl_info) != 0;

    if (result)
    {
        rtl_string_newFromStr(path, dl_info.dli_fname);
    }
#else
    (void) address;
    (void) path;
#endif
    return result;
}

#ifndef DISABLE_DYNLOADING

/*****************************************************************************/
/* osl_loadModule */
/*****************************************************************************/

oslModule SAL_CALL osl_loadModule(rtl_uString *ustrModuleName, sal_Int32 nRtldMode)
{
    oslModule pModule=nullptr;
    rtl_uString* ustrTmp = nullptr;

    SAL_WARN_IF(ustrModuleName == nullptr, "sal.osl", "string is not valid");

    /* ensure ustrTmp hold valid string */
    if (osl_getSystemPathFromFileURL(ustrModuleName, &ustrTmp) != osl_File_E_None)
        rtl_uString_assign(&ustrTmp, ustrModuleName);

    if (ustrTmp)
    {
        char buffer[PATH_MAX];

        if (UnicodeToText(buffer, PATH_MAX, ustrTmp->buffer, ustrTmp->length))
            pModule = osl_loadModuleAscii(buffer, nRtldMode);
        rtl_uString_release(ustrTmp);
    }

    return pModule;
}

/*****************************************************************************/
/* osl_loadModuleAscii */
/*****************************************************************************/

oslModule SAL_CALL osl_loadModuleAscii(const char *pModuleName, sal_Int32 nRtldMode)
{
#if HAVE_UNIX_DLAPI
    SAL_WARN_IF(
        ((nRtldMode & SAL_LOADMODULE_LAZY) != 0
         && (nRtldMode & SAL_LOADMODULE_NOW) != 0),
        "sal.osl", "only either LAZY or NOW");
    if (pModuleName)
    {
        int rtld_mode =
            ((nRtldMode & SAL_LOADMODULE_NOW) ? RTLD_NOW : RTLD_LAZY) |
            ((nRtldMode & SAL_LOADMODULE_GLOBAL) ? RTLD_GLOBAL : RTLD_LOCAL);
        void* pLib = dlopen(pModuleName, rtld_mode);

        SAL_WARN_IF(
            pLib == nullptr, "sal.osl",
            "dlopen(" << pModuleName << ", " << rtld_mode << "): "
                << dlerror());
        return pLib;
    }
#else
    (void) pModuleName;
    (void) nRtldMode;
#endif
    return nullptr;
}

oslModule osl_loadModuleRelativeAscii(
    oslGenericFunction baseModule, char const * relativePath, sal_Int32 mode)
{
    assert(relativePath && "illegal argument");
    if (relativePath[0] == '/') {
        return osl_loadModuleAscii(relativePath, mode);
    }
    rtl_String * path = nullptr;
    rtl_String * suffix = nullptr;
    oslModule module;
    if (!getModulePathFromAddress(
            reinterpret_cast< void * >(baseModule), &path))
    {
        return nullptr;
    }
    rtl_string_newFromStr_WithLength(
        &path, path->buffer,
        (rtl_str_lastIndexOfChar_WithLength(path->buffer, path->length, '/')
         + 1));
        /* cut off everything after the last slash; should the original path
           contain no slash, the resulting path is the empty string */
    rtl_string_newFromStr(&suffix, relativePath);
    rtl_string_newConcat(&path, path, suffix);
    rtl_string_release(suffix);
    module = osl_loadModuleAscii(path->buffer, mode);
    rtl_string_release(path);
    return module;
}

#endif // !DISABLE_DYNLOADING

/*****************************************************************************/
/* osl_getModuleHandle */
/*****************************************************************************/

sal_Bool SAL_CALL
osl_getModuleHandle(rtl_uString *, oslModule *pResult)
{
#if HAVE_UNIX_DLAPI
    *pResult = static_cast<oslModule>(RTLD_DEFAULT);
    return true;
#else
    *pResult = nullptr;
    return false;
#endif
}

/*****************************************************************************/
/* osl_unloadModule */
/*****************************************************************************/
void SAL_CALL osl_unloadModule(oslModule hModule)
{
#if !defined(DISABLE_DYNLOADING) && HAVE_UNIX_DLAPI
    if (hModule)
    {
        int nRet = dlclose(hModule);
        SAL_INFO_IF(
            nRet != 0, "sal.osl", "dlclose(" << hModule << "): " << dlerror());
    }
#else
    (void) hModule;
#endif
}

namespace {

void * getSymbol(oslModule module, char const * symbol)
{
    assert(symbol != nullptr);
#if HAVE_UNIX_DLAPI
    // We do want to use dlsym() also in the DISABLE_DYNLOADING case
    // just to look up symbols in the static executable, I think:
    void * p = dlsym(module, symbol);
    SAL_INFO_IF(
        p == nullptr, "sal.osl",
        "dlsym(" << module << ", " << symbol << "): " << dlerror());
#else
    (void) module;
    (void) symbol;
    void *p = nullptr;
#endif
    return p;
}

}

/*****************************************************************************/
/* osl_getSymbol */
/*****************************************************************************/
void* SAL_CALL
osl_getSymbol(oslModule Module, rtl_uString* pSymbolName)
{
    // Arbitrarily using UTF-8:
    OString s;
    if (!OUString::unacquired(&pSymbolName).convertToString(
            &s, RTL_TEXTENCODING_UTF8,
            (RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR |
             RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR)))
    {
        SAL_INFO(
            "sal.osl", "cannot convert \"" << OUString::unacquired(&pSymbolName)
                << "\" to UTF-8");
        return nullptr;
    }
    if (s.indexOf('\0') != -1) {
        SAL_INFO("sal.osl", "\"" << s << "\" contains embedded NUL");
        return nullptr;
    }
    return getSymbol(Module, s.getStr());
}

/*****************************************************************************/
/* osl_getAsciiFunctionSymbol */
/*****************************************************************************/
oslGenericFunction SAL_CALL
osl_getAsciiFunctionSymbol(oslModule Module, const char *pSymbol)
{
    return reinterpret_cast<oslGenericFunction>(getSymbol(Module, pSymbol));
        // requires conditionally-supported conversion from void * to function
        // pointer
}

/*****************************************************************************/
/* osl_getFunctionSymbol */
/*****************************************************************************/
oslGenericFunction SAL_CALL
osl_getFunctionSymbol(oslModule module, rtl_uString *puFunctionSymbolName)
{
    return reinterpret_cast<oslGenericFunction>(
        osl_getSymbol(module, puFunctionSymbolName));
        // requires conditionally-supported conversion from void * to function
        // pointer
}

/*****************************************************************************/
/* osl_getModuleURLFromAddress */
/*****************************************************************************/
sal_Bool SAL_CALL osl_getModuleURLFromAddress(void * addr, rtl_uString ** ppLibraryUrl)
{
    bool result = false;
    rtl_String * path = nullptr;
    if (getModulePathFromAddress(addr, &path))
    {
        rtl_string2UString(ppLibraryUrl,
                           path->buffer,
                           path->length,
                           osl_getThreadTextEncoding(),
                           OSTRING_TO_OUSTRING_CVTFLAGS);

        SAL_WARN_IF(
            *ppLibraryUrl == nullptr, "sal.osl", "rtl_string2UString failed");
        auto const e = osl_getFileURLFromSystemPath(*ppLibraryUrl, ppLibraryUrl);
        if (e == osl_File_E_None)
        {
            SAL_INFO("sal.osl", "osl_getModuleURLFromAddress(" << addr << ") => " << OUString(*ppLibraryUrl));

            result = true;
        }
        else
        {
            SAL_WARN(
                "sal.osl",
                "osl_getModuleURLFromAddress(" << addr << "), osl_getFileURLFromSystemPath("
                    << OUString::unacquired(ppLibraryUrl) << ") failed with " << e);
            result = false;
        }
        rtl_string_release(path);
    }
    return result;
}

/*****************************************************************************/
/* osl_getModuleURLFromFunctionAddress */
/*****************************************************************************/
sal_Bool SAL_CALL osl_getModuleURLFromFunctionAddress(oslGenericFunction addr, rtl_uString ** ppLibraryUrl)
{
    return osl_getModuleURLFromAddress(
        reinterpret_cast<void*>(addr), ppLibraryUrl);
        // requires conditionally-supported conversion from function pointer to
        // void *
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

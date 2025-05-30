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

#include <config_features.h>

#include <osl/process.h>

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <osl/diagnose.h>
#include <osl/file.hxx>
#include <osl/module.h>
#include <osl/thread.h>
#include <rtl/alloc.h>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>

#include "file_path_helper.hxx"

#include "uunxapi.hxx"
#include "nlsupport.hxx"

#ifdef ANDROID
#include <osl/detail/android-bootstrap.h>
#endif

#if defined(MACOSX) || defined(IOS)
#include <mach-o/dyld.h>

namespace {

oslProcessError bootstrap_getExecutableFile(rtl_uString ** ppFileURL)
{
    oslProcessError result = osl_Process_E_NotFound;

    char   buffer[PATH_MAX];
    uint32_t buflen = sizeof(buffer);

    if (_NSGetExecutablePath (buffer, &buflen) == 0)
    {
        /* Determine absolute path. */
        char abspath[PATH_MAX];
        if (realpath (buffer, abspath) != nullptr)
        {
            /* Convert from utf8 to unicode. */
            rtl_uString * pAbsPath = nullptr;
            rtl_string2UString (
                &pAbsPath,
                abspath, rtl_str_getLength (abspath),
                RTL_TEXTENCODING_UTF8,
                OSTRING_TO_OUSTRING_CVTFLAGS);

            if (pAbsPath)
            {
                /* Convert from path to url. */
                if (osl_getFileURLFromSystemPath (pAbsPath, ppFileURL) == osl_File_E_None)
                {
                    /* Success. */
                    result = osl_Process_E_None;
                }
                rtl_uString_release (pAbsPath);
            }
        }
    }

    return result;
}

}

#else
#include <dlfcn.h>

namespace {

oslProcessError bootstrap_getExecutableFile(rtl_uString ** ppFileURL)
{
#ifdef EMSCRIPTEN
    // Just return some dummy file: URL for now to see what happens
    OUString fileURL = "vnd.sun.star.pathname:/instdir/program/soffice";
    rtl_uString_acquire(fileURL.pData);
    *ppFileURL = fileURL.pData;
    return osl_Process_E_None;
#else
    oslProcessError result = osl_Process_E_NotFound;

#ifdef ANDROID
    /* Now with just a single DSO, this one from lo-bootstrap.c is as good as
     * any */
    void * addr = dlsym (RTLD_DEFAULT, "JNI_OnLoad");
#else
#if defined __linux
    // The below code looking for "main" with dlsym() will typically
    // fail, as there is little reason for "main" to be exported, in
    // the dlsym() sense, from an executable. But Linux has
    // /proc/self/exe, try using that.
    char buf[PATH_MAX];
    int rc = readlink("/proc/self/exe", buf, sizeof(buf));
    if (rc > 0 && rc < PATH_MAX)
    {
        buf[rc] = '\0';
        OUString path = OUString::fromUtf8(buf);
        OUString fileURL;
        if (osl::File::getFileURLFromSystemPath(path, fileURL) == osl::File::E_None)
        {
            rtl_uString_acquire(fileURL.pData);
            *ppFileURL = fileURL.pData;
            return osl_Process_E_None;
        }
    }
#endif
    /* Determine address of "main()" function. */
    void * addr = dlsym (RTLD_DEFAULT, "main");
#endif
    if (addr != nullptr)
    {
        /* Determine module URL. */
        if (osl_getModuleURLFromAddress (addr, ppFileURL))
        {
            /* Success. */
            result = osl_Process_E_None;
        }
    }

    return result;
#endif
}

}

#endif

namespace {

struct CommandArgs_Impl
{
    pthread_mutex_t m_mutex;
    sal_uInt32      m_nCount;
    rtl_uString **  m_ppArgs;
};

}

static struct CommandArgs_Impl g_command_args =
{
    PTHREAD_MUTEX_INITIALIZER,
    0,
    nullptr
};

oslProcessError SAL_CALL osl_getExecutableFile (rtl_uString ** ppustrFile)
{
    pthread_mutex_lock (&(g_command_args.m_mutex));
    if (g_command_args.m_nCount == 0)
    {
        pthread_mutex_unlock (&(g_command_args.m_mutex));
        return bootstrap_getExecutableFile(ppustrFile);
    }

    /* CommandArgs set. Obtain argv[0]. */
    rtl_uString_assign (ppustrFile, g_command_args.m_ppArgs[0]);
    pthread_mutex_unlock (&(g_command_args.m_mutex));
    return osl_Process_E_None;
}

sal_uInt32 SAL_CALL osl_getCommandArgCount()
{
    sal_uInt32 result = 0;

    pthread_mutex_lock (&(g_command_args.m_mutex));
    SAL_INFO_IF(
        g_command_args.m_nCount == 0, "sal.osl",
        "osl_getCommandArgCount w/o prior call to osl_setCommandArgs");
    if (g_command_args.m_nCount > 0)
        result = g_command_args.m_nCount - 1;
    pthread_mutex_unlock (&(g_command_args.m_mutex));

    return result;
}

oslProcessError SAL_CALL osl_getCommandArg (sal_uInt32 nArg, rtl_uString ** strCommandArg)
{
    oslProcessError result = osl_Process_E_NotFound;

    pthread_mutex_lock (&(g_command_args.m_mutex));
    assert(g_command_args.m_nCount > 0);
    if (g_command_args.m_nCount > (nArg + 1))
    {
        rtl_uString_assign (strCommandArg, g_command_args.m_ppArgs[nArg + 1]);
        result = osl_Process_E_None;
    }
    pthread_mutex_unlock (&(g_command_args.m_mutex));

    return result;
}

void SAL_CALL osl_setCommandArgs (int argc, char ** argv)
{
    assert(argc > 0);
    pthread_mutex_lock (&(g_command_args.m_mutex));
    SAL_WARN_IF(g_command_args.m_nCount != 0, "sal.osl", "args already set");
    if (g_command_args.m_nCount == 0)
    {
        rtl_uString** ppArgs = static_cast<rtl_uString**>(rtl_allocateZeroMemory (argc * sizeof(rtl_uString*)));
        if (ppArgs != nullptr)
        {
            rtl_TextEncoding encoding = osl_getThreadTextEncoding();
            for (int i = 0; i < argc; i++)
            {
                rtl_string2UString (
                    &(ppArgs[i]),
                    argv[i], rtl_str_getLength (argv[i]), encoding,
                    OSTRING_TO_OUSTRING_CVTFLAGS);
            }
            if (ppArgs[0] != nullptr)
            {
#if HAVE_FEATURE_MACOSX_SANDBOX
                // If we are called with a relative path in argv[0] in a sandboxed process
                // osl::realpath() fails. So just use bootstrap_getExecutableFile() instead.
                // Somewhat silly to use argv[0] and tediously figure out the absolute path from it
                // anyway.
                bootstrap_getExecutableFile(&ppArgs[0]);
                OUString pArg0(ppArgs[0]);
                osl_getFileURLFromSystemPath (pArg0.pData, &(ppArgs[0]));
#else
#if !defined(ANDROID) && !defined(IOS) // No use searching PATH on Android or iOS
                /* see @ osl_getExecutableFile(). */
                if (rtl_ustr_indexOfChar (rtl_uString_getStr(ppArgs[0]), '/') == -1)
                {
                    rtl_uString * pSearchPath = nullptr;
                    osl_getEnvironment (u"PATH"_ustr.pData, &pSearchPath);
                    if (pSearchPath)
                    {
                        rtl_uString * pSearchResult = nullptr;
                        osl_searchPath (ppArgs[0], pSearchPath, &pSearchResult);
                        if (pSearchResult)
                        {
                            rtl_uString_assign (&(ppArgs[0]), pSearchResult);
                            rtl_uString_release (pSearchResult);
                        }
                        rtl_uString_release (pSearchPath);
                    }
                }
#endif
                OUString pArg0;
                if (osl::realpath (OUString::unacquired(&ppArgs[0]), pArg0))
                {
                    osl_getFileURLFromSystemPath (pArg0.pData, &(ppArgs[0]));
                }
#endif // !HAVE_FEATURE_MACOSX_SANDBOX
            }
            g_command_args.m_nCount = argc;
            g_command_args.m_ppArgs = ppArgs;
        }
    }
    pthread_mutex_unlock (&(g_command_args.m_mutex));
}

oslProcessError SAL_CALL osl_getEnvironment(rtl_uString* pustrEnvVar, rtl_uString** ppustrValue)
{
    oslProcessError  result   = osl_Process_E_NotFound;
    rtl_TextEncoding encoding = osl_getThreadTextEncoding();
    rtl_String* pstr_env_var  = nullptr;

    OSL_PRECOND(pustrEnvVar, "osl_getEnvironment(): Invalid parameter");
    OSL_PRECOND(ppustrValue, "osl_getEnvironment(): Invalid parameter");

    rtl_uString2String(
        &pstr_env_var,
        rtl_uString_getStr(pustrEnvVar), rtl_uString_getLength(pustrEnvVar), encoding,
        OUSTRING_TO_OSTRING_CVTFLAGS);
    if (pstr_env_var != nullptr)
    {
        const char* p_env_var = getenv (rtl_string_getStr (pstr_env_var));
        if (p_env_var != nullptr)
        {
            rtl_string2UString(
                ppustrValue,
                p_env_var, strlen(p_env_var), encoding,
                OSTRING_TO_OUSTRING_CVTFLAGS);
            OSL_ASSERT(*ppustrValue != nullptr);

            result = osl_Process_E_None;
        }
        rtl_string_release(pstr_env_var);
    }

    return result;
}

oslProcessError SAL_CALL osl_setEnvironment(rtl_uString* pustrEnvVar, rtl_uString* pustrValue)
{
    oslProcessError  result   = osl_Process_E_Unknown;
    rtl_TextEncoding encoding = osl_getThreadTextEncoding();
    rtl_String* pstr_env_var  = nullptr;
    rtl_String* pstr_val  = nullptr;

    OSL_PRECOND(pustrEnvVar, "osl_setEnvironment(): Invalid parameter");
    OSL_PRECOND(pustrValue, "osl_setEnvironment(): Invalid parameter");

    rtl_uString2String(
        &pstr_env_var,
        rtl_uString_getStr(pustrEnvVar), rtl_uString_getLength(pustrEnvVar), encoding,
        OUSTRING_TO_OSTRING_CVTFLAGS);

    rtl_uString2String(
        &pstr_val,
        rtl_uString_getStr(pustrValue), rtl_uString_getLength(pustrValue), encoding,
        OUSTRING_TO_OSTRING_CVTFLAGS);

    if (pstr_env_var != nullptr && pstr_val != nullptr)
    {
#if defined (__sun)
        rtl_String * pBuffer = NULL;

        sal_Int32 nCapacity = rtl_stringbuffer_newFromStringBuffer( &pBuffer,
            rtl_string_getLength(pstr_env_var) + rtl_string_getLength(pstr_val) + 1,
            pstr_env_var );
        rtl_stringbuffer_insert( &pBuffer, &nCapacity, pBuffer->length, "=", 1);
        rtl_stringbuffer_insert( &pBuffer, &nCapacity, pBuffer->length,
            rtl_string_getStr(pstr_val), rtl_string_getLength(pstr_val) );

        rtl_string_acquire(pBuffer); // argument to putenv must leak on success

        if (putenv(rtl_string_getStr(pBuffer)) == 0)
            result = osl_Process_E_None;
        else
            rtl_string_release(pBuffer);
#else
        if (setenv(rtl_string_getStr(pstr_env_var), rtl_string_getStr(pstr_val), 1) == 0)
            result = osl_Process_E_None;
#endif
    }

    if (pstr_val)
        rtl_string_release(pstr_val);

    if (pstr_env_var != nullptr)
        rtl_string_release(pstr_env_var);

    return result;
}

oslProcessError SAL_CALL osl_clearEnvironment(rtl_uString* pustrEnvVar)
{
    oslProcessError  result   = osl_Process_E_Unknown;
    rtl_TextEncoding encoding = osl_getThreadTextEncoding();
    rtl_String* pstr_env_var  = nullptr;

    OSL_PRECOND(pustrEnvVar, "osl_setEnvironment(): Invalid parameter");

    rtl_uString2String(
        &pstr_env_var,
        rtl_uString_getStr(pustrEnvVar), rtl_uString_getLength(pustrEnvVar), encoding,
        OUSTRING_TO_OSTRING_CVTFLAGS);

    if (pstr_env_var)
    {
#if defined (__sun)
        rtl_String * pBuffer = NULL;

        sal_Int32 nCapacity = rtl_stringbuffer_newFromStringBuffer( &pBuffer,
            rtl_string_getLength(pstr_env_var) + 1, pstr_env_var );
        rtl_stringbuffer_insert( &pBuffer, &nCapacity, pBuffer->length, "=", 1);

        rtl_string_acquire(pBuffer); // argument to putenv must leak on success

        if (putenv(rtl_string_getStr(pBuffer)) == 0)
            result = osl_Process_E_None;
        else
            rtl_string_release(pBuffer);
#elif (defined(MACOSX) || defined(NETBSD) || defined(FREEBSD))
        // MacOSX baseline is 10.4, which has an old-school void return
        // for unsetenv.
        // See: http://developer.apple.com/mac/library/documentation/Darwin/Reference/ManPages/10.4/man3/unsetenv.3.html?useVersion=10.4
        unsetenv(rtl_string_getStr(pstr_env_var));
        result = osl_Process_E_None;
#else
        if (unsetenv(rtl_string_getStr(pstr_env_var)) == 0)
            result = osl_Process_E_None;
#endif
        rtl_string_release(pstr_env_var);
    }

    return result;
}

oslProcessError SAL_CALL osl_getProcessWorkingDir(rtl_uString **ppustrWorkingDir)
{
    oslProcessError result = osl_Process_E_Unknown;
    char buffer[PATH_MAX];

    OSL_PRECOND(ppustrWorkingDir, "osl_getProcessWorkingDir(): Invalid parameter");

    if (getcwd (buffer, sizeof(buffer)) != nullptr)
    {
        rtl_uString* ustrTmp = nullptr;

        rtl_string2UString(
            &ustrTmp,
            buffer, strlen(buffer), osl_getThreadTextEncoding(),
            OSTRING_TO_OUSTRING_CVTFLAGS);
        if (ustrTmp != nullptr)
        {
            if (osl_getFileURLFromSystemPath (ustrTmp, ppustrWorkingDir) == osl_File_E_None)
                result = osl_Process_E_None;
            rtl_uString_release (ustrTmp);
        }
    }

    return result;
}

namespace {

struct ProcessLocale_Impl
{
    pthread_mutex_t m_mutex;
    rtl_Locale *    m_pLocale;
};

}

static struct ProcessLocale_Impl g_process_locale =
{
    PTHREAD_MUTEX_INITIALIZER,
    nullptr
};

oslProcessError SAL_CALL osl_getProcessLocale( rtl_Locale ** ppLocale )
{
    oslProcessError result = osl_Process_E_Unknown;
    OSL_PRECOND(ppLocale, "osl_getProcessLocale(): Invalid parameter.");
    if (ppLocale)
    {
        pthread_mutex_lock(&(g_process_locale.m_mutex));

        if (g_process_locale.m_pLocale == nullptr)
            imp_getProcessLocale (&(g_process_locale.m_pLocale));
        *ppLocale = g_process_locale.m_pLocale;
        result = osl_Process_E_None;

        pthread_mutex_unlock (&(g_process_locale.m_mutex));
    }
    return result;
}

oslProcessError SAL_CALL osl_setProcessLocale( rtl_Locale * pLocale )
{
    OSL_PRECOND(pLocale, "osl_setProcessLocale(): Invalid parameter.");

    pthread_mutex_lock(&(g_process_locale.m_mutex));
    g_process_locale.m_pLocale = pLocale;
    pthread_mutex_unlock (&(g_process_locale.m_mutex));

    return osl_Process_E_None;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

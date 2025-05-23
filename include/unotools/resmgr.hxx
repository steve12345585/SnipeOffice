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
#ifndef INCLUDED_UNOTOOLS_RESMGR_HXX
#define INCLUDED_UNOTOOLS_RESMGR_HXX

#include <config_options.h>
#include <unotools/unotoolsdllapi.h>
#include <unotools/syslocale.hxx>
#include <rtl/ustring.hxx>
#include <string_view>

class LanguageTag;

typedef OUString (*ResHookProc)(const OUString& rStr);

struct UNOTOOLS_DLLPUBLIC TranslateId
{
    const char* mpContext;
    const char8_t* mpId;

    inline constexpr TranslateId()
        : mpContext(nullptr), mpId(nullptr) {}
    inline TranslateId(const char* pContext, const char* pId)
        : mpContext(pContext), mpId(reinterpret_cast<char8_t const *>(pId)) {}
    inline constexpr TranslateId(const char* pContext, const char8_t* pId)
        : mpContext(pContext), mpId(pId) {}

    char const * getId() const { return reinterpret_cast<char const *>(mpId); }

    inline operator bool() const { return mpId != nullptr; }

    bool operator==(const TranslateId& other) const;
    inline bool operator!=(const TranslateId& other) const { return !operator==(other); }
};

struct UNLESS_MERGELIBS(UNOTOOLS_DLLPUBLIC) TranslateNId
{
    const char* mpContext;
    const char* mpSingular;
    const char* mpPlural;

    inline TranslateNId()
        : mpContext(nullptr), mpSingular(nullptr), mpPlural(nullptr) {}
    inline TranslateNId(const char* pContext, const char* pSingular, const char* pPlural)
        : mpContext(pContext), mpSingular(pSingular), mpPlural(pPlural) {}

    inline operator bool() const { return mpContext != nullptr; }

    bool operator==(const TranslateNId& other) const;
    inline bool operator!=(const TranslateNId& other) const { return !operator==(other); }
};

namespace Translate
{
    UNOTOOLS_DLLPUBLIC std::locale Create(std::string_view aPrefixName, const LanguageTag& rLocale = SvtSysLocale().GetUILanguageTag());
    UNOTOOLS_DLLPUBLIC OUString get(TranslateId sContextAndId, const std::locale &loc);
    UNOTOOLS_DLLPUBLIC OUString getLanguage(const std::locale& loc);
    UNOTOOLS_DLLPUBLIC OUString nget(TranslateNId aContextSingularPlural, int n, const std::locale &loc);
    UNOTOOLS_DLLPUBLIC void SetReadStringHook( ResHookProc pProc );
    UNOTOOLS_DLLPUBLIC ResHookProc GetReadStringHook();
    UNOTOOLS_DLLPUBLIC OUString ExpandVariables(const OUString& rString);
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

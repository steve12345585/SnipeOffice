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

#include <memory>
#include <optional>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/i18n/ForbiddenCharacters.hpp>
#include <i18nlangtag/languagetag.hxx>
#include "swdllapi.h"

enum class SvtScriptType : sal_uInt8;
namespace com::sun::star::i18n { class XBreakIterator; }
namespace com::sun::star::uno { class XComponentContext; }

class SW_DLLPUBLIC SwBreakIt
{
    css::uno::Reference< css::uno::XComponentContext > m_xContext;
    css::uno::Reference<css::i18n::XBreakIterator> m_xBreak;

    std::unique_ptr<LanguageTag> m_xLanguageTag;   ///< language tag of the current locale
    std::optional<css::i18n::ForbiddenCharacters> m_oForbidden;

    LanguageType m_aForbiddenLang; ///< language of the current forbiddenChar struct

    void GetLocale_( const LanguageType aLang );
    void GetLocale_( const LanguageTag& rLanguageTag );
    void GetForbidden_( const LanguageType  aLang );

    SwBreakIt(SwBreakIt const&) = delete;
    SwBreakIt& operator=(SwBreakIt const&) = delete;

    // private (see @ Create_, Delete_).
    explicit SwBreakIt(css::uno::Reference<css::uno::XComponentContext> xContext);

public:
    // private (see @ source/core/bastyp/init.cxx).
    static void Create_(
        const css::uno::Reference< css::uno::XComponentContext > & rxContext);
    static void Delete_();

public:
    static SwBreakIt * Get();

    css::uno::Reference< css::i18n::XBreakIterator > const & GetBreakIter() const
    {
        return m_xBreak;
    }

    const css::lang::Locale& GetLocale( const LanguageType aLang )
    {
        if (!m_xLanguageTag || m_xLanguageTag->getLanguageType() != aLang)
            GetLocale_(aLang);
        return m_xLanguageTag->getLocale();
    }

    const css::lang::Locale& GetLocale( const LanguageTag& rLanguageTag )
    {
        // Use LanguageType comparison instead of LanguageTag::operator!=()
        // because here the LanguageTag is already a known LanguageType value
        // assigned, so LanguageTag does not need to convert to BCP47 for
        // comparison.
        if (!m_xLanguageTag || m_xLanguageTag->getLanguageType() != rLanguageTag.getLanguageType())
            GetLocale_(rLanguageTag);
        return m_xLanguageTag->getLocale();
    }

    const LanguageTag& GetLanguageTag( const LanguageType aLang )
    {
        if (!m_xLanguageTag || m_xLanguageTag->getLanguageType() != aLang)
            GetLocale_(aLang);
        return *m_xLanguageTag;
    }

    const LanguageTag& GetLanguageTag( const LanguageTag& rLanguageTag )
    {
        // Use LanguageType comparison instead of LanguageTag::operator!=()
        // because here the LanguageTag is already a known LanguageType value
        // assigned, so LanguageTag does not need to convert to BCP47 for
        // comparison.
        if (!m_xLanguageTag || m_xLanguageTag->getLanguageType() != rLanguageTag.getLanguageType())
            GetLocale_( rLanguageTag );
        return *m_xLanguageTag;
    }

    const css::i18n::ForbiddenCharacters& GetForbidden( const LanguageType aLang )
    {
        if (!m_oForbidden || m_aForbiddenLang != aLang)
            GetForbidden_( aLang );
        return *m_oForbidden;
    }

    sal_uInt16 GetRealScriptOfText( const OUString& rText, sal_Int32 nPos ) const;
    SvtScriptType GetAllScriptsOfText( const OUString& rText ) const;

    sal_Int32 getGraphemeCount(const OUString& rStr,
        sal_Int32 nStart, sal_Int32 nEnd) const;
    sal_Int32 getGraphemeCount(const OUString& rStr) const
    {
        return getGraphemeCount(rStr, 0, rStr.getLength());
    }
};

#define SW_BREAKITER()  SwBreakIt::Get()

// @@@ backward compatibility @@@
SW_DLLPUBLIC extern SwBreakIt* g_pBreakIt;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <iostream>
#include <mutex>
#include <string_view>

#include <osl/file.hxx>
#include <tools/debug.hxx>

#include <sal/config.h>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include "simpleguesser.hxx"
#include "guess.hxx"

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/linguistic2/XLanguageGuessing.hpp>
#include <unotools/pathoptions.hxx>
#include <osl/thread.h>

#include <sal/macros.h>

#ifdef SYSTEM_LIBEXTTEXTCAT
#include <libexttextcat/textcat.h>
#else
#include <textcat.h>
#endif

using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::linguistic2;

static std::mutex & GetLangGuessMutex()
{
    static std::mutex aMutex;
    return aMutex;
}

namespace {

class LangGuess_Impl :
    public ::cppu::WeakImplHelper<
        XLanguageGuessing,
        XServiceInfo >
{
    SimpleGuesser   m_aGuesser;
    bool            m_bInitialized;

    virtual ~LangGuess_Impl() override {}
    void    EnsureInitialized();

public:
    LangGuess_Impl();
    LangGuess_Impl(const LangGuess_Impl&) = delete;
    LangGuess_Impl& operator=(const LangGuess_Impl&) = delete;

    // XServiceInfo implementation
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // XLanguageGuessing implementation
    virtual css::lang::Locale SAL_CALL guessPrimaryLanguage( const OUString& aText, ::sal_Int32 nStartPos, ::sal_Int32 nLen ) override;
    virtual void SAL_CALL disableLanguages( const css::uno::Sequence< css::lang::Locale >& aLanguages ) override;
    virtual void SAL_CALL enableLanguages( const css::uno::Sequence< css::lang::Locale >& aLanguages ) override;
    virtual css::uno::Sequence< css::lang::Locale > SAL_CALL getAvailableLanguages(  ) override;
    virtual css::uno::Sequence< css::lang::Locale > SAL_CALL getEnabledLanguages(  ) override;
    virtual css::uno::Sequence< css::lang::Locale > SAL_CALL getDisabledLanguages(  ) override;

    // implementation specific
    /// @throws RuntimeException
    void SetFingerPrintsDB( std::u16string_view fileName );
};

}

LangGuess_Impl::LangGuess_Impl() :
    m_bInitialized( false )
{
}

void LangGuess_Impl::EnsureInitialized()
{
    if (m_bInitialized)
        return;

    // set this to true at the very start to prevent loops because of
    // implicitly called functions below
    m_bInitialized = true;

    // set default fingerprint path to where those get installed
    OUString aPhysPath;
    OUString aURL( SvtPathOptions().GetFingerprintPath() );
    osl::FileBase::getSystemPathFromFileURL( aURL, aPhysPath );
#ifdef _WIN32
    aPhysPath += "\\";
#else
    aPhysPath += "/";
#endif

    SetFingerPrintsDB( aPhysPath );

#if !defined(EXTTEXTCAT_VERSION_MAJOR)

    // disable currently not functional languages...
    struct LangCountry
    {
        const char *pLang;
        const char *pCountry;
    };
    LangCountry aDisable[] =
    {
        // not functional in modified libtextcat, but fixed in >= libexttextcat 3.1.0
        // which is the first with EXTTEXTCAT_VERSION_MAJOR defined
        {"sco", ""}, {"zh", "CN"}, {"zh", "TW"}, {"ja", ""}, {"ko", ""},
        {"ka", ""}, {"hi", ""}, {"mr", ""}, {"ne", ""}, {"sa", ""},
        {"ta", ""}, {"th", ""}, {"qu", ""}, {"yi", ""}
    };
    sal_Int32 nNum = SAL_N_ELEMENTS(aDisable);
    Sequence< Locale > aDisableSeq( nNum );
    Locale *pDisableSeq = aDisableSeq.getArray();
    for (sal_Int32 i = 0;  i < nNum;  ++i)
    {
        Locale aLocale;
        aLocale.Language = OUString::createFromAscii( aDisable[i].pLang );
        aLocale.Country  = OUString::createFromAscii( aDisable[i].pCountry );
        pDisableSeq[i] = aLocale;
    }
    disableLanguages( aDisableSeq );
    DBG_ASSERT( nNum == getDisabledLanguages().getLength(), "size mismatch" );
#endif
}

Locale SAL_CALL LangGuess_Impl::guessPrimaryLanguage(
        const OUString& rText,
        ::sal_Int32 nStartPos,
        ::sal_Int32 nLen )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    if (nStartPos < 0 || nLen < 0 || nStartPos + nLen > rText.getLength())
        throw lang::IllegalArgumentException();

    OString o( OUStringToOString( rText.subView(nStartPos, nLen), RTL_TEXTENCODING_UTF8 ) );
    Guess g = m_aGuesser.GuessPrimaryLanguage(o.getStr());
    lang::Locale aRes;
    aRes.Language   = OUString::createFromAscii( g.GetLanguage() );
    aRes.Country    = OUString::createFromAscii( g.GetCountry() );
    return aRes;
}

#define DEFAULT_CONF_FILE_NAME "fpdb.conf"

void LangGuess_Impl::SetFingerPrintsDB(
        std::u16string_view filePath )
{
    //! text encoding for file name / path needs to be in the same encoding the OS uses
    OString path = OUStringToOString( filePath, osl_getThreadTextEncoding() );
    OString conf_file_path = path + DEFAULT_CONF_FILE_NAME;

    m_aGuesser.SetDBPath(conf_file_path.getStr(), path.getStr());
}

uno::Sequence< Locale > SAL_CALL LangGuess_Impl::getAvailableLanguages(  )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    Sequence< css::lang::Locale > aRes;
    std::vector<Guess> gs = m_aGuesser.GetAllManagedLanguages();
    aRes.realloc(gs.size());

    css::lang::Locale *pRes = aRes.getArray();

    for(size_t i = 0; i < gs.size() ; i++ ){
        css::lang::Locale current_aRes;
        current_aRes.Language   = OUString::createFromAscii( gs[i].GetLanguage() );
        current_aRes.Country    = OUString::createFromAscii( gs[i].GetCountry() );
        pRes[i] = std::move(current_aRes);
    }

    return aRes;
}

uno::Sequence< Locale > SAL_CALL LangGuess_Impl::getEnabledLanguages(  )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    Sequence< css::lang::Locale > aRes;
    std::vector<Guess> gs = m_aGuesser.GetAvailableLanguages();
    aRes.realloc(gs.size());

    css::lang::Locale *pRes = aRes.getArray();

    for(size_t i = 0; i < gs.size() ; i++ ){
        css::lang::Locale current_aRes;
        current_aRes.Language   = OUString::createFromAscii( gs[i].GetLanguage() );
        current_aRes.Country    = OUString::createFromAscii( gs[i].GetCountry() );
        pRes[i] = std::move(current_aRes);
    }

    return aRes;
}

uno::Sequence< Locale > SAL_CALL LangGuess_Impl::getDisabledLanguages(  )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    Sequence< css::lang::Locale > aRes;
    std::vector<Guess> gs = m_aGuesser.GetUnavailableLanguages();
    aRes.realloc(gs.size());

    css::lang::Locale *pRes = aRes.getArray();

    for(size_t i = 0; i < gs.size() ; i++ ){
        css::lang::Locale current_aRes;
        current_aRes.Language   = OUString::createFromAscii( gs[i].GetLanguage() );
        current_aRes.Country    = OUString::createFromAscii( gs[i].GetCountry() );
        pRes[i] = std::move(current_aRes);
    }

    return aRes;
}

void SAL_CALL LangGuess_Impl::disableLanguages(
        const uno::Sequence< Locale >& rLanguages )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    for (const Locale& rLanguage : rLanguages)
    {
        std::string language;

        OString l = OUStringToOString( rLanguage.Language, RTL_TEXTENCODING_ASCII_US );
        OString c = OUStringToOString( rLanguage.Country, RTL_TEXTENCODING_ASCII_US );

        language += l.getStr();
        language += "-";
        language += c.getStr();
        m_aGuesser.DisableLanguage(language);
    }
}

void SAL_CALL LangGuess_Impl::enableLanguages(
        const uno::Sequence< Locale >& rLanguages )
{
    std::scoped_lock aGuard( GetLangGuessMutex() );

    EnsureInitialized();

    for (const Locale& rLanguage : rLanguages)
    {
        std::string language;

        OString l = OUStringToOString( rLanguage.Language, RTL_TEXTENCODING_ASCII_US );
        OString c = OUStringToOString( rLanguage.Country, RTL_TEXTENCODING_ASCII_US );

        language += l.getStr();
        language += "-";
        language += c.getStr();
        m_aGuesser.EnableLanguage(language);
    }
}

OUString SAL_CALL LangGuess_Impl::getImplementationName(  )
{
    return u"com.sun.star.lingu2.LanguageGuessing"_ustr;
}

sal_Bool SAL_CALL LangGuess_Impl::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence<OUString> SAL_CALL LangGuess_Impl::getSupportedServiceNames(  )
{
    return { u"com.sun.star.linguistic2.LanguageGuessing"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
lingucomponent_LangGuess_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new LangGuess_Impl());
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

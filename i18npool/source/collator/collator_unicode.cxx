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

#include <config_locales.h>

#include <sal/log.hxx>
#include <rtl/ustrbuf.hxx>

#include <lrl_include.hxx>

#include <i18nlangtag/languagetag.hxx>
#include <i18nlangtag/languagetagicu.hxx>
#include <collator_unicode.hxx>
#include <localedata.hxx>
#include <com/sun/star/i18n/CollatorOptions.hpp>
#include <cppuhelper/supportsservice.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::i18n;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

namespace i18npool {

constexpr OUString implementationName = u"com.sun.star.i18n.Collator_Unicode"_ustr;

Collator_Unicode::Collator_Unicode()
{
    collator = nullptr;
    uca_base = nullptr;
#ifndef DISABLE_DYNLOADING
    hModule = nullptr;
#endif
}

Collator_Unicode::~Collator_Unicode()
{
    collator.reset();
    uca_base.reset();
#ifndef DISABLE_DYNLOADING
    if (hModule) osl_unloadModule(hModule);
#endif
}

#ifdef DISABLE_DYNLOADING

extern "C" {

// For DISABLE_DYNLOADING the generated functions have names that
// start with get_collator_data_ to avoid clashing with a few
// functions in the generated libindex_data that are called just
// get_zh_pinyin for instance.

const sal_uInt8* get_collator_data_ca_charset();
const sal_uInt8* get_collator_data_cu_charset();
const sal_uInt8* get_collator_data_dz_charset();
const sal_uInt8* get_collator_data_hu_charset();
const sal_uInt8* get_collator_data_ja_charset();
const sal_uInt8* get_collator_data_ja_phonetic_alphanumeric_first();
const sal_uInt8* get_collator_data_ja_phonetic_alphanumeric_last();
const sal_uInt8* get_collator_data_ko_charset();
const sal_uInt8* get_collator_data_ku_alphanumeric();
const sal_uInt8* get_collator_data_ln_charset();
const sal_uInt8* get_collator_data_my_dictionary();
const sal_uInt8* get_collator_data_ne_charset();
const sal_uInt8* get_collator_data_sid_charset();
const sal_uInt8* get_collator_data_vro_alphanumeric();
const sal_uInt8* get_collator_data_zh_TW_charset();
const sal_uInt8* get_collator_data_zh_TW_radical();
const sal_uInt8* get_collator_data_zh_TW_stroke();
const sal_uInt8* get_collator_data_zh_charset();
const sal_uInt8* get_collator_data_zh_pinyin();
const sal_uInt8* get_collator_data_zh_radical();
const sal_uInt8* get_collator_data_zh_stroke();
const sal_uInt8* get_collator_data_zh_zhuyin();

size_t get_collator_data_ca_charset_length();
size_t get_collator_data_cu_charset_length();
size_t get_collator_data_dz_charset_length();
size_t get_collator_data_hu_charset_length();
size_t get_collator_data_ja_charset_length();
size_t get_collator_data_ja_phonetic_alphanumeric_first_length();
size_t get_collator_data_ja_phonetic_alphanumeric_last_length();
size_t get_collator_data_ko_charset_length();
size_t get_collator_data_ku_alphanumeric_length();
size_t get_collator_data_ln_charset_length();
size_t get_collator_data_my_dictionary_length();
size_t get_collator_data_ne_charset_length();
size_t get_collator_data_sid_charset_length();
size_t get_collator_data_vro_alphanumeric_length();
size_t get_collator_data_zh_TW_charset_length();
size_t get_collator_data_zh_TW_radical_length();
size_t get_collator_data_zh_TW_stroke_length();
size_t get_collator_data_zh_charset_length();
size_t get_collator_data_zh_pinyin_length();
size_t get_collator_data_zh_radical_length();
size_t get_collator_data_zh_stroke_length();
size_t get_collator_data_zh_zhuyin_length();

}

#endif

sal_Int32 SAL_CALL
Collator_Unicode::compareSubstring( const OUString& str1, sal_Int32 off1, sal_Int32 len1,
    const OUString& str2, sal_Int32 off2, sal_Int32 len2)
{
    return collator->compare(reinterpret_cast<const UChar *>(str1.getStr()) + off1, len1, reinterpret_cast<const UChar *>(str2.getStr()) + off2, len2);
}

sal_Int32 SAL_CALL
Collator_Unicode::compareString( const OUString& str1, const OUString& str2)
{
    return collator->compare(reinterpret_cast<const UChar *>(str1.getStr()), str1.getLength(),
                             reinterpret_cast<const UChar *>(str2.getStr()), str2.getLength());
}

#ifndef DISABLE_DYNLOADING

extern "C" { static void thisModule() {} }

#endif

sal_Int32 SAL_CALL
Collator_Unicode::loadCollatorAlgorithm(const OUString& rAlgorithm, const lang::Locale& rLocale, sal_Int32 options)
{
    if (!collator) {
        UErrorCode status = U_ZERO_ERROR;
        OUString rule = LocaleDataImpl::get()->getCollatorRuleByAlgorithm(rLocale, rAlgorithm);
        if (!rule.isEmpty()) {
            collator.reset( new icu::RuleBasedCollator(reinterpret_cast<const UChar *>(rule.getStr()), status) );
            if (! U_SUCCESS(status)) {
                OUString message = "icu::RuleBasedCollator ctor failed: " + OUString::createFromAscii(u_errorName(status));
                SAL_WARN("i18npool", message);
                throw RuntimeException(message);
            }
        }
        if (!collator && OUString(LOCAL_RULE_LANGS).indexOf(rLocale.Language) >= 0) {
            const sal_uInt8* (*func)() = nullptr;
            size_t (*funclen)() = nullptr;

#ifndef DISABLE_DYNLOADING
            static constexpr OUString sModuleName( u"" SAL_MODULENAME( "i18npool" ) ""_ustr );
            hModule = osl_loadModuleRelative( &thisModule, sModuleName.pData, SAL_LOADMODULE_DEFAULT );
            if (hModule) {
                OUStringBuffer aBuf("get_collator_data_" + rLocale.Language + "_");
                if ( rLocale.Language == "zh" ) {
                    OUString func_base = aBuf.makeStringAndClear();
                    if (u"TW HK MO"_ustr.indexOf(rLocale.Country) >= 0)
                    {
                        func = reinterpret_cast<const sal_uInt8* (*)()>(osl_getFunctionSymbol(hModule,
                                    OUString(func_base + "TW_" + rAlgorithm).pData));
                        funclen = reinterpret_cast<size_t (*)()>(osl_getFunctionSymbol(hModule,
                                    OUString(func_base + "TW_" + rAlgorithm + "_length").pData));
                    }
                    if (!func)
                    {
                        func = reinterpret_cast<const sal_uInt8* (*)()>(osl_getFunctionSymbol(
                                hModule, OUString(func_base + rAlgorithm).pData));
                        funclen = reinterpret_cast<size_t (*)()>(osl_getFunctionSymbol(
                                hModule, OUString(func_base + rAlgorithm + "_length").pData));
                    }
                } else {
                    if ( rLocale.Language == "ja" ) {
                        // replace algorithm name to implementation name.
                        if (rAlgorithm == "phonetic (alphanumeric first)")
                            aBuf.append("phonetic_alphanumeric_first");
                        else if (rAlgorithm == "phonetic (alphanumeric last)")
                            aBuf.append("phonetic_alphanumeric_last");
                        else
                            aBuf.append(rAlgorithm);
                    } else {
                        aBuf.append(rAlgorithm);
                    }
                    OUString func_base = aBuf.makeStringAndClear();
                    OUString funclen_base = func_base + "_length";
                    func = reinterpret_cast<const sal_uInt8* (*)()>(osl_getFunctionSymbol(hModule, func_base.pData));
                    funclen = reinterpret_cast<size_t (*)()>(osl_getFunctionSymbol(hModule, funclen_base.pData));
                }
            }
#else
            if (false) {
                ;
#if WITH_LOCALE_ALL || WITH_LOCALE_ca
            } else if ( rLocale.Language == "ca" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_ca_charset;
                    funclen = get_collator_data_ca_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_cu
            } else if ( rLocale.Language == "cu" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_cu_charset;
                    funclen = get_collator_data_cu_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_dz
            } else if ( rLocale.Language == "dz" || rLocale.Language == "bo" ) {
                // 'bo' Tibetan uses the same collation rules as 'dz' Dzongkha
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_dz_charset;
                    funclen = get_collator_data_dz_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_hu
            } else if ( rLocale.Language == "hu" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_hu_charset;
                    funclen = get_collator_data_hu_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ja
            } else if ( rLocale.Language == "ja" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_ja_charset;
                    funclen = get_collator_data_ja_charset_length;
                }
                else if ( rAlgorithm == "phonetic (alphanumeric first)" )
                {
                    func = get_collator_data_ja_phonetic_alphanumeric_first;
                    funclen = get_collator_data_ja_phonetic_alphanumeric_first_length;
                }
                else if ( rAlgorithm == "phonetic (alphanumeric last)" )
                {
                    func = get_collator_data_ja_phonetic_alphanumeric_last;
                    funclen = get_collator_data_ja_phonetic_alphanumeric_last_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ku
            } else if ( rLocale.Language == "ku" ) {
                if ( rAlgorithm == "alphanumeric" )
                {
                    func = get_collator_data_ku_alphanumeric;
                    funclen = get_collator_data_ku_alphanumeric_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ln
            } else if ( rLocale.Language == "ln" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_ln_charset;
                    funclen = get_collator_data_ln_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_my
            } else if ( rLocale.Language == "my" ) {
                if ( rAlgorithm == "dictionary" )
                {
                    func = get_collator_data_my_dictionary;
                    funclen = get_collator_data_my_dictionary_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_ne
            } else if ( rLocale.Language == "ne" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_ne_charset;
                    funclen = get_collator_data_ne_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_sid
            } else if ( rLocale.Language == "sid" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_sid_charset;
                    funclen = get_collator_data_sid_charset_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_vro
            } else if ( rLocale.Language == "vro" ) {
                if ( rAlgorithm == "alphanumeric" )
                {
                    func = get_collator_data_vro_alphanumeric;
                    funclen = get_collator_data_vro_alphanumeric_length;
                }
#endif
#if WITH_LOCALE_ALL || WITH_LOCALE_zh
            } else if ( rLocale.Language == "zh" && (rLocale.Country == "TW" || rLocale.Country == "HK" || rLocale.Country == "MO") ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_zh_TW_charset;
                    funclen = get_collator_data_zh_TW_charset_length;
                }
                else if ( rAlgorithm == "radical" )
                {
                    func = get_collator_data_zh_TW_radical;
                    funclen = get_collator_data_zh_TW_radical_length;
                }
                else if ( rAlgorithm == "stroke" )
                {
                    func = get_collator_data_zh_TW_stroke;
                    funclen = get_collator_data_zh_TW_stroke_length;
                }
            } else if ( rLocale.Language == "zh" ) {
                if ( rAlgorithm == "charset" )
                {
                    func = get_collator_data_zh_charset;
                    funclen = get_collator_data_zh_charset_length;
                }
                else if ( rAlgorithm == "pinyin" )
                {
                    func = get_collator_data_zh_pinyin;
                    funclen = get_collator_data_zh_pinyin_length;
                }
                else if ( rAlgorithm == "radical" )
                {
                    func = get_collator_data_zh_radical;
                    funclen = get_collator_data_zh_radical_length;
                }
                else if ( rAlgorithm == "stroke" )
                {
                    func = get_collator_data_zh_stroke;
                    funclen = get_collator_data_zh_stroke_length;
                }
                else if ( rAlgorithm == "zhuyin" )
                {
                    func = get_collator_data_zh_zhuyin;
                    funclen = get_collator_data_zh_zhuyin_length;
                }
#endif
            }
#endif // DISABLE_DYNLOADING
            if (func && funclen) {
                const sal_uInt8* ruleImage=func();
                size_t ruleImageSize = funclen();

                // Not only changed ICU 53.1 the API behavior that a negative
                // length (ruleImageSize) now leads to failure, but also that
                // the base RuleBasedCollator passed as uca_base here needs to
                // have a base->tailoring == CollationRoot::getRoot() otherwise
                // the init bails out as well, as it does for the previously
                // used "empty" RuleBasedCollator.
                // The default collator of the en-US locale would also fulfill
                // the requirement. The collator of the actual locale or the
                // NULL (default) locale does not.
                uca_base.reset( static_cast<icu::RuleBasedCollator*>(icu::Collator::createInstance(
                            icu::Locale::getRoot(), status)) );
                if (! U_SUCCESS(status)) {
                    OUString message = "icu::Collator::createInstance() failed: " + OUString::createFromAscii(u_errorName(status));
                    SAL_WARN("i18npool", message);
                    throw RuntimeException(message);
                }
                collator.reset( new icu::RuleBasedCollator(
                        reinterpret_cast<const uint8_t*>(ruleImage), ruleImageSize, uca_base.get(), status) );
                if (! U_SUCCESS(status)) {
                    OUString message = "icu::RuleBasedCollator ctor failed: " + OUString::createFromAscii(u_errorName(status));
                    SAL_WARN("i18npool", message);
                    throw RuntimeException(message);
                }
            }
        }
        if (!collator) {
            /** ICU collators are loaded using a locale only.
                ICU uses Variant as collation algorithm name (like de__PHONEBOOK
                locale), note the empty territory (Country) designator in this special
                case here.
                But sometimes the mapping fails, eg for German (from Germany) phonebook, we'll have "de_DE_PHONEBOOK"
                this one won't be remapping to collation keyword specifiers "de@collation=phonebook"
                See http://userguide.icu-project.org/locale#TOC-Variant-code, Level 2 canonicalization, 8.
                So let variant empty and use the fourth arg of icuLocale "keywords"
                See LanguageTagIcu::getIcuLocale from i18nlangtag/source/languagetag/languagetagicu.cxx
                The icu::Locale constructor changes the algorithm name to
                uppercase itself, so we don't have to bother with that.
            */
            icu::Locale icuLocale( LanguageTagIcu::getIcuLocale( LanguageTag( rLocale),
                        u"", rAlgorithm.isEmpty() ? u""_ustr : "collation=" + rAlgorithm));

            // FIXME: apparently we get here in LOKit case only. When the language is Japanese, we pass "ja@collation=phonetic (alphanumeric first)" to ICU
            // and ICU does not like this (U_ILLEGAL_ARGUMENT_ERROR). Subsequently LOKit crashes, because collator is nullptr.
            if (!strcmp(icuLocale.getLanguage(), "ja"))
                icuLocale = icu::Locale::getJapanese();

            // load ICU collator
            collator.reset( static_cast<icu::RuleBasedCollator*>( icu::Collator::createInstance(icuLocale, status) ) );
            if (! U_SUCCESS(status)) {
                OUString message = "icu::Collator::createInstance() failed: " + OUString::createFromAscii(u_errorName(status));
                SAL_WARN("i18npool", message);
                throw RuntimeException(message);
            }
        }
    }

    if (options & CollatorOptions::CollatorOptions_IGNORE_CASE_ACCENT)
        collator->setStrength(icu::Collator::PRIMARY);
    else if (options & CollatorOptions::CollatorOptions_IGNORE_CASE)
        collator->setStrength(icu::Collator::SECONDARY);
    else
        collator->setStrength(icu::Collator::TERTIARY);

    return 0;
}


OUString SAL_CALL
Collator_Unicode::getImplementationName()
{
    return implementationName;
}

sal_Bool SAL_CALL
Collator_Unicode::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL
Collator_Unicode::getSupportedServiceNames()
{
    Sequence< OUString > aRet { implementationName };
    return aRet;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

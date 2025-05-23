/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <i18nlangtag/languagetagicu.hxx>
#include <i18nlangtag/languagetag.hxx>


// static
icu::Locale LanguageTagIcu::getIcuLocale( const LanguageTag & rLanguageTag )
{
    if (rLanguageTag.isIsoLocale())
    {
        // The simple case.
        const css::lang::Locale& rLocale = rLanguageTag.getLocale();
        if (rLocale.Country.isEmpty())
            return icu::Locale( OUStringToOString( rLocale.Language, RTL_TEXTENCODING_ASCII_US).getStr());
        return icu::Locale(
                OUStringToOString( rLocale.Language, RTL_TEXTENCODING_ASCII_US).getStr(),
                OUStringToOString( rLocale.Country, RTL_TEXTENCODING_ASCII_US).getStr());
    }

    /* TODO: could we optimize this for the isIsoODF() case where only a script
     * is added? */

    // Let ICU decide how it wants a BCP47 string stuffed into its Locale.
    return icu::Locale::createFromName(
            OUStringToOString( rLanguageTag.getBcp47(), RTL_TEXTENCODING_ASCII_US).getStr());
}


// static
icu::Locale LanguageTagIcu::getIcuLocale( const LanguageTag & rLanguageTag, std::u16string_view rVariant, std::u16string_view rKeywords )
{
    /* FIXME: how should this work with any BCP47? */
    return icu::Locale(
            OUStringToOString( rLanguageTag.getLanguage(), RTL_TEXTENCODING_ASCII_US).getStr(),
            OUStringToOString( rLanguageTag.getCountry(), RTL_TEXTENCODING_ASCII_US).getStr(),
            OUStringToOString( rVariant, RTL_TEXTENCODING_ASCII_US).getStr(),
            OUStringToOString( rKeywords, RTL_TEXTENCODING_ASCII_US).getStr()
           );
}

// static
OUString LanguageTagIcu::getDisplayName( const LanguageTag & rLanguageTag, const LanguageTag & rDisplayLanguage )
{
    // This will be initialized by the first call; as the UI language doesn't
    // change the tag mostly stays the same, unless someone overrides it for a
    // call here, and thus obtaining the UI icu::Locale has to be done only
    // once.
    static thread_local LanguageTag aUITag( LANGUAGE_SYSTEM);
    static thread_local icu::Locale aUILocale;

    if (aUITag != rDisplayLanguage)
    {
        aUITag = rDisplayLanguage;
        aUILocale = getIcuLocale( rDisplayLanguage);
    }

    icu::Locale aLocale( getIcuLocale( rLanguageTag));
    icu::UnicodeString aResult;
    aLocale.getDisplayName( aUILocale, aResult);
    return OUString( reinterpret_cast<const sal_Unicode*>(aResult.getBuffer()), aResult.length());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

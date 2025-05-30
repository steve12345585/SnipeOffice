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

#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <tools/debug.hxx>
#include <unotools/configitem.hxx>
#include <unotools/lingucfg.hxx>
#include <unotools/linguprops.hxx>
#include <comphelper/getexpandeduri.hxx>
#include <comphelper/processfactory.hxx>
#include <o3tl/string_view.hxx>
#include <mutex>

#include "itemholder1.hxx"

using namespace com::sun::star;

constexpr OUStringLiteral FILE_PROTOCOL = u"file:///";

namespace
{
    std::mutex& theSvtLinguConfigItemMutex()
    {
        static std::mutex SINGLETON;
        return SINGLETON;
    }
}

static bool lcl_SetLocale( LanguageType &rLanguage, const uno::Any &rVal )
{
    bool bSucc = false;

    lang::Locale aNew;
    if (rVal >>= aNew)  // conversion successful?
    {
        LanguageType nNew = LanguageTag::convertToLanguageType( aNew, false);
        if (nNew != rLanguage)
        {
            rLanguage = nNew;
            bSucc = true;
        }
    }
    return bSucc;
}

static OUString lcl_LanguageToCfgLocaleStr( LanguageType nLanguage )
{
    OUString aRes;
    if (LANGUAGE_SYSTEM != nLanguage)
        aRes = LanguageTag::convertToBcp47( nLanguage );
    return aRes;
}

static LanguageType lcl_CfgAnyToLanguage( const uno::Any &rVal )
{
    OUString aTmp;
    rVal >>= aTmp;
    return (aTmp.isEmpty()) ? LANGUAGE_SYSTEM : LanguageTag::convertToLanguageTypeWithFallback( aTmp );
}

SvtLinguOptions::SvtLinguOptions()
    : bROActiveDics(false)
    , bROActiveConvDics(false)
    , nHyphMinLeading(2)
    , nHyphMinTrailing(2)
    , nHyphMinWordLength(0)
    , bROHyphMinLeading(false)
    , bROHyphMinTrailing(false)
    , bROHyphMinWordLength(false)
    , nDefaultLanguage(LANGUAGE_NONE)
    , nDefaultLanguage_CJK(LANGUAGE_NONE)
    , nDefaultLanguage_CTL(LANGUAGE_NONE)
    , bRODefaultLanguage(false)
    , bRODefaultLanguage_CJK(false)
    , bRODefaultLanguage_CTL(false)
    , bIsSpellSpecial(true)
    , bIsSpellAuto(false)
    , bIsSpellReverse(false)
    , bROIsSpellSpecial(false)
    , bROIsSpellAuto(false)
    , bROIsSpellReverse(false)
    , bIsHyphSpecial(true)
    , bIsHyphAuto(false)
    , bROIsHyphSpecial(false)
    , bROIsHyphAuto(false)
    , bIsUseDictionaryList(true)
    , bIsIgnoreControlCharacters(true)
    , bROIsUseDictionaryList(false)
    , bROIsIgnoreControlCharacters(false)
    , bIsSpellWithDigits(false)
    , bIsSpellUpperCase(false)
    , bIsSpellClosedCompound(true)
    , bIsSpellHyphenatedCompound(true)
    , bROIsSpellWithDigits(false)
    , bROIsSpellUpperCase(false)
    , bROIsSpellClosedCompound(false)
    , bROIsSpellHyphenatedCompound(false)
    , bIsIgnorePostPositionalWord(true)
    , bIsAutoCloseDialog(false)
    , bIsShowEntriesRecentlyUsedFirst(false)
    , bIsAutoReplaceUniqueEntries(false)
    , bIsDirectionToSimplified(true)
    , bIsUseCharacterVariants(false)
    , bIsTranslateCommonTerms(false)
    , bIsReverseMapping(false)
    , bROIsIgnorePostPositionalWord(false)
    , bROIsAutoCloseDialog(false)
    , bROIsShowEntriesRecentlyUsedFirst(false)
    , bROIsAutoReplaceUniqueEntries(false)
    , bROIsDirectionToSimplified(false)
    , bROIsUseCharacterVariants(false)
    , bROIsTranslateCommonTerms(false)
    , bROIsReverseMapping(false)
    , nDataFilesChangedCheckValue(0)
    , bRODataFilesChangedCheckValue(false)
    , bIsGrammarAuto(false)
    , bIsGrammarInteractive(false)
    , bROIsGrammarAuto(false)
    , bROIsGrammarInteractive(false)
{
}

class SvtLinguConfigItem : public utl::ConfigItem
{
    SvtLinguOptions     aOpt;

    static bool GetHdlByName( sal_Int32 &rnHdl, std::u16string_view rPropertyName, bool bFullPropName = false );
    static uno::Sequence< OUString > GetPropertyNames();
    void                LoadOptions( const uno::Sequence< OUString > &rProperyNames );
    bool                SaveOptions( const uno::Sequence< OUString > &rProperyNames );

    SvtLinguConfigItem(const SvtLinguConfigItem&) = delete;
    SvtLinguConfigItem& operator=(const SvtLinguConfigItem&) = delete;
    virtual void    ImplCommit() override;

public:
    SvtLinguConfigItem();

    // utl::ConfigItem
    virtual void    Notify( const css::uno::Sequence< OUString > &rPropertyNames ) override;

    // make some protected functions of utl::ConfigItem public
    using utl::ConfigItem::GetNodeNames;
    using utl::ConfigItem::GetProperties;
    //using utl::ConfigItem::PutProperties;
    //using utl::ConfigItem::SetSetProperties;
    using utl::ConfigItem::ReplaceSetProperties;
    //using utl::ConfigItem::GetReadOnlyStates;

    css::uno::Any
            GetProperty( std::u16string_view rPropertyName ) const;
    css::uno::Any
            GetProperty( sal_Int32 nPropertyHandle ) const;

    bool    SetProperty( std::u16string_view rPropertyName,
                         const css::uno::Any &rValue );
    bool    SetProperty( sal_Int32 nPropertyHandle,
                         const css::uno::Any &rValue );

    void GetOptions( SvtLinguOptions& ) const;

    bool    IsReadOnly( std::u16string_view rPropertyName ) const;
    bool    IsReadOnly( sal_Int32 nPropertyHandle ) const;
};

SvtLinguConfigItem::SvtLinguConfigItem() :
    utl::ConfigItem( u"Office.Linguistic"_ustr )
{
    const uno::Sequence< OUString > aPropertyNames = GetPropertyNames();
    LoadOptions( aPropertyNames );
    ClearModified();

    // request notify events when properties change
    EnableNotification( aPropertyNames );
}

void SvtLinguConfigItem::Notify( const uno::Sequence< OUString > &rPropertyNames )
{
    {
        std::unique_lock aGuard(theSvtLinguConfigItemMutex());
        LoadOptions( rPropertyNames );
    }
    NotifyListeners(ConfigurationHints::NONE);
}

void SvtLinguConfigItem::ImplCommit()
{
    SaveOptions( GetPropertyNames() );
}

namespace {

struct NamesToHdl
{
    OUString     aFullPropName;      // full qualified name as used in configuration
    OUString     aPropName;          // property name only (atom) of above
    sal_Int32    nHdl;               // numeric handle representing the property
};

}

NamesToHdl constexpr aNamesToHdl[] =
{
{/*  0 */    u"General/DefaultLocale"_ustr,                         UPN_DEFAULT_LOCALE,                    UPH_DEFAULT_LOCALE},
{/*  1 */    u"General/DictionaryList/ActiveDictionaries"_ustr,     UPN_ACTIVE_DICTIONARIES,               UPH_ACTIVE_DICTIONARIES},
{/*  2 */    u"General/DictionaryList/IsUseDictionaryList"_ustr,    UPN_IS_USE_DICTIONARY_LIST,            UPH_IS_USE_DICTIONARY_LIST},
{/*  3 */    u"General/IsIgnoreControlCharacters"_ustr,             UPN_IS_IGNORE_CONTROL_CHARACTERS,      UPH_IS_IGNORE_CONTROL_CHARACTERS},
{/*  5 */    u"General/DefaultLocale_CJK"_ustr,                     UPN_DEFAULT_LOCALE_CJK,                UPH_DEFAULT_LOCALE_CJK},
{/*  6 */    u"General/DefaultLocale_CTL"_ustr,                     UPN_DEFAULT_LOCALE_CTL,                UPH_DEFAULT_LOCALE_CTL},

{/*  7 */    u"SpellChecking/IsSpellUpperCase"_ustr,                UPN_IS_SPELL_UPPER_CASE,               UPH_IS_SPELL_UPPER_CASE},
{/*  8 */    u"SpellChecking/IsSpellWithDigits"_ustr,               UPN_IS_SPELL_WITH_DIGITS,              UPH_IS_SPELL_WITH_DIGITS},
{/*  9 */    u"SpellChecking/IsSpellAuto"_ustr,                     UPN_IS_SPELL_AUTO,                     UPH_IS_SPELL_AUTO},
{/* 10 */    u"SpellChecking/IsSpellSpecial"_ustr,                  UPN_IS_SPELL_SPECIAL,                  UPH_IS_SPELL_SPECIAL},
{/* 11 */    u"SpellChecking/IsSpellClosedCompound"_ustr,           UPN_IS_SPELL_CLOSED_COMPOUND,          UPH_IS_SPELL_CLOSED_COMPOUND},
{/* 12 */    u"SpellChecking/IsSpellHyphenatedCompound"_ustr,       UPN_IS_SPELL_HYPHENATED_COMPOUND,      UPH_IS_SPELL_HYPHENATED_COMPOUND},
{/* 13 */    u"SpellChecking/IsReverseDirection"_ustr,              UPN_IS_WRAP_REVERSE,                   UPH_IS_WRAP_REVERSE},

{/* 14 */    u"Hyphenation/MinLeading"_ustr,                        UPN_HYPH_MIN_LEADING,                  UPH_HYPH_MIN_LEADING},
{/* 15 */    u"Hyphenation/MinTrailing"_ustr,                       UPN_HYPH_MIN_TRAILING,                 UPH_HYPH_MIN_TRAILING},
{/* 16 */    u"Hyphenation/MinWordLength"_ustr,                     UPN_HYPH_MIN_WORD_LENGTH,              UPH_HYPH_MIN_WORD_LENGTH},
{/* 17*/     u"Hyphenation/IsHyphSpecial"_ustr,                     UPN_IS_HYPH_SPECIAL,                   UPH_IS_HYPH_SPECIAL},
{/* 18 */    u"Hyphenation/IsHyphAuto"_ustr,                        UPN_IS_HYPH_AUTO,                      UPH_IS_HYPH_AUTO},

{/* 19 */    u"TextConversion/ActiveConversionDictionaries"_ustr,   UPN_ACTIVE_CONVERSION_DICTIONARIES,        UPH_ACTIVE_CONVERSION_DICTIONARIES},
{/* 20 */    u"TextConversion/IsIgnorePostPositionalWord"_ustr,     UPN_IS_IGNORE_POST_POSITIONAL_WORD,        UPH_IS_IGNORE_POST_POSITIONAL_WORD},
{/* 21 */    u"TextConversion/IsAutoCloseDialog"_ustr,              UPN_IS_AUTO_CLOSE_DIALOG,                  UPH_IS_AUTO_CLOSE_DIALOG},
{/* 22 */    u"TextConversion/IsShowEntriesRecentlyUsedFirst"_ustr, UPN_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST,   UPH_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST},
{/* 23 */    u"TextConversion/IsAutoReplaceUniqueEntries"_ustr,     UPN_IS_AUTO_REPLACE_UNIQUE_ENTRIES,        UPH_IS_AUTO_REPLACE_UNIQUE_ENTRIES},
{/* 24 */    u"TextConversion/IsDirectionToSimplified"_ustr,        UPN_IS_DIRECTION_TO_SIMPLIFIED,            UPH_IS_DIRECTION_TO_SIMPLIFIED},
{/* 25 */    u"TextConversion/IsUseCharacterVariants"_ustr,         UPN_IS_USE_CHARACTER_VARIANTS,             UPH_IS_USE_CHARACTER_VARIANTS},
{/* 26 */    u"TextConversion/IsTranslateCommonTerms"_ustr,         UPN_IS_TRANSLATE_COMMON_TERMS,             UPH_IS_TRANSLATE_COMMON_TERMS},
{/* 27 */    u"TextConversion/IsReverseMapping"_ustr,               UPN_IS_REVERSE_MAPPING,                    UPH_IS_REVERSE_MAPPING},

{/* 28 */    u"ServiceManager/DataFilesChangedCheckValue"_ustr,     UPN_DATA_FILES_CHANGED_CHECK_VALUE,        UPH_DATA_FILES_CHANGED_CHECK_VALUE},

{/* 29 */    u"GrammarChecking/IsAutoCheck"_ustr,                   UPN_IS_GRAMMAR_AUTO,                      UPH_IS_GRAMMAR_AUTO},
{/* 30 */    u"GrammarChecking/IsInteractiveCheck"_ustr,            UPN_IS_GRAMMAR_INTERACTIVE,               UPH_IS_GRAMMAR_INTERACTIVE},

            /* similar to entry 0 (thus no own configuration entry) but with different property name and type */
{            u""_ustr,                                         UPN_DEFAULT_LANGUAGE,                      UPH_DEFAULT_LANGUAGE},

{            u""_ustr,                                         u""_ustr,                                      -1}
};

uno::Sequence< OUString > SvtLinguConfigItem::GetPropertyNames()
{
    uno::Sequence< OUString > aNames;
    aNames.realloc(std::size(aNamesToHdl));
    OUString *pNames = aNames.getArray();
    sal_Int32 nIdx = 0;
    for (auto const & nameToHdl: aNamesToHdl)
    {
        if (!nameToHdl.aFullPropName.isEmpty())
            pNames[ nIdx++ ] = nameToHdl.aFullPropName;
    }
    aNames.realloc( nIdx );

    return aNames;
}

bool SvtLinguConfigItem::GetHdlByName(
    sal_Int32 &rnHdl,
    std::u16string_view rPropertyName,
    bool bFullPropName )
{
    NamesToHdl const *pEntry = &aNamesToHdl[0];

    if (bFullPropName)
    {
        while (pEntry && !pEntry->aFullPropName.isEmpty())
        {
            if (pEntry->aFullPropName == rPropertyName)
            {
                rnHdl = pEntry->nHdl;
                break;
            }
            ++pEntry;
        }
        return pEntry && !pEntry->aFullPropName.isEmpty();
    }
    else
    {
        while (pEntry && !pEntry->aFullPropName.isEmpty())
        {
            if (rPropertyName == pEntry->aPropName )
            {
                rnHdl = pEntry->nHdl;
                break;
            }
            ++pEntry;
        }
        return pEntry && !pEntry->aFullPropName.isEmpty();
    }
}

uno::Any SvtLinguConfigItem::GetProperty( std::u16string_view rPropertyName ) const
{
    sal_Int32 nHdl;
    return GetHdlByName( nHdl, rPropertyName ) ? GetProperty( nHdl ) : uno::Any();
}

uno::Any SvtLinguConfigItem::GetProperty( sal_Int32 nPropertyHandle ) const
{
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());

    uno::Any aRes;

    const sal_Int16 *pnVal = nullptr;
    const LanguageType *plVal = nullptr;
    const bool  *pbVal = nullptr;
    const sal_Int32 *pnInt32Val = nullptr;

    const SvtLinguOptions &rOpt = const_cast< SvtLinguConfigItem * >(this)->aOpt;
    switch (nPropertyHandle)
    {
        case UPH_IS_USE_DICTIONARY_LIST :   pbVal = &rOpt.bIsUseDictionaryList; break;
        case UPH_IS_IGNORE_CONTROL_CHARACTERS : pbVal = &rOpt.bIsIgnoreControlCharacters;   break;
        case UPH_IS_HYPH_AUTO :             pbVal = &rOpt.bIsHyphAuto;  break;
        case UPH_IS_HYPH_SPECIAL :          pbVal = &rOpt.bIsHyphSpecial;   break;
        case UPH_IS_SPELL_AUTO :            pbVal = &rOpt.bIsSpellAuto; break;
        case UPH_IS_SPELL_SPECIAL :         pbVal = &rOpt.bIsSpellSpecial;  break;
        case UPH_IS_WRAP_REVERSE :          pbVal = &rOpt.bIsSpellReverse;  break;
        case UPH_DEFAULT_LANGUAGE :         plVal = &rOpt.nDefaultLanguage; break;
        case UPH_IS_SPELL_CLOSED_COMPOUND:  pbVal = &rOpt.bIsSpellClosedCompound;       break;
        case UPH_IS_SPELL_HYPHENATED_COMPOUND:  pbVal = &rOpt.bIsSpellHyphenatedCompound;    break;
        case UPH_IS_SPELL_WITH_DIGITS :     pbVal = &rOpt.bIsSpellWithDigits;   break;
        case UPH_IS_SPELL_UPPER_CASE :      pbVal = &rOpt.bIsSpellUpperCase;        break;
        case UPH_HYPH_MIN_LEADING :         pnVal = &rOpt.nHyphMinLeading;      break;
        case UPH_HYPH_MIN_TRAILING :        pnVal = &rOpt.nHyphMinTrailing; break;
        case UPH_HYPH_MIN_WORD_LENGTH :     pnVal = &rOpt.nHyphMinWordLength;   break;
        case UPH_ACTIVE_DICTIONARIES :
        {
            aRes <<= rOpt.aActiveDics;
            break;
        }
        case UPH_ACTIVE_CONVERSION_DICTIONARIES :
        {
            aRes <<= rOpt.aActiveConvDics;
            break;
        }
        case UPH_DEFAULT_LOCALE :
        {
            aRes <<= LanguageTag::convertToLocale( rOpt.nDefaultLanguage, false);
            break;
        }
        case UPH_DEFAULT_LOCALE_CJK :
        {
            aRes <<= LanguageTag::convertToLocale( rOpt.nDefaultLanguage_CJK, false);
            break;
        }
        case UPH_DEFAULT_LOCALE_CTL :
        {
            aRes <<= LanguageTag::convertToLocale( rOpt.nDefaultLanguage_CTL, false);
            break;
        }
        case UPH_IS_IGNORE_POST_POSITIONAL_WORD :       pbVal = &rOpt.bIsIgnorePostPositionalWord; break;
        case UPH_IS_AUTO_CLOSE_DIALOG :                 pbVal = &rOpt.bIsAutoCloseDialog; break;
        case UPH_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST :  pbVal = &rOpt.bIsShowEntriesRecentlyUsedFirst; break;
        case UPH_IS_AUTO_REPLACE_UNIQUE_ENTRIES :       pbVal = &rOpt.bIsAutoReplaceUniqueEntries; break;

        case UPH_IS_DIRECTION_TO_SIMPLIFIED:            pbVal = &rOpt.bIsDirectionToSimplified; break;
        case UPH_IS_USE_CHARACTER_VARIANTS :            pbVal = &rOpt.bIsUseCharacterVariants; break;
        case UPH_IS_TRANSLATE_COMMON_TERMS :            pbVal = &rOpt.bIsTranslateCommonTerms; break;
        case UPH_IS_REVERSE_MAPPING :                   pbVal = &rOpt.bIsReverseMapping; break;

        case UPH_DATA_FILES_CHANGED_CHECK_VALUE :       pnInt32Val = &rOpt.nDataFilesChangedCheckValue; break;
        case UPH_IS_GRAMMAR_AUTO:                       pbVal = &rOpt.bIsGrammarAuto; break;
        case UPH_IS_GRAMMAR_INTERACTIVE:                pbVal = &rOpt.bIsGrammarInteractive; break;
        default :
            SAL_WARN( "unotools.config", "unexpected property handle" );
    }

    if (pbVal)
        aRes <<= *pbVal;
    else if (pnVal)
        aRes <<= *pnVal;
    else if (plVal)
        aRes <<= static_cast<sal_Int16>(static_cast<sal_uInt16>(*plVal));
    else if (pnInt32Val)
        aRes <<= *pnInt32Val;

    return aRes;
}

bool SvtLinguConfigItem::SetProperty( std::u16string_view rPropertyName, const uno::Any &rValue )
{
    bool bSucc = false;
    sal_Int32 nHdl;
    if (GetHdlByName( nHdl, rPropertyName ))
        bSucc = SetProperty( nHdl, rValue );
    return bSucc;
}

bool SvtLinguConfigItem::SetProperty( sal_Int32 nPropertyHandle, const uno::Any &rValue )
{
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());

    bool bSucc = false;
    if (!rValue.hasValue())
        return bSucc;

    bool bMod = false;

    sal_Int16 *pnVal = nullptr;
    LanguageType *plVal = nullptr;
    bool  *pbVal = nullptr;
    sal_Int32 *pnInt32Val = nullptr;

    SvtLinguOptions &rOpt = aOpt;
    switch (nPropertyHandle)
    {
        case UPH_IS_USE_DICTIONARY_LIST :   pbVal = &rOpt.bIsUseDictionaryList;    break;
        case UPH_IS_IGNORE_CONTROL_CHARACTERS : pbVal = &rOpt.bIsIgnoreControlCharacters;  break;
        case UPH_IS_HYPH_AUTO :             pbVal = &rOpt.bIsHyphAuto; break;
        case UPH_IS_HYPH_SPECIAL :          pbVal = &rOpt.bIsHyphSpecial;  break;
        case UPH_IS_SPELL_AUTO :            pbVal = &rOpt.bIsSpellAuto;    break;
        case UPH_IS_SPELL_SPECIAL :         pbVal = &rOpt.bIsSpellSpecial; break;
        case UPH_IS_WRAP_REVERSE :          pbVal = &rOpt.bIsSpellReverse; break;
        case UPH_DEFAULT_LANGUAGE :         plVal = &rOpt.nDefaultLanguage;    break;
        case UPH_IS_SPELL_CLOSED_COMPOUND:  pbVal = &rOpt.bIsSpellClosedCompound;      break;
        case UPH_IS_SPELL_HYPHENATED_COMPOUND:  pbVal = &rOpt.bIsSpellHyphenatedCompound;    break;
        case UPH_IS_SPELL_WITH_DIGITS :     pbVal = &rOpt.bIsSpellWithDigits;  break;
        case UPH_IS_SPELL_UPPER_CASE :      pbVal = &rOpt.bIsSpellUpperCase;       break;
        case UPH_HYPH_MIN_LEADING :         pnVal = &rOpt.nHyphMinLeading;     break;
        case UPH_HYPH_MIN_TRAILING :        pnVal = &rOpt.nHyphMinTrailing;    break;
        case UPH_HYPH_MIN_WORD_LENGTH :     pnVal = &rOpt.nHyphMinWordLength;  break;
        case UPH_ACTIVE_DICTIONARIES :
        {
            rValue >>= rOpt.aActiveDics;
            bMod = true;
            break;
        }
        case UPH_ACTIVE_CONVERSION_DICTIONARIES :
        {
            rValue >>= rOpt.aActiveConvDics;
            bMod = true;
            break;
        }
        case UPH_DEFAULT_LOCALE :
        {
            bSucc = lcl_SetLocale( rOpt.nDefaultLanguage, rValue );
            bMod = bSucc;
            break;
        }
        case UPH_DEFAULT_LOCALE_CJK :
        {
            bSucc = lcl_SetLocale( rOpt.nDefaultLanguage_CJK, rValue );
            bMod = bSucc;
            break;
        }
        case UPH_DEFAULT_LOCALE_CTL :
        {
            bSucc = lcl_SetLocale( rOpt.nDefaultLanguage_CTL, rValue );
            bMod = bSucc;
            break;
        }
        case UPH_IS_IGNORE_POST_POSITIONAL_WORD :       pbVal = &rOpt.bIsIgnorePostPositionalWord; break;
        case UPH_IS_AUTO_CLOSE_DIALOG :                 pbVal = &rOpt.bIsAutoCloseDialog; break;
        case UPH_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST :  pbVal = &rOpt.bIsShowEntriesRecentlyUsedFirst; break;
        case UPH_IS_AUTO_REPLACE_UNIQUE_ENTRIES :       pbVal = &rOpt.bIsAutoReplaceUniqueEntries; break;

        case UPH_IS_DIRECTION_TO_SIMPLIFIED :           pbVal = &rOpt.bIsDirectionToSimplified; break;
        case UPH_IS_USE_CHARACTER_VARIANTS :            pbVal = &rOpt.bIsUseCharacterVariants; break;
        case UPH_IS_TRANSLATE_COMMON_TERMS :            pbVal = &rOpt.bIsTranslateCommonTerms; break;
        case UPH_IS_REVERSE_MAPPING :                   pbVal = &rOpt.bIsReverseMapping; break;

        case UPH_DATA_FILES_CHANGED_CHECK_VALUE :       pnInt32Val = &rOpt.nDataFilesChangedCheckValue; break;
        case UPH_IS_GRAMMAR_AUTO:                       pbVal = &rOpt.bIsGrammarAuto; break;
        case UPH_IS_GRAMMAR_INTERACTIVE:                pbVal = &rOpt.bIsGrammarInteractive; break;
        default :
            SAL_WARN( "unotools.config", "unexpected property handle" );
    }

    if (pbVal)
    {
        bool bNew = bool();
        if (rValue >>= bNew)
        {
            if (bNew != *pbVal)
            {
                *pbVal = bNew;
                bMod = true;
            }
            bSucc = true;
        }
    }
    else if (pnVal)
    {
        sal_Int16 nNew = sal_Int16();
        if (rValue >>= nNew)
        {
            if (nNew != *pnVal)
            {
                *pnVal = nNew;
                bMod = true;
            }
            bSucc = true;
        }
    }
    else if (plVal)
    {
        sal_Int16 nNew = sal_Int16();
        if (rValue >>= nNew)
        {
            if (nNew != static_cast<sal_uInt16>(*plVal))
            {
                *plVal = LanguageType(static_cast<sal_uInt16>(nNew));
                bMod = true;
            }
            bSucc = true;
        }
    }
    else if (pnInt32Val)
    {
        sal_Int32 nNew = sal_Int32();
        if (rValue >>= nNew)
        {
            if (nNew != *pnInt32Val)
            {
                *pnInt32Val = nNew;
                bMod = true;
            }
            bSucc = true;
        }
    }

    if (bMod)
        SetModified();

    NotifyListeners(ConfigurationHints::NONE);
    return bSucc;
}

void SvtLinguConfigItem::GetOptions(SvtLinguOptions &rOptions) const
{
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());
    rOptions = aOpt;
}

void SvtLinguConfigItem::LoadOptions( const uno::Sequence< OUString > &rProperyNames )
{
    bool bRes = false;

    const OUString *pProperyNames = rProperyNames.getConstArray();
    sal_Int32 nProps = rProperyNames.getLength();

    const uno::Sequence< uno::Any > aValues = GetProperties( rProperyNames );
    const uno::Sequence< sal_Bool > aROStates = GetReadOnlyStates( rProperyNames );

    if (nProps  &&  aValues.getLength() == nProps &&  aROStates.getLength() == nProps)
    {
        SvtLinguOptions &rOpt = aOpt;

        const uno::Any *pValue = aValues.getConstArray();
        const sal_Bool *pROStates = aROStates.getConstArray();
        for (sal_Int32 i = 0;  i < nProps;  ++i)
        {
            const uno::Any &rVal = pValue[i];
            sal_Int32 nPropertyHandle(0);
            GetHdlByName( nPropertyHandle, pProperyNames[i], true );
            switch ( nPropertyHandle )
            {
                case UPH_DEFAULT_LOCALE :
                    { rOpt.bRODefaultLanguage = pROStates[i]; rOpt.nDefaultLanguage = lcl_CfgAnyToLanguage( rVal ); } break;
                case UPH_ACTIVE_DICTIONARIES :
                    { rOpt.bROActiveDics = pROStates[i]; rVal >>= rOpt.aActiveDics;   } break;
                case UPH_IS_USE_DICTIONARY_LIST :
                    { rOpt.bROIsUseDictionaryList = pROStates[i]; rVal >>= rOpt.bIsUseDictionaryList;  } break;
                case UPH_IS_IGNORE_CONTROL_CHARACTERS :
                    { rOpt.bROIsIgnoreControlCharacters = pROStates[i]; rVal >>= rOpt.bIsIgnoreControlCharacters;    } break;
                case UPH_DEFAULT_LOCALE_CJK :
                    { rOpt.bRODefaultLanguage_CJK = pROStates[i]; rOpt.nDefaultLanguage_CJK = lcl_CfgAnyToLanguage( rVal );    } break;
                case UPH_DEFAULT_LOCALE_CTL :
                    { rOpt.bRODefaultLanguage_CTL = pROStates[i]; rOpt.nDefaultLanguage_CTL = lcl_CfgAnyToLanguage( rVal );    } break;

                case UPH_IS_SPELL_UPPER_CASE :
                    { rOpt.bROIsSpellUpperCase = pROStates[i]; rVal >>= rOpt.bIsSpellUpperCase; } break;
                case UPH_IS_SPELL_WITH_DIGITS :
                    { rOpt.bROIsSpellWithDigits = pROStates[i]; rVal >>= rOpt.bIsSpellWithDigits;    } break;
                case UPH_IS_SPELL_CLOSED_COMPOUND :
                    { rOpt.bROIsSpellClosedCompound = pROStates[i]; rVal >>= rOpt.bIsSpellClosedCompound;    } break;
                case UPH_IS_SPELL_HYPHENATED_COMPOUND :
                    { rOpt.bROIsSpellHyphenatedCompound = pROStates[i]; rVal >>= rOpt.bIsSpellHyphenatedCompound;    } break;

                case UPH_IS_SPELL_AUTO :
                    { rOpt.bROIsSpellAuto = pROStates[i]; rVal >>= rOpt.bIsSpellAuto;  } break;
                case UPH_IS_SPELL_SPECIAL :
                    { rOpt.bROIsSpellSpecial = pROStates[i]; rVal >>= rOpt.bIsSpellSpecial;   } break;
                case UPH_IS_WRAP_REVERSE :
                    { rOpt.bROIsSpellReverse = pROStates[i]; rVal >>= rOpt.bIsSpellReverse;   } break;

                case UPH_HYPH_MIN_LEADING :
                    { rOpt.bROHyphMinLeading = pROStates[i]; rVal >>= rOpt.nHyphMinLeading;   } break;
                case UPH_HYPH_MIN_TRAILING :
                    { rOpt.bROHyphMinTrailing = pROStates[i]; rVal >>= rOpt.nHyphMinTrailing;  } break;
                case UPH_HYPH_MIN_WORD_LENGTH :
                    { rOpt.bROHyphMinWordLength = pROStates[i]; rVal >>= rOpt.nHyphMinWordLength;    } break;
                case UPH_IS_HYPH_SPECIAL :
                    { rOpt.bROIsHyphSpecial = pROStates[i]; rVal >>= rOpt.bIsHyphSpecial;    } break;
                case UPH_IS_HYPH_AUTO :
                    { rOpt.bROIsHyphAuto = pROStates[i]; rVal >>= rOpt.bIsHyphAuto;   } break;

                case UPH_ACTIVE_CONVERSION_DICTIONARIES : { rOpt.bROActiveConvDics = pROStates[i]; rVal >>= rOpt.aActiveConvDics;   } break;

                case UPH_IS_IGNORE_POST_POSITIONAL_WORD :
                    { rOpt.bROIsIgnorePostPositionalWord = pROStates[i]; rVal >>= rOpt.bIsIgnorePostPositionalWord;  } break;
                case UPH_IS_AUTO_CLOSE_DIALOG :
                    { rOpt.bROIsAutoCloseDialog = pROStates[i]; rVal >>= rOpt.bIsAutoCloseDialog;  } break;
                case UPH_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST :
                    { rOpt.bROIsShowEntriesRecentlyUsedFirst = pROStates[i]; rVal >>= rOpt.bIsShowEntriesRecentlyUsedFirst;  } break;
                case UPH_IS_AUTO_REPLACE_UNIQUE_ENTRIES :
                    { rOpt.bROIsAutoReplaceUniqueEntries = pROStates[i]; rVal >>= rOpt.bIsAutoReplaceUniqueEntries;  } break;

                case UPH_IS_DIRECTION_TO_SIMPLIFIED :
                    {
                        rOpt.bROIsDirectionToSimplified = pROStates[i];
                        if( ! (rVal >>= rOpt.bIsDirectionToSimplified) )
                        {
                            //default is locale dependent:
                            if (MsLangId::isTraditionalChinese(rOpt.nDefaultLanguage_CJK))
                            {
                                rOpt.bIsDirectionToSimplified = false;
                            }
                            else
                            {
                                rOpt.bIsDirectionToSimplified = true;
                            }
                        }
                    } break;
                case UPH_IS_USE_CHARACTER_VARIANTS :
                    { rOpt.bROIsUseCharacterVariants = pROStates[i]; rVal >>= rOpt.bIsUseCharacterVariants;  } break;
                case UPH_IS_TRANSLATE_COMMON_TERMS :
                    { rOpt.bROIsTranslateCommonTerms = pROStates[i]; rVal >>= rOpt.bIsTranslateCommonTerms;  } break;
                case UPH_IS_REVERSE_MAPPING :
                    { rOpt.bROIsReverseMapping = pROStates[i]; rVal >>= rOpt.bIsReverseMapping;  } break;

                case UPH_DATA_FILES_CHANGED_CHECK_VALUE :
                    { rOpt.bRODataFilesChangedCheckValue = pROStates[i]; rVal >>= rOpt.nDataFilesChangedCheckValue;  } break;

                case UPH_IS_GRAMMAR_AUTO:
                    { rOpt.bROIsGrammarAuto = pROStates[i]; rVal >>= rOpt.bIsGrammarAuto; }
                break;
                case UPH_IS_GRAMMAR_INTERACTIVE:
                    { rOpt.bROIsGrammarInteractive = pROStates[i]; rVal >>= rOpt.bIsGrammarInteractive; }
                break;

                default:
                    SAL_WARN( "unotools.config", "unexpected case" );
            }
        }

        bRes = true;
    }
    DBG_ASSERT( bRes, "LoadOptions failed" );
}

bool SvtLinguConfigItem::SaveOptions( const uno::Sequence< OUString > &rProperyNames )
{
    if (!IsModified())
        return true;

    std::unique_lock aGuard(theSvtLinguConfigItemMutex());

    bool bRet = false;

    sal_Int32 nProps = rProperyNames.getLength();
    uno::Sequence< uno::Any > aValues( nProps );
    uno::Any *pValue = aValues.getArray();

    if (nProps  &&  aValues.getLength() == nProps)
    {
        const SvtLinguOptions &rOpt = aOpt;

        OUString aTmp( lcl_LanguageToCfgLocaleStr( rOpt.nDefaultLanguage ) );
        *pValue++ <<= aTmp;                               //   0
        *pValue++ <<= rOpt.aActiveDics;                   //   1
        *pValue++ <<= rOpt.bIsUseDictionaryList;        //   2
        *pValue++ <<= rOpt.bIsIgnoreControlCharacters;  //   3
        aTmp = lcl_LanguageToCfgLocaleStr( rOpt.nDefaultLanguage_CJK );
        *pValue++ <<= aTmp;                               //   5
        aTmp = lcl_LanguageToCfgLocaleStr( rOpt.nDefaultLanguage_CTL );
        *pValue++ <<= aTmp;                               //   6

        *pValue++ <<= rOpt.bIsSpellUpperCase;          //   7
        *pValue++ <<= rOpt.bIsSpellWithDigits;         //   8
        *pValue++ <<= rOpt.bIsSpellAuto;               //   9
        *pValue++ <<= rOpt.bIsSpellSpecial;            //  10
        *pValue++ <<= rOpt.bIsSpellClosedCompound;     //  11
        *pValue++ <<= rOpt.bIsSpellHyphenatedCompound; //  12
        *pValue++ <<= rOpt.bIsSpellReverse;            //  13

        *pValue++ <<= rOpt.nHyphMinLeading;            //  14
        *pValue++ <<= rOpt.nHyphMinTrailing;           //  15
        *pValue++ <<= rOpt.nHyphMinWordLength;         //  16
        *pValue++ <<= rOpt.bIsHyphSpecial;             //  17
        *pValue++ <<= rOpt.bIsHyphAuto;                //  18

        *pValue++ <<= rOpt.aActiveConvDics;               //   19

        *pValue++ <<= rOpt.bIsIgnorePostPositionalWord; //  20
        *pValue++ <<= rOpt.bIsAutoCloseDialog;          //  21
        *pValue++ <<= rOpt.bIsShowEntriesRecentlyUsedFirst; //  22
        *pValue++ <<= rOpt.bIsAutoReplaceUniqueEntries; //  23

        *pValue++ <<= rOpt.bIsDirectionToSimplified; //  24
        *pValue++ <<= rOpt.bIsUseCharacterVariants; //  25
        *pValue++ <<= rOpt.bIsTranslateCommonTerms; //  26
        *pValue++ <<= rOpt.bIsReverseMapping; //  27

        *pValue++ <<= rOpt.nDataFilesChangedCheckValue; //  28
        *pValue++ <<= rOpt.bIsGrammarAuto; //  29
        *pValue++ <<= rOpt.bIsGrammarInteractive; // 30

        bRet |= PutProperties( rProperyNames, aValues );
    }

    if (bRet)
        ClearModified();

    return bRet;
}

bool SvtLinguConfigItem::IsReadOnly( std::u16string_view rPropertyName ) const
{
    bool bReadOnly = false;
    sal_Int32 nHdl;
    if (GetHdlByName( nHdl, rPropertyName ))
        bReadOnly = IsReadOnly( nHdl );
    return bReadOnly;
}

bool SvtLinguConfigItem::IsReadOnly( sal_Int32 nPropertyHandle ) const
{
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());

    bool bReadOnly = false;

    const SvtLinguOptions &rOpt = const_cast< SvtLinguConfigItem * >(this)->aOpt;
    switch(nPropertyHandle)
    {
        case UPH_IS_USE_DICTIONARY_LIST         : bReadOnly = rOpt.bROIsUseDictionaryList; break;
        case UPH_IS_IGNORE_CONTROL_CHARACTERS   : bReadOnly = rOpt.bROIsIgnoreControlCharacters; break;
        case UPH_IS_HYPH_AUTO                   : bReadOnly = rOpt.bROIsHyphAuto; break;
        case UPH_IS_HYPH_SPECIAL                : bReadOnly = rOpt.bROIsHyphSpecial; break;
        case UPH_IS_SPELL_AUTO                  : bReadOnly = rOpt.bROIsSpellAuto; break;
        case UPH_IS_SPELL_SPECIAL               : bReadOnly = rOpt.bROIsSpellSpecial; break;
        case UPH_IS_WRAP_REVERSE                : bReadOnly = rOpt.bROIsSpellReverse; break;
        case UPH_DEFAULT_LANGUAGE               : bReadOnly = rOpt.bRODefaultLanguage; break;
        case UPH_IS_SPELL_CLOSED_COMPOUND       : bReadOnly = rOpt.bROIsSpellClosedCompound; break;
        case UPH_IS_SPELL_HYPHENATED_COMPOUND   : bReadOnly = rOpt.bROIsSpellHyphenatedCompound; break;
        case UPH_IS_SPELL_WITH_DIGITS           : bReadOnly = rOpt.bROIsSpellWithDigits; break;
        case UPH_IS_SPELL_UPPER_CASE            : bReadOnly = rOpt.bROIsSpellUpperCase; break;
        case UPH_HYPH_MIN_LEADING               : bReadOnly = rOpt.bROHyphMinLeading; break;
        case UPH_HYPH_MIN_TRAILING              : bReadOnly = rOpt.bROHyphMinTrailing; break;
        case UPH_HYPH_MIN_WORD_LENGTH           : bReadOnly = rOpt.bROHyphMinWordLength; break;
        case UPH_ACTIVE_DICTIONARIES            : bReadOnly = rOpt.bROActiveDics; break;
        case UPH_ACTIVE_CONVERSION_DICTIONARIES : bReadOnly = rOpt.bROActiveConvDics; break;
        case UPH_DEFAULT_LOCALE                 : bReadOnly = rOpt.bRODefaultLanguage; break;
        case UPH_DEFAULT_LOCALE_CJK             : bReadOnly = rOpt.bRODefaultLanguage_CJK; break;
        case UPH_DEFAULT_LOCALE_CTL             : bReadOnly = rOpt.bRODefaultLanguage_CTL; break;
        case UPH_IS_IGNORE_POST_POSITIONAL_WORD :       bReadOnly = rOpt.bROIsIgnorePostPositionalWord; break;
        case UPH_IS_AUTO_CLOSE_DIALOG :                 bReadOnly = rOpt.bROIsAutoCloseDialog; break;
        case UPH_IS_SHOW_ENTRIES_RECENTLY_USED_FIRST :  bReadOnly = rOpt.bROIsShowEntriesRecentlyUsedFirst; break;
        case UPH_IS_AUTO_REPLACE_UNIQUE_ENTRIES :       bReadOnly = rOpt.bROIsAutoReplaceUniqueEntries; break;
        case UPH_IS_DIRECTION_TO_SIMPLIFIED : bReadOnly = rOpt.bROIsDirectionToSimplified; break;
        case UPH_IS_USE_CHARACTER_VARIANTS : bReadOnly = rOpt.bROIsUseCharacterVariants; break;
        case UPH_IS_TRANSLATE_COMMON_TERMS : bReadOnly = rOpt.bROIsTranslateCommonTerms; break;
        case UPH_IS_REVERSE_MAPPING :        bReadOnly = rOpt.bROIsReverseMapping; break;
        case UPH_DATA_FILES_CHANGED_CHECK_VALUE :       bReadOnly = rOpt.bRODataFilesChangedCheckValue; break;
        case UPH_IS_GRAMMAR_AUTO:                       bReadOnly = rOpt.bROIsGrammarAuto; break;
        case UPH_IS_GRAMMAR_INTERACTIVE:                bReadOnly = rOpt.bROIsGrammarInteractive; break;
        default :
            SAL_WARN( "unotools.config", "unexpected property handle" );
    }
    return bReadOnly;
}

static SvtLinguConfigItem *pCfgItem = nullptr;
static sal_Int32           nCfgItemRefCount = 0;

constexpr OUString aG_Dictionaries = u"Dictionaries"_ustr;

SvtLinguConfig::SvtLinguConfig()
{
    // Global access, must be guarded (multithreading)
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());
    ++nCfgItemRefCount;
}

SvtLinguConfig::~SvtLinguConfig()
{
    if (pCfgItem && pCfgItem->IsModified())
        pCfgItem->Commit();

    std::unique_lock aGuard(theSvtLinguConfigItemMutex());

    if (--nCfgItemRefCount <= 0)
    {
        delete pCfgItem;
        pCfgItem = nullptr;
    }
}

SvtLinguConfigItem & SvtLinguConfig::GetConfigItem()
{
    // Global access, must be guarded (multithreading)
    std::unique_lock aGuard(theSvtLinguConfigItemMutex());
    if (!pCfgItem)
    {
        pCfgItem = new SvtLinguConfigItem;
        aGuard.unlock();
        ItemHolder1::holdConfigItem(EItem::LinguConfig);
    }
    return *pCfgItem;
}

uno::Sequence< OUString > SvtLinguConfig::GetNodeNames( const OUString &rNode ) const
{
    return GetConfigItem().GetNodeNames( rNode );
}

uno::Sequence< uno::Any > SvtLinguConfig::GetProperties( const uno::Sequence< OUString > &rNames ) const
{
    return GetConfigItem().GetProperties(rNames);
}

bool SvtLinguConfig::ReplaceSetProperties(
        const OUString &rNode, const uno::Sequence< beans::PropertyValue >& rValues )
{
    return GetConfigItem().ReplaceSetProperties( rNode, rValues );
}

uno::Any SvtLinguConfig::GetProperty( std::u16string_view rPropertyName ) const
{
    return GetConfigItem().GetProperty( rPropertyName );
}

uno::Any SvtLinguConfig::GetProperty( sal_Int32 nPropertyHandle ) const
{
    return GetConfigItem().GetProperty( nPropertyHandle );
}

bool SvtLinguConfig::SetProperty( std::u16string_view rPropertyName, const uno::Any &rValue )
{
    return GetConfigItem().SetProperty( rPropertyName, rValue );
}

bool SvtLinguConfig::SetProperty( sal_Int32 nPropertyHandle, const uno::Any &rValue )
{
    return GetConfigItem().SetProperty( nPropertyHandle, rValue );
}

void SvtLinguConfig::GetOptions( SvtLinguOptions &rOptions ) const
{
    GetConfigItem().GetOptions(rOptions);
}

bool SvtLinguConfig::IsReadOnly( std::u16string_view rPropertyName ) const
{
    return GetConfigItem().IsReadOnly( rPropertyName );
}

bool SvtLinguConfig::GetElementNamesFor(
     const OUString &rNodeName,
     uno::Sequence< OUString > &rElementNames ) const
{
    bool bSuccess = false;
    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName( rNodeName ), uno::UNO_QUERY );
        if (!xNA)
            return false;
        rElementNames = xNA->getElementNames();
        bSuccess = true;
    }
    catch (uno::Exception &)
    {
    }
    return bSuccess;
}

bool SvtLinguConfig::GetSupportedDictionaryFormatsFor(
    const OUString &rSetName,
    const OUString &rSetEntry,
    uno::Sequence< OUString > &rFormatList ) const
{
    if (rSetName.isEmpty() || rSetEntry.isEmpty())
        return false;
    bool bSuccess = false;
    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName( rSetName ), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName( rSetEntry ), uno::UNO_QUERY );
        if (!xNA)
            return false;
        if (xNA->getByName( u"SupportedDictionaryFormats"_ustr ) >>= rFormatList)
            bSuccess = true;
        DBG_ASSERT( rFormatList.hasElements(), "supported dictionary format list is empty" );
    }
    catch (uno::Exception &)
    {
    }
    return bSuccess;
}

bool SvtLinguConfig::GetLocaleListFor( const OUString &rSetName, const OUString &rSetEntry, css::uno::Sequence< OUString > &rLocaleList ) const
{
    if (rSetName.isEmpty() || rSetEntry.isEmpty())
        return false;
    bool bSuccess = false;
    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName( rSetName ), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName( rSetEntry ), uno::UNO_QUERY );
        if (!xNA)
            return false;
        if (xNA->getByName( u"Locales"_ustr ) >>= rLocaleList)
            bSuccess = true;
        DBG_ASSERT( rLocaleList.hasElements(), "Locale list is empty" );
    }
    catch (uno::Exception &)
    {
    }
    return bSuccess;
}

static bool lcl_GetFileUrlFromOrigin(
    OUString /*out*/ &rFileUrl,
    const OUString &rOrigin )
{
    OUString aURL(
        comphelper::getExpandedUri(
            comphelper::getProcessComponentContext(), rOrigin));
    if (aURL.startsWith( FILE_PROTOCOL ))
    {
        rFileUrl = aURL;
        return true;
    }
    else
    {
        SAL_WARN(
            "unotools.config", "not a file URL, <" << aURL << ">" );
        return false;
    }
}

bool SvtLinguConfig::GetDictionaryEntry(
    const OUString &rNodeName,
    SvtLinguConfigDictionaryEntry &rDicEntry ) const
{
    if (rNodeName.isEmpty())
        return false;
    bool bSuccess = false;
    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY_THROW );
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY_THROW );
        xNA.set( xNA->getByName( aG_Dictionaries ), uno::UNO_QUERY_THROW );
        xNA.set( xNA->getByName( rNodeName ), uno::UNO_QUERY_THROW );

        // read group data...
        uno::Sequence< OUString >  aLocations;
        OUString                   aFormatName;
        uno::Sequence< OUString >  aLocaleNames;
        bSuccess =  (xNA->getByName( u"Locations"_ustr ) >>= aLocations)  &&
                    (xNA->getByName( u"Format"_ustr )    >>= aFormatName) &&
                    (xNA->getByName( u"Locales"_ustr )   >>= aLocaleNames);
        DBG_ASSERT( aLocations.hasElements(), "Dictionary locations not set" );
        DBG_ASSERT( !aFormatName.isEmpty(), "Dictionary format name not set" );
        DBG_ASSERT( aLocaleNames.hasElements(), "No locales set for the dictionary" );

        // if successful continue
        if (bSuccess)
        {
            // get file URL's for the locations
            for (OUString& rLocation : asNonConstRange(aLocations))
            {
                if (!lcl_GetFileUrlFromOrigin( rLocation, rLocation ))
                    bSuccess = false;
            }

            // if everything was fine return the result
            if (bSuccess)
            {
                rDicEntry.aLocations    = std::move(aLocations);
                rDicEntry.aFormatName   = aFormatName;
                rDicEntry.aLocaleNames  = std::move(aLocaleNames);
            }
        }
    }
    catch (uno::Exception &)
    {
    }
    return bSuccess;
}

uno::Sequence< OUString > SvtLinguConfig::GetDisabledDictionaries() const
{
    uno::Sequence< OUString > aResult;
    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY );
        if (!xNA)
            return aResult;
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return aResult;
        xNA->getByName( u"DisabledDictionaries"_ustr ) >>= aResult;
    }
    catch (uno::Exception &)
    {
    }
    return aResult;
}

std::vector< SvtLinguConfigDictionaryEntry > SvtLinguConfig::GetActiveDictionariesByFormat(
    std::u16string_view rFormatName ) const
{
    std::vector< SvtLinguConfigDictionaryEntry > aRes;
    if (rFormatName.empty())
        return aRes;

    try
    {
        uno::Sequence< OUString > aElementNames;
        GetElementNamesFor( aG_Dictionaries, aElementNames );

        const uno::Sequence< OUString > aDisabledDics( GetDisabledDictionaries() );

        SvtLinguConfigDictionaryEntry aDicEntry;
        for (const OUString& rElementName : aElementNames)
        {
            // does dictionary match the format we are looking for?
            if (GetDictionaryEntry( rElementName, aDicEntry ) &&
                aDicEntry.aFormatName == rFormatName)
            {
                // check if it is active or not
                bool bDicIsActive = std::none_of(aDisabledDics.begin(), aDisabledDics.end(),
                    [&rElementName](const OUString& rDic) { return rDic == rElementName; });

                if (bDicIsActive)
                {
                    DBG_ASSERT( !aDicEntry.aFormatName.isEmpty(),
                            "FormatName not set" );
                    DBG_ASSERT( aDicEntry.aLocations.hasElements(),
                            "Locations not set" );
                    DBG_ASSERT( aDicEntry.aLocaleNames.hasElements(),
                            "Locales not set" );
                    aRes.push_back( aDicEntry );
                }
            }
        }
    }
    catch (uno::Exception &)
    {
    }

    return aRes;
}

uno::Reference< util::XChangesBatch > const & SvtLinguConfig::GetMainUpdateAccess() const
{
    if (m_xMainUpdateAccess)
        return m_xMainUpdateAccess;
    try
    {
        // get configuration provider
        const uno::Reference< uno::XComponentContext >& xContext = comphelper::getProcessComponentContext();
        uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider =
                configuration::theDefaultProvider::get( xContext );

        // get configuration update access
        beans::PropertyValue aValue;
        aValue.Name  = "nodepath";
        aValue.Value <<= u"org.openoffice.Office.Linguistic"_ustr;
        uno::Sequence< uno::Any > aProps{ uno::Any(aValue) };
        m_xMainUpdateAccess.set(
                xConfigurationProvider->createInstanceWithArguments(
                    u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr, aProps),
                    uno::UNO_QUERY );
    }
    catch (uno::Exception &)
    {
    }

    return m_xMainUpdateAccess;
}

OUString SvtLinguConfig::GetVendorImageUrl_Impl(
    const OUString &rServiceImplName,
    const OUString &rImageName ) const
{
    OUString aRes;
    try
    {
        uno::Reference< container::XNameAccess > xImagesNA( GetMainUpdateAccess(), uno::UNO_QUERY_THROW );
        xImagesNA.set( xImagesNA->getByName(u"Images"_ustr), uno::UNO_QUERY_THROW );

        uno::Reference< container::XNameAccess > xNA( xImagesNA->getByName(u"ServiceNameEntries"_ustr), uno::UNO_QUERY_THROW );
        xNA.set( xNA->getByName( rServiceImplName ), uno::UNO_QUERY_THROW );
        uno::Any aAny(xNA->getByName(u"VendorImagesNode"_ustr));
        OUString aVendorImagesNode;
        if (aAny >>= aVendorImagesNode)
        {
            xNA = std::move(xImagesNA);
            xNA.set( xNA->getByName(u"VendorImages"_ustr), uno::UNO_QUERY_THROW );
            xNA.set( xNA->getByName( aVendorImagesNode ), uno::UNO_QUERY_THROW );
            aAny = xNA->getByName( rImageName );
            OUString aTmp;
            if (aAny >>= aTmp)
            {
                if (lcl_GetFileUrlFromOrigin( aTmp, aTmp ))
                    aRes = aTmp;
            }
        }
    }
    catch (uno::Exception &)
    {
        DBG_UNHANDLED_EXCEPTION("unotools");
    }
    return aRes;
}

OUString SvtLinguConfig::GetSpellAndGrammarContextSuggestionImage(
    const OUString &rServiceImplName
) const
{
    OUString   aRes;
    if (!rServiceImplName.isEmpty())
    {
        aRes = GetVendorImageUrl_Impl( rServiceImplName, u"SpellAndGrammarContextMenuSuggestionImage"_ustr );
    }
    return aRes;
}

OUString SvtLinguConfig::GetSpellAndGrammarContextDictionaryImage(
    const OUString &rServiceImplName
) const
{
    OUString   aRes;
    if (!rServiceImplName.isEmpty())
    {
        aRes = GetVendorImageUrl_Impl( rServiceImplName, u"SpellAndGrammarContextMenuDictionaryImage"_ustr );
    }
    return aRes;
}

OUString SvtLinguConfig::GetSynonymsContextImage(
    const OUString &rServiceImplName
) const
{
    OUString   aRes;
    if (!rServiceImplName.isEmpty())
        aRes = GetVendorImageUrl_Impl(rServiceImplName, u"SynonymsContextMenuImage"_ustr);
    return aRes;
}

bool SvtLinguConfig::HasGrammarChecker() const
{
    bool bRes = false;

    try
    {
        uno::Reference< container::XNameAccess > xNA( GetMainUpdateAccess(), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName(u"ServiceManager"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return false;
        xNA.set( xNA->getByName(u"GrammarCheckerList"_ustr), uno::UNO_QUERY );
        if (!xNA)
            return false;

        uno::Sequence< OUString > aElementNames( xNA->getElementNames() );
        bRes = aElementNames.hasElements();
    }
    catch (const uno::Exception&)
    {
    }

    return bRes;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

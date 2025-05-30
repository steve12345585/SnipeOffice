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


#include <transliterationImpl.hxx>
#include <servicename.hxx>

#include <com/sun/star/i18n/LocaleData2.hpp>
#include <com/sun/star/i18n/TransliterationType.hpp>
#include <com/sun/star/i18n/TransliterationModulesExtra.hpp>

#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/string_view.hxx>
#include <rtl/ustring.hxx>

#include <algorithm>
#include <mutex>
#include <numeric>

using namespace com::sun::star::uno;
using namespace com::sun::star::i18n;
using namespace com::sun::star::lang;


namespace i18npool {

#define TmItem1( name ) \
  {TransliterationModules_##name, TransliterationModulesNew_##name, #name}

#define TmItem2( name ) \
  {TransliterationModules(0), TransliterationModulesNew_##name, #name}

namespace {

// Ignore Module list
struct TMList {
  TransliterationModules        tm;
  TransliterationModulesNew     tmn;
  const char                   *implName;
};

}

TMList const TMlist[] = {                //      Modules      ModulesNew
  TmItem1 (IGNORE_CASE),                        // 0. (1<<8        256) (7)
  TmItem1 (IGNORE_WIDTH),                       // 1. (1<<9        512) (8)
  TmItem1 (IGNORE_KANA),                        // 2. (1<<10      1024) (9)
// No enum define for this trans. application has to use impl name to load it
//  TmItem1 (IGNORE_CASE_SIMPLE),                       // (1<<11      1024) (66)

  {TransliterationModules_IgnoreTraditionalKanji_ja_JP,
   TransliterationModulesNew_IgnoreTraditionalKanji_ja_JP, "ignoreTraditionalKanji_ja_JP"},
                                                // 3. (1<<12      4096) (10)
  {TransliterationModules_IgnoreTraditionalKana_ja_JP,
   TransliterationModulesNew_IgnoreTraditionalKana_ja_JP, "ignoreTraditionalKana_ja_JP"},
                                                // 4. (1<<13      8192) (11)
  {TransliterationModules_IgnoreMinusSign_ja_JP, TransliterationModulesNew_IgnoreMinusSign_ja_JP,
   "ignoreMinusSign_ja_JP"},                    // 5. (1<<13     16384) (12)
  {TransliterationModules_IgnoreIterationMark_ja_JP,
   TransliterationModulesNew_IgnoreIterationMark_ja_JP, "ignoreIterationMark_ja_JP"},
                                                // 6. (1<<14     32768) (13)
  {TransliterationModules_IgnoreSeparator_ja_JP, TransliterationModulesNew_IgnoreSeparator_ja_JP,
   "ignoreSeparator_ja_JP"},                    // 7. (1<<15     65536) (14)
  {TransliterationModules_IgnoreSize_ja_JP, TransliterationModulesNew_IgnoreSize_ja_JP,
   "ignoreSize_ja_JP"},                         // 15. (1<<23  16777216) (22)
  {TransliterationModules_IgnoreMiddleDot_ja_JP, TransliterationModulesNew_IgnoreMiddleDot_ja_JP,
   "ignoreMiddleDot_ja_JP"},                    // 17. (1<<25  67108864) (24)
  {TransliterationModules_IgnoreSpace_ja_JP, TransliterationModulesNew_IgnoreSpace_ja_JP,
   "ignoreSpace_ja_JP"},                        // 18. (1<<26 134217728) (25)
  {TransliterationModules_IgnoreZiZu_ja_JP, TransliterationModulesNew_IgnoreZiZu_ja_JP,
   "ignoreZiZu_ja_JP"},                         // 8. (1<<16    131072) (15)
  {TransliterationModules_IgnoreBaFa_ja_JP, TransliterationModulesNew_IgnoreBaFa_ja_JP,
   "ignoreBaFa_ja_JP"},                         // 9. (1<<17    262144) (16)
  {TransliterationModules_IgnoreTiJi_ja_JP, TransliterationModulesNew_IgnoreTiJi_ja_JP,
   "ignoreTiJi_ja_JP"},                         // 10. (1<<18    524288) (17)
  {TransliterationModules_IgnoreHyuByu_ja_JP, TransliterationModulesNew_IgnoreHyuByu_ja_JP,
   "ignoreHyuByu_ja_JP"},                       // 11. (1<<19   1048576) (18)
  {TransliterationModules_IgnoreSeZe_ja_JP, TransliterationModulesNew_IgnoreSeZe_ja_JP,
   "ignoreSeZe_ja_JP"},                         // 12. (1<<20   2097152) (19)
  {TransliterationModules_IgnoreIandEfollowedByYa_ja_JP,
   TransliterationModulesNew_IgnoreIandEfollowedByYa_ja_JP, "ignoreIandEfollowedByYa_ja_JP"},
                                                // 13. (1<<21   4194304) (20)
  {TransliterationModules_IgnoreKiKuFollowedBySa_ja_JP,
   TransliterationModulesNew_IgnoreKiKuFollowedBySa_ja_JP, "ignoreKiKuFollowedBySa_ja_JP"},
                                                // 14. (1<<22   8388608) (21)
  {TransliterationModules_IgnoreProlongedSoundMark_ja_JP,
   TransliterationModulesNew_IgnoreProlongedSoundMark_ja_JP, "ignoreProlongedSoundMark_ja_JP"},
                                                // 16. (1<<24  33554432) (23)

  TmItem1 (UPPERCASE_LOWERCASE),        // 19. (1) (1)
  TmItem1 (LOWERCASE_UPPERCASE),        // 20. (2) (2)
  TmItem1 (HALFWIDTH_FULLWIDTH),        // 21. (3) (3)
  TmItem1 (FULLWIDTH_HALFWIDTH),        // 22. (4) (4)
  TmItem1 (KATAKANA_HIRAGANA),          // 23. (5) (5)
  TmItem1 (HIRAGANA_KATAKANA),          // 24. (6) (6)

  {TransliterationModules_SmallToLarge_ja_JP, TransliterationModulesNew_SmallToLarge_ja_JP,
   "smallToLarge_ja_JP"},               // 25. (1<<27 268435456) (26)
  {TransliterationModules_LargeToSmall_ja_JP, TransliterationModulesNew_LargeToSmall_ja_JP,
   "largeToSmall_ja_JP"},               // 26. (1<<28 536870912) (27)
  TmItem2 (NumToTextLower_zh_CN),       // 27. () (28)
  TmItem2 (NumToTextUpper_zh_CN),       // 28. () (29)
  TmItem2 (NumToTextLower_zh_TW),       // 29. () (30)
  TmItem2 (NumToTextUpper_zh_TW),       // 30. () (31)
  TmItem2 (NumToTextFormalHangul_ko),   // 31. () (32)
  TmItem2 (NumToTextFormalLower_ko),    // 32. () (33)
  TmItem2 (NumToTextFormalUpper_ko),    // 33. () (34)
  TmItem2 (NumToTextInformalHangul_ko), // 34. () (35)
  TmItem2 (NumToTextInformalLower_ko),  // 35. () (36)
  TmItem2 (NumToTextInformalUpper_ko),  // 36. () (37)
  TmItem2 (NumToCharLower_zh_CN),       // 37. () (38)
  TmItem2 (NumToCharUpper_zh_CN),       // 38. () (39)
  TmItem2 (NumToCharLower_zh_TW),       // 39. () (40)
  TmItem2 (NumToCharUpper_zh_TW),       // 40. () (41)
  TmItem2 (NumToCharHangul_ko),         // 41. () (42)
  TmItem2 (NumToCharLower_ko),          // 42. () (43)
  TmItem2 (NumToCharUpper_ko),          // 43. () (44)
  TmItem2 (NumToCharFullwidth),         // 44. () (45)
  TmItem2 (NumToCharKanjiShort_ja_JP),  // 45. () (46)
  TmItem2 (TextToNumLower_zh_CN),       // 46. () (47)
  TmItem2 (TextToNumUpper_zh_CN),       // 47. () (48)
  TmItem2 (TextToNumLower_zh_TW),       // 48. () (49)
  TmItem2 (TextToNumUpper_zh_TW),       // 49. () (50)
  TmItem2 (TextToNumFormalHangul_ko),   // 50. () (51)
  TmItem2 (TextToNumFormalLower_ko),    // 51. () (52)
  TmItem2 (TextToNumFormalUpper_ko),    // 52. () (53)
  TmItem2 (TextToNumInformalHangul_ko), // 53. () (54)
  TmItem2 (TextToNumInformalLower_ko),  // 54. () (55)
  TmItem2 (TextToNumInformalUpper_ko),  // 55. () (56)

  TmItem2 (CharToNumLower_zh_CN),       // 56. () (59)
  TmItem2 (CharToNumUpper_zh_CN),       // 57. () (60)
  TmItem2 (CharToNumLower_zh_TW),       // 58. () (61)
  TmItem2 (CharToNumUpper_zh_TW),       // 59. () (62)
  TmItem2 (CharToNumHangul_ko),         // 60. () (63)
  TmItem2 (CharToNumLower_ko),          // 61. () (64)
  TmItem2 (CharToNumUpper_ko),          // 62. () (65)

// no enum defined for these trans. application has to use impl name to load them
//  TmItem2 (NumToCharArabic_Indic),    // () (67)
//  TmItem2 (NumToCharEstern_Arabic_Indic),// () (68)
//  TmItem2 (NumToCharIndic),           // () (69)
//  TmItem2 (NumToCharThai),            // () (70)
  {TransliterationModules(0), TransliterationModulesNew(0),  nullptr}
};

// Constructor/Destructor
TransliterationImpl::TransliterationImpl(const Reference <XComponentContext>& xContext) : mxContext(xContext)
{
    numCascade = 0;
    caseignoreOnly = true;

    mxLocaledata.set(LocaleData2::create(xContext));
}

TransliterationImpl::~TransliterationImpl()
{
    mxLocaledata.clear();
    clear();
}


// Methods
OUString SAL_CALL
TransliterationImpl::getName()
{
    if (numCascade == 1 && bodyCascade[0].is())
        return bodyCascade[0]->getName();
    if (numCascade < 1)
        return ( u"Not Loaded"_ustr);
    throw RuntimeException();
}

sal_Int16 SAL_CALL
TransliterationImpl::getType()
{
    if (numCascade > 1)
        return (TransliterationType::CASCADE|TransliterationType::IGNORE);
    if (numCascade > 0 && bodyCascade[0].is())
        return bodyCascade[0]->getType();
    throw RuntimeException();
}

static TransliterationModules operator&(TransliterationModules lhs, TransliterationModules rhs) {
    return TransliterationModules(sal_Int32(lhs) & sal_Int32(rhs));
}
static TransliterationModules operator|(TransliterationModules lhs, TransliterationModules rhs) {
    return TransliterationModules(sal_Int32(lhs) | sal_Int32(rhs));
}

void SAL_CALL
TransliterationImpl::loadModule( TransliterationModules modType, const Locale& rLocale )
{
    clear();
    if (bool(modType & TransliterationModules_IGNORE_MASK) &&
        bool(modType & TransliterationModules_NON_IGNORE_MASK))
    {
        throw RuntimeException();
    } else if (bool(modType & TransliterationModules_IGNORE_MASK)) {
#define TransliterationModules_IGNORE_CASE_MASK (TransliterationModules_IGNORE_CASE | \
                                                TransliterationModules_IGNORE_WIDTH | \
                                                TransliterationModules_IGNORE_KANA)
        TransliterationModules mask = ((modType & TransliterationModules_IGNORE_CASE_MASK) == modType) ?
                TransliterationModules_IGNORE_CASE_MASK : TransliterationModules_IGNORE_MASK;
        for (sal_Int16 i = 0; bool(TMlist[i].tm & mask); i++) {
            if (bool(modType & TMlist[i].tm))
                if (loadModuleByName(OUString::createFromAscii(TMlist[i].implName),
                                                bodyCascade[numCascade], rLocale))
                    numCascade++;
        }
        // additional transliterations from TransliterationModulesExtra (we cannot extend TransliterationModules)
        if (bool(modType & TransliterationModules(TransliterationModulesExtra::IGNORE_DIACRITICS_CTL)))
        {
            if (loadModuleByName(u"ignoreDiacritics_CTL", bodyCascade[numCascade], rLocale))
                numCascade++;
        }
        if (bool(modType & TransliterationModules(TransliterationModulesExtra::IGNORE_KASHIDA_CTL)))
            if (loadModuleByName(u"ignoreKashida_CTL", bodyCascade[numCascade], rLocale))
                numCascade++;

    } else if (bool(modType & TransliterationModules_NON_IGNORE_MASK)) {
        for (sal_Int16 i = 0; bool(TMlist[i].tm); i++) {
            if (TMlist[i].tm == modType) {
                if (loadModuleByName(OUString::createFromAscii(TMlist[i].implName), bodyCascade[numCascade], rLocale))
                    numCascade++;
                break;
            }
        }
    }
}

void SAL_CALL
TransliterationImpl::loadModuleNew( const Sequence < TransliterationModulesNew > & modType, const Locale& rLocale )
{
    clear();
    TransliterationModules mask = TransliterationModules_END_OF_MODULE;
    sal_Int32 count = modType.getLength();
    if (count > maxCascade)
        throw RuntimeException(); // could not handle more than maxCascade
    for (sal_Int32 i = 0; i < count; i++) {
        for (sal_Int16 j = 0; bool(TMlist[j].tmn); j++) {
            if (TMlist[j].tmn == modType[i]) {
                if (mask == TransliterationModules_END_OF_MODULE)
                    mask = bool(TMlist[i].tm) && bool(TMlist[i].tm & TransliterationModules_IGNORE_MASK) ?
                        TransliterationModules_IGNORE_MASK : TransliterationModules_NON_IGNORE_MASK;
                else if (mask == TransliterationModules_IGNORE_MASK &&
                        (TMlist[i].tm&TransliterationModules_IGNORE_MASK) == TransliterationModules_END_OF_MODULE)
                    throw RuntimeException(); // could not mess up ignore trans. with non_ignore trans.
                if (loadModuleByName(OUString::createFromAscii(TMlist[j].implName), bodyCascade[numCascade], rLocale))
                    numCascade++;
                break;
            }
        }
    }
}

void SAL_CALL
TransliterationImpl::loadModuleByImplName(const OUString& implName, const Locale& rLocale)
{
    clear();
    if (loadModuleByName(implName, bodyCascade[numCascade], rLocale))
        numCascade++;
}


void SAL_CALL
TransliterationImpl::loadModulesByImplNames(const Sequence< OUString >& implNameList, const Locale& rLocale )
{
    if (implNameList.getLength() > maxCascade || implNameList.getLength() <= 0)
        throw RuntimeException();

    clear();
    for (const auto& rName : implNameList)
        if (loadModuleByName(rName, bodyCascade[numCascade], rLocale))
            numCascade++;
}


Sequence<OUString> SAL_CALL
TransliterationImpl::getAvailableModules( const Locale& rLocale, sal_Int16 sType )
{
    const Sequence<OUString> translist = mxLocaledata->getTransliterations(rLocale);
    std::vector<OUString> r;
    r.reserve(translist.getLength());
    Reference<XExtendedTransliteration> body;
    for (const auto& rTrans : translist)
    {
        if (loadModuleByName(rTrans, body, rLocale)) {
            if (body->getType() & sType)
                r.push_back(rTrans);
            body.clear();
        }
    }
    return comphelper::containerToSequence(r);
}


OUString SAL_CALL
TransliterationImpl::transliterate( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount,
                    Sequence< sal_Int32 >& offset )
{
    if (numCascade == 0)
        return inStr;

    if (numCascade == 1)
    {
        if ( startPos == 0 && nCount == inStr.getLength() )
            return bodyCascade[0]->transliterate( inStr, 0, nCount, offset);
        else
        {
            OUString tmpStr = inStr.copy(startPos, nCount);
            tmpStr = bodyCascade[0]->transliterate(tmpStr, 0, nCount, offset);
            if ( startPos )
            {
                for (sal_Int32 & j : asNonConstRange(offset))
                    j += startPos;
            }
            return tmpStr;
        }
    }
    else
    {
        OUString tmpStr = inStr.copy(startPos, nCount);

        auto [begin, end] = asNonConstRange(offset);
        std::iota(begin, end, startPos);

        Sequence<sal_Int32> from(nCount);
        Sequence<sal_Int32> to = offset;
        for (sal_Int32 i = 0; i < numCascade; i++) {
            tmpStr = bodyCascade[i]->transliterate(tmpStr, 0, nCount, from);

            nCount = tmpStr.getLength();

            assert(from.getLength() == nCount);
            from.swap(to);
            for (sal_Int32& ix : asNonConstRange(to))
                ix = from[ix];
        }
        offset = std::move(to);
        return tmpStr;
    }
}


OUString SAL_CALL
TransliterationImpl::folding( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount,
        Sequence< sal_Int32 >& offset )
{
    if (numCascade == 0)
        return inStr;

    if (offset.getLength() != nCount)
        offset.realloc(nCount);
    if (numCascade == 1)
    {
        if ( startPos == 0 && nCount == inStr.getLength() )
            return bodyCascade[0]->folding( inStr, 0, nCount, offset);
        else
        {
            OUString tmpStr = inStr.copy(startPos, nCount);
            tmpStr = bodyCascade[0]->folding(tmpStr, 0, nCount, offset);
            if ( startPos )
            {
                for (sal_Int32 & j : asNonConstRange(offset))
                    j += startPos;
            }
            return tmpStr;
        }
    }
    else
    {
        OUString tmpStr = inStr.copy(startPos, nCount);

        auto [begin, end] = asNonConstRange(offset);
        std::iota(begin, end, startPos);

        Sequence<sal_Int32> from;
        Sequence<sal_Int32> to = offset;

        for (sal_Int32 i = 0; i < numCascade; i++) {
            tmpStr = bodyCascade[i]->folding(tmpStr, 0, nCount, from);

            nCount = tmpStr.getLength();

            assert(from.getLength() == nCount);
            from.swap(to);
            for (sal_Int32& ix : asNonConstRange(to))
                ix = from[ix];
        }
        offset = std::move(to);
        return tmpStr;
    }
}

OUString SAL_CALL
TransliterationImpl::transliterateString2String( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount )
{
    if (numCascade == 0)
        return inStr;
    else if (numCascade == 1)
        return bodyCascade[0]->transliterateString2String( inStr, startPos, nCount);
    else {
        OUString tmpStr = bodyCascade[0]->transliterateString2String(inStr, startPos, nCount);

        for (sal_Int32 i = 1; i < numCascade; i++)
            tmpStr = bodyCascade[i]->transliterateString2String(tmpStr, 0, tmpStr.getLength());
        return tmpStr;
    }
}

OUString SAL_CALL
TransliterationImpl::transliterateChar2String( sal_Unicode inChar )
{
    if (numCascade == 0)
        return OUString(&inChar, 1);
    else if (numCascade == 1)
        return bodyCascade[0]->transliterateChar2String( inChar);
    else {
        OUString tmpStr = bodyCascade[0]->transliterateChar2String(inChar);

        for (sal_Int32 i = 1; i < numCascade; i++)
            tmpStr = bodyCascade[i]->transliterateString2String(tmpStr, 0, tmpStr.getLength());
        return tmpStr;
    }
}

sal_Unicode SAL_CALL
TransliterationImpl::transliterateChar2Char( sal_Unicode inChar )
{
    sal_Unicode tmpChar = inChar;
    for (sal_Int32 i = 0; i < numCascade; i++)
        tmpChar = bodyCascade[i]->transliterateChar2Char(tmpChar);
    return tmpChar;
}


sal_Bool SAL_CALL
TransliterationImpl::equals(
    const OUString& str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32& nMatch1,
    const OUString& str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32& nMatch2)
{
    // since this is an API function make it user fail safe
    if ( nCount1 < 0 ) {
        pos1 += nCount1;
        nCount1 = -nCount1;
    }
    if ( nCount2 < 0 ) {
        pos2 += nCount2;
        nCount2 = -nCount2;
    }
    if ( !nCount1 || !nCount2 ||
            pos1 >= str1.getLength() || pos2 >= str2.getLength() ||
            pos1 < 0 || pos2 < 0 ) {
        nMatch1 = nMatch2 = 0;
        // two empty strings return true, else false
        return !nCount1 && !nCount2 && pos1 == str1.getLength() && pos2 == str2.getLength();
    }
    if ( pos1 + nCount1 > str1.getLength() )
        nCount1 = str1.getLength() - pos1;
    if ( pos2 + nCount2 > str2.getLength() )
        nCount2 = str2.getLength() - pos2;

    if (caseignoreOnly && caseignore.is())
        return caseignore->equals(str1, pos1, nCount1, nMatch1, str2, pos2, nCount2, nMatch2);

    Sequence<sal_Int32> offset1, offset2;

    OUString tmpStr1 = folding(str1, pos1, nCount1, offset1);
    OUString tmpStr2 = folding(str2, pos2, nCount2, offset2);
    // Length of offset1 and offset2 may still be 0 if there was no folding
    // necessary!

    const sal_Unicode *p1 = tmpStr1.getStr();
    const sal_Unicode *p2 = tmpStr2.getStr();
    sal_Int32 i, nLen = ::std::min( tmpStr1.getLength(), tmpStr2.getLength());
    for (i = 0; i < nLen; ++i, ++p1, ++p2 ) {
        if (*p1 != *p2) {
            // return number of matched code points so far
            nMatch1 = (i < offset1.getLength()) ? offset1.getConstArray()[i] : i;
            nMatch2 = (i < offset2.getLength()) ? offset2.getConstArray()[i] : i;
            return false;
        }
    }
    // i==nLen
    if ( tmpStr1.getLength() != tmpStr2.getLength() ) {
        // return number of matched code points so far
        nMatch1 = (i <= offset1.getLength()) ? offset1.getConstArray()[i-1] + 1 : i;
        nMatch2 = (i <= offset2.getLength()) ? offset2.getConstArray()[i-1] + 1 : i;
        return false;
    } else {
        nMatch1 = nCount1;
        nMatch2 = nCount2;
        return true;
    }
}

Sequence< OUString >
TransliterationImpl::getRange(const Sequence< OUString > &inStrs,
                const sal_Int32 length, sal_Int16 _numCascade)
{
    if (_numCascade >= numCascade || ! bodyCascade[_numCascade].is())
        return inStrs;

    sal_Int32 j_tmp = 0;
    constexpr sal_Int32 nMaxOutput = 2;
    const sal_Int32 nMaxOutputLength = nMaxOutput*length;
    std::vector<OUString> ostr;
    ostr.reserve(nMaxOutputLength);
    for (sal_Int32 j = 0; j < length; j+=2) {
        const Sequence< OUString > temp = bodyCascade[_numCascade]->transliterateRange(inStrs[j], inStrs[j+1]);

        for (const auto& rStr : temp) {
            if ( j_tmp++ >= nMaxOutputLength ) throw RuntimeException();
            ostr.push_back(rStr);
        }
    }

    return getRange(comphelper::containerToSequence(ostr), j_tmp, ++_numCascade);
}


Sequence< OUString > SAL_CALL
TransliterationImpl::transliterateRange( const OUString& str1, const OUString& str2 )
{
    if (numCascade == 1)
        return bodyCascade[0]->transliterateRange(str1, str2);

    Sequence< OUString > ostr{ str1, str2 };

    return getRange(ostr, 2, 0);
}


sal_Int32 SAL_CALL
TransliterationImpl::compareSubstring(
    const OUString& str1, sal_Int32 off1, sal_Int32 len1,
    const OUString& str2, sal_Int32 off2, sal_Int32 len2)
{
    if (caseignoreOnly && caseignore.is())
        return caseignore->compareSubstring(str1, off1, len1, str2, off2, len2);

    Sequence <sal_Int32> offset;

    OUString in_str1 = transliterate(str1, off1, len1, offset);
    OUString in_str2 = transliterate(str2, off2, len2, offset);
    const sal_Unicode* unistr1 = in_str1.getStr();
    const sal_Unicode* unistr2 = in_str2.getStr();
    sal_Int32 strlen1 = in_str1.getLength();
    sal_Int32 strlen2 = in_str2.getLength();

    while (strlen1 && strlen2) {
        if (*unistr1 != *unistr2)
           return *unistr1 > *unistr2 ? 1 : -1;

        unistr1++; unistr2++; strlen1--; strlen2--;
    }
    return strlen1 == strlen2 ? 0 : (strlen1 > strlen2 ? 1 : -1);
}


sal_Int32 SAL_CALL
TransliterationImpl::compareString(const OUString& str1, const OUString& str2 )
{
    if (caseignoreOnly && caseignore.is())
        return caseignore->compareString(str1, str2);
    else
        return compareSubstring(str1, 0, str1.getLength(), str2, 0, str2.getLength());
}


void
TransliterationImpl::clear()
{
    for (sal_Int32 i = 0; i < numCascade; i++)
        if (bodyCascade[i].is())
            bodyCascade[i].clear();
    numCascade = 0;
    caseignore.clear();
    caseignoreOnly = true;
}

namespace
{
    /** structure to cache the last transliteration body used. */
    struct TransBody
    {
        OUString Name;
        css::uno::Reference< css::i18n::XExtendedTransliteration > Body;
    };
}

void TransliterationImpl::loadBody( OUString const &implName, Reference<XExtendedTransliteration>& body )
{
    assert(!implName.isEmpty());
    static std::mutex transBodyMutex;
    std::unique_lock guard(transBodyMutex);
    static TransBody lastTransBody;
    if (implName != lastTransBody.Name)
    {
        lastTransBody.Body.set(
            mxContext->getServiceManager()->createInstanceWithContext(implName, mxContext), UNO_QUERY_THROW);
        lastTransBody.Name = implName;
    }
    body = lastTransBody.Body;
}

bool
TransliterationImpl::loadModuleByName( std::u16string_view implName,
        Reference<XExtendedTransliteration>& body, const Locale& rLocale)
{
    OUString cname = OUString::Concat(TRLT_IMPLNAME_PREFIX) + implName;
    loadBody(cname, body);
    if (body.is()) {
        body->loadModule(TransliterationModules(0), rLocale); // toUpper/toLoad need rLocale

        // if the module is ignore case/kana/width, load caseignore for equals/compareString mothed
        for (sal_Int16 i = 0; i < 3; i++) {
            if (o3tl::equalsAscii(implName, TMlist[i].implName)) {
                if (i == 0) // current module is caseignore
                    body->loadModule(TMlist[0].tm, rLocale); // caseignore need to setup module name
                if (! caseignore.is()) {
                    OUString bname = TRLT_IMPLNAME_PREFIX +
                                OUString::createFromAscii(TMlist[0].implName);
                    loadBody(bname, caseignore);
                }
                if (caseignore.is())
                    caseignore->loadModule(TMlist[i].tm, rLocale);
                return true;
            }
        }
        caseignoreOnly = false; // has other module than just ignore case/kana/width
    }
    return body.is();
}

OUString SAL_CALL
TransliterationImpl::getImplementationName()
{
    return u"com.sun.star.i18n.Transliteration"_ustr;
}

sal_Bool SAL_CALL
TransliterationImpl::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL
TransliterationImpl::getSupportedServiceNames()
{
    return { u"com.sun.star.i18n.Transliteration"_ustr };
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_i18n_Transliteration_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new i18npool::TransliterationImpl(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/i18n/MultipleCharsOutputException.hpp>
#include <com/sun/star/i18n/TransliterationType.hpp>
#include <o3tl/temporary.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>

#include <textToPronounce_zh.hxx>
#include <indexentrysupplier_asian.hxx>

using namespace com::sun::star::i18n;
using namespace com::sun::star::uno;

namespace i18npool {

sal_Int16 SAL_CALL TextToPronounce_zh::getType()
{
    return TransliterationType::ONE_TO_ONE| TransliterationType::IGNORE;
}

const sal_Unicode*
TextToPronounce_zh::getPronounce(const sal_Unicode ch)
{
    static const sal_Unicode emptyString[]={0};
    if (idx) {
        sal_uInt16 address = idx[0][ch>>8];
        if (address != 0xFFFF)
            return reinterpret_cast<sal_Unicode const *>(
                &idx[2][idx[1][address + (ch & 0xFF)]]);
    }
    return emptyString;
}

OUString
TextToPronounce_zh::foldingImpl(const OUString & inStr, sal_Int32 startPos,
        sal_Int32 nCount, Sequence< sal_Int32 >* pOffset)
{
    OUStringBuffer sb;
    const sal_Unicode * chArr = inStr.getStr() + startPos;

    if (startPos < 0)
        throw RuntimeException();

    if (startPos + nCount > inStr.getLength())
        nCount = inStr.getLength() - startPos;

    auto ppOffset = pOffset ? pOffset->getArray() : nullptr;
    if (ppOffset)
        ppOffset[0] = 0;
    for (sal_Int32 i = 0; i < nCount; i++) {
        OUString pron(getPronounce(chArr[i]));
        sb.append(pron);

        if (ppOffset)
            ppOffset[i + 1] = (*pOffset)[i] + pron.getLength();
    }
    return sb.makeStringAndClear();
}

OUString SAL_CALL
TextToPronounce_zh::transliterateChar2String( sal_Unicode inChar)
{
    return OUString(getPronounce(inChar));
}

sal_Unicode SAL_CALL
TextToPronounce_zh::transliterateChar2Char( sal_Unicode inChar)
{
    const sal_Unicode* pron=getPronounce(inChar);
    if (!pron || !pron[0])
        return 0;
    if (pron[1])
        throw MultipleCharsOutputException();
    return *pron;
}

sal_Bool SAL_CALL
TextToPronounce_zh::equals( const OUString & str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32 & nMatch1,
        const OUString & str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32 & nMatch2)
{
    sal_Int32 realCount;
    int i;  // loop variable
    const sal_Unicode * s1, * s2;

    if (nCount1 + pos1 > str1.getLength())
        nCount1 = str1.getLength() - pos1;

    if (nCount2 + pos2 > str2.getLength())
        nCount2 = str2.getLength() - pos2;

    realCount = std::min(nCount1, nCount2);

    s1 = str1.getStr() + pos1;
    s2 = str2.getStr() + pos2;
    for (i = 0; i < realCount; i++) {
        const sal_Unicode *pron1 = getPronounce(*s1++);
        const sal_Unicode *pron2 = getPronounce(*s2++);
        if (pron1 != pron2) {
            nMatch1 = nMatch2 = i;
            return false;
        }
    }
    nMatch1 = nMatch2 = realCount;
    return (nCount1 == nCount2);
}

TextToPinyin_zh_CN::TextToPinyin_zh_CN() :
    TextToPronounce_zh(get_zh_pinyin)
{
        transliterationName = "ChineseCharacterToPinyin";
        implementationName = "com.sun.star.i18n.Transliteration.TextToPinyin_zh_CN";
}

TextToChuyin_zh_TW::TextToChuyin_zh_TW() :
    TextToPronounce_zh(get_zh_zhuyin)
{
        transliterationName = "ChineseCharacterToChuyin";
        implementationName = "com.sun.star.i18n.Transliteration.TextToChuyin_zh_TW";
}

TextToPronounce_zh::TextToPronounce_zh(sal_uInt16 const ** (*function)(sal_Int16 &))
{
    idx = function(o3tl::temporary(sal_Int16()));
}

TextToPronounce_zh::~TextToPronounce_zh()
{
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include <textconversion.hxx>
#include <com/sun/star/i18n/TextConversionType.hpp>
#include <com/sun/star/i18n/TextConversionOption.hpp>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <com/sun/star/linguistic2/ConversionDirection.hpp>
#include <com/sun/star/linguistic2/ConversionDictionaryType.hpp>
#include <com/sun/star/linguistic2/ConversionDictionaryList.hpp>
#include <memory>

using namespace com::sun::star::lang;
using namespace com::sun::star::i18n;
using namespace com::sun::star::linguistic2;
using namespace com::sun::star::uno;


namespace i18npool {

TextConversion_zh::TextConversion_zh( const Reference < XComponentContext >& xContext )
    : TextConversionService("com.sun.star.i18n.TextConversion_zh")
{
    xCDL = ConversionDictionaryList::create(xContext);
}

static sal_Unicode getOneCharConversion(sal_Unicode ch, const sal_Unicode* Data, const sal_uInt16* Index)
{
    if (Data && Index) {
        sal_Unicode address = Index[ch>>8];
        if (address != 0xFFFF)
            address = Data[address + (ch & 0xFF)];
        return (address != 0xFFFF) ? address : ch;
    } else {
        return ch;
    }
}

#ifdef DISABLE_DYNLOADING

extern "C" {


}

#endif

OUString
TextConversion_zh::getCharConversion(std::u16string_view aText, sal_Int32 nStartPos, sal_Int32 nLength, bool toSChinese, sal_Int32 nConversionOptions)
{
    const sal_Unicode *Data;
    const sal_uInt16 *Index;

    if (toSChinese) {
        Data = getSTC_CharData_T2S();
        Index = getSTC_CharIndex_T2S();
    } else if (nConversionOptions & TextConversionOption::USE_CHARACTER_VARIANTS) {
        Data = getSTC_CharData_S2V();
        Index = getSTC_CharIndex_S2V();
    } else {
        Data = getSTC_CharData_S2T();
        Index = getSTC_CharIndex_S2T();
    }

    rtl_uString * newStr = rtl_uString_alloc(nLength);
    for (sal_Int32 i = 0; i < nLength; i++)
        newStr->buffer[i] =
            getOneCharConversion(aText[nStartPos+i], Data, Index);
    return OUString(newStr, SAL_NO_ACQUIRE); //take ownership
}

OUString
TextConversion_zh::getWordConversion(std::u16string_view aText, sal_Int32 nStartPos, sal_Int32 nLength, bool toSChinese, sal_Int32 nConversionOptions, Sequence<sal_Int32>& offset)
{
    sal_Int32 dictLen = 0;
    sal_Int32 maxLen = 0;
    const sal_uInt16 *index;
    const sal_uInt16 *entry;
    const sal_Unicode *charData;
    const sal_uInt16 *charIndex;
    bool one2one=true;

    const sal_Unicode *wordData = getSTC_WordData(dictLen);
    if (toSChinese) {
        index = getSTC_WordIndex_T2S(maxLen);
        entry = getSTC_WordEntry_T2S();
        charData = getSTC_CharData_T2S();
        charIndex = getSTC_CharIndex_T2S();
    } else {
        index = getSTC_WordIndex_S2T(maxLen);
        entry = getSTC_WordEntry_S2T();
        if (nConversionOptions & TextConversionOption::USE_CHARACTER_VARIANTS) {
            charData = getSTC_CharData_S2V();
            charIndex = getSTC_CharIndex_S2V();
        } else {
            charData = getSTC_CharData_S2T();
            charIndex = getSTC_CharIndex_S2T();
        }
    }

    if ((!wordData || !index || !entry) && !xCDL.is()) // no word mapping defined, do char2char conversion.
        return getCharConversion(aText, nStartPos, nLength, toSChinese, nConversionOptions);

    std::unique_ptr<sal_Unicode[]> newStr(new sal_Unicode[nLength * 2 + 1]);
    sal_Int32 currPos = 0, count = 0;
    auto offsetRange = asNonConstRange(offset);
    while (currPos < nLength) {
        sal_Int32 len = nLength - currPos;
        bool found = false;
        if (len > maxLen)
            len = maxLen;
        for (; len > 0 && ! found; len--) {
            OUString word( aText.substr(nStartPos + currPos, len) );
            sal_Int32 current = 0;
            // user dictionary
            if (xCDL.is()) {
                Sequence < OUString > conversions;
                try {
                    conversions = xCDL->queryConversions(word, 0, len,
                            aLocale, ConversionDictionaryType::SCHINESE_TCHINESE,
                            /*toSChinese ?*/ ConversionDirection_FROM_LEFT /*: ConversionDirection_FROM_RIGHT*/,
                            nConversionOptions);
                }
                catch ( NoSupportException & ) {
                    // clear reference (when there is no user dictionary) in order
                    // to not always have to catch this exception again
                    // in further calls. (save time)
                    xCDL = nullptr;
                }
                catch (...) {
                    // catch all other exceptions to allow
                    // querying the system dictionary in the next line
                }
                if (conversions.hasElements()) {
                    if (offset.hasElements()) {
                        if (word.getLength() != conversions[0].getLength())
                            one2one=false;
                        while (current < conversions[0].getLength()) {
                            offsetRange[count] = nStartPos + currPos + (current *
                                    word.getLength() / conversions[0].getLength());
                            newStr[count++] = conversions[0][current++];
                        }
                        // offset[count-1] = nStartPos + currPos + word.getLength() - 1;
                    } else {
                        while (current < conversions[0].getLength())
                            newStr[count++] = conversions[0][current++];
                    }
                    currPos += word.getLength();
                    found = true;
                }
            }

            if (wordData && !found && index[len+1] - index[len] > 0) {
                sal_Int32 bottom = static_cast<sal_Int32>(index[len]);
                sal_Int32 top = static_cast<sal_Int32>(index[len+1]) - 1;

                while (bottom <= top && !found) {
                    current = (top + bottom) / 2;
                    const sal_Int32 result = rtl_ustr_compare(
                        word.getStr(), wordData + entry[current]);
                    if (result < 0)
                        top = current - 1;
                    else if (result > 0)
                        bottom = current + 1;
                    else {
                        if (toSChinese)   // Traditionary/Simplified conversion,
                            for (current = entry[current]-1; current > 0 && wordData[current-1]; current--) ;
                        else  // Simplified/Traditionary conversion, forwards search for next word
                            current = entry[current] + word.getLength() + 1;
                        sal_Int32 start=current;
                        if (offset.hasElements()) {
                            if (word.getLength() != static_cast<sal_Int32>(std::u16string_view(&wordData[current]).size()))
                                one2one=false;
                            sal_Int32 convertedLength=std::u16string_view(&wordData[current]).size();
                            while (wordData[current]) {
                                offsetRange[count]=nStartPos + currPos + ((current-start) *
                                    word.getLength() / convertedLength);
                                newStr[count++] = wordData[current++];
                            }
                            // offset[count-1]=nStartPos + currPos + word.getLength() - 1;
                        } else {
                            while (wordData[current])
                                newStr[count++] = wordData[current++];
                        }
                        currPos += word.getLength();
                        found = true;
                    }
                }
            }
        }
        if (!found) {
            if (offset.hasElements())
                offsetRange[count]=nStartPos+currPos;
            newStr[count++] =
                getOneCharConversion(aText[nStartPos+currPos], charData, charIndex);
            currPos++;
        }
    }
    if (offset.hasElements())
        offset.realloc(one2one ? 0 : count);
    OUString aRet(newStr.get(), count);
    return aRet;
}

TextConversionResult SAL_CALL
TextConversion_zh::getConversions( const OUString& aText, sal_Int32 nStartPos, sal_Int32 nLength,
    const Locale& rLocale, sal_Int16 nConversionType, sal_Int32 nConversionOptions)
{
    TextConversionResult result;

    result.Candidates =
        { getConversion( aText, nStartPos, nLength, rLocale, nConversionType, nConversionOptions) };
    result.Boundary.startPos = nStartPos;
    result.Boundary.endPos = nStartPos + nLength;

    return result;
}

OUString SAL_CALL
TextConversion_zh::getConversion( const OUString& aText, sal_Int32 nStartPos, sal_Int32 nLength,
    const Locale& rLocale, sal_Int16 nConversionType, sal_Int32 nConversionOptions)
{
    if (rLocale.Language != "zh" || ( nConversionType != TextConversionType::TO_SCHINESE && nConversionType != TextConversionType::TO_TCHINESE) )
        throw NoSupportException(); // Conversion type is not supported in this service.

    aLocale=rLocale;
    bool toSChinese = nConversionType == TextConversionType::TO_SCHINESE;

    if (nConversionOptions & TextConversionOption::CHARACTER_BY_CHARACTER)
        // char to char dictionary
        return getCharConversion(aText, nStartPos, nLength, toSChinese, nConversionOptions);
    else {
        Sequence <sal_Int32> offset;
        // word to word dictionary
        return  getWordConversion(aText, nStartPos, nLength, toSChinese, nConversionOptions, offset);
    }
}

OUString SAL_CALL
TextConversion_zh::getConversionWithOffset( const OUString& aText, sal_Int32 nStartPos, sal_Int32 nLength,
    const Locale& rLocale, sal_Int16 nConversionType, sal_Int32 nConversionOptions, Sequence<sal_Int32>& offset)
{
    if (rLocale.Language != "zh" || ( nConversionType != TextConversionType::TO_SCHINESE && nConversionType != TextConversionType::TO_TCHINESE) )
        throw NoSupportException(); // Conversion type is not supported in this service.

    aLocale=rLocale;
    bool toSChinese = nConversionType == TextConversionType::TO_SCHINESE;

    if (nConversionOptions & TextConversionOption::CHARACTER_BY_CHARACTER) {
        offset.realloc(0);
        // char to char dictionary
        return getCharConversion(aText, nStartPos, nLength, toSChinese, nConversionOptions);
    } else {
        if (offset.getLength() < 2*nLength)
            offset.realloc(2*nLength);
        // word to word dictionary
        return  getWordConversion(aText, nStartPos, nLength, toSChinese, nConversionOptions, offset);
    }
}

sal_Bool SAL_CALL
TextConversion_zh::interactiveConversion( const Locale& /*rLocale*/, sal_Int16 /*nTextConversionType*/, sal_Int32 /*nTextConversionOptions*/ )
{
    return false;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

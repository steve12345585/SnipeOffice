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

#include <cassert>
#include <cstdlib>

#include <osl/diagnose.h>
#include <rtl/tencinfo.h>

#include "strimp.hxx"
#include <rtl/character.hxx>
#include <rtl/string.h>

#include <rtl/math.h>

/* ======================================================================= */

#if USE_SDT_PROBES
#define RTL_LOG_STRING_BITS         8
#endif

#include "strtmpl.hxx"

/* ======================================================================= */

sal_Int32 SAL_CALL rtl_str_valueOfFloat(char * pStr, float f) noexcept
{
    return rtl::str::valueOfFP<RTL_STR_MAX_VALUEOFFLOAT>(pStr, f);
}

sal_Int32 SAL_CALL rtl_str_valueOfDouble(char * pStr, double d) noexcept
{
    return rtl::str::valueOfFP<RTL_STR_MAX_VALUEOFDOUBLE>(pStr, d);
}

float SAL_CALL rtl_str_toFloat(char const * pStr) noexcept
{
    assert(pStr);
    return static_cast<float>(rtl_math_stringToDouble(pStr, pStr + rtl_str_getLength(pStr),
                                           '.', 0, nullptr, nullptr));
}

double SAL_CALL rtl_str_toDouble(char const * pStr) noexcept
{
    assert(pStr);
    return rtl_math_stringToDouble(pStr, pStr + rtl_str_getLength(pStr), '.', 0,
                                   nullptr, nullptr);
}

/* ======================================================================= */

static int rtl_ImplGetFastUTF8ByteLen( const sal_Unicode* pStr, sal_Int32 nLen )
{
    int                 n;
    sal_Unicode         c;
    sal_uInt32          nUCS4Char;
    const sal_Unicode*  pEndStr;

    n = 0;
    pEndStr  = pStr+nLen;
    while ( pStr < pEndStr )
    {
        c = *pStr;

        if ( c < 0x80 )
            n++;
        else if ( c < 0x800 )
            n += 2;
        else
        {
            if ( !rtl::isHighSurrogate(c) )
                n += 3;
            else
            {
                nUCS4Char = c;

                if ( pStr+1 < pEndStr )
                {
                    c = *(pStr+1);
                    if ( rtl::isLowSurrogate(c) )
                    {
                        nUCS4Char = rtl::combineSurrogates(nUCS4Char, c);
                        pStr++;
                    }
                }

                if ( nUCS4Char < 0x10000 )
                    n += 3;
                else if ( nUCS4Char < 0x200000 )
                    n += 4;
                else if ( nUCS4Char < 0x4000000 )
                    n += 5;
                else
                    n += 6;
            }
        }

        pStr++;
    }

    return n;
}

/* ----------------------------------------------------------------------- */

static bool rtl_impl_convertUStringToString(rtl_String ** pTarget,
                                                  sal_Unicode const * pSource,
                                                  sal_Int32 nLength,
                                                  rtl_TextEncoding nEncoding,
                                                  sal_uInt32 nFlags,
                                                  bool bCheckErrors)
{
    assert(pTarget != nullptr);
    assert(pSource != nullptr || nLength == 0);
    assert(nLength >= 0);
    OSL_ASSERT(nLength == 0 || rtl_isOctetTextEncoding(nEncoding));

    if ( !nLength )
        rtl_string_new( pTarget );
    else
    {
        rtl_String*                 pTemp;
        rtl_UnicodeToTextConverter  hConverter;
        sal_uInt32                  nInfo;
        sal_Size                    nSrcChars;
        sal_Size                    nDestBytes;
        sal_Size                    nNewLen;
        sal_Size                    nNotConvertedChars;
        sal_Size                    nMaxCharLen;

        /* Optimization for UTF-8 - we try to calculate the exact length */
        /* For all other encoding we try a good estimation */
        if ( nEncoding == RTL_TEXTENCODING_UTF8 )
        {
            nNewLen = rtl_ImplGetFastUTF8ByteLen( pSource, nLength );
            /* Includes the string only ASCII, then we could copy
               the buffer faster */
            if ( nNewLen == static_cast<sal_Size>(nLength) )
            {
                char* pBuffer;
                if ( *pTarget )
                    rtl_string_release( *pTarget );
                *pTarget = rtl_string_ImplAlloc( nLength );
                assert(*pTarget != nullptr);
                pBuffer = (*pTarget)->buffer;
                do
                {
                    /* Check ASCII range */
                    OSL_ENSURE( *pSource <= 127,
                                "rtl_uString2String() - UTF8 test is encoding is wrong" );

                    *pBuffer = static_cast<char>(static_cast<unsigned char>(*pSource));
                    pBuffer++;
                    pSource++;
                    nLength--;
                }
                while ( nLength );
                return true;
            }

            nMaxCharLen = 4;
        }
        else
        {
            rtl_TextEncodingInfo aTextEncInfo;
            aTextEncInfo.StructSize = sizeof( aTextEncInfo );
            if ( !rtl_getTextEncodingInfo( nEncoding, &aTextEncInfo ) )
            {
                aTextEncInfo.AverageCharSize    = 1;
                aTextEncInfo.MaximumCharSize    = 8;
            }

            nNewLen = nLength * static_cast<sal_Size>(aTextEncInfo.AverageCharSize);
            nMaxCharLen = aTextEncInfo.MaximumCharSize;
        }

        nFlags |= RTL_UNICODETOTEXT_FLAGS_FLUSH;
        hConverter = rtl_createUnicodeToTextConverter( nEncoding );

        for (;;)
        {
            pTemp = rtl_string_ImplAlloc( nNewLen );
            assert(pTemp != nullptr);
            nDestBytes = rtl_convertUnicodeToText( hConverter, nullptr,
                                                   pSource, nLength,
                                                   pTemp->buffer, nNewLen,
                                                   nFlags,
                                                   &nInfo, &nSrcChars );
            if (bCheckErrors && (nInfo & RTL_UNICODETOTEXT_INFO_ERROR) != 0)
            {
                rtl_freeString(pTemp);
                rtl_destroyUnicodeToTextConverter(hConverter);
                return false;
            }

            if ((nInfo & RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL) == 0)
                break;

            /* Buffer not big enough, try again with enough space */
            rtl_freeString( pTemp );

            /* Try with the max. count of characters with
               additional overhead for replacing functionality */
            nNotConvertedChars = nLength-nSrcChars;
            nNewLen = nDestBytes+(nNotConvertedChars*nMaxCharLen)+nNotConvertedChars+4;
        }

        /* Set the buffer to the correct size or is there to
           much overhead, reallocate to the correct size */
        if ( nNewLen > nDestBytes+8 )
        {
            rtl_String* pTemp2 = rtl_string_ImplAlloc( nDestBytes );
            assert(pTemp2 != nullptr);
            rtl::str::Copy(pTemp2->buffer, pTemp->buffer, nDestBytes);
            rtl_freeString( pTemp );
            pTemp = pTemp2;
        }
        else
        {
            pTemp->length = nDestBytes;
            pTemp->buffer[nDestBytes] = 0;
        }

        rtl_destroyUnicodeToTextConverter( hConverter );
        if ( *pTarget )
            rtl_string_release( *pTarget );
        *pTarget = pTemp;

        /* Results the conversion in an empty buffer -
           create an empty string */
        if ( pTemp && !nDestBytes )
            rtl_string_new( pTarget );
    }
    return true;
}

void SAL_CALL rtl_uString2String( rtl_String** ppThis,
                                  const sal_Unicode* pUStr,
                                  sal_Int32 nULen,
                                  rtl_TextEncoding eTextEncoding,
                                  sal_uInt32 nCvtFlags ) noexcept
{
    rtl_impl_convertUStringToString(ppThis, pUStr, nULen, eTextEncoding,
                                    nCvtFlags, false);
}

sal_Bool SAL_CALL rtl_convertUStringToString(rtl_String ** pTarget,
                                             sal_Unicode const * pSource,
                                             sal_Int32 nLength,
                                             rtl_TextEncoding nEncoding,
                                             sal_uInt32 nFlags) noexcept
{
    return rtl_impl_convertUStringToString(pTarget, pSource, nLength, nEncoding,
                                           nFlags, true);
}

void rtl_string_newReplaceFirst(
    rtl_String ** newStr, rtl_String * str, char const * from,
    sal_Int32 fromLength, char const * to, sal_Int32 toLength,
    sal_Int32 * index) noexcept
{
    assert(str != nullptr);
    assert(index != nullptr);
    assert(*index >= 0 && *index <= str->length);
    assert(fromLength >= 0);
    assert(toLength >= 0);
    sal_Int32 i = rtl_str_indexOfStr_WithLength(
        str->buffer + *index, str->length - *index, from, fromLength);
    if (i == -1) {
        rtl_string_assign(newStr, str);
    } else {
        assert(i <= str->length - *index);
        i += *index;
        assert(fromLength <= str->length);
        if (str->length - fromLength > SAL_MAX_INT32 - toLength) {
            std::abort();
        }
        rtl::str::newReplaceStrAt(newStr, str, i, fromLength, to, toLength);
    }
    *index = i;
}

void rtl_string_newReplaceAll(
    rtl_String ** newStr, rtl_String * str, char const * from,
    sal_Int32 fromLength, char const * to, sal_Int32 toLength) noexcept
{
    rtl::str::newReplaceAllFromIndex(newStr, str, from, fromLength, to, toLength, 0);
}

sal_Int32 SAL_CALL rtl_str_getLength(const char* pStr) noexcept
{
    return rtl::str::getLength(pStr);
}

sal_Int32 SAL_CALL rtl_str_compare(const char* pStr1, const char* pStr2) noexcept
{
    return rtl::str::compare(rtl::str::null_terminated(pStr1), rtl::str::null_terminated(pStr2),
                             rtl::str::CompareNormal(), rtl::str::noShortening);
}

sal_Int32 SAL_CALL rtl_str_compare_WithLength(const char* pStr1, sal_Int32 nStr1Len,
                                              const char* pStr2, sal_Int32 nStr2Len) noexcept
{
    return rtl::str::compare(rtl::str::with_length(pStr1, nStr1Len),
                             rtl::str::with_length(pStr2, nStr2Len),
                             rtl::str::CompareNormal(), rtl::str::noShortening);
}

sal_Int32 SAL_CALL rtl_str_shortenedCompare_WithLength(const char* pStr1, sal_Int32 nStr1Len,
                                                       const char* pStr2, sal_Int32 nStr2Len,
                                                       sal_Int32 nShortenedLength) noexcept
{
    return rtl::str::compare(rtl::str::with_length(pStr1, nStr1Len),
                             rtl::str::with_length(pStr2, nStr2Len),
                             rtl::str::CompareNormal(), nShortenedLength);
}

sal_Int32 SAL_CALL rtl_str_reverseCompare_WithLength(const char* pStr1, sal_Int32 nStr1Len,
                                                     const char* pStr2, sal_Int32 nStr2Len) noexcept
{
    return rtl::str::reverseCompare_WithLengths(pStr1, nStr1Len, pStr2, nStr2Len,
                                                rtl::str::CompareNormal());
}

sal_Int32 SAL_CALL rtl_str_compareIgnoreAsciiCase(const char* pStr1, const char* pStr2) noexcept
{
    return rtl::str::compare(rtl::str::null_terminated(pStr1), rtl::str::null_terminated(pStr2),
                             rtl::str::CompareIgnoreAsciiCase(), rtl::str::noShortening);
}

sal_Int32 SAL_CALL rtl_str_compareIgnoreAsciiCase_WithLength(const char* pStr1, sal_Int32 nStr1Len,
                                                             const char* pStr2, sal_Int32 nStr2Len) noexcept
{
    return rtl::str::compare(rtl::str::with_length(pStr1, nStr1Len),
                             rtl::str::with_length(pStr2, nStr2Len),
                             rtl::str::CompareIgnoreAsciiCase(), rtl::str::noShortening);
}

sal_Int32 SAL_CALL rtl_str_shortenedCompareIgnoreAsciiCase_WithLength(
    const char* pStr1, sal_Int32 nStr1Len, const char* pStr2, sal_Int32 nStr2Len,
    sal_Int32 nShortenedLength) noexcept
{
    return rtl::str::compare(rtl::str::with_length(pStr1, nStr1Len),
                             rtl::str::with_length(pStr2, nStr2Len),
                             rtl::str::CompareIgnoreAsciiCase(), nShortenedLength);
}

sal_Int32 SAL_CALL rtl_str_hashCode(const char* pStr) noexcept
{
    return rtl::str::hashCode(pStr);
}

sal_Int32 SAL_CALL rtl_str_hashCode_WithLength(const char* pStr, sal_Int32 nLen) noexcept
{
    return rtl::str::hashCode_WithLength(pStr, nLen);
}

sal_Int32 SAL_CALL rtl_str_indexOfChar(const char* pStr, char c) noexcept
{
    return rtl::str::indexOfChar(pStr, c);
}

sal_Int32 SAL_CALL rtl_str_indexOfChar_WithLength(const char* pStr, sal_Int32 nLen, char c) noexcept
{
    return rtl::str::indexOfChar_WithLength(pStr, nLen, c);
}

sal_Int32 SAL_CALL rtl_str_lastIndexOfChar(const char* pStr, char c) noexcept
{
    return rtl::str::lastIndexOfChar(pStr, c);
}

sal_Int32 SAL_CALL rtl_str_lastIndexOfChar_WithLength(const char* pStr, sal_Int32 nLen, char c) noexcept
{
    return rtl::str::lastIndexOfChar_WithLength(pStr, nLen, c);
}

sal_Int32 SAL_CALL rtl_str_indexOfStr(const char* pStr, const char* pSubStr) noexcept
{
    return rtl::str::indexOfStr(pStr, pSubStr);
}

sal_Int32 SAL_CALL rtl_str_indexOfStr_WithLength(const char* pStr, sal_Int32 nStrLen,
                                                 const char* pSubStr, sal_Int32 nSubLen) noexcept
{
    return rtl::str::indexOfStr_WithLength(pStr, nStrLen, pSubStr, nSubLen);
}

sal_Int32 SAL_CALL rtl_str_lastIndexOfStr(const char* pStr, const char* pSubStr) noexcept
{
    return rtl::str::lastIndexOfStr(pStr, pSubStr);
}

sal_Int32 SAL_CALL rtl_str_lastIndexOfStr_WithLength(const char* pStr, sal_Int32 nStrLen,
                                                     const char* pSubStr, sal_Int32 nSubLen) noexcept
{
    return rtl::str::lastIndexOfStr_WithLength(pStr, nStrLen, pSubStr, nSubLen);
}

void SAL_CALL rtl_str_replaceChar(char* pStr, char cOld, char cNew) noexcept
{
    return rtl::str::replaceChars(rtl::str::null_terminated(pStr), rtl::str::FromTo(cOld, cNew));
}

void SAL_CALL rtl_str_replaceChar_WithLength(char* pStr, sal_Int32 nLen, char cOld, char cNew) noexcept
{
    return rtl::str::replaceChars(rtl::str::with_length(pStr, nLen), rtl::str::FromTo(cOld, cNew));
}

void SAL_CALL rtl_str_toAsciiLowerCase(char* pStr) noexcept
{
    return rtl::str::replaceChars(rtl::str::null_terminated(pStr), rtl::str::toAsciiLower);
}

void SAL_CALL rtl_str_toAsciiLowerCase_WithLength(char* pStr, sal_Int32 nLen) noexcept
{
    return rtl::str::replaceChars(rtl::str::with_length(pStr, nLen), rtl::str::toAsciiLower);
}

void SAL_CALL rtl_str_toAsciiUpperCase(char* pStr) noexcept
{
    return rtl::str::replaceChars(rtl::str::null_terminated(pStr), rtl::str::toAsciiUpper);
}

void SAL_CALL rtl_str_toAsciiUpperCase_WithLength(char* pStr, sal_Int32 nLen) noexcept
{
    return rtl::str::replaceChars(rtl::str::with_length(pStr, nLen), rtl::str::toAsciiUpper);
}

sal_Int32 SAL_CALL rtl_str_trim(char* pStr) noexcept { return rtl::str::trim(pStr); }

sal_Int32 SAL_CALL rtl_str_trim_WithLength(char* pStr, sal_Int32 nLen) noexcept
{
    return rtl::str::trim_WithLength(pStr, nLen);
}

sal_Int32 SAL_CALL rtl_str_valueOfBoolean(char* pStr, sal_Bool b) noexcept
{
    return rtl::str::valueOfBoolean(pStr, b);
}

sal_Int32 SAL_CALL rtl_str_valueOfChar(char* pStr, char c) noexcept
{
    return rtl::str::valueOfChar(pStr, c);
}

sal_Int32 SAL_CALL rtl_str_valueOfInt32(char* pStr, sal_Int32 n, sal_Int16 nRadix) noexcept
{
    return rtl::str::valueOfInt<RTL_STR_MAX_VALUEOFINT32>(pStr, n, nRadix);
}

sal_Int32 SAL_CALL rtl_str_valueOfInt64(char* pStr, sal_Int64 n, sal_Int16 nRadix) noexcept
{
    return rtl::str::valueOfInt<RTL_STR_MAX_VALUEOFINT64>(pStr, n, nRadix);
}

sal_Int32 SAL_CALL rtl_str_valueOfUInt64(char* pStr, sal_uInt64 n, sal_Int16 nRadix) noexcept
{
    return rtl::str::valueOfInt<RTL_STR_MAX_VALUEOFUINT64>(pStr, n, nRadix);
}

sal_Bool SAL_CALL rtl_str_toBoolean(const char* pStr) noexcept
{
    return rtl::str::toBoolean(pStr);
}

sal_Int32 SAL_CALL rtl_str_toInt32(const char* pStr, sal_Int16 nRadix) noexcept
{
    return rtl::str::toInt<sal_Int32>(rtl::str::null_terminated(pStr), nRadix);
}

sal_Int64 SAL_CALL rtl_str_toInt64(const char* pStr, sal_Int16 nRadix) noexcept
{
    return rtl::str::toInt<sal_Int64>(rtl::str::null_terminated(pStr), nRadix);
}

sal_Int64 SAL_CALL rtl_str_toInt64_WithLength(const char* pStr, sal_Int16 nRadix,
                                              sal_Int32 nStrLength) noexcept
{
    return rtl::str::toInt<sal_Int64>(rtl::str::with_length(pStr, nStrLength), nRadix);
}

sal_uInt32 SAL_CALL rtl_str_toUInt32(const char* pStr, sal_Int16 nRadix) noexcept
{
    return rtl::str::toInt<sal_uInt32>(rtl::str::null_terminated(pStr), nRadix);
}

sal_uInt64 SAL_CALL rtl_str_toUInt64(const char* pStr, sal_Int16 nRadix) noexcept
{
    return rtl::str::toInt<sal_uInt64>(rtl::str::null_terminated(pStr), nRadix);
}

rtl_String* rtl_string_ImplAlloc(sal_Int32 nLen) { return rtl::str::Alloc<rtl_String>(nLen); }

void SAL_CALL rtl_string_acquire(rtl_String* pThis) noexcept
{
    return rtl::str::acquire(pThis);
}

void SAL_CALL rtl_string_release(rtl_String* pThis) noexcept
{
    return rtl::str::release(pThis);
}

void SAL_CALL rtl_string_new(rtl_String** ppThis) noexcept
{
    return rtl::str::new_(ppThis);
}

rtl_String* SAL_CALL rtl_string_alloc(sal_Int32 nLen) noexcept
{
    assert(nLen >= 0);
    return rtl::str::Alloc<rtl_String>(nLen);
}

void SAL_CALL rtl_string_new_WithLength(rtl_String** ppThis, sal_Int32 nLen) noexcept
{
    rtl::str::new_WithLength(ppThis, nLen);
}

void SAL_CALL rtl_string_newFromString(rtl_String** ppThis, const rtl_String* pStr) noexcept
{
    rtl::str::newFromString(ppThis, pStr);
}

void SAL_CALL rtl_string_newFromStr(rtl_String** ppThis, const char* pCharStr) noexcept
{
    rtl::str::newFromStr(ppThis, pCharStr);
}

void SAL_CALL rtl_string_newFromStr_WithLength(rtl_String** ppThis, const char* pCharStr,
                                               sal_Int32 nLen) noexcept
{
    rtl::str::newFromStr_WithLength(ppThis, pCharStr, nLen);
}

void SAL_CALL rtl_string_newFromSubString(rtl_String** ppThis, const rtl_String* pFrom,
                                          sal_Int32 beginIndex, sal_Int32 count) noexcept
{
    rtl::str::newFromSubString(ppThis, pFrom, beginIndex, count);
}

// Used when creating from string literals.
void SAL_CALL rtl_string_newFromLiteral(rtl_String** ppThis, const char* pCharStr, sal_Int32 nLen,
                                        sal_Int32 allocExtra) noexcept
{
    rtl::str::newFromStr_WithLength(ppThis, pCharStr, nLen, allocExtra);
}

void SAL_CALL rtl_string_assign(rtl_String** ppThis, rtl_String* pStr) noexcept
{
    rtl::str::assign(ppThis, pStr);
}

sal_Int32 SAL_CALL rtl_string_getLength(const rtl_String* pThis) noexcept
{
    return rtl::str::getLength(pThis);
}

char* SAL_CALL rtl_string_getStr(rtl_String* pThis) noexcept
{
    return rtl::str::getStr(pThis);
}

void SAL_CALL rtl_string_newConcat(rtl_String** ppThis, rtl_String* pLeft, rtl_String* pRight) noexcept
{
    rtl::str::newConcat(ppThis, pLeft, pRight);
}

void SAL_CALL rtl_string_ensureCapacity(rtl_String** ppThis, sal_Int32 size) noexcept
{
    rtl::str::ensureCapacity(ppThis, size);
}

void SAL_CALL rtl_string_newReplaceStrAt(rtl_String** ppThis, rtl_String* pStr, sal_Int32 nIndex,
                                         sal_Int32 nCount, rtl_String* pNewSubStr) noexcept
{
    rtl::str::newReplaceStrAt(ppThis, pStr, nIndex, nCount, pNewSubStr);
}

void SAL_CALL rtl_string_newReplaceStrAt_WithLength(rtl_String** ppThis, rtl_String* pStr, sal_Int32 nIndex,
                                         sal_Int32 nCount, char const * subStr, sal_Int32 substrLen) noexcept
{
    rtl::str::newReplaceStrAt(ppThis, pStr, nIndex, nCount, subStr, substrLen);
}

void SAL_CALL rtl_string_newReplace(rtl_String** ppThis, rtl_String* pStr, char cOld, char cNew) noexcept
{
    rtl::str::newReplaceChars(ppThis, pStr, rtl::str::FromTo(cOld, cNew));
}

void SAL_CALL rtl_string_newToAsciiLowerCase(rtl_String** ppThis, rtl_String* pStr) noexcept
{
    rtl::str::newReplaceChars(ppThis, pStr, rtl::str::toAsciiLower);
}

void SAL_CALL rtl_string_newToAsciiUpperCase(rtl_String** ppThis, rtl_String* pStr) noexcept
{
    rtl::str::newReplaceChars(ppThis, pStr, rtl::str::toAsciiUpper);
}

void SAL_CALL rtl_string_newTrim(rtl_String** ppThis, rtl_String* pStr) noexcept
{
    rtl::str::newTrim(ppThis, pStr);
}

sal_Int32 SAL_CALL rtl_string_getToken(rtl_String** ppThis, rtl_String* pStr, sal_Int32 nToken,
                                       char cTok, sal_Int32 nIndex) noexcept
{
    return rtl::str::getToken(ppThis, pStr, nToken, cTok, nIndex);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

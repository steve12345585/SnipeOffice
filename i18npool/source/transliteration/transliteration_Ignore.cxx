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

#include <com/sun/star/i18n/TransliterationType.hpp>

#include <transliteration_Ignore.hxx>
#include <i18nutil/oneToOneMapping.hxx>

using namespace com::sun::star::i18n;
using namespace com::sun::star::uno;

namespace i18npool {

sal_Bool SAL_CALL
transliteration_Ignore::equals(const OUString& str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32& nMatch1,
        const OUString& str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32& nMatch2 )
{
    Sequence< sal_Int32 > offset1;
    Sequence< sal_Int32 > offset2;

    // The method folding is defined in a sub class.
    OUString s1 = folding( str1, pos1, nCount1, offset1);
    OUString s2 = folding( str2, pos2, nCount2, offset2);

    const sal_Unicode * p1 = s1.getStr();
    const sal_Unicode * p2 = s2.getStr();
    sal_Int32 length = std::min(s1.getLength(), s2.getLength());
    sal_Int32 nmatch;

    for ( nmatch = 0; nmatch < length; nmatch++)
        if (*p1++ != *p2++)
            break;

    if (nmatch > 0) {
        nMatch1 = offset1[ nmatch - 1 ] + 1; // Subtract 1 from nmatch because the index starts from zero.
        nMatch2 = offset2[ nmatch - 1 ] + 1; // And then, add 1 to position because it means the number of character matched.
    }
    else {
        nMatch1 = 0;  // No character was matched.
        nMatch2 = 0;
    }

    return (nmatch == s1.getLength()) && (nmatch == s2.getLength());
}


Sequence< OUString > SAL_CALL
transliteration_Ignore::transliterateRange( const OUString& str1, const OUString& str2 )
{
    if (str1.isEmpty() || str2.isEmpty())
        throw RuntimeException();

    return { str1.copy(0, 1), str2.copy(0, 1) };
}


sal_Int16 SAL_CALL
transliteration_Ignore::getType()
{
    // The type is also defined in com/sun/star/util/TransliterationType.hdl
    return TransliterationType::IGNORE;
}


OUString
transliteration_Ignore::transliterateImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount,
        Sequence< sal_Int32 >* pOffset)
{
    // The method folding is defined in a sub class.
    return foldingImpl( inStr, startPos, nCount, pOffset);
}

Sequence< OUString >
transliteration_Ignore::transliterateRange( const OUString& str1, const OUString& str2,
        XTransliteration& t1, XTransliteration& t2 )
{
    if (str1.isEmpty() || str2.isEmpty())
        throw RuntimeException();

    Sequence< sal_Int32 > offset;
    OUString s11 = t1.transliterate( str1, 0, 1, offset );
    OUString s12 = t1.transliterate( str2, 0, 1, offset );
    OUString s21 = t2.transliterate( str1, 0, 1, offset );
    OUString s22 = t2.transliterate( str2, 0, 1, offset );

    if ( (s11 == s21) && (s12 == s22) ) {
        return { s11, s12 };
    }
    return { s11, s12, s21, s22 };
}

OUString
transliteration_Ignore::foldingImpl( const OUString& inStr, sal_Int32 startPos,
    sal_Int32 nCount, Sequence< sal_Int32 >* pOffset)
{
    // Create a string buffer which can hold nCount + 1 characters.
    // The reference count is 1 now.
    rtl_uString * newStr = rtl_uString_alloc(nCount);
    sal_Unicode * dst = newStr->buffer;
    const sal_Unicode * src = inStr.getStr() + startPos;

    // Allocate nCount length to offset argument.
    sal_Int32 *p = nullptr;
    sal_Int32 position = 0;
    if (pOffset) {
        pOffset->realloc( nCount );
        p = pOffset->getArray();
        position = startPos;
    }

    if (map) {
        sal_Unicode previousChar = *src ++;
        sal_Unicode currentChar;

        // Translation
        while (-- nCount > 0) {
            currentChar = *src ++;

            const Mapping *m;
            for (m = map; m->replaceChar; m++) {
                if (previousChar == m->previousChar &&  currentChar == m->currentChar ) {
                    if (pOffset) {
                        if (! m->two2one)
                            *p++ = position;
                        position++;
                        *p++ = position++;
                    }
                    *dst++ = m->replaceChar;
                    if (!m->two2one)
                        *dst++ = currentChar;
                    previousChar = *src++;
                    nCount--;
                    break;
                }
            }

            if (! m->replaceChar) {
                if (pOffset)
                    *p ++ = position ++;
                *dst ++ = previousChar;
                previousChar = currentChar;
            }
        }

        if (nCount == 0) {
            if (pOffset)
                *p = position;
            *dst ++ = previousChar;
        }
    } else {
        // Translation
        while (nCount -- > 0) {
            sal_Unicode c = *src++;
            c = func ? func( c) : (*table)[ c ];
            if (c != 0xffff)
                *dst ++ = c;
            if (pOffset) {
                if (c != 0xffff)
                    *p ++ = position;
                position++;
            }
        }
    }
    newStr->length = sal_Int32(dst - newStr->buffer);
    if (pOffset)
      pOffset->realloc(newStr->length);
    *dst = u'\0';

    return OUString(newStr, SAL_NO_ACQUIRE); // take ownership
}

sal_Unicode SAL_CALL
transliteration_Ignore::transliterateChar2Char( sal_Unicode inChar)
{
    return func ? func( inChar) : table ? (*table)[ inChar ] : inChar;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

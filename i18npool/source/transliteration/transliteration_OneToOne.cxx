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

#include <transliteration_OneToOne.hxx>
#include <i18nutil/oneToOneMapping.hxx>

#include <numeric>

using namespace com::sun::star::i18n;
using namespace com::sun::star::uno;

namespace i18npool {

sal_Int16 SAL_CALL transliteration_OneToOne::getType()
{
        // This type is also defined in com/sun/star/util/TransliterationType.hdl
        return TransliterationType::ONE_TO_ONE;
}

OUString
transliteration_OneToOne::foldingImpl( const OUString& /*inStr*/, sal_Int32 /*startPos*/,
        sal_Int32 /*nCount*/, Sequence< sal_Int32 >* /*pOffset*/)
{
        throw RuntimeException();
}

sal_Bool SAL_CALL
transliteration_OneToOne::equals( const OUString& /*str1*/, sal_Int32 /*pos1*/, sal_Int32 /*nCount1*/,
        sal_Int32& /*nMatch1*/, const OUString& /*str2*/, sal_Int32 /*pos2*/, sal_Int32 /*nCount2*/, sal_Int32& /*nMatch2*/ )
{
    throw RuntimeException();
}

Sequence< OUString > SAL_CALL
transliteration_OneToOne::transliterateRange( const OUString& /*str1*/, const OUString& /*str2*/ )
{
    throw RuntimeException();
}

OUString
transliteration_OneToOne::transliterateImpl( const OUString& inStr, sal_Int32 startPos,
    sal_Int32 nCount, Sequence< sal_Int32 >* pOffset)
{
    // Create a string buffer which can hold nCount + 1 characters.
    // The reference count is 1 now.
    rtl_uString * newStr = rtl_uString_alloc(nCount);
    sal_Unicode * dst = newStr->buffer;
    const sal_Unicode * src = inStr.getStr() + startPos;

    // Allocate nCount length to offset argument.
    if (pOffset) {
        pOffset->realloc( nCount );
        auto [begin, end] = asNonConstRange(*pOffset);
        std::iota(begin, end, startPos);
    }

    // Translation
    while (nCount -- > 0) {
    sal_Unicode c = *src++;
    *dst ++ = func ? func( c) : (*table)[ c ];
    }
    *dst = u'\0';

    return OUString(newStr, SAL_NO_ACQUIRE); // take ownership
}

sal_Unicode SAL_CALL
transliteration_OneToOne::transliterateChar2Char( sal_Unicode inChar)
{
    return func ? func( inChar) : (*table)[ inChar ];
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

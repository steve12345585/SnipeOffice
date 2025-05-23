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

#include <transliteration_commonclass.hxx>
#include <cppuhelper/supportsservice.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::i18n;
using namespace ::com::sun::star::lang;

namespace i18npool {

transliteration_commonclass::transliteration_commonclass()
{
    transliterationName = "";
    implementationName = "";
}

OUString SAL_CALL transliteration_commonclass::getName()
{
    return OUString::createFromAscii(transliterationName);
}

void SAL_CALL transliteration_commonclass::loadModule( TransliterationModules /*modName*/, const Locale& rLocale )
{
    aLocale = rLocale;
}


void SAL_CALL
transliteration_commonclass::loadModuleNew( const Sequence < TransliterationModulesNew >& /*modName*/, const Locale& /*rLocale*/ )
{
    throw RuntimeException();
}


void SAL_CALL
transliteration_commonclass::loadModuleByImplName( const OUString& /*implName*/, const Locale& /*rLocale*/ )
{
    throw RuntimeException();
}

void SAL_CALL
transliteration_commonclass::loadModulesByImplNames(const Sequence< OUString >& /*modNamelist*/, const Locale& /*rLocale*/)
{
    throw RuntimeException();
}

Sequence< OUString > SAL_CALL
transliteration_commonclass::getAvailableModules( const Locale& /*rLocale*/, sal_Int16 /*sType*/ )
{
    throw RuntimeException();
}

sal_Int32 SAL_CALL
transliteration_commonclass::compareSubstring(
        const OUString& str1, sal_Int32 off1, sal_Int32 len1,
        const OUString& str2, sal_Int32 off2, sal_Int32 len2)
{
    Sequence <sal_Int32> offset1(2*len1);
    Sequence <sal_Int32> offset2(2*len2);

    OUString in_str1 = transliterate(str1, off1, len1, offset1);
    OUString in_str2 = transliterate(str2, off2, len2, offset2);
    sal_Int32 strlen1 = in_str1.getLength();
    sal_Int32 strlen2 = in_str2.getLength();
    const sal_Unicode* unistr1 = in_str1.getStr();
    const sal_Unicode* unistr2 = in_str2.getStr();

    while (strlen1 && strlen2)
    {
        sal_Int32 ret = *unistr1 - *unistr2;
        if (ret)
            return ret;

        unistr1++;
        unistr2++;
        strlen1--;
        strlen2--;
    }
    return strlen1 - strlen2;
}

sal_Int32 SAL_CALL
transliteration_commonclass::compareString( const OUString& str1, const OUString& str2 )
{
    return compareSubstring(str1, 0, str1.getLength(), str2, 0, str2.getLength());
}

OUString SAL_CALL
transliteration_commonclass::transliterateString2String( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount )
{
    return transliterateImpl(inStr, startPos, nCount, nullptr);
}

OUString SAL_CALL
transliteration_commonclass::transliterateChar2String( sal_Unicode inChar )
{
    return transliteration_commonclass::transliterateString2String(OUString(&inChar, 1), 0, 1);
}

OUString SAL_CALL transliteration_commonclass::getImplementationName()
{
    return OUString::createFromAscii(implementationName);
}

sal_Bool SAL_CALL transliteration_commonclass::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL transliteration_commonclass::getSupportedServiceNames()
{
    return { u"com.sun.star.i18n.Transliteration.l10n"_ustr };
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

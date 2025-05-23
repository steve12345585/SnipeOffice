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

#include <o3tl/temporary.hxx>
#include <rtl/ustrbuf.hxx>
#include <collatorImpl.hxx>
#include <indexentrysupplier_asian.hxx>
#include "data/indexdata_alphanumeric.h"

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;

namespace i18npool {

IndexEntrySupplier_asian::IndexEntrySupplier_asian(
    const Reference < XComponentContext >& rxContext ) : IndexEntrySupplier_Common(rxContext)
{
    implementationName = "com.sun.star.i18n.IndexEntrySupplier_asian";
}

IndexEntrySupplier_asian::~IndexEntrySupplier_asian()
{
}

OUString SAL_CALL
IndexEntrySupplier_asian::getIndexCharacter( const OUString& rIndexEntry,
    const Locale& rLocale, const OUString& rAlgorithm )
{
    sal_uInt32 ch = rIndexEntry.iterateCodePoints(&o3tl::temporary(sal_Int32(0)), 0);

    const sal_uInt16** (*func)(sal_Int16&)=nullptr;
    if ( rLocale.Language == "zh" && u"TW HK MO"_ustr.indexOf(rLocale.Country) >= 0 ) {
        if ( rAlgorithm == "radical" )
            func = get_indexdata_zh_TW_radical;
        else if ( rAlgorithm == "stroke" )
            func = get_indexdata_zh_TW_stroke;
    }
    if (!func) {
        if ( rLocale.Language == "ko" ) {
            if ( rAlgorithm == "dict" )
                func = get_indexdata_ko_dict;
        } else if ( rLocale.Language == "zh" ) {
            if ( rAlgorithm == "pinyin" )
                func = get_indexdata_zh_pinyin;
            else if ( rAlgorithm == "radical" )
                func = get_indexdata_zh_radical;
            else if ( rAlgorithm == "stroke" )
                func = get_indexdata_zh_stroke;
            else if ( rAlgorithm == "zhuyin" )
                func = get_indexdata_zh_zhuyin;
        }
    }
    if (func) {
        sal_Int16 max_index;
        const sal_uInt16** idx=func(max_index);
        if (static_cast<sal_Int16>(ch >> 8) <= max_index) {
            sal_uInt16 address=idx[0][ch >> 8];
            if (address != 0xFFFF) {
                address=idx[1][address+(ch & 0xFF)];
                return idx[2]
                    ? OUString(
                        reinterpret_cast<const sal_Unicode *>(&idx[2][address]))
                    : OUString(sal_Unicode(address));
            }
        }
    }

    // using alphanumeric index for non-define string
    return OUString(&idxStr[(ch & 0xFFFFFF00) ? 0 : ch], 1);
}

OUString SAL_CALL
IndexEntrySupplier_asian::getIndexKey( const OUString& rIndexEntry,
    const OUString& rPhoneticEntry, const Locale& rLocale)
{
    return getIndexCharacter(getEntry(rIndexEntry, rPhoneticEntry, rLocale), rLocale, aAlgorithm);
}

sal_Int16 SAL_CALL
IndexEntrySupplier_asian::compareIndexEntry(
    const OUString& rIndexEntry1, const OUString& rPhoneticEntry1, const Locale& rLocale1,
    const OUString& rIndexEntry2, const OUString& rPhoneticEntry2, const Locale& rLocale2 )
{
    sal_Int32 result = collator->compareString(getEntry(rIndexEntry1, rPhoneticEntry1, rLocale1),
                                    getEntry(rIndexEntry2, rPhoneticEntry2, rLocale2));

    // equivalent of phonetic entries does not mean equivalent of index entries.
    // we have to continue comparing index entry here.
    if (result == 0 && usePhonetic && !rPhoneticEntry1.isEmpty() &&
            rLocale1.Language == rLocale2.Language && rLocale1.Country == rLocale2.Country &&
            rLocale1.Variant == rLocale2.Variant)
        result = collator->compareString(rIndexEntry1, rIndexEntry2);
    return sal::static_int_cast< sal_Int16 >(result); // result in { -1, 0, 1 }
}

OUString SAL_CALL
IndexEntrySupplier_asian::getPhoneticCandidate( const OUString& rIndexEntry,
        const Locale& rLocale )
{
    sal_uInt16 const **(*func)(sal_Int16&)=nullptr;
    if ( rLocale.Language == "zh" )
        func = (u"TW HK MO"_ustr.indexOf(rLocale.Country) >= 0) ?  get_zh_zhuyin : get_zh_pinyin;
    else if ( rLocale.Language == "ko" )
        func = get_ko_phonetic;

    if (func) {
        OUStringBuffer candidate;
        sal_Int16 max_index;
        sal_uInt16 const ** idx=func(max_index);
        for (sal_Int32 i=0,j=0; i < rIndexEntry.getLength(); i=j) {
            sal_uInt32 ch = rIndexEntry.iterateCodePoints(&j);
            if (static_cast<sal_Int16>(ch>>8) <= max_index) {
                sal_uInt16 address = idx[0][ch>>8];
                if (address != 0xFFFF) {
                    address = idx[1][address + (ch & 0xFF)];
                    if ( i > 0 && rLocale.Language == "zh" )
                        candidate.append(" ");
                    if (idx[2])
                        candidate.append(
                            reinterpret_cast<const sal_Unicode *>(&idx[2][address]));
                    else
                        candidate.append(sal_Unicode(address));
                } else
                    candidate.append(" ");
            }
        }
        return candidate.makeStringAndClear();
    }
    return OUString();
}

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

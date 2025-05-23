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

#pragma once

#include "indexentrysupplier_common.hxx"

namespace i18npool {




class IndexEntrySupplier_ja_phonetic : public IndexEntrySupplier_Common {
public:
    IndexEntrySupplier_ja_phonetic( const css::uno::Reference < css::uno::XComponentContext >& rxContext ) : IndexEntrySupplier_Common(rxContext) {
        implementationName = "com.sun.star.i18n.IndexEntrySupplier_ja_phonetic";
    };
    virtual OUString SAL_CALL getIndexCharacter( const OUString& rIndexEntry,
        const css::lang::Locale& rLocale, const OUString& rSortAlgorithm ) override;
    virtual OUString SAL_CALL getIndexKey( const OUString& IndexEntry,
        const OUString& PhoneticEntry, const css::lang::Locale& rLocale ) override;
    virtual sal_Int16 SAL_CALL compareIndexEntry( const OUString& IndexEntry1,
        const OUString& PhoneticEntry1, const css::lang::Locale& rLocale1,
        const OUString& IndexEntry2, const OUString& PhoneticEntry2,
        const css::lang::Locale& rLocale2 ) override;
};

#define INDEXENTRYSUPPLIER_JA_PHONETIC( algorithm, algo_descr ) \
class IndexEntrySupplier_##algorithm final : public IndexEntrySupplier_ja_phonetic {\
public:\
    IndexEntrySupplier_##algorithm (const css::uno::Reference < css::uno::XComponentContext >& rxContext) : IndexEntrySupplier_ja_phonetic (rxContext) {\
        implementationName = "com.sun.star.i18n.IndexEntrySupplier_ja_phonetic" algo_descr;\
    };\
    virtual sal_Bool SAL_CALL loadAlgorithm(\
        const css::lang::Locale& rLocale,\
        const OUString& SortAlgorithm, sal_Int32 collatorOptions ) override;\
};

/** descriptions formed by concatenating strings here must match names in .component file */
INDEXENTRYSUPPLIER_JA_PHONETIC( ja_phonetic_alphanumeric_first_by_syllable,  " (alphanumeric first) (grouped by syllable)" )
INDEXENTRYSUPPLIER_JA_PHONETIC( ja_phonetic_alphanumeric_first_by_consonant, " (alphanumeric first) (grouped by consonant)" )
INDEXENTRYSUPPLIER_JA_PHONETIC( ja_phonetic_alphanumeric_last_by_syllable,   " (alphanumeric last) (grouped by syllable)" )
INDEXENTRYSUPPLIER_JA_PHONETIC( ja_phonetic_alphanumeric_last_by_consonant,  " (alphanumeric last) (grouped by consonant)" )

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

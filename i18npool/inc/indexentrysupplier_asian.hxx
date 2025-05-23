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




class IndexEntrySupplier_asian final : public IndexEntrySupplier_Common {
public:
    IndexEntrySupplier_asian( const css::uno::Reference < css::uno::XComponentContext >& rxContext );
    virtual ~IndexEntrySupplier_asian() override;

    OUString SAL_CALL getIndexCharacter( const OUString& rIndexEntry,
            const css::lang::Locale& rLocale, const OUString& rAlgorithm ) override;
    OUString SAL_CALL getIndexKey( const OUString& rIndexEntry,
            const OUString& rPhoneticEntry, const css::lang::Locale& rLocale) override;
    sal_Int16 SAL_CALL compareIndexEntry(
            const OUString& rIndexEntry1, const OUString& rPhoneticEntry1,
            const css::lang::Locale& rLocale1,
            const OUString& rIndexEntry2, const OUString& rPhoneticEntry2,
            const css::lang::Locale& rLocale2 ) override;
    OUString SAL_CALL getPhoneticCandidate( const OUString& rIndexEntry,
            const css::lang::Locale& rLocale ) override;
};

}

extern "C" {

const sal_uInt16** get_indexdata_ko_dict(sal_Int16&);
const sal_uInt16** get_indexdata_zh_TW_radical(sal_Int16&);
const sal_uInt16** get_indexdata_zh_TW_stroke(sal_Int16&);
const sal_uInt16** get_indexdata_zh_pinyin(sal_Int16&);
const sal_uInt16** get_indexdata_zh_radical(sal_Int16&);
const sal_uInt16** get_indexdata_zh_stroke(sal_Int16&);
const sal_uInt16** get_indexdata_zh_zhuyin(sal_Int16&);

const sal_uInt16** get_ko_phonetic(sal_Int16&);
const sal_uInt16** get_zh_pinyin(sal_Int16&);
const sal_uInt16** get_zh_zhuyin(sal_Int16&);

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

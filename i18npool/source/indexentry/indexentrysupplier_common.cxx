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

#include <collatorImpl.hxx>
#include <indexentrysupplier_common.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <localedata.hxx>
#include <o3tl/temporary.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;

namespace i18npool {

IndexEntrySupplier_Common::IndexEntrySupplier_Common(const Reference < uno::XComponentContext >& rxContext)
{
    implementationName = "com.sun.star.i18n.IndexEntrySupplier_Common";
    collator = new CollatorImpl(rxContext);
    usePhonetic = false;
}

IndexEntrySupplier_Common::~IndexEntrySupplier_Common()
{
}

Sequence < lang::Locale > SAL_CALL IndexEntrySupplier_Common::getLocaleList()
{
    throw RuntimeException();
}

Sequence < OUString > SAL_CALL IndexEntrySupplier_Common::getAlgorithmList( const lang::Locale& )
{
    throw RuntimeException();
}

OUString SAL_CALL IndexEntrySupplier_Common::getPhoneticCandidate( const OUString&,
    const lang::Locale& )
{
    return OUString();
}

sal_Bool SAL_CALL IndexEntrySupplier_Common::usePhoneticEntry( const lang::Locale& )
{
    throw RuntimeException();
}

sal_Bool SAL_CALL IndexEntrySupplier_Common::loadAlgorithm( const lang::Locale& rLocale,
    const OUString& rAlgorithm, sal_Int32 collatorOptions )
{
    usePhonetic = LocaleDataImpl::get()->isPhonetic(rLocale, rAlgorithm);
    collator->loadCollatorAlgorithm(rAlgorithm, rLocale, collatorOptions);
    aLocale = rLocale;
    aAlgorithm = rAlgorithm;
    return true;
}

OUString SAL_CALL IndexEntrySupplier_Common::getIndexKey( const OUString& rIndexEntry,
    const OUString&, const lang::Locale& )
{
    sal_uInt32 indexChar=rIndexEntry.iterateCodePoints(&o3tl::temporary(sal_Int32(0)), 0);
    return OUString(&indexChar, 1);
}

sal_Int16 SAL_CALL IndexEntrySupplier_Common::compareIndexEntry(
    const OUString& rIndexEntry1, const OUString&, const lang::Locale&,
    const OUString& rIndexEntry2, const OUString&, const lang::Locale& )
{
    return sal::static_int_cast< sal_Int16 >(
        collator->compareString(rIndexEntry1, rIndexEntry2));
        // return value of compareString in { -1, 0, 1 }
}

OUString SAL_CALL IndexEntrySupplier_Common::getIndexCharacter( const OUString& rIndexEntry,
    const lang::Locale& rLocale, const OUString& )
{
    return getIndexKey(rIndexEntry, rIndexEntry, rLocale);
}

OUString SAL_CALL IndexEntrySupplier_Common::getIndexFollowPageWord( sal_Bool,
    const lang::Locale& )
{
    throw RuntimeException();
}

const OUString&
IndexEntrySupplier_Common::getEntry( const OUString& IndexEntry,
    const OUString& PhoneticEntry, const lang::Locale& rLocale ) const
{
    // The condition for using phonetic entry is:
    // usePhonetic is set for the algorithm;
    // rLocale for phonetic entry is same as aLocale for algorithm,
    // which means Chinese phonetic will not be used for Japanese algorithm;
    // phonetic entry is not blank.
    if (usePhonetic && !PhoneticEntry.isEmpty() && rLocale.Language == aLocale.Language &&
            rLocale.Country == aLocale.Country && rLocale.Variant == aLocale.Variant)
        return PhoneticEntry;
    else
        return IndexEntry;
}

OUString SAL_CALL
IndexEntrySupplier_Common::getImplementationName()
{
    return OUString::createFromAscii( implementationName );
}

sal_Bool SAL_CALL
IndexEntrySupplier_Common::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL
IndexEntrySupplier_Common::getSupportedServiceNames()
{
    Sequence< OUString > aRet { OUString::createFromAscii( implementationName ) };
    return aRet;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

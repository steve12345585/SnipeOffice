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

#include <comphelper/processfactory.hxx>
#include <com/sun/star/i18n/IndexEntrySupplier.hpp>
#include <toxwrap.hxx>
#include <comphelper/diagnose_ex.hxx>

using namespace ::com::sun::star;

IndexEntrySupplierWrapper::IndexEntrySupplierWrapper()
{
    const uno::Reference< uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();

    try {
        m_xIES = i18n::IndexEntrySupplier::create(xContext);
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "IndexEntrySupplierWrapper" );
    }
}

IndexEntrySupplierWrapper::~IndexEntrySupplierWrapper()
{
}

OUString IndexEntrySupplierWrapper::GetIndexKey( const OUString& rText,
                                                 const OUString& rTextReading,
                                                 const css::lang::Locale& rLocale ) const
{
    OUString sRet;
    try {
        sRet = m_xIES->getIndexKey( rText, rTextReading, rLocale );
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "getIndexKey" );
    }
    return sRet;
}

OUString IndexEntrySupplierWrapper::GetFollowingText( bool bMorePages ) const
{
    OUString sRet;
    try {
        sRet = m_xIES->getIndexFollowPageWord( bMorePages, m_aLcl );
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "getIndexFollowPageWord" );
    }
    return sRet;
}

css::uno::Sequence< OUString > IndexEntrySupplierWrapper::GetAlgorithmList( const css::lang::Locale& rLcl ) const
{
    uno::Sequence< OUString > sRet;

    try {
        sRet = m_xIES->getAlgorithmList( rLcl );
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "getAlgorithmList" );
    }
    return sRet;
}

bool IndexEntrySupplierWrapper::LoadAlgorithm(
        const css::lang::Locale& rLcl,
        const OUString& sSortAlgorithm, tools::Long nOptions ) const
{
    bool bRet = false;
    try {
        bRet = m_xIES->loadAlgorithm( rLcl, sSortAlgorithm, nOptions );
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "loadAlgorithm" );
    }
    return bRet;
}

sal_Int16 IndexEntrySupplierWrapper::CompareIndexEntry(
            const OUString& rText1, const OUString& rTextReading1,
            const css::lang::Locale& rLocale1,
            const OUString& rText2, const OUString& rTextReading2,
            const css::lang::Locale& rLocale2 ) const
{
    sal_Int16 nRet = 0;
    try {
        nRet = m_xIES->compareIndexEntry( rText1, rTextReading1, rLocale1,
                                        rText2, rTextReading2, rLocale2 );
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw.core", "compareIndexEntry" );
    }
    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <utility>
#include <vector>

#include "myucp_datasupplier.hxx"
#include <ContentHelper.hxx>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::container;

using namespace dbaccess;


DataSupplier::DataSupplier( const rtl::Reference< ODocumentContainer >& rContent )
: m_xContent( rContent )
{
}

DataSupplier::~DataSupplier()
{
}

OUString DataSupplier::queryContentIdentifierString( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(nIndex) < m_aResults.size() )
    {
        OUString aId = m_aResults[ nIndex ]->aId;
        if ( !aId.isEmpty() )
        {
            // Already cached.
            return aId;
        }
    }

    if ( getResult( rResultSetGuard, nIndex ) )
    {
        OUString aId = m_xContent->getIdentifier()->getContentIdentifier();

        if ( !aId.isEmpty() )
            aId += "/";

        aId += m_aResults[ nIndex ]->rData.aTitle;

        m_aResults[ nIndex ]->aId = aId;
        return aId;
    }
    return OUString();
}

Reference< XContentIdentifier >
DataSupplier::queryContentIdentifier( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(nIndex) < m_aResults.size() )
    {
        Reference< XContentIdentifier > xId = m_aResults[ nIndex ]->xId;
        if ( xId.is() )
        {
            // Already cached.
            return xId;
        }
    }

    OUString aId = queryContentIdentifierString( rResultSetGuard, nIndex );
    if ( !aId.isEmpty() )
    {
        Reference< XContentIdentifier > xId = new ::ucbhelper::ContentIdentifier( aId );
        m_aResults[ nIndex ]->xId = xId;
        return xId;
    }
    return Reference< XContentIdentifier >();
}

Reference< XContent >
DataSupplier::queryContent( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 _nIndex )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(_nIndex) < m_aResults.size() )
    {
        rtl::Reference< OContentHelper > xContent = m_aResults[ _nIndex ]->xContent;
        if ( xContent.is() )
        {
            // Already cached.
            return xContent;
        }
    }

    Reference< XContentIdentifier > xId = queryContentIdentifier( rResultSetGuard, _nIndex );
    if ( xId.is() )
    {
        try
        {
            Reference< XContent > xContent;
            OUString sName = xId->getContentIdentifier();
            sName = sName.copy(sName.lastIndexOf('/')+1);

            m_aResults[ _nIndex ]->xContent = m_xContent->getContent(sName);

            xContent = m_aResults[ _nIndex ]->xContent.get();
            return xContent;

        }
        catch ( IllegalIdentifierException& )
        {
        }
    }
    return Reference< XContent >();
}

bool DataSupplier::getResult( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(nIndex) < m_aResults.size() )
    {
        // Result already present.
        return true;
    }

    // Result not (yet) present.

    if ( m_bCountFinal )
        return false;

    // Try to obtain result...

    sal_uInt32 nOldCount = m_aResults.size();
    bool bFound = false;

    // @@@ Obtain data and put it into result list...
    Sequence< OUString> aSeq = m_xContent->getElementNames();
    if ( nIndex < sal::static_int_cast< sal_uInt32 >( aSeq.getLength() ) )
    {
        m_aResults.reserve(nIndex + 1);
        const OUString* pEnd = aSeq.begin() + nIndex + 1;
        for (const OUString* pIter = aSeq.begin() + nOldCount; pIter != pEnd; ++pIter)
            m_aResults.emplace_back(
                            new ResultListEntry( m_xContent->getContent(*pIter)->getContentProperties() ) );

        // Result obtained.
        bFound = true;
    }

    if ( !bFound )
        m_bCountFinal = true;

    rtl::Reference< ::ucbhelper::ResultSet > xResultSet = getResultSet();
    if ( xResultSet.is() )
    {
        // Callbacks follow!
        aGuard.clear();

        if ( static_cast<size_t>(nOldCount) < m_aResults.size() )
            xResultSet->rowCountChanged( rResultSetGuard, nOldCount, m_aResults.size() );

        if ( m_bCountFinal )
            xResultSet->rowCountFinal(rResultSetGuard);
    }

    return bFound;
}

sal_uInt32 DataSupplier::totalCount(std::unique_lock<std::mutex>& rResultSetGuard)
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    if ( m_bCountFinal )
        return m_aResults.size();

    sal_uInt32 nOldCount = m_aResults.size();

    // @@@ Obtain data and put it into result list...
    Sequence< OUString> aSeq = m_xContent->getElementNames();
    // FIXME: this adds everything from aSeq to m_aResults, unlike similar code in getResult,
    // which skips nOldCount entries in aSeq - which is correct?
    for (auto& name : aSeq)
        m_aResults.emplace_back(
                        new ResultListEntry( m_xContent->getContent(name)->getContentProperties() ) );

    m_bCountFinal = true;

    rtl::Reference< ::ucbhelper::ResultSet > xResultSet = getResultSet();
    if ( xResultSet.is() )
    {
        // Callbacks follow!
        aGuard.clear();

        if ( static_cast<size_t>(nOldCount) < m_aResults.size() )
            xResultSet->rowCountChanged( rResultSetGuard, nOldCount, m_aResults.size() );

        xResultSet->rowCountFinal(rResultSetGuard);
    }

    return m_aResults.size();
}

sal_uInt32 DataSupplier::currentCount()
{
    return m_aResults.size();
}

bool DataSupplier::isCountFinal()
{
    return m_bCountFinal;
}

Reference< XRow >
DataSupplier::queryPropertyValues( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex  )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(nIndex) < m_aResults.size() )
    {
        Reference< XRow > xRow = m_aResults[ nIndex ]->xRow;
        if ( xRow.is() )
        {
            // Already cached.
            return xRow;
        }
    }

    if ( getResult( rResultSetGuard, nIndex ) )
    {
        if ( !m_aResults[ nIndex ]->xContent.is() )
            queryContent(rResultSetGuard, nIndex);

        Reference< XRow > xRow = m_aResults[ nIndex ]->xContent->getPropertyValues(getResultSet()->getProperties());
        m_aResults[ nIndex ]->xRow = xRow;
        return xRow;
    }

    return Reference< XRow >();
}

void DataSupplier::releasePropertyValues( sal_uInt32 nIndex )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( static_cast<size_t>(nIndex) < m_aResults.size() )
        m_aResults[ nIndex ]->xRow.clear();
}

void DataSupplier::close()
{
}

void DataSupplier::validate()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

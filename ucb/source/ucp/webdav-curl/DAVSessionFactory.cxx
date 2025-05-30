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

#include <memory>
#include "DAVSessionFactory.hxx"
#include "CurlSession.hxx"
#include "CurlUri.hxx"

using namespace http_dav_ucp;
using namespace com::sun::star;

DAVSessionFactory::~DAVSessionFactory()
{
}

rtl::Reference< DAVSession > DAVSessionFactory::createDAVSession(
                const OUString & inUri,
                const uno::Sequence< beans::NamedValue >& rFlags,
                const uno::Reference< uno::XComponentContext > & rxContext )
{
    std::unique_lock aGuard( m_aMutex );

    if (!m_xProxyDecider)
        m_xProxyDecider.reset( new ucbhelper::InternetProxyDecider( rxContext ) );

    Map::iterator aIt = std::find_if(m_aMap.begin(), m_aMap.end(),
        [&inUri, &rFlags](const Map::value_type& rEntry) { return rEntry.second->CanUse( inUri, rFlags ); });

    if ( aIt == m_aMap.end() )
    {
        rtl::Reference< CurlSession > xElement(
            new CurlSession(rxContext, this, inUri, rFlags, *m_xProxyDecider) );

        aIt = m_aMap.emplace(  inUri, xElement.get() ).first;

        aIt->second->m_aContainerIt = aIt;
        return aIt->second;
    }
    else if ( osl_atomic_increment( &aIt->second->m_nRefCount ) > 1 )
    {
        rtl::Reference< DAVSession > xElement( aIt->second );
        osl_atomic_decrement( &aIt->second->m_nRefCount );
        return xElement;
    }
    else
    {
        osl_atomic_decrement( &aIt->second->m_nRefCount );
        aIt->second->m_aContainerIt = m_aMap.end();

        rtl::Reference< CurlSession > xNewStorage = new CurlSession(rxContext, this, inUri, rFlags, *m_xProxyDecider);
        aIt->second = xNewStorage.get();
        aIt->second->m_aContainerIt = aIt;
        return xNewStorage;
    }
}

void DAVSessionFactory::releaseElement( const DAVSession * pElement )
{
    assert( pElement );
    std::unique_lock aGuard( m_aMutex );
    if ( pElement->m_aContainerIt != m_aMap.end() )
        m_aMap.erase( pElement->m_aContainerIt );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

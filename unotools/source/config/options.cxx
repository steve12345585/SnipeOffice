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
#include <unotools/options.hxx>

#include <algorithm>

using utl::detail::Options;
using utl::ConfigurationBroadcaster;

utl::ConfigurationListener::~ConfigurationListener() {}

ConfigurationBroadcaster::ConfigurationBroadcaster()
: m_nBroadcastBlocked( 0 )
, m_nBlockedHint( ConfigurationHints::NONE )
{
}

ConfigurationBroadcaster::ConfigurationBroadcaster(ConfigurationBroadcaster const & rSource)
: mpList( rSource.mpList ? new IMPL_ConfigurationListenerList(*rSource.mpList) : nullptr )
, m_nBroadcastBlocked( rSource.m_nBroadcastBlocked )
, m_nBlockedHint( rSource.m_nBlockedHint )
{
}

ConfigurationBroadcaster::~ConfigurationBroadcaster()
{
}

ConfigurationBroadcaster & ConfigurationBroadcaster::operator =(
    ConfigurationBroadcaster const & other)
{
    if (&other != this) {
        mpList.reset(
            other.mpList == nullptr ? nullptr : new IMPL_ConfigurationListenerList(*other.mpList));
        m_nBroadcastBlocked = other.m_nBroadcastBlocked;
        m_nBlockedHint = other.m_nBlockedHint;
    }
    return *this;
}

void ConfigurationBroadcaster::AddListener( utl::ConfigurationListener* pListener )
{
    if ( !mpList )
        mpList.reset(new IMPL_ConfigurationListenerList);
    mpList->push_back( pListener );
}

void ConfigurationBroadcaster::RemoveListener( utl::ConfigurationListener const * pListener )
{
    if ( mpList ) {
        auto it = std::find(mpList->begin(), mpList->end(), pListener);
        if ( it != mpList->end() )
            mpList->erase( it );
    }
}

void ConfigurationBroadcaster::NotifyListeners( ConfigurationHints nHint )
{
    if ( m_nBroadcastBlocked )
        m_nBlockedHint |= nHint;
    else
    {
        nHint |= m_nBlockedHint;
        m_nBlockedHint = ConfigurationHints::NONE;
        if ( mpList ) {
            for ( size_t n = 0; n < mpList->size(); n++ )
                (*mpList)[ n ]->ConfigurationChanged( this, nHint );
        }
    }
}

void ConfigurationBroadcaster::BlockBroadcasts( bool bBlock )
{
    if ( bBlock )
        ++m_nBroadcastBlocked;
    else if ( m_nBroadcastBlocked )
    {
        if ( --m_nBroadcastBlocked == 0 )
            NotifyListeners( ConfigurationHints::NONE );
    }
}

Options::Options()
{
}

Options::~Options()
{
}

void Options::ConfigurationChanged( ConfigurationBroadcaster*, ConfigurationHints nHint )
{
    NotifyListeners( nHint );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

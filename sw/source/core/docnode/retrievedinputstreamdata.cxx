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

#include <retrievedinputstreamdata.hxx>
#include <retrieveinputstreamconsumer.hxx>
#include <vcl/svapp.hxx>

// #i73788#

SwRetrievedInputStreamDataManager::tDataKey SwRetrievedInputStreamDataManager::snNextKeyValue = 1;

SwRetrievedInputStreamDataManager& SwRetrievedInputStreamDataManager::GetManager()
{
    static SwRetrievedInputStreamDataManager theSwRetrievedInputStreamDataManager;
    return theSwRetrievedInputStreamDataManager;
}

SwRetrievedInputStreamDataManager::tDataKey SwRetrievedInputStreamDataManager::ReserveData(
                        std::weak_ptr< SwAsyncRetrieveInputStreamThreadConsumer > const & pThreadConsumer )
{
    std::unique_lock aGuard(maMutex);

    // create empty data container for given thread Consumer
    tDataKey nDataKey( snNextKeyValue );
    maInputStreamData[nDataKey] = tData(pThreadConsumer);

    // prepare next data key value
    if ( snNextKeyValue < SAL_MAX_UINT64 )
    {
        ++snNextKeyValue;
    }
    else
    {
        snNextKeyValue = 1;
    }

    return nDataKey;
}

void SwRetrievedInputStreamDataManager::PushData(
        const tDataKey nDataKey,
        css::uno::Reference<css::io::XInputStream> const & xInputStream,
        const bool bIsStreamReadOnly )
{
    std::unique_lock aGuard(maMutex);

    std::map< tDataKey, tData >::iterator aIter = maInputStreamData.find( nDataKey );

    if ( aIter == maInputStreamData.end() )
        return;

    // Fill data container.
    (*aIter).second.mxInputStream = xInputStream;
    (*aIter).second.mbIsStreamReadOnly = bIsStreamReadOnly;

    // post user event to process the retrieved input stream data
    if ( GetpApp() )
    {

        tDataKey* pDataKey = new tDataKey;
        *pDataKey = nDataKey;
        Application::PostUserEvent( LINK( this, SwRetrievedInputStreamDataManager, LinkedInputStreamReady ), pDataKey );
    }
    else
    {
        // no application available -> discard data
        maInputStreamData.erase( aIter );
    }
}

bool SwRetrievedInputStreamDataManager::PopData( const tDataKey nDataKey,
                                                 tData& rData )
{
    std::unique_lock aGuard(maMutex);

    bool bDataProvided( false );

    std::map< tDataKey, tData >::iterator aIter = maInputStreamData.find( nDataKey );

    if ( aIter != maInputStreamData.end() )
    {
        rData.mpThreadConsumer = (*aIter).second.mpThreadConsumer;
        rData.mxInputStream = (*aIter).second.mxInputStream;
        rData.mbIsStreamReadOnly = (*aIter).second.mbIsStreamReadOnly;

        maInputStreamData.erase( aIter );

        bDataProvided = true;
    }

    return bDataProvided;
}

/** callback function, which is triggered by input stream data manager on
    filling of the data container to provide retrieved input stream to the
    thread Consumer using <Application::PostUserEvent(..)>

    #i73788#
    Note: This method has to be run in the main thread.
*/
IMPL_STATIC_LINK( SwRetrievedInputStreamDataManager, LinkedInputStreamReady,
                  void*, p, void )
{
    SwRetrievedInputStreamDataManager::tDataKey* pDataKey = static_cast<SwRetrievedInputStreamDataManager::tDataKey*>(p);
    if ( !pDataKey )
    {
        return;
    }

    SwRetrievedInputStreamDataManager& rDataManager =
                            SwRetrievedInputStreamDataManager::GetManager();
    SwRetrievedInputStreamDataManager::tData aInputStreamData;
    if ( rDataManager.PopData( *pDataKey, aInputStreamData ) )
    {
        std::shared_ptr< SwAsyncRetrieveInputStreamThreadConsumer > pThreadConsumer =
                                    aInputStreamData.mpThreadConsumer.lock();
        if ( pThreadConsumer )
        {
            pThreadConsumer->ApplyInputStream( aInputStreamData.mxInputStream,
                                               aInputStreamData.mbIsStreamReadOnly );
        }
    }
    delete pDataKey;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

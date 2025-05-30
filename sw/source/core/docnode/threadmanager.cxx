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

#include "cancellablejob.hxx"
#include "threadmanager.hxx"
#include <threadlistener.hxx>

#include <osl/diagnose.h>

#include <algorithm>

#include <com/sun/star/util/XJobManager.hpp>

using namespace ::com::sun::star;

/** class to manage threads

    #i73788#
*/
const std::deque< ThreadManager::tThreadData >::size_type ThreadManager::snStartedSize = 10;

ThreadManager::ThreadManager( uno::Reference< util::XJobManager > const & rThreadJoiner )
    : mrThreadJoiner( rThreadJoiner ),
      mnThreadIDCounter( 0 ),
      maStartNewThreadIdle("SW ThreadManager StartNewThreadIdle"),
      mbStartingOfThreadsSuspended( false )
{
}

void ThreadManager::Init()
{
    mpThreadListener = std::make_shared<ThreadListener>( *this );

    maStartNewThreadIdle.SetPriority( TaskPriority::LOWEST );
    maStartNewThreadIdle.SetInvokeHandler( LINK( this, ThreadManager, TryToStartNewThread ) );
}

ThreadManager::~ThreadManager()
{
    maWaitingForStartThreads.clear();
    maStartedThreads.clear();
}

std::weak_ptr< IFinishedThreadListener > ThreadManager::GetThreadListenerWeakRef() const
{
    return mpThreadListener;
}

void ThreadManager::NotifyAboutFinishedThread( const oslInterlockedCount nThreadID )
{
    RemoveThread( nThreadID, true );
}

oslInterlockedCount ThreadManager::AddThread(
                            const rtl::Reference< ObservableThread >& rThread )

{
    std::unique_lock aGuard(maMutex);

    // create new thread
    tThreadData aThreadData;
    oslInterlockedCount nNewThreadID( osl_atomic_increment( &mnThreadIDCounter ) );
    {
        aThreadData.nThreadID = nNewThreadID;

        aThreadData.pThread = rThread;
        aThreadData.aJob = new CancellableJob( aThreadData.pThread );

        aThreadData.pThread->setPriority( osl_Thread_PriorityBelowNormal );
        mpThreadListener->ListenToThread( aThreadData.nThreadID,
                                          *(aThreadData.pThread) );
    }

    // add thread to manager
    if ( maStartedThreads.size() < snStartedSize &&
         !mbStartingOfThreadsSuspended )
    {
        // Try to start thread
        if ( !StartThread( aThreadData ) )
        {
            // No success on starting thread
            // If no more started threads exist, but still threads are waiting,
            // setup Timer to start thread from waiting ones
            if ( maStartedThreads.empty() && !maWaitingForStartThreads.empty() )
            {
                maStartNewThreadIdle.Start();
            }
        }
    }
    else
    {
        // Thread will be started later
        maWaitingForStartThreads.push_back( aThreadData );
    }

    return nNewThreadID;
}

void ThreadManager::RemoveThread( const oslInterlockedCount nThreadID,
                                  const bool bThreadFinished )
{
    // --> SAFE ----
    std::unique_lock aGuard(maMutex);

    std::deque< tThreadData >::iterator aIter =
                std::find_if( maStartedThreads.begin(), maStartedThreads.end(),
                              ThreadPred( nThreadID ) );

    if ( aIter != maStartedThreads.end() )
    {
        tThreadData aTmpThreadData( *aIter );

        maStartedThreads.erase( aIter );

        if ( bThreadFinished )
        {
            // release thread as job from thread joiner instance
            css::uno::Reference< css::util::XJobManager > rThreadJoiner( mrThreadJoiner );
            if ( rThreadJoiner.is() )
            {
                rThreadJoiner->releaseJob( aTmpThreadData.aJob );
            }
            else
            {
                OSL_FAIL( "<ThreadManager::RemoveThread(..)> - ThreadJoiner already gone!" );
            }
        }

        // Try to start thread from waiting ones
        aGuard.unlock();
        TryToStartNewThread( nullptr );
    }
    else
    {
        aIter = std::find_if( maWaitingForStartThreads.begin(),
                              maWaitingForStartThreads.end(), ThreadPred( nThreadID ) );

        if ( aIter != maWaitingForStartThreads.end() )
        {
            maWaitingForStartThreads.erase( aIter );
        }
    }
    // <-- SAFE ----
}

bool ThreadManager::StartWaitingThread()
{
    if ( !maWaitingForStartThreads.empty() )
    {
        tThreadData aThreadData( maWaitingForStartThreads.front() );
        maWaitingForStartThreads.pop_front();
        return StartThread( aThreadData );
    }
    else
    {
        return false;
    }
}

bool ThreadManager::StartThread( const tThreadData& rThreadData )
{
    bool bThreadStarted( false );

    if ( rThreadData.pThread->create() )
    {
        // start of thread successful.
        bThreadStarted = true;

        maStartedThreads.push_back( rThreadData );

        // register thread as job at thread joiner instance
        css::uno::Reference< css::util::XJobManager > rThreadJoiner( mrThreadJoiner );
        if ( rThreadJoiner.is() )
        {
            rThreadJoiner->registerJob( rThreadData.aJob );
        }
        else
        {
            OSL_FAIL( "<ThreadManager::StartThread(..)> - ThreadJoiner already gone!" );
        }
    }
    else
    {
        // thread couldn't be started.
        maWaitingForStartThreads.push_front( rThreadData );
    }

    return bThreadStarted;
}

IMPL_LINK_NOARG(ThreadManager, TryToStartNewThread, Timer *, void)
{
    std::unique_lock aGuard(maMutex);

    if ( mbStartingOfThreadsSuspended )
        return;

    // Try to start thread from waiting ones
    if ( !StartWaitingThread() )
    {
        // No success on starting thread
        // If no more started threads exist, but still threads are waiting,
        // setup Timer to start thread from waiting ones
        if ( maStartedThreads.empty() && !maWaitingForStartThreads.empty() )
        {
            maStartNewThreadIdle.Start();
        }
    }
}

void ThreadManager::ResumeStartingOfThreads()
{
    std::unique_lock aGuard(maMutex);

    mbStartingOfThreadsSuspended = false;

    while ( maStartedThreads.size() < snStartedSize &&
            !maWaitingForStartThreads.empty() )
    {
        if ( !StartWaitingThread() )
        {
            // No success on starting thread
            // If no more started threads exist, but still threads are waiting,
            // setup Timer to start thread from waiting ones
            if ( maStartedThreads.empty() && !maWaitingForStartThreads.empty() )
            {
                maStartNewThreadIdle.Start();
                break;
            }
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

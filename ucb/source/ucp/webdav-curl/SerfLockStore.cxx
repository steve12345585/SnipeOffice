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

#include <chrono>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <osl/time.h>
#include <osl/thread.hxx>
#include <salhelper/thread.hxx>
#include <condition_variable>

#include <com/sun/star/ucb/LockScope.hpp>
#include <thread>

#include "CurlSession.hxx"
#include "SerfLockStore.hxx"

using namespace http_dav_ucp;

namespace http_dav_ucp {

class TickerThread : public salhelper::Thread
{
    bool m_bFinish;
    SerfLockStore & m_rLockStore;

public:

    explicit TickerThread( SerfLockStore & rLockStore )
        : Thread( "WebDavTickerThread" ), m_bFinish( false ),
          m_rLockStore( rLockStore ) {}

    void finish() { m_bFinish = true; }

private:

    virtual void execute();
};

} // namespace http_dav_ucp


void TickerThread::execute()
{
    osl_setThreadName("http_dav_ucp::TickerThread");
    SAL_INFO("ucb.ucp.webdav", "TickerThread: start.");

    std::unique_lock aGuard(m_rLockStore.m_aMutex);

    while (!m_bFinish)
    {
        auto sleep_duration = m_rLockStore.refreshLocks(aGuard);

        if (sleep_duration == std::chrono::milliseconds::max())
        {
            // Wait until a lock is added or shutdown
            m_rLockStore.m_aCondition.wait(
                aGuard, [this] { return !m_rLockStore.m_aLockInfoMap.empty() || m_bFinish; });
        }
        else
        {
            // Wait until the next deadline or a notification
            m_rLockStore.m_aCondition.wait_for(aGuard, sleep_duration);
        }
    }

    SAL_INFO("ucb.ucp.webdav", "TickerThread: stop.");
}


SerfLockStore::SerfLockStore()
{
}


SerfLockStore::~SerfLockStore()
{
    std::unique_lock aGuard(m_aMutex);
    stopTicker(aGuard);
    aGuard.lock(); // actually no threads should even try to access members now

    // release active locks, if any.
    SAL_WARN_IF( !m_aLockInfoMap.empty(), "ucb.ucp.webdav",
                "SerfLockStore::~SerfLockStore - Releasing active locks!" );

    for ( auto& rLockInfo : m_aLockInfoMap )
    {
        rLockInfo.second.m_xSession->NonInteractive_UNLOCK(rLockInfo.first);
    }
}

void SerfLockStore::startTicker(std::unique_lock<std::mutex> & /* rGuard is held */)
{
    if ( !m_pTickerThread.is() )
    {
        m_pTickerThread = new TickerThread( *this );
        m_pTickerThread->launch();
    }
}

void SerfLockStore::stopTicker(std::unique_lock<std::mutex> & rGuard)
{
    rtl::Reference<TickerThread> pTickerThread;

    if (m_pTickerThread.is())
    {
        m_pTickerThread->finish(); // needs mutex
        // the TickerThread may run refreshLocks() at most once after this
        pTickerThread = m_pTickerThread;

        m_pTickerThread.clear();
    }

    rGuard.unlock();

    if (pTickerThread.is() && pTickerThread->getIdentifier() != osl::Thread::getCurrentIdentifier())
    {
        pTickerThread->join(); // without m_aMutex locked (to prevent deadlock)
    }
}

bool SerfLockStore::joinThreads()
{
    std::unique_lock aGuard(m_aMutex);
    // FIXME: cure could be worse than the problem; we don't
    // want to block on a long-standing webdav lock refresh request.
    // perhaps we should timeout on a condition instead if a request
    // is in progress.
    if (m_pTickerThread.is())
        stopTicker(aGuard);
    return true;
}

void SerfLockStore::startThreads()
{
    std::unique_lock aGuard( m_aMutex );
    if (!m_aLockInfoMap.empty())
        startTicker(aGuard);
}

OUString const*
SerfLockStore::getLockTokenForURI(OUString const& rURI, css::ucb::Lock const*const pLock)
{
    assert(rURI.startsWith("http://") || rURI.startsWith("https://"));

    std::unique_lock aGuard( m_aMutex );

    auto const it(m_aLockInfoMap.find(rURI));

    if (it == m_aLockInfoMap.end())
    {
        return nullptr;
    }
    if (!pLock) // any lock will do
    {
        return &it->second.m_sToken;
    }
    // 0: EXCLUSIVE 1: SHARED
    if (it->second.m_Lock.Scope == ucb::LockScope_SHARED && pLock->Scope == ucb::LockScope_EXCLUSIVE)
    {
        return nullptr;
    }
    assert(it->second.m_Lock.Type == pLock->Type); // only WRITE possible
    if (it->second.m_Lock.Depth < pLock->Depth)
    {
        return nullptr;
    }
    // Only own locks are expected in the lock store, but depending on the
    // server it->second.m_Lock.Owner may contain the string this UCP passed in
    // the LOCK request, or a user identifier generated by the server (happens
    // with Sharepoint), so just ignore it here.
    // ignore Timeout ?
    return &it->second.m_sToken;
}

void SerfLockStore::addLock(const OUString& rURI, ucb::Lock const& rLock, const OUString& sToken,
                            rtl::Reference<CurlSession> const& xSession,
                            sal_Int32 nLastChanceToSendRefreshRequest)
{
    assert(rURI.startsWith("http://") || rURI.startsWith("https://"));
    std::unique_lock aGuard(m_aMutex);

    m_aLockInfoMap[rURI] = LockInfo(sToken, rLock, xSession, nLastChanceToSendRefreshRequest);
    m_aCondition.notify_all(); // Wake up the TickerThread

    startTicker(aGuard);
}


void SerfLockStore::removeLock(const OUString& rURI)
{
    std::unique_lock aGuard( m_aMutex );

    removeLockImpl(aGuard, rURI);
}

void SerfLockStore::removeLockImpl(std::unique_lock<std::mutex> & rGuard, const OUString& rURI)
{
    assert(rURI.startsWith("http://") || rURI.startsWith("https://"));

    m_aLockInfoMap.erase(rURI);

    if ( m_aLockInfoMap.empty() )
    {
        stopTicker(rGuard);
    }
}

std::chrono::milliseconds SerfLockStore::refreshLocks(std::unique_lock<std::mutex>& rGuard)
{
    assert(rGuard.owns_lock());
    (void)rGuard;

    TimeValue currentTimeVal;
    osl_getSystemTime(&currentTimeVal);
    sal_Int32 currentTime = currentTimeVal.Seconds;

    ::std::vector<OUString> authFailedLocks;
    std::chrono::milliseconds min_remaining = std::chrono::milliseconds::max();

    for ( auto& rLockInfo : m_aLockInfoMap )
    {
        LockInfo & rInfo = rLockInfo.second;
        if ( rInfo.m_nLastChanceToSendRefreshRequest != -1 )
        {
            // 30 seconds or less remaining until lock expires?
            sal_Int32 deadline = rInfo.m_nLastChanceToSendRefreshRequest - 30;
            if ( deadline <= currentTime )
            {
                // refresh the lock.
                sal_Int32 nlastChanceToSendRefreshRequest = -1;
                bool isAuthFailed(false);
                if (rInfo.m_xSession->NonInteractive_LOCK(
                         rLockInfo.first, rLockInfo.second.m_sToken,
                         nlastChanceToSendRefreshRequest,
                         isAuthFailed))
                {
                    rInfo.m_nLastChanceToSendRefreshRequest
                        = nlastChanceToSendRefreshRequest;
                }
                else
                {
                    if (isAuthFailed)
                    {
                        authFailedLocks.push_back(rLockInfo.first);
                    }
                    // refresh failed. stop auto-refresh.
                    rInfo.m_nLastChanceToSendRefreshRequest = -1;
                }
            }
            if (rInfo.m_nLastChanceToSendRefreshRequest != -1)
            {
                sal_Int32 remaining = (rInfo.m_nLastChanceToSendRefreshRequest - 30) - currentTime;
                if (remaining > 0)
                {
                    auto remaining_ms = std::chrono::seconds(remaining);
                    if ( remaining_ms < min_remaining )
                        min_remaining
                            = std::chrono::duration_cast<std::chrono::milliseconds>(remaining_ms);
                }
            }
        }
    }

    for (auto const& rLock : authFailedLocks)
    {
        removeLockImpl(rGuard, rLock);
    }

    return min_remaining;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

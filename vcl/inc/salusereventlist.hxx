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

#include <sal/config.h>
#include <vcl/dllapi.h>
#include <mutex>
#include <osl/thread.hxx>

#include <list>
#include <o3tl/sorted_vector.hxx>

class SalFrame;
enum class SalEvent;

typedef o3tl::sorted_vector< SalFrame* > SalFrameSet;

class VCL_PLUGIN_PUBLIC SalUserEventList
{
public:
    struct SalUserEvent
    {
        SalFrame*     m_pFrame;
        void*         m_pData;
        SalEvent      m_nEvent;

        SalUserEvent( SalFrame* pFrame, void* pData, SalEvent nEvent )
                : m_pFrame( pFrame ),
                  m_pData( pData ),
                  m_nEvent( nEvent )
        {}

        bool operator==(const SalUserEvent &aEvent) const
        {
            return m_pFrame == aEvent.m_pFrame
                && m_pData == aEvent.m_pData
                && m_nEvent== aEvent.m_nEvent;
        }
    };

protected:
    mutable std::mutex         m_aUserEventsMutex;
    std::list< SalUserEvent >  m_aUserEvents;
    std::list< SalUserEvent >  m_aProcessingUserEvents;
    bool                       m_bAllUserEventProcessedSignaled;
    SalFrameSet                m_aFrames;
    oslThreadIdentifier        m_aProcessingThread;

    virtual void ProcessEvent( SalUserEvent aEvent ) = 0;
    virtual void TriggerUserEventProcessing() = 0;
    virtual void TriggerAllUserEventsProcessed() {}

    inline bool HasUserEvents_NoLock() const;
public:
    SalUserEventList();
    virtual ~SalUserEventList() COVERITY_NOEXCEPT_FALSE;

    inline const SalFrameSet& getFrames() const;
    inline SalFrame* anyFrame() const;
    void insertFrame( SalFrame* pFrame );
    void eraseFrame( SalFrame* pFrame );
    inline bool isFrameAlive( const SalFrame* pFrame ) const;

    void PostEvent( SalFrame* pFrame, void* pData, SalEvent nEvent );
    void RemoveEvent( SalFrame* pFrame, void* pData, SalEvent nEvent );
    inline bool HasUserEvents() const;

    bool DispatchUserEvents( bool bHandleAllCurrentEvents );
};

inline SalFrame* SalUserEventList::anyFrame() const
{
    if ( m_aFrames.empty() )
        return nullptr;
    return *m_aFrames.begin();
}

inline bool SalUserEventList::isFrameAlive( const SalFrame* pFrame ) const
{
    auto it = m_aFrames.find( const_cast<SalFrame*>( pFrame ) );
    return it != m_aFrames.end();
}

inline bool SalUserEventList::HasUserEvents() const
{
    std::unique_lock aGuard( m_aUserEventsMutex );
    return HasUserEvents_NoLock();
}

inline bool SalUserEventList::HasUserEvents_NoLock() const
{
    return !(m_aUserEvents.empty() && m_aProcessingUserEvents.empty());
}

inline void SalUserEventList::PostEvent( SalFrame* pFrame, void* pData, SalEvent nEvent )
{
    std::unique_lock aGuard( m_aUserEventsMutex );
    m_aUserEvents.push_back( SalUserEvent( pFrame, pData, nEvent ) );
    m_bAllUserEventProcessedSignaled = false;
    TriggerUserEventProcessing();
}

inline const SalFrameSet& SalUserEventList::getFrames() const
{
    return m_aFrames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

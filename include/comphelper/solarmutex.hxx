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

#ifndef INCLUDED_COMPHELPER_SOLARMUTEX_HXX
#define INCLUDED_COMPHELPER_SOLARMUTEX_HXX

#include <sal/config.h>

#include <assert.h>
#include <atomic>
#include <thread>

#include <osl/mutex.hxx>
#include <comphelper/comphelperdllapi.h>

namespace comphelper {


/**
 * SolarMutex, needed for VCL's Application::GetSolarMutex().
 *
 * The SolarMutex is the one big recursive code lock used
 * to protect the vast majority of the LibreOffice code-base,
 * in particular anything that is graphical and the cores of
 * the applications.
 *
 * Treat this as a singleton, as its constructor sets a global
 * pointing at itself.
 */
class COMPHELPER_DLLPUBLIC SolarMutex {
public:
    typedef void (*BeforeReleaseHandler) ();

    SolarMutex();
    virtual ~SolarMutex();

    void SetBeforeReleaseHandler( const BeforeReleaseHandler& rLink )
         { m_aBeforeReleaseHandler = rLink; }

    void acquire( sal_uInt32 nLockCount = 1 );
    sal_uInt32 release( bool bUnlockAll = false );

    virtual bool tryToAcquire();

    // returns true, if the mutex is owned by the current thread
    virtual bool IsCurrentThread() const;

    /// Help components to get the SolarMutex easily.
    static SolarMutex *get();

protected:
    virtual sal_uInt32 doRelease( bool bUnlockAll );
    virtual void doAcquire( sal_uInt32 nLockCount );

    osl::Mutex            m_aMutex;
    sal_uInt32            m_nCount;

private:
    std::atomic<std::thread::id> m_nThreadId;

    SolarMutex(const SolarMutex&) = delete;
    SolarMutex& operator=(const SolarMutex&) = delete;

    BeforeReleaseHandler  m_aBeforeReleaseHandler;
};

inline void SolarMutex::acquire( sal_uInt32 nLockCount )
{
    assert( nLockCount > 0 );
    doAcquire( nLockCount );
}

inline sal_uInt32 SolarMutex::release( bool bUnlockAll )
{
     return doRelease( bUnlockAll );
}

}

#endif // INCLUDED_COMPHELPER_SOLARMUTEX_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

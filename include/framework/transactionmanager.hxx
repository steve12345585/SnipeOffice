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

#include <mutex>

#include <framework/gate.hxx>

namespace framework{

/*-************************************************************************************************************
    @descr          Describe different states of a feature of following implementation.
                    During lifetime of an object different working states occur:
                        initialization - working - closing - closed
                    If you wish to implement thread safe classes you should use this feature to protect
                    your code against calls at wrong time. e.g. you are not full initialized but somewhere
                    call an interface method (initialize phase means startup time from creating object till
                    calling specified first method e.g. XInitialization::initialize()!) then you should refuse
                    this call. The same for closing/disposing the object!
*//*-*************************************************************************************************************/
enum EWorkingMode
{
    E_INIT       ,   // We stand in an init method  -> some calls are accepted - some ones are rejected
    E_WORK       ,   // Object is ready for working -> all calls are accepted
    E_BEFORECLOSE,   // We stand in a close method  -> some calls are accepted - some ones are rejected
    E_CLOSE          // Object is dead!             -> all calls are rejected!
};

/*-************************************************************************************************************
    @descr          A transaction object should support throwing exceptions if user used it at wrong working mode.
                    e.g. We can throw a DisposedException if user try to work and our mode is E_CLOSE!
                    But sometimes he doesn't need this feature - will handle it by himself.
                    Then we must differ between some exception-modi:
                        E_HARDEXCEPTIONS        We throw exceptions for all working modes different from E_WORK!
                        E_SOFTEXCEPTIONS        We throw exceptions for all working modes different from E_WORK AND E_INCLOSE!
                                                This mode is useful for impl-methods which should be callable from dispose() method!

                                                e.g.    void dispose()
                                                        {
                                                            m_aTransactionManager.setWorkingMode( E_BEFORECLOSE );
                                                            ...
                                                            impl_setA( 0 );
                                                            ...
                                                            m_aTransactionManager.setWorkingMode( E_CLOSE );
                                                        }

                                                        void impl_setA( int nA )
                                                        {
                                                            ERejectReason       EReason;
                                                            TransactionGuard    aTransactionGuard( m_aTransactionManager, E_SOFTEXCEPTIONS, eReason );

                                                            m_nA = nA;
                                                        }

                                                Normally (if E_HARDEXCEPTIONS was used!) creation of guard
                                                will throw an exception ... but using of E_SOFTEXCEPTIONS suppress it
                                                and member "A" can be set.
*//*-*************************************************************************************************************/
enum EExceptionMode
{
    E_HARDEXCEPTIONS,
    E_SOFTEXCEPTIONS
};

/*-************************************************************************************************************
    @short          implement a transaction manager to support non breakable interface methods
    @descr          Use it to support non breakable interface methods without using any thread
                    synchronization like e.g. mutex, rw-lock!
                    That protect your code against wrong calls at wrong time ... e.g. calls after disposing an object!
                    Use combination of EExceptionMode and ERejectReason to detect rejected requests
                    and react for it. You can enable automatically throwing of exceptions too.

    @devstatus      draft
*//*-*************************************************************************************************************/
class TransactionManager
{

    //  public methods

    public:

                                   TransactionManager           (                                              );
                                   ~TransactionManager          (                                              );
                                   TransactionManager(const TransactionManager&) = delete;
        TransactionManager&        operator=(const TransactionManager&) = delete;
        void               setWorkingMode               ( EWorkingMode eMode                           );
        EWorkingMode       getWorkingMode               (                                              ) const;
        /// @throws css::uno::RuntimeException
        /// @throws css::lang::DisposedException
        void               registerTransaction          ( EExceptionMode eMode );
        /// @throws css::uno::RuntimeException
        /// @throws css::lang::DisposedException
        void               unregisterTransaction        (                                              );

    private:

        mutable std::mutex      m_aAccessLock;   /// regulate access on internal member of this instance
        Gate                    m_aBarrier;   /// used to block transactions requests during change or work mode
        EWorkingMode            m_eWorkingMode;   /// current working mode of object which use this manager (used to reject calls at wrong time)
        sal_Int32               m_nTransactionCount;   /// every transaction request is registered by this counter

};      //  class TransactionManager

}       //  namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

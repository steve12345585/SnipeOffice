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

#include <com/sun/star/document/XActionLockable.hpp>
#include <mutex>

namespace framework{

/** @short  implements a guard, which can use the interface
            <type scope="css::document">XActionLockable</type>.

    @descr  This guard should be used to be sure, that any lock will be
            released. Otherwise the locked document can hinder the office on shutdown!
*/
class ActionLockGuard final
{

    // member

    private:
        std::mutex m_mutex;

        /** @short  points to the object, which can be locked from outside. */
        css::uno::Reference< css::document::XActionLockable > m_xActionLock;

        /** @short  knows if a lock exists on the internal lock object
                    forced by this guard instance. */
        bool m_bActionLocked;

    // interface

    public:

        /** @short  default ctor to initialize a "non working guard".

            @descr  That can be useful in cases, where no resource still exists,
                    but will be available next time. Then this guard can be used
                    in a mode "use guard for more than one resources".
         */
        ActionLockGuard()
            : m_bActionLocked(false)
        {
        }

        /** @short  release this guard instance and make sure, that no lock
                    will exist afterwards on the internal wrapped resource.
         */
        ~ActionLockGuard()
        {
            unlock();
        }

        /** @short  set a new resource for locking at this guard.

            @descr  This call will fail, if an internal resource already exists
                    and is currently locked.

            @param  xLock
                    points to the outside resource, which should be locked.

            @return sal_True, if new resource could be set and locked.
                    sal_False otherwise.
         */
        bool setResource(const css::uno::Reference< css::document::XActionLockable >& xLock)
        {
            std::unique_lock g(m_mutex);

            if (m_bActionLocked || !xLock.is())
                return false;

            m_xActionLock = xLock;
            m_xActionLock->addActionLock();
            m_bActionLocked = m_xActionLock->isActionLocked();

            return true;
        }

        /** @short  set a new resource for locking at this guard.

            @descr  This call will fail, if an internal resource already exists
                    and is currently locked.

            @param  xLock
                    points to the outside resource, which should be locked.

            @return sal_True, if new resource could be set and locked.
                    sal_False otherwise.
         */
        void freeResource()
        {
            // SAFE -> ..........................
            std::unique_lock aMutexLock(m_mutex);

            css::uno::Reference< css::document::XActionLockable > xLock   = m_xActionLock;
            bool                                                  bLocked = m_bActionLocked;

            m_xActionLock.clear();
            m_bActionLocked = false;

            aMutexLock.unlock();
            // <- SAFE ..........................

            if (bLocked && xLock.is())
                xLock->removeActionLock();
        }

        /** @short  unlock the internal wrapped resource, if it's not already done. */
        void unlock()
        {
            std::unique_lock g(m_mutex);
            if (m_bActionLocked && m_xActionLock.is())
            {
                m_xActionLock->removeActionLock();
                // don't check for any locks here ...
                // May another guard use the same lock object :-(
                m_bActionLocked = false;
            }
        }
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

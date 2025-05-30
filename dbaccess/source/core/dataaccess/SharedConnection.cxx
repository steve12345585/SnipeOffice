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

#include "SharedConnection.hxx"
#include <comphelper/uno3.hxx>

namespace dbaccess
{
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace connectivity;

OSharedConnection::OSharedConnection(Reference<XAggregation>& _rxProxyConnection)
{
    setDelegation(_rxProxyConnection);
}

OSharedConnection::~OSharedConnection() {}

Reference<XStatement> SAL_CALL OSharedConnection::createStatement()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->createStatement();
}

Reference<XPreparedStatement> SAL_CALL OSharedConnection::prepareStatement(const OUString& sql)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->prepareStatement(sql);
}

Reference<XPreparedStatement> SAL_CALL OSharedConnection::prepareCall(const OUString& sql)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->prepareCall(sql);
}

OUString SAL_CALL OSharedConnection::nativeSQL(const OUString& sql)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->nativeSQL(sql);
}

sal_Bool SAL_CALL OSharedConnection::getAutoCommit()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->getAutoCommit();
}

void SAL_CALL OSharedConnection::commit()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    m_xConnection->commit();
}

void SAL_CALL OSharedConnection::rollback()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    m_xConnection->rollback();
}

sal_Bool SAL_CALL OSharedConnection::isClosed()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    if (!m_xConnection.is())
        return true;

    return m_xConnection->isClosed();
}

Reference<XDatabaseMetaData> SAL_CALL OSharedConnection::getMetaData()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->getMetaData();
}

sal_Bool SAL_CALL OSharedConnection::isReadOnly()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->isReadOnly();
}

OUString SAL_CALL OSharedConnection::getCatalog()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->getCatalog();
}

sal_Int32 SAL_CALL OSharedConnection::getTransactionIsolation()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->getTransactionIsolation();
}

Reference<css::container::XNameAccess> SAL_CALL OSharedConnection::getTypeMap()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    return m_xConnection->getTypeMap();
}

} // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

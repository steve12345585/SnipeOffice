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

#include "NConnection.hxx"
#include "NDatabaseMetaData.hxx"
#include "NCatalog.hxx"
#include <com/sun/star/sdbc/TransactionIsolation.hpp>
#include "NPreparedStatement.hxx"
#include "NStatement.hxx"
#include <connectivity/dbexception.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>

using namespace connectivity::evoab;
using namespace dbtools;


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;

OEvoabConnection::OEvoabConnection(OEvoabDriver const & _rDriver)
    : m_rDriver(_rDriver)
    , m_eSDBCAddressType(SDBCAddress::EVO_LOCAL)
{
}

OEvoabConnection::~OEvoabConnection()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    if(!isClosed()) {
        acquire();
        close();
    }
}


// XServiceInfo

IMPLEMENT_SERVICE_INFO(OEvoabConnection, u"com.sun.star.sdbc.drivers.evoab.Connection"_ustr, u"com.sun.star.sdbc.Connection"_ustr)


void OEvoabConnection::construct(const OUString& url, const Sequence< PropertyValue >& info)
{
    osl_atomic_increment( &m_refCount );
    SAL_INFO("connectivity.evoab2", "OEvoabConnection::construct()::url = " << url );

    OUString sPassword;
    const char pPwd[] = "password";

    const PropertyValue *pIter      = info.getConstArray();
    const PropertyValue *pEnd       = pIter + info.getLength();
    for(;pIter != pEnd;++pIter)
    {
            if(pIter->Name == pPwd)
            {
                    pIter->Value >>= sPassword;
                    break;
            }
    }

    if ( url == "sdbc:address:evolution:groupwise" )
        setSDBCAddressType(SDBCAddress::EVO_GWISE);
    else if ( url == "sdbc:address:evolution:ldap" )
        setSDBCAddressType(SDBCAddress::EVO_LDAP);
    else
        setSDBCAddressType(SDBCAddress::EVO_LOCAL);
    setURL(url);
    setPassword(OUStringToOString(sPassword,RTL_TEXTENCODING_UTF8));
    osl_atomic_decrement( &m_refCount );
}


OUString SAL_CALL OEvoabConnection::nativeSQL( const OUString& _sSql )
{
    // when you need to transform SQL92 to you driver specific you can do it here
    return _sSql;
}

Reference< XDatabaseMetaData > SAL_CALL OEvoabConnection::getMetaData(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    Reference< XDatabaseMetaData > xMetaData = m_xMetaData;
    if(!xMetaData.is())
    {
        xMetaData = new OEvoabDatabaseMetaData(this);
        m_xMetaData = xMetaData;
    }

    return xMetaData;
}

css::uno::Reference< XTablesSupplier > OEvoabConnection::createCatalog()
{
    ::osl::MutexGuard aGuard( m_aMutex );
    Reference< XTablesSupplier > xTab = m_xCatalog;
    if(!xTab.is())
    {
         xTab = new OEvoabCatalog(this);
         m_xCatalog = xTab;
    }
    return xTab;
}

Reference< XStatement > SAL_CALL OEvoabConnection::createStatement(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    Reference< XStatement > xStmt = new OStatement(this);
    m_aStatements.push_back(WeakReferenceHelper(xStmt));
    return xStmt;
}

Reference< XPreparedStatement > SAL_CALL OEvoabConnection::prepareStatement( const OUString& sql )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    rtl::Reference<OEvoabPreparedStatement> pStmt = new OEvoabPreparedStatement( this );
    pStmt->construct( sql );

    m_aStatements.push_back(WeakReferenceHelper(*pStmt));
    return pStmt;
}

Reference< XPreparedStatement > SAL_CALL OEvoabConnection::prepareCall( const OUString& /*sql*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::prepareCall"_ustr, *this );
    return nullptr;
}
sal_Bool SAL_CALL OEvoabConnection::isClosed(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    return OConnection_BASE::rBHelper.bDisposed;
}


// XCloseable
void SAL_CALL OEvoabConnection::close(  )
{
    {  // we just dispose us
        ::osl::MutexGuard aGuard( m_aMutex );
        checkDisposed(OConnection_BASE::rBHelper.bDisposed);
    }
    dispose();
}


// XWarningsSupplier
Any SAL_CALL OEvoabConnection::getWarnings(  )
{
    return m_aWarnings.getWarnings();
}
void SAL_CALL OEvoabConnection::clearWarnings(  )
{
    m_aWarnings.clearWarnings();
}


void OEvoabConnection::disposing()
{
    // we noticed that we should be destroyed in near future so we have to dispose our statements
    ::osl::MutexGuard aGuard(m_aMutex);
    OConnection_BASE::disposing();
}

// -------------------------------- stubbed methods ------------------------------------------------
void SAL_CALL OEvoabConnection::setAutoCommit( sal_Bool /*autoCommit*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setAutoCommit"_ustr, *this );
}
sal_Bool SAL_CALL OEvoabConnection::getAutoCommit(  )
{
    return true;
}
void SAL_CALL OEvoabConnection::commit(  )
{
}
void SAL_CALL OEvoabConnection::rollback(  )
{
}
void SAL_CALL OEvoabConnection::setReadOnly( sal_Bool /*readOnly*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setReadOnly"_ustr, *this );
}
sal_Bool SAL_CALL OEvoabConnection::isReadOnly(  )
{
    return false;
}
void SAL_CALL OEvoabConnection::setCatalog( const OUString& /*catalog*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setCatalog"_ustr, *this );
}

OUString SAL_CALL OEvoabConnection::getCatalog(  )
{
    return OUString();
}
void SAL_CALL OEvoabConnection::setTransactionIsolation( sal_Int32 /*level*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setTransactionIsolation"_ustr, *this );
}

sal_Int32 SAL_CALL OEvoabConnection::getTransactionIsolation(  )
{
    return TransactionIsolation::NONE;
}

Reference< css::container::XNameAccess > SAL_CALL OEvoabConnection::getTypeMap(  )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::getTypeMap"_ustr, *this );
    return nullptr;
}
void SAL_CALL OEvoabConnection::setTypeMap( const Reference< css::container::XNameAccess >& /*typeMap*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( u"XConnection::setTypeMap"_ustr, *this );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

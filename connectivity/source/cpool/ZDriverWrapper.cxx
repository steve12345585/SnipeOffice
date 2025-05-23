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

#include "ZDriverWrapper.hxx"
#include "ZConnectionPool.hxx"
#include <osl/diagnose.h>


namespace connectivity
{


    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::beans;

    ODriverWrapper::ODriverWrapper( Reference< XAggregation >& _rxAggregateDriver, OConnectionPool* _pPool )
        :m_pConnectionPool(_pPool)
    {
        OSL_ENSURE(_rxAggregateDriver.is(), "ODriverWrapper::ODriverWrapper: invalid aggregate!");
        OSL_ENSURE(m_pConnectionPool.is(), "ODriverWrapper::ODriverWrapper: invalid connection pool!");

        osl_atomic_increment( &m_refCount );
        if (_rxAggregateDriver.is())
        {
            // transfer the (one and only) real ref to the aggregate to our member
            m_xDriverAggregate = _rxAggregateDriver;
            _rxAggregateDriver = nullptr;

            // a second "real" reference
            m_xDriver.set(m_xDriverAggregate, UNO_QUERY);
            OSL_ENSURE(m_xDriver.is(), "ODriverWrapper::ODriverWrapper: invalid aggregate (no XDriver)!");

            // set ourself as delegator
            m_xDriverAggregate->setDelegator( getXWeak() );
        }
        osl_atomic_decrement( &m_refCount );
    }


    ODriverWrapper::~ODriverWrapper()
    {
        if (m_xDriverAggregate.is())
            m_xDriverAggregate->setDelegator(nullptr);
    }


    Any SAL_CALL ODriverWrapper::queryInterface( const Type& _rType )
    {
        Any aReturn = ODriverWrapper_BASE::queryInterface(_rType);
        return aReturn.hasValue() ? aReturn : (m_xDriverAggregate.is() ? m_xDriverAggregate->queryAggregation(_rType) : aReturn);
    }


    Reference< XConnection > SAL_CALL ODriverWrapper::connect( const OUString& url, const Sequence< PropertyValue >& info )
    {
        Reference< XConnection > xConnection;
        if (m_pConnectionPool.is())
            // route this through the pool
            xConnection = m_pConnectionPool->getConnectionWithInfo( url, info );
        else if (m_xDriver.is())
            xConnection = m_xDriver->connect( url, info );

        return xConnection;
    }


    sal_Bool SAL_CALL ODriverWrapper::acceptsURL( const OUString& url )
    {
        return m_xDriver.is() && m_xDriver->acceptsURL(url);
    }


    Sequence< DriverPropertyInfo > SAL_CALL ODriverWrapper::getPropertyInfo( const OUString& url, const Sequence< PropertyValue >& info )
    {
        Sequence< DriverPropertyInfo > aInfo;
        if (m_xDriver.is())
            aInfo = m_xDriver->getPropertyInfo(url, info);
        return aInfo;
    }


    sal_Int32 SAL_CALL ODriverWrapper::getMajorVersion(  )
    {
        return m_xDriver.is() ? m_xDriver->getMajorVersion() : 0;
    }


    sal_Int32 SAL_CALL ODriverWrapper::getMinorVersion(  )
    {
        return m_xDriver.is() ? m_xDriver->getMinorVersion() : 0;
    }


}   // namespace connectivity


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

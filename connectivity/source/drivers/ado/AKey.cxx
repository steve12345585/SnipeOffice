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

#include <ado/AKey.hxx>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <comphelper/servicehelper.hxx>
#include <ado/AColumns.hxx>
#include <ado/AConnection.hxx>

using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;


OAdoKey::OAdoKey(bool _bCase,OConnection* _pConnection, ADOKey* _pKey)
    : OKey_ADO(_bCase)
    ,m_pConnection(_pConnection)
{
    construct();
    m_aKey.set(_pKey);
    fillPropertyValues();
}

OAdoKey::OAdoKey(bool _bCase,OConnection* _pConnection)
    : OKey_ADO(_bCase)
    ,m_pConnection(_pConnection)
{
    construct();
    m_aKey.Create();
}

void OAdoKey::refreshColumns()
{
    ::std::vector< OUString> aVector;

    WpADOColumns aColumns;
    if ( m_aKey.IsValid() )
    {
        aColumns = m_aKey.get_Columns();
        aColumns.fillElementNames(aVector);
    }

    if(m_pColumns)
        m_pColumns->reFill(aVector);
    else
        m_pColumns.reset(new OColumns(*this, m_aMutex, aVector, aColumns, isCaseSensitive(), m_pConnection));
}

void OAdoKey::setFastPropertyValue_NoBroadcast(sal_Int32 nHandle,const Any& rValue)
{
    if(m_aKey.IsValid())
    {
        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                {
                    OUString aVal;
                    rValue >>= aVal;
                    m_aKey.put_Name(aVal);
                    ADOS::ThrowException(m_pConnection->getConnection(),*this);
                }
                break;
            case PROPERTY_ID_TYPE:
                {
                    sal_Int32 nVal=0;
                    rValue >>= nVal;
                    m_aKey.put_Type(Map2KeyRule(nVal));
                    ADOS::ThrowException(m_pConnection->getConnection(),*this);
                }
                break;
            case PROPERTY_ID_REFERENCEDTABLE:
                {
                    OUString aVal;
                    rValue >>= aVal;
                    m_aKey.put_RelatedTable(aVal);
                    ADOS::ThrowException(m_pConnection->getConnection(),*this);
                }
                break;
            case PROPERTY_ID_UPDATERULE:
                {
                    sal_Int32 nVal=0;
                    rValue >>= nVal;
                    m_aKey.put_UpdateRule(Map2Rule(nVal));
                    ADOS::ThrowException(m_pConnection->getConnection(),*this);
                }
                break;
            case PROPERTY_ID_DELETERULE:
                {
                    sal_Int32 nVal=0;
                    rValue >>= nVal;
                    m_aKey.put_DeleteRule(Map2Rule(nVal));
                    ADOS::ThrowException(m_pConnection->getConnection(),*this);
                }
                break;
        }
    }
    OKey_ADO::setFastPropertyValue_NoBroadcast(nHandle,rValue);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <sal/config.h>

#include <cstddef>
#include <string_view>

#include <ado/AConnection.hxx>
#include <ado/ADatabaseMetaData.hxx>
#include <ado/ADriver.hxx>
#include <ado/AStatement.hxx>
#include <ado/ACallableStatement.hxx>
#include <ado/APreparedStatement.hxx>
#include <ado/ACatalog.hxx>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/sdbc/TransactionIsolation.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <comphelper/servicehelper.hxx>
#include <connectivity/dbexception.hxx>
#include <o3tl/string_view.hxx>
#include <osl/file.hxx>
#include <systools/win32/oleauto.hxx>
#include <strings.hrc>

using namespace dbtools;
using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;


IMPLEMENT_SERVICE_INFO(OConnection,"com.sun.star.sdbcx.AConnection","com.sun.star.sdbc.Connection");

OConnection::OConnection(ODriver*   _pDriver)
                         : m_xCatalog(nullptr),
                         m_pDriver(_pDriver),
                         m_nEngineType(0),
                         m_bClosed(false),
                         m_bAutocommit(true)
{
    osl_atomic_increment( &m_refCount );

    sal::systools::COMReference<IClassFactory2> pIUnknown;
    if (!FAILED(pIUnknown.CoGetClassObject(ADOS::CLSID_ADOCONNECTION_21, CLSCTX_INPROC_SERVER)))
    {
        HRESULT hr = pIUnknown->CreateInstanceLic(nullptr,
                                            nullptr,
                                            ADOS::IID_ADOCONNECTION_21,
                                            ADOS::GetKeyStr(),
                                            reinterpret_cast<void**>(&m_aAdoConnection));

        if( !FAILED( hr ) )
        {
            OSL_ENSURE(m_aAdoConnection, "OConnection::OConnection: invalid ADO object!");
        }
    }

    osl_atomic_decrement( &m_refCount );
}

OConnection::~OConnection()
{
}

void OConnection::construct(std::u16string_view url,const Sequence< PropertyValue >& info)
{
    osl_atomic_increment( &m_refCount );

    setConnectionInfo(info);

    std::size_t nLen = url.find(':');
    nLen = url.find(':',nLen == std::u16string_view::npos ? 0 : nLen+1);
    std::u16string_view aDSN(url.substr(nLen == std::u16string_view::npos ? 0 : nLen+1));
    OUString aUID,aPWD;
    o3tl::starts_with(aDSN, u"access:", &aDSN);

    sal_Int32 nTimeout = 20;
    for (const auto& propval : info)
    {
        if (propval.Name == "Timeout")
            propval.Value >>= nTimeout;
        else if (propval.Name == "user")
            propval.Value >>= aUID;
        else if (propval.Name == "password")
            propval.Value >>= aPWD;
    }
    try
    {
        if(m_aAdoConnection)
        {
            if(m_aAdoConnection.Open(aDSN,aUID,aPWD,adConnectUnspecified))
                m_aAdoConnection.PutCommandTimeout(nTimeout);
            else
                ADOS::ThrowException(m_aAdoConnection,*this);
            if(m_aAdoConnection.get_State() != adStateOpen)
                throwGenericSQLException( STR_NO_CONNECTION,*this );

            WpADOProperties aProps = m_aAdoConnection.get_Properties();
            if(aProps.IsValid())
            {
                OTools::putValue(aProps, std::u16string_view(u"ACE OLEDB:ODBC Parsing"), true);
                OLEVariant aVar(
                    OTools::getValue(aProps, std::u16string_view(u"ACE OLEDB:Engine Type")));
                if(!aVar.isNull() && !aVar.isEmpty())
                    m_nEngineType = aVar.getInt32();
            }
            buildTypeInfo();
            //bErg = TRUE;
        }
        else
            ::dbtools::throwFunctionSequenceException(*this);

    }
    catch(const Exception& )
    {
        osl_atomic_decrement( &m_refCount );
        throw;
    }
    osl_atomic_decrement( &m_refCount );
}

Reference< XStatement > SAL_CALL OConnection::createStatement(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    Reference< XStatement > xStmt = new OStatement(this);
    m_aStatements.push_back(WeakReferenceHelper(xStmt));
    return xStmt;
}

Reference< XPreparedStatement > SAL_CALL OConnection::prepareStatement( const OUString& sql )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    Reference< XPreparedStatement > xPStmt = new OPreparedStatement(this, sql);
    m_aStatements.push_back(WeakReferenceHelper(xPStmt));
    return xPStmt;
}

Reference< XPreparedStatement > SAL_CALL OConnection::prepareCall( const OUString& sql )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    Reference< XPreparedStatement > xPStmt = new OCallableStatement(this, sql);
    m_aStatements.push_back(WeakReferenceHelper(xPStmt));
    return xPStmt;
}

OUString SAL_CALL OConnection::nativeSQL( const OUString& _sql )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    OUString sql = _sql;
    WpADOProperties aProps = m_aAdoConnection.get_Properties();
    if(aProps.IsValid())
    {
        OTools::putValue(aProps, std::u16string_view(u"ACE OLEDB:ODBC Parsing"), true);
        WpADOCommand aCommand;
        aCommand.Create();
        aCommand.put_ActiveConnection(static_cast<IDispatch*>(m_aAdoConnection));
        aCommand.put_CommandText(sql);
        sql = aCommand.get_CommandText();
    }

    return sql;
}

void SAL_CALL OConnection::setAutoCommit( sal_Bool autoCommit )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    m_bAutocommit = autoCommit;
    if(!autoCommit)
        m_aAdoConnection.BeginTrans();
    else
        m_aAdoConnection.RollbackTrans();
}

sal_Bool SAL_CALL OConnection::getAutoCommit(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    return m_bAutocommit;
}

void SAL_CALL OConnection::commit(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    m_aAdoConnection.CommitTrans();
}

void SAL_CALL OConnection::rollback(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    m_aAdoConnection.RollbackTrans();
}

sal_Bool SAL_CALL OConnection::isClosed(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    return OConnection_BASE::rBHelper.bDisposed && !m_aAdoConnection.get_State();
}

Reference< XDatabaseMetaData > SAL_CALL OConnection::getMetaData(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    Reference< XDatabaseMetaData > xMetaData = m_xMetaData;
    if(!xMetaData.is())
    {
        xMetaData = new ODatabaseMetaData(this);
        m_xMetaData = xMetaData;
    }

    return xMetaData;
}

void SAL_CALL OConnection::setReadOnly( sal_Bool readOnly )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    m_aAdoConnection.put_Mode(readOnly ? adModeRead : adModeReadWrite);
    ADOS::ThrowException(m_aAdoConnection,*this);
}

sal_Bool SAL_CALL OConnection::isReadOnly(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    return m_aAdoConnection.get_Mode() == adModeRead;
}

void SAL_CALL OConnection::setCatalog( const OUString& catalog )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    m_aAdoConnection.PutDefaultDatabase(catalog);
    ADOS::ThrowException(m_aAdoConnection,*this);
}

OUString SAL_CALL OConnection::getCatalog(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    return m_aAdoConnection.GetDefaultDatabase();
}

void SAL_CALL OConnection::setTransactionIsolation( sal_Int32 level )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    IsolationLevelEnum eIso;
    switch(level)
    {
        case TransactionIsolation::NONE:
            eIso = adXactUnspecified;
            break;
        case TransactionIsolation::READ_UNCOMMITTED:
            eIso = adXactReadUncommitted;
            break;
        case TransactionIsolation::READ_COMMITTED:
            eIso = adXactReadCommitted;
            break;
        case TransactionIsolation::REPEATABLE_READ:
            eIso = adXactRepeatableRead;
            break;
        case TransactionIsolation::SERIALIZABLE:
            eIso = adXactSerializable;
            break;
        default:
            OSL_FAIL("OConnection::setTransactionIsolation invalid level");
            return;
    }
    m_aAdoConnection.put_IsolationLevel(eIso);
    ADOS::ThrowException(m_aAdoConnection,*this);
}

sal_Int32 SAL_CALL OConnection::getTransactionIsolation(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    sal_Int32 nRet = 0;
    switch(m_aAdoConnection.get_IsolationLevel())
    {
        case adXactUnspecified:
            nRet = TransactionIsolation::NONE;
            break;
        case adXactReadUncommitted:
            nRet = TransactionIsolation::READ_UNCOMMITTED;
            break;
        case adXactReadCommitted:
            nRet = TransactionIsolation::READ_COMMITTED;
            break;
        case adXactRepeatableRead:
            nRet = TransactionIsolation::REPEATABLE_READ;
            break;
        case adXactSerializable:
            nRet = TransactionIsolation::SERIALIZABLE;
            break;
        default:
            OSL_FAIL("OConnection::setTransactionIsolation invalid level");
    }
    ADOS::ThrowException(m_aAdoConnection,*this);
    return nRet;
}

Reference< css::container::XNameAccess > SAL_CALL OConnection::getTypeMap(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OConnection_BASE::rBHelper.bDisposed);


    return nullptr;
}

void SAL_CALL OConnection::setTypeMap( const Reference< css::container::XNameAccess >& /*typeMap*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XConnection::setTypeMap", *this );
}

// XCloseable
void SAL_CALL OConnection::close(  )
{
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        checkDisposed(OConnection_BASE::rBHelper.bDisposed);

    }
    dispose();
}

// XWarningsSupplier
Any SAL_CALL OConnection::getWarnings(  )
{
    return Any();
}

void SAL_CALL OConnection::clearWarnings(  )
{
}

void OConnection::buildTypeInfo()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    ADORecordset *pRecordset = m_aAdoConnection.getTypeInfo();
    if ( pRecordset )
    {
        pRecordset->AddRef();
        VARIANT_BOOL bIsAtBOF;
        pRecordset->get_BOF(&bIsAtBOF);

        bool bOk = true;
        if ( bIsAtBOF == VARIANT_TRUE )
            bOk = SUCCEEDED(pRecordset->MoveNext());

        if ( bOk )
        {
            // HACK for access
            static const char s_sVarChar[] = "VarChar";
            do
            {
                sal_Int32 nPos = 1;
                OExtendedTypeInfo* aInfo            = new OExtendedTypeInfo;
                aInfo->aSimpleType.aTypeName        = ADOS::getField(pRecordset,nPos++).get_Value().getString();
                aInfo->eType                        = static_cast<DataTypeEnum>(ADOS::getField(pRecordset,nPos++).get_Value().getInt32());
                if ( aInfo->eType == adWChar && aInfo->aSimpleType.aTypeName == s_sVarChar )
                    aInfo->eType = adVarWChar;
                aInfo->aSimpleType.nType            = static_cast<sal_Int16>(ADOS::MapADOType2Jdbc(aInfo->eType));
                aInfo->aSimpleType.nPrecision       = ADOS::getField(pRecordset,nPos++).get_Value().getInt32();
                nPos++; // aLiteralPrefix
                nPos++; // aLiteralSuffix
                nPos++; // aCreateParams
                nPos++; // bNullable
                nPos++; // bCaseSensitive
                nPos++; // nSearchType
                nPos++; // bUnsigned
                nPos++; // bCurrency
                nPos++; // bAutoIncrement
                aInfo->aSimpleType.aLocalTypeName   = ADOS::getField(pRecordset,nPos++).get_Value().getString();
                nPos++; // nMinimumScale
                aInfo->aSimpleType.nMaximumScale    = ADOS::getField(pRecordset,nPos++).get_Value().getInt16();
                if ( adCurrency == aInfo->eType && !aInfo->aSimpleType.nMaximumScale)
                {
                    aInfo->aSimpleType.nMaximumScale = 4;
                }
                nPos++; // nNumPrecRadix
                // Now that we have the type info, save it
                // in the Hashtable if we don't already have an
                // entry for this SQL type.

                m_aTypeInfo.emplace(aInfo->eType,aInfo);
            }
            while ( SUCCEEDED(pRecordset->MoveNext()) );
        }
        pRecordset->Release();
    }
}

void OConnection::disposing()
{
    ::osl::MutexGuard aGuard(m_aMutex);

    OConnection_BASE::disposing();

    m_bClosed   = true;
    m_xMetaData = css::uno::WeakReference< css::sdbc::XDatabaseMetaData>();
    m_xCatalog.clear();
    m_pDriver   = nullptr;

    m_aAdoConnection.Close();

    for (auto& rEntry : m_aTypeInfo)
        delete rEntry.second;

    m_aTypeInfo.clear();

    m_aAdoConnection.clear();
}

sal_Int64 SAL_CALL OConnection::getSomething( const css::uno::Sequence< sal_Int8 >& rId )
{
    return comphelper::getSomethingImpl(rId, this,
                                        comphelper::FallbackToGetSomethingOf<OConnection_BASE>{});
}

Sequence< sal_Int8 > OConnection::getUnoTunnelId()
{
    static const comphelper::UnoIdInit implId;
    return implId.getSeq();
}

const OExtendedTypeInfo* OConnection::getTypeInfoFromType(const OTypeInfoMap& _rTypeInfo,
                           DataTypeEnum _nType,
                           const OUString& _sTypeName,
                           sal_Int32 _nPrecision,
                           sal_Int32 _nScale,
                           bool& _brForceToType)
{
    const OExtendedTypeInfo* pTypeInfo = nullptr;
    _brForceToType = false;
    // search for type
    std::pair<OTypeInfoMap::const_iterator, OTypeInfoMap::const_iterator> aPair = _rTypeInfo.equal_range(_nType);
    OTypeInfoMap::const_iterator aIter = aPair.first;
    if(aIter != _rTypeInfo.end()) // compare with end is correct here
    {
        for(;aIter != aPair.second;++aIter)
        {
            // search the best matching type
            OExtendedTypeInfo* pInfo = aIter->second;
            if  (   (   !_sTypeName.getLength()
                    ||  (pInfo->aSimpleType.aTypeName.equalsIgnoreAsciiCase(_sTypeName))
                    )
                &&  (pInfo->aSimpleType.nPrecision      >= _nPrecision)
                &&  (pInfo->aSimpleType.nMaximumScale   >= _nScale)

                )
                break;
        }

        if (aIter == aPair.second)
        {
            for(aIter = aPair.first; aIter != aPair.second; ++aIter)
            {
                // search the best matching type (now comparing the local names)
                if  (   (aIter->second->aSimpleType.aLocalTypeName.equalsIgnoreAsciiCase(_sTypeName))
                    &&  (aIter->second->aSimpleType.nPrecision      >= _nPrecision)
                    &&  (aIter->second->aSimpleType.nMaximumScale   >= _nScale)
                    )
                {
// we can not assert here because we could be in d&d
/*
                    OSL_FAIL((  OString("getTypeInfoFromType: assuming column type ")
                        +=  OString(aIter->second->aTypeName.getStr(), aIter->second->aTypeName.getLength(), osl_getThreadTextEncoding())
                        +=  OString("\" (expected type name ")
                        +=  OString(_sTypeName.getStr(), _sTypeName.getLength(), osl_getThreadTextEncoding())
                        +=  OString(" matches the type's local name).")).getStr());
*/
                    break;
                }
            }
        }

        if (aIter == aPair.second)
        {   // no match for the names, no match for the local names
            // -> drop the precision and the scale restriction, accept any type with the property
            // type id (nType)

            // we can not assert here because we could be in d&d
            pTypeInfo = aPair.first->second;
            _brForceToType = true;
        }
        else
            pTypeInfo = aIter->second;
    }
    else if ( _sTypeName.getLength() )
    {
        ::comphelper::UStringMixEqual aCase(false);
        // search for typeinfo where the typename is equal _sTypeName
        OTypeInfoMap::const_iterator aFind = std::find_if(_rTypeInfo.begin(), _rTypeInfo.end(),
            [&aCase, &_sTypeName] (const OTypeInfoMap::value_type& typeInfo) {
                return aCase(typeInfo.second->getDBName(), _sTypeName);
            });

        if(aFind != _rTypeInfo.end())
            pTypeInfo = aFind->second;
    }

// we can not assert here because we could be in d&d
//  OSL_ENSURE(pTypeInfo, "getTypeInfoFromType: no type info found for this type!");
    return pTypeInfo;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

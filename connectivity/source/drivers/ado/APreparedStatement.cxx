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

#include <connectivity/sqlparse.hxx>
#include <ado/APreparedStatement.hxx>
#include <com/sun/star/sdbc/DataType.hpp>
#include <ado/AResultSetMetaData.hxx>
#include <ado/AResultSet.hxx>
#include <ado/ADriver.hxx>
#include <com/sun/star/lang/DisposedException.hpp>
#include <cppuhelper/typeprovider.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbexception.hxx>
#include <connectivity/dbtools.hxx>
#include <rtl/ref.hxx>
#include <strings.hrc>

#include <limits>

#define CHECK_RETURN(x)                                                 \
    if(!x)                                                              \
        ADOS::ThrowException(m_pConnection->getConnection(),*this);

#ifdef max
#   undef max
#endif

using namespace connectivity::ado;
using namespace connectivity;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::util;

IMPLEMENT_SERVICE_INFO(OPreparedStatement,"com.sun.star.sdbcx.APreparedStatement","com.sun.star.sdbc.PreparedStatement");

OPreparedStatement::OPreparedStatement( OConnection* _pConnection, const OUString& sql)
    : OPreparedStatement_BASE(_pConnection)
{
    osl_atomic_increment( &m_refCount );

    OSQLParser aParser(_pConnection->getDriver()->getContext());
    OUString sErrorMessage;
    OUString sNewSql;
    std::unique_ptr<OSQLParseNode> pNode = aParser.parseTree(sErrorMessage,sql);
    if(pNode)
    {   // special handling for parameters
        //  we recursive replace all occurrences of ? in the statement and
        //  replace them with name like "parame" */
        sal_Int32 nParameterCount = 0;
        replaceParameterNodeName(pNode.get(), "parame", nParameterCount);
        pNode->parseNodeToStr( sNewSql, _pConnection );
    }
    else
        sNewSql = sql;
    CHECK_RETURN(m_Command.put_CommandText(sNewSql))
    CHECK_RETURN(m_Command.put_Prepared(VARIANT_TRUE))
    m_pParameters = m_Command.get_Parameters();
    m_pParameters->AddRef();
    m_pParameters->Refresh();

    osl_atomic_decrement( &m_refCount );
}

OPreparedStatement::~OPreparedStatement()
{
    if (m_pParameters)
    {
        OSL_FAIL( "OPreparedStatement::~OPreparedStatement: not disposed!" );
        m_pParameters->Release();
        m_pParameters = nullptr;
    }
}

Reference< XResultSetMetaData > SAL_CALL OPreparedStatement::getMetaData(  )
{
    if(!m_xMetaData.is() && m_RecordSet.IsValid())
        m_xMetaData = new OResultSetMetaData(m_RecordSet);
    return m_xMetaData;
}

void OPreparedStatement::disposing()
{
    m_xMetaData.clear();
    if (m_pParameters)
    {
        m_pParameters->Release();
        m_pParameters = nullptr;
    }
    OStatement_Base::disposing();
}

void SAL_CALL OPreparedStatement::close(  )
{

    {
        ::osl::MutexGuard aGuard( m_aMutex );
        checkDisposed(OStatement_BASE::rBHelper.bDisposed);

    }
    dispose();

}

sal_Bool SAL_CALL OPreparedStatement::execute(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);

    clearWarnings ();

    // Call SQLExecute
    try {
        ADORecordset* pSet=nullptr;
        CHECK_RETURN(m_Command.Execute(m_RecordsAffected,m_Parameters,adCmdUnknown,&pSet))
        m_RecordSet.set(pSet);
    }
    catch (SQLWarning&)
    {
        //TODO: Save pointer to warning and save with ResultSet
        // object once it is created.
    }
    return m_RecordSet.IsValid();
}

sal_Int32 SAL_CALL OPreparedStatement::executeUpdate(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);


    ADORecordset* pSet=nullptr;
    CHECK_RETURN(m_Command.Execute(m_RecordsAffected,m_Parameters,adCmdUnknown,&pSet))
    if ( VT_ERROR == m_RecordsAffected.getType() )
    {
        ADOS::ThrowException(m_pConnection->getConnection(),*this);
        // to be sure that we get the error really thrown
        throw SQLException();
    }
    m_RecordSet.set(pSet);
    return m_RecordsAffected.getInt32();
}

void OPreparedStatement::setParameter(sal_Int32 parameterIndex, const DataTypeEnum& _eType,
                                      sal_Int32 _nSize,const OLEVariant& Val)
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);


    sal_Int32 nCount = 0;
    m_pParameters->get_Count(&nCount);
    if(nCount < (parameterIndex-1))
    {
        OUString sDefaultName = "parame" + OUString::number(parameterIndex);
        ADOParameter* pParam = m_Command.CreateParameter(sDefaultName,_eType,adParamInput,_nSize,Val);
        if(pParam)
        {
            m_pParameters->Append(pParam);
        }
    }
    else
    {
        WpADOParameter aParam;
        m_pParameters->get_Item(OLEVariant(sal_Int32(parameterIndex-1)),&aParam);
        if(aParam)
        {
            DataTypeEnum eType = aParam.GetADOType();
            if ( _eType != eType && _eType != adDBTimeStamp )
            {
                aParam.put_Type(_eType);
                eType = _eType;
                aParam.put_Size(_nSize);
            }

            if ( adVarBinary == eType && aParam.GetAttributes() == adParamLong )
            {
                aParam.AppendChunk(Val);
            }
            else
                CHECK_RETURN(aParam.PutValue(Val));
        }
    }
    ADOS::ThrowException(m_pConnection->getConnection(),*this);
}

void SAL_CALL OPreparedStatement::setString( sal_Int32 parameterIndex, const OUString& x )
{
    setParameter( parameterIndex, adLongVarWChar, std::numeric_limits< sal_Int32 >::max(), x );
}

Reference< XConnection > SAL_CALL OPreparedStatement::getConnection(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);

    return static_cast<Reference< XConnection >>(m_pConnection);
}

Reference< XResultSet > SAL_CALL OPreparedStatement::executeQuery(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);

    // first clear the old things
    m_xMetaData.clear();
    disposeResultSet();
    if(m_RecordSet.IsValid())
        m_RecordSet.Close();
    m_RecordSet.clear();

    // then create the new ones
    m_RecordSet.Create();
    OLEVariant aCmd;
    aCmd.setIDispatch(m_Command);
    OLEVariant aCon;
    aCon.setNoArg();
    CHECK_RETURN(m_RecordSet.put_CacheSize(m_nFetchSize))
    CHECK_RETURN(m_RecordSet.put_MaxRecords(m_nMaxRows))
    CHECK_RETURN(m_RecordSet.Open(aCmd,aCon,m_eCursorType,m_eLockType,adOpenUnspecified))
    CHECK_RETURN(m_RecordSet.get_CacheSize(m_nFetchSize))
    CHECK_RETURN(m_RecordSet.get_MaxRecords(m_nMaxRows))
    CHECK_RETURN(m_RecordSet.get_CursorType(m_eCursorType))
    CHECK_RETURN(m_RecordSet.get_LockType(m_eLockType))

    rtl::Reference<OResultSet> pSet = new OResultSet(m_RecordSet,this);
    pSet->construct();
    pSet->setMetaData(getMetaData());
    m_xResultSet = pSet.get();

    return pSet;
}

void SAL_CALL OPreparedStatement::setBoolean( sal_Int32 parameterIndex, sal_Bool x )
{
    setParameter(parameterIndex,adBoolean,sizeof(x),bool(x));
}

void SAL_CALL OPreparedStatement::setByte( sal_Int32 parameterIndex, sal_Int8 x )
{
    setParameter(parameterIndex,adTinyInt,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setDate( sal_Int32 parameterIndex, const Date& x )
{
    setParameter(parameterIndex,adDBDate,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setTime( sal_Int32 parameterIndex, const css::util::Time& x )
{
    setParameter(parameterIndex,adDBTime,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setTimestamp( sal_Int32 parameterIndex, const DateTime& x )
{
    setParameter(parameterIndex,adDBTimeStamp,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setDouble( sal_Int32 parameterIndex, double x )
{
    setParameter(parameterIndex,adDouble,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setFloat( sal_Int32 parameterIndex, float x )
{
    setParameter(parameterIndex,adSingle,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setInt( sal_Int32 parameterIndex, sal_Int32 x )
{
    setParameter(parameterIndex,adInteger,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setLong( sal_Int32 parameterIndex, sal_Int64 x )
{
    setParameter(parameterIndex,adBigInt,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setNull( sal_Int32 parameterIndex, sal_Int32 /*sqlType*/ )
{
    OLEVariant aVal;
    aVal.setNull();
    setParameter(parameterIndex,adEmpty,0,aVal);
}

void SAL_CALL OPreparedStatement::setClob( sal_Int32 /*parameterIndex*/, const Reference< XClob >& /*x*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XRowUpdate::setClob", *this );
}

void SAL_CALL OPreparedStatement::setBlob( sal_Int32 /*parameterIndex*/, const Reference< XBlob >& /*x*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XRowUpdate::setBlob", *this );
}

void SAL_CALL OPreparedStatement::setArray( sal_Int32 /*parameterIndex*/, const Reference< XArray >& /*x*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XRowUpdate::setArray", *this );
}

void SAL_CALL OPreparedStatement::setRef( sal_Int32 /*parameterIndex*/, const Reference< XRef >& /*x*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XRowUpdate::setRef", *this );
}

void SAL_CALL OPreparedStatement::setObjectWithInfo( sal_Int32 parameterIndex, const Any& x, sal_Int32 sqlType, sal_Int32 scale )
{
    switch(sqlType)
    {
        case DataType::DECIMAL:
        case DataType::NUMERIC:
            setString(parameterIndex,::comphelper::getString(x));
            break;
        default:
            ::dbtools::setObjectWithInfo(this,parameterIndex,x,sqlType,scale);
            break;
    }
}

void SAL_CALL OPreparedStatement::setObjectNull( sal_Int32 parameterIndex, sal_Int32 sqlType, const OUString& /*typeName*/ )
{
    setNull(parameterIndex,sqlType);
}

void SAL_CALL OPreparedStatement::setObject( sal_Int32 parameterIndex, const Any& x )
{
    if(!::dbtools::implSetObject(this,parameterIndex,x))
    {
        const OUString sError( m_pConnection->getResources().getResourceStringWithSubstitution(
                STR_UNKNOWN_PARA_TYPE,
                "$position$", OUString::number(parameterIndex)
             ) );
        ::dbtools::throwGenericSQLException(sError,*this);
    }
}

void SAL_CALL OPreparedStatement::setShort( sal_Int32 parameterIndex, sal_Int16 x )
{
    setParameter(parameterIndex,adSmallInt,sizeof(x),x);
}

void SAL_CALL OPreparedStatement::setBytes( sal_Int32 parameterIndex, const Sequence< sal_Int8 >& x )
{
    setParameter(parameterIndex,adVarBinary,sizeof(sal_Int8)*x.getLength(),x);
}

void SAL_CALL OPreparedStatement::setCharacterStream( sal_Int32 /*parameterIndex*/, const Reference< css::io::XInputStream >& /*x*/, sal_Int32 /*length*/ )
{
    ::dbtools::throwFeatureNotImplementedSQLException( "XParameters::setCharacterStream", *this );
}

void SAL_CALL OPreparedStatement::setBinaryStream( sal_Int32 parameterIndex, const Reference< css::io::XInputStream >& x, sal_Int32 length )
{
    if(x.is())
    {
        Sequence< sal_Int8 > aData;
        x->readBytes(aData,length);
        setBytes(parameterIndex,aData);
    }
}

void SAL_CALL OPreparedStatement::clearParameters(  )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    checkDisposed(OStatement_BASE::rBHelper.bDisposed);


    if(m_pParameters)
    {
        sal_Int32 nCount = 0;
        m_pParameters->get_Count(&nCount);
        OLEVariant aVal;
        aVal.setEmpty();
        for(sal_Int32 i=0;i<nCount;++i)
        {
            WpADOParameter aParam;
            m_pParameters->get_Item(OLEVariant(i),&aParam);
            if(aParam)
            {
                CHECK_RETURN(aParam.PutValue(aVal));
            }
        }
    }
}

void OPreparedStatement::replaceParameterNodeName(OSQLParseNode const * _pNode,
                                                  const OUString& _sDefaultName,
                                                  sal_Int32& _rParameterCount)
{
    sal_Int32 nCount = _pNode->count();
    for(sal_Int32 i=0;i < nCount;++i)
    {
        OSQLParseNode* pChildNode = _pNode->getChild(i);
        if(SQL_ISRULE(pChildNode,parameter) && pChildNode->count() == 1)
        {
            OSQLParseNode* pNewNode = new OSQLParseNode(OUString(":") ,SQLNodeType::Punctuation,0);
            pChildNode->replaceAndDelete(pChildNode->getChild(0), pNewNode);
            OUString sParameterName = _sDefaultName + OUString::number(++_rParameterCount);
            pChildNode->append(new OSQLParseNode( sParameterName,SQLNodeType::Name,0));
        }
        else
            replaceParameterNodeName(pChildNode,_sDefaultName,_rParameterCount);

    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <connectivity/dbtools.hxx>
#include <connectivity/dbconversion.hxx>
#include <connectivity/dbcharset.hxx>
#include <SQLStatementHelper.hxx>
#include <unotools/confignode.hxx>
#include <resource/sharedresources.hxx>
#include <strings.hrc>
#include <com/sun/star/sdb/XSingleSelectQueryComposer.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdbc/XDataSource.hpp>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/DriverManager.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbcx/XKeysSupplier.hpp>
#include <com/sun/star/sdbcx/XDataDefinitionSupplier.hpp>
#include <com/sun/star/sdbcx/Privilege.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/sdbc/KeyRule.hpp>
#include <com/sun/star/sdbcx/KeyType.hpp>
#include <TConnection.hxx>
#include <connectivity/sdbcx/VColumn.hxx>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/container/XChild.hpp>

#include <comphelper/types.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <unotools/sharedunocomponent.hxx>
#include <algorithm>
#include <string_view>

namespace dbtools
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::frame;
    using namespace connectivity;
    using namespace comphelper;

OUString createStandardTypePart(const Reference< XPropertySet >& xColProp,const Reference< XConnection>& _xConnection,std::u16string_view _sCreatePattern)
{

    Reference<XDatabaseMetaData> xMetaData = _xConnection->getMetaData();

    ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();

    OUString sTypeName;
    sal_Int32       nDataType   = 0;
    sal_Int32       nPrecision  = 0;
    sal_Int32       nScale      = 0;

    nDataType = nPrecision = nScale = 0;

    xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_TYPENAME))           >>= sTypeName;
    xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_TYPE))               >>= nDataType;
    xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_PRECISION))          >>= nPrecision;
    xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_SCALE))              >>= nScale;

    OUStringBuffer aSql;

    // check if the user enter a specific string to create autoincrement values
    OUString sAutoIncrementValue;
    Reference<XPropertySetInfo> xPropInfo = xColProp->getPropertySetInfo();
    if ( xPropInfo.is() && xPropInfo->hasPropertyByName(rPropMap.getNameByIndex(PROPERTY_ID_AUTOINCREMENTCREATION)) )
        xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_AUTOINCREMENTCREATION)) >>= sAutoIncrementValue;
    // look if we have to use precisions
    bool bUseLiteral = false;
    OUString sPrefix,sPostfix,sCreateParams;
    {
        Reference<XResultSet> xRes = xMetaData->getTypeInfo();
        if(xRes.is())
        {
            Reference<XRow> xRow(xRes,UNO_QUERY);
            while(xRes->next())
            {
                OUString sTypeName2Cmp = xRow->getString(1);
                sal_Int32 nType = xRow->getShort(2);
                sPrefix = xRow->getString (4);
                sPostfix = xRow->getString (5);
                sCreateParams = xRow->getString(6);
                // first identical type will be used if typename is empty
                if ( sTypeName.isEmpty() && nType == nDataType )
                    sTypeName = sTypeName2Cmp;

                if( sTypeName.equalsIgnoreAsciiCase(sTypeName2Cmp) && nType == nDataType && !sCreateParams.isEmpty() && !xRow->wasNull())
                {
                    bUseLiteral = true;
                    break;
                }
            }
        }
    }

    if ( !sAutoIncrementValue.isEmpty() )
    {
        sal_Int32 nIndex = sTypeName.indexOf(sAutoIncrementValue);
        if (nIndex != -1)
            sTypeName = sTypeName.replaceAt(nIndex,sTypeName.getLength() - nIndex, u"");
    }

    if ( (nPrecision > 0 || nScale > 0) && bUseLiteral )
    {
        bool bTimed = (nDataType == DataType::TIME ||
                       nDataType == DataType::TIME_WITH_TIMEZONE ||
                       nDataType == DataType::TIMESTAMP ||
                       nDataType == DataType::TIMESTAMP_WITH_TIMEZONE);

        sal_Int32 nParenPos = (nDataType == DataType::TIME_WITH_TIMEZONE ||
                               nDataType == DataType::TIMESTAMP_WITH_TIMEZONE) ?
                               sTypeName.indexOf(' ') :
                               sTypeName.indexOf('(');

        if ( nParenPos == -1 )
            aSql.append(sTypeName);
        else
            aSql.append(sTypeName.subView(0, nParenPos));
        aSql.append("(");

        if ( nPrecision > 0 && !bTimed )
        {
            aSql.append(nPrecision);
            if ( (nScale > 0) || (!_sCreatePattern.empty() && sCreateParams.indexOf(_sCreatePattern) != -1) )
                aSql.append(",");
        }
        if ( (nScale > 0) || ( !_sCreatePattern.empty() && sCreateParams.indexOf(_sCreatePattern) != -1 ) || bTimed )
            aSql.append(nScale);

        if ( nParenPos == -1 )
            aSql.append(")");
        else
        {
            if ( bTimed )
                aSql.append(")");
            else
                nParenPos = sTypeName.indexOf(')',nParenPos);
            aSql.append(sTypeName.subView(nParenPos));
        }
    }
    else
        aSql.append(sTypeName); // simply add the type name

    OUString aDefault = ::comphelper::getString(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_DEFAULTVALUE)));
    if ( !aDefault.isEmpty() )
    {
        aSql.append(" DEFAULT " + sPrefix + aDefault + sPostfix);
    } // if ( aDefault.getLength() )

    return aSql.makeStringAndClear();
}

OUString createStandardColumnPart(const Reference< XPropertySet >& xColProp,const Reference< XConnection>& _xConnection,ISQLStatementHelper* _pHelper,std::u16string_view _sCreatePattern)
{
    Reference<XDatabaseMetaData> xMetaData = _xConnection->getMetaData();

    ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();

    bool bIsAutoIncrement = false;
    xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT))    >>= bIsAutoIncrement;

    const OUString sQuoteString = xMetaData->getIdentifierQuoteString();
    OUStringBuffer aSql(::dbtools::quoteName(sQuoteString,::comphelper::getString(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_NAME)))));

    // check if the user enter a specific string to create autoincrement values
    OUString sAutoIncrementValue;
    Reference<XPropertySetInfo> xPropInfo = xColProp->getPropertySetInfo();
    if ( xPropInfo.is() && xPropInfo->hasPropertyByName(rPropMap.getNameByIndex(PROPERTY_ID_AUTOINCREMENTCREATION)) )
        xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_AUTOINCREMENTCREATION)) >>= sAutoIncrementValue;

    aSql.append(" " + createStandardTypePart(xColProp, _xConnection, _sCreatePattern));

    if(::comphelper::getINT32(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_ISNULLABLE))) == ColumnValue::NO_NULLS)
        aSql.append(" NOT NULL");

    if ( bIsAutoIncrement && !sAutoIncrementValue.isEmpty())
    {
        aSql.append(" " + sAutoIncrementValue);
    }

    if ( _pHelper )
        _pHelper->addComment(xColProp,aSql);

    return aSql.makeStringAndClear();
}


OUString createStandardCreateStatement(const Reference< XPropertySet >& descriptor,const Reference< XConnection>& _xConnection,ISQLStatementHelper* _pHelper,std::u16string_view _sCreatePattern)
{
    OUStringBuffer aSql("CREATE TABLE ");
    OUString sCatalog,sSchema,sTable,sComposedName;

    Reference<XDatabaseMetaData> xMetaData = _xConnection->getMetaData();
    ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();

    descriptor->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_CATALOGNAME))  >>= sCatalog;
    descriptor->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_SCHEMANAME))   >>= sSchema;
    descriptor->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_NAME))         >>= sTable;

    sComposedName = ::dbtools::composeTableName( xMetaData, sCatalog, sSchema, sTable, true, ::dbtools::EComposeRule::InTableDefinitions );
    if ( sComposedName.isEmpty() )
        ::dbtools::throwFunctionSequenceException(_xConnection);

    aSql.append(sComposedName + " (");

    // columns
    Reference<XColumnsSupplier> xColumnSup(descriptor,UNO_QUERY);
    Reference<XIndexAccess> xColumns(xColumnSup->getColumns(),UNO_QUERY);
    // check if there are columns
    if(!xColumns.is() || !xColumns->getCount())
        ::dbtools::throwFunctionSequenceException(_xConnection);

    Reference< XPropertySet > xColProp;

    sal_Int32 nCount = xColumns->getCount();
    for(sal_Int32 i=0;i<nCount;++i)
    {
        if ( (xColumns->getByIndex(i) >>= xColProp) && xColProp.is() )
        {
            aSql.append(
                createStandardColumnPart(xColProp,_xConnection,_pHelper,_sCreatePattern)
                + ",");
        }
    }
    return aSql.makeStringAndClear();
}
namespace
{
    OUString generateColumnNames(const Reference<XIndexAccess>& _xColumns,const Reference<XDatabaseMetaData>& _xMetaData)
    {
        ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();

        const OUString sQuote(_xMetaData->getIdentifierQuoteString());
        OUStringBuffer sSql( " (" );
        Reference< XPropertySet > xColProp;

        sal_Int32 nColCount  = _xColumns->getCount();
        for(sal_Int32 i=0;i<nColCount;++i)
        {
            if ( (_xColumns->getByIndex(i) >>= xColProp) && xColProp.is() )
                sSql.append( ::dbtools::quoteName(sQuote,::comphelper::getString(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_NAME)))) +
                            ",");
        }

        if ( nColCount )
            sSql[sSql.getLength()-1] = ')';
        return sSql.makeStringAndClear();
    }
}

OUString createStandardKeyStatement(const Reference< XPropertySet >& descriptor,const Reference< XConnection>& _xConnection)
{
    Reference<XDatabaseMetaData> xMetaData = _xConnection->getMetaData();
    ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();

    OUStringBuffer aSql;
    // keys
    Reference<XKeysSupplier> xKeySup(descriptor,UNO_QUERY);
    Reference<XIndexAccess> xKeys = xKeySup->getKeys();
    if ( xKeys.is() )
    {
        Reference< XPropertySet > xColProp;
        Reference<XIndexAccess> xColumns;
        Reference<XColumnsSupplier> xColumnSup;
        OUString sCatalog,sSchema,sTable,sComposedName;
        bool bPKey = false;
        for(sal_Int32 i=0;i<xKeys->getCount();++i)
        {
            if ( (xKeys->getByIndex(i) >>= xColProp) && xColProp.is() )
            {

                sal_Int32 nKeyType      = ::comphelper::getINT32(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_TYPE)));

                if ( nKeyType == KeyType::PRIMARY )
                {
                    if(bPKey)
                        ::dbtools::throwFunctionSequenceException(_xConnection);

                    bPKey = true;
                    xColumnSup.set(xColProp,UNO_QUERY);
                    xColumns.set(xColumnSup->getColumns(),UNO_QUERY);
                    if(!xColumns.is() || !xColumns->getCount())
                        ::dbtools::throwFunctionSequenceException(_xConnection);

                    aSql.append(" PRIMARY KEY " + generateColumnNames(xColumns,xMetaData));
                }
                else if(nKeyType == KeyType::UNIQUE)
                {
                    xColumnSup.set(xColProp,UNO_QUERY);
                    xColumns.set(xColumnSup->getColumns(),UNO_QUERY);
                    if(!xColumns.is() || !xColumns->getCount())
                        ::dbtools::throwFunctionSequenceException(_xConnection);

                    aSql.append(" UNIQUE " + generateColumnNames(xColumns,xMetaData));
                }
                else if(nKeyType == KeyType::FOREIGN)
                {
                    sal_Int32 nDeleteRule   = getINT32(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_DELETERULE)));

                    xColumnSup.set(xColProp,UNO_QUERY);
                    xColumns.set(xColumnSup->getColumns(),UNO_QUERY);
                    if(!xColumns.is() || !xColumns->getCount())
                        ::dbtools::throwFunctionSequenceException(_xConnection);

                    aSql.append(" FOREIGN KEY ");
                    OUString sRefTable = getString(xColProp->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_REFERENCEDTABLE)));
                    ::dbtools::qualifiedNameComponents(xMetaData,
                                                        sRefTable,
                                                        sCatalog,
                                                        sSchema,
                                                        sTable,
                                                        ::dbtools::EComposeRule::InDataManipulation);
                    sComposedName = ::dbtools::composeTableName( xMetaData, sCatalog, sSchema, sTable, true, ::dbtools::EComposeRule::InTableDefinitions );


                    if ( sComposedName.isEmpty() )
                        ::dbtools::throwFunctionSequenceException(_xConnection);

                    aSql.append(generateColumnNames(xColumns,xMetaData));

                    switch(nDeleteRule)
                    {
                        case KeyRule::CASCADE:
                            aSql.append(" ON DELETE CASCADE ");
                            break;
                        case KeyRule::RESTRICT:
                            aSql.append(" ON DELETE RESTRICT ");
                            break;
                        case KeyRule::SET_NULL:
                            aSql.append(" ON DELETE SET NULL ");
                            break;
                        case KeyRule::SET_DEFAULT:
                            aSql.append(" ON DELETE SET DEFAULT ");
                            break;
                        default:
                            ;
                    }
                }
            }
        }
    }

    if ( !aSql.isEmpty() )
    {
        if ( aSql[aSql.getLength() - 1] == ',' )
            aSql[aSql.getLength() - 1] = ')';
        else
            aSql.append(")");
    }

    return aSql.makeStringAndClear();

}

OUString createSqlCreateTableStatement(  const Reference< XPropertySet >& descriptor,
                                         const Reference< XConnection>& _xConnection)
{
    OUString aSql = ::dbtools::createStandardCreateStatement(descriptor,_xConnection,nullptr,{});
    const OUString sKeyStmt = ::dbtools::createStandardKeyStatement(descriptor,_xConnection);
    if ( !sKeyStmt.isEmpty() )
        aSql += sKeyStmt;
    else
    {
        if ( aSql.endsWith(",") )
            aSql = aSql.replaceAt(aSql.getLength()-1, 1, u")");
        else
            aSql += ")";
    }
    return aSql;
}
namespace
{
    Reference<XPropertySet> lcl_createSDBCXColumn(const Reference<XNameAccess>& _xPrimaryKeyColumns,
                                          const Reference<XConnection>& _xConnection,
                                          const Any& _aCatalog,
                                          const OUString& _aSchema,
                                          const OUString& _aTable,
                                          const OUString& _rQueryName,
                                          const OUString& _rName,
                                          bool _bCase,
                                          bool _bQueryForInfo,
                                          bool _bIsAutoIncrement,
                                          bool _bIsCurrency,
                                          sal_Int32 _nDataType)
    {
        Reference<XDatabaseMetaData> xMetaData = _xConnection->getMetaData();
        Reference< XResultSet > xResult = xMetaData->getColumns(_aCatalog, _aSchema, _aTable, _rQueryName);
        OUString sCatalog;
        _aCatalog >>= sCatalog;

        if ( !xResult.is() )
            return nullptr;

        rtl::Reference<connectivity::sdbcx::OColumn> xProp;
        UStringMixEqual aMixCompare(_bCase);
        Reference< XRow > xRow(xResult,UNO_QUERY);
        while( xResult->next() )
        {
            if ( aMixCompare(xRow->getString(4),_rName) )
            {
                sal_Int32       nField5 = xRow->getInt(5);
                OUString aField6 = xRow->getString(6);
                sal_Int32       nField7 = xRow->getInt(7)
                            ,   nField9 = xRow->getInt(9)
                            ,   nField11= xRow->getInt(11);
                OUString sField12 = xRow->getString(12),
                                sField13 = xRow->getString(13);
                ::comphelper::disposeComponent(xRow);

                bool bAutoIncrement = _bIsAutoIncrement
                        ,bIsCurrency    = _bIsCurrency;
                if ( _bQueryForInfo )
                {
                    const OUString sQuote = xMetaData->getIdentifierQuoteString();
                    OUString sQuotedName  = ::dbtools::quoteName(sQuote,_rName);
                    OUString sComposedName = composeTableNameForSelect(_xConnection, getString( _aCatalog ), _aSchema, _aTable );

                    ColumnInformationMap aInfo((UStringMixLess(_bCase)));
                    collectColumnInformation(_xConnection,sComposedName,sQuotedName,aInfo);
                    ColumnInformationMap::const_iterator aIter = aInfo.begin();
                    if ( aIter != aInfo.end() )
                    {
                        bAutoIncrement  = aIter->second.first.first;
                        bIsCurrency     = aIter->second.first.second;
                        if ( DataType::OTHER == nField5 )
                            nField5     = aIter->second.second;
                    }
                }
                else if ( DataType::OTHER == nField5 )
                    nField5 = _nDataType;

                if ( nField11 != ColumnValue::NO_NULLS )
                {
                    try
                    {
                        if ( _xPrimaryKeyColumns.is() )
                        {
                            if ( _xPrimaryKeyColumns->hasByName(_rName) )
                                nField11 = ColumnValue::NO_NULLS;

                        }
                        else
                        {
                            Reference< XResultSet > xPKeys = xMetaData->getPrimaryKeys( _aCatalog, _aSchema, _aTable );
                            Reference< XRow > xPKeyRow( xPKeys, UNO_QUERY_THROW );
                            while( xPKeys->next() ) // there can be only one primary key
                            {
                                OUString sKeyColumn = xPKeyRow->getString(4);
                                if ( aMixCompare(_rName,sKeyColumn) )
                                {
                                    nField11 = ColumnValue::NO_NULLS;
                                    break;
                                }
                            }
                        }
                    }
                    catch(SQLException&)
                    {
                        TOOLS_WARN_EXCEPTION( "connectivity.commontools", "lcl_createSDBCXColumn" );
                    }
                }

                xProp = new connectivity::sdbcx::OColumn(_rName,
                                            aField6,
                                            sField13,
                                            sField12,
                                            nField11,
                                            nField7,
                                            nField9,
                                            nField5,
                                            bAutoIncrement,
                                            false,
                                            bIsCurrency,
                                            _bCase,
                                            sCatalog,
                                            _aSchema,
                                            _aTable);

                break;
            }
        }

        return xProp;
    }

    Reference< XModel> lcl_getXModel(const Reference< XInterface>& _xIface)
    {
        Reference< XInterface > xParent = _xIface;
        Reference< XModel > xModel(xParent,UNO_QUERY);
        while( xParent.is() && !xModel.is() )
        {
            Reference<XChild> xChild(xParent,UNO_QUERY);
            xParent.set(xChild.is() ? xChild->getParent() : Reference< XInterface >(),UNO_QUERY);
            xModel.set(xParent,UNO_QUERY);
        }
        return xModel;
    }
}

Reference<XPropertySet> createSDBCXColumn(const Reference<XPropertySet>& _xTable,
                                          const Reference<XConnection>& _xConnection,
                                          const OUString& _rName,
                                          bool _bCase,
                                          bool _bQueryForInfo,
                                          bool _bIsAutoIncrement,
                                          bool _bIsCurrency,
                                          sal_Int32 _nDataType)
{
    Reference<XPropertySet> xProp;
    OSL_ENSURE(_xTable.is(),"Table is NULL!");
    if ( !_xTable.is() )
        return xProp;

    ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();
    Any aCatalog = _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_CATALOGNAME));
    OUString sCatalog;
    aCatalog >>= sCatalog;

    OUString aSchema, aTable;
    _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_SCHEMANAME))  >>= aSchema;
    _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_NAME))        >>= aTable;

    Reference<XNameAccess> xPrimaryKeyColumns = getPrimaryKeyColumns_throw(_xTable);

    xProp = lcl_createSDBCXColumn(xPrimaryKeyColumns,_xConnection,aCatalog, aSchema, aTable, _rName,_rName,_bCase,_bQueryForInfo,_bIsAutoIncrement,_bIsCurrency,_nDataType);
    if ( !xProp.is() )
    {
        xProp = lcl_createSDBCXColumn(xPrimaryKeyColumns,_xConnection,aCatalog, aSchema, aTable, u"%"_ustr,_rName,_bCase,_bQueryForInfo,_bIsAutoIncrement,_bIsCurrency,_nDataType);
        if ( !xProp.is() )
            xProp = new connectivity::sdbcx::OColumn(_rName,
                                                OUString(),OUString(),OUString(),
                                                ColumnValue::NULLABLE_UNKNOWN,
                                                0,
                                                0,
                                                DataType::VARCHAR,
                                                _bIsAutoIncrement,
                                                false,
                                                _bIsCurrency,
                                                _bCase,
                                                sCatalog,
                                                aSchema,
                                                aTable);

    }

    return xProp;
}


bool getBooleanDataSourceSetting( const Reference< XConnection >& _rxConnection, const char* _pAsciiSettingName )
{
    return getBooleanDataSourceSetting(_rxConnection, OUString::createFromAscii( _pAsciiSettingName ));
}

bool getBooleanDataSourceSetting( const Reference< XConnection >& _rxConnection, const OUString & rSettingName )
{
    bool bValue( false );
    try
    {
        Reference< XPropertySet> xDataSourceProperties( findDataSource( _rxConnection ), UNO_QUERY );
        OSL_ENSURE( xDataSourceProperties.is(), "::dbtools::getBooleanDataSourceSetting: somebody is using this with a non-SDB-level connection!" );
        if ( xDataSourceProperties.is() )
        {
            Reference< XPropertySet > xSettings(
                xDataSourceProperties->getPropertyValue(u"Settings"_ustr),
                UNO_QUERY_THROW
            );
            OSL_VERIFY( xSettings->getPropertyValue( rSettingName ) >>= bValue );
        }
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
    }
    return bValue;
}

bool getDataSourceSetting( const Reference< XInterface >& _xChild, const OUString& _sAsciiSettingsName,
    Any& /* [out] */ _rSettingsValue )
{
    bool bIsPresent = false;
    try
    {
        const Reference< XPropertySet> xDataSourceProperties( findDataSource( _xChild ), UNO_QUERY );
        if ( !xDataSourceProperties.is() )
            return false;

        const Reference< XPropertySet > xSettings(
                xDataSourceProperties->getPropertyValue(u"Settings"_ustr),
                UNO_QUERY_THROW
            );

        _rSettingsValue = xSettings->getPropertyValue( _sAsciiSettingsName );
        bIsPresent = true;
    }
    catch( const Exception& )
    {
        bIsPresent = false;
    }
    return bIsPresent;
}

bool getDataSourceSetting( const Reference< XInterface >& _rxDataSource, const char* _pAsciiSettingsName,
    Any& /* [out] */ _rSettingsValue )
{
    OUString sAsciiSettingsName = OUString::createFromAscii(_pAsciiSettingsName);
    return getDataSourceSetting( _rxDataSource, sAsciiSettingsName,_rSettingsValue );
}

bool isDataSourcePropertyEnabled(const Reference<XInterface>& _xProp, const OUString& _sProperty, bool _bDefault)
{
    bool bEnabled = _bDefault;
    try
    {
        Reference< XPropertySet> xProp(findDataSource(_xProp),UNO_QUERY);
        if ( xProp.is() )
        {
            Sequence< PropertyValue > aInfo;
            xProp->getPropertyValue(u"Info"_ustr) >>= aInfo;
            const PropertyValue* pValue =std::find_if(std::cbegin(aInfo),
                                                std::cend(aInfo),
                                                [&_sProperty](const PropertyValue& lhs)
                                                { return lhs.Name == _sProperty; });
            if ( pValue != std::cend(aInfo) )
                pValue->Value >>= bEnabled;
        }
    }
    catch(SQLException&)
    {
        DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
    }
    return bEnabled;
}

Reference< XTablesSupplier> getDataDefinitionByURLAndConnection(
            const OUString& _rsUrl,
            const Reference< XConnection>& _xConnection,
            const Reference< XComponentContext >& _rxContext)
{
    Reference< XTablesSupplier> xTablesSup;
    try
    {
        Reference< XDriverManager2 > xManager = DriverManager::create( _rxContext );
        Reference< XDataDefinitionSupplier > xSupp( xManager->getDriverByURL( _rsUrl ), UNO_QUERY );

        if ( xSupp.is() )
        {
            xTablesSup = xSupp->getDataDefinitionByConnection( _xConnection );
            OSL_ENSURE(xTablesSup.is(),"No table supplier!");
        }
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
    }
    return xTablesSup;
}


sal_Int32 getTablePrivileges(const Reference< XDatabaseMetaData>& _xMetaData,
                             const OUString& _sCatalog,
                             const OUString& _sSchema,
                             const OUString& _sTable)
{
    OSL_ENSURE(_xMetaData.is(),"Invalid metadata!");
    sal_Int32 nPrivileges = 0;
    try
    {
        Any aVal;
        if(!_sCatalog.isEmpty())
            aVal <<= _sCatalog;
        Reference< XResultSet > xPrivileges = _xMetaData->getTablePrivileges(aVal, _sSchema, _sTable);
        Reference< XRow > xCurrentRow(xPrivileges, UNO_QUERY);

        const OUString sUserWorkingFor = _xMetaData->getUserName();
        static const char sSELECT[] = "SELECT";
        static const char sINSERT[] = "INSERT";
        static const char sUPDATE[] = "UPDATE";
        static const char sDELETE[] = "DELETE";
        static const char sREAD[] = "READ";
        static const char sCREATE[] = "CREATE";
        static const char sALTER[] = "ALTER";
        static const char sREFERENCE[] = "REFERENCE";
        static const char sDROP[] = "DROP";

        if ( xCurrentRow.is() )
        {
            // after creation the set is positioned before the first record, per definition
            OUString sPrivilege, sGrantee;
            while ( xPrivileges->next() )
            {
                sGrantee    = xCurrentRow->getString(5);
                sPrivilege  = xCurrentRow->getString(6);

                if (!sUserWorkingFor.equalsIgnoreAsciiCase(sGrantee))
                    continue;

                if (sPrivilege.equalsIgnoreAsciiCase(sSELECT))
                    nPrivileges |= Privilege::SELECT;
                else if (sPrivilege.equalsIgnoreAsciiCase(sINSERT))
                    nPrivileges |= Privilege::INSERT;
                else if (sPrivilege.equalsIgnoreAsciiCase(sUPDATE))
                    nPrivileges |= Privilege::UPDATE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sDELETE))
                    nPrivileges |= Privilege::DELETE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sREAD))
                    nPrivileges |= Privilege::READ;
                else if (sPrivilege.equalsIgnoreAsciiCase(sCREATE))
                    nPrivileges |= Privilege::CREATE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sALTER))
                    nPrivileges |= Privilege::ALTER;
                else if (sPrivilege.equalsIgnoreAsciiCase(sREFERENCE))
                    nPrivileges |= Privilege::REFERENCE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sDROP))
                    nPrivileges |= Privilege::DROP;
            }
        }
        disposeComponent(xPrivileges);

        // Some drivers put a table privilege as soon as any column has the privilege,
        // some drivers only if all columns have the privilege.
        // To unify the situation, collect column privileges here, too.
        Reference< XResultSet > xColumnPrivileges = _xMetaData->getColumnPrivileges(aVal, _sSchema, _sTable, u"%"_ustr);
        Reference< XRow > xColumnCurrentRow(xColumnPrivileges, UNO_QUERY);
        if ( xColumnCurrentRow.is() )
        {
            // after creation the set is positioned before the first record, per definition
            OUString sPrivilege, sGrantee;
            while ( xColumnPrivileges->next() )
            {
                sGrantee    = xColumnCurrentRow->getString(6);
                sPrivilege  = xColumnCurrentRow->getString(7);

                if (!sUserWorkingFor.equalsIgnoreAsciiCase(sGrantee))
                    continue;

                if (sPrivilege.equalsIgnoreAsciiCase(sSELECT))
                    nPrivileges |= Privilege::SELECT;
                else if (sPrivilege.equalsIgnoreAsciiCase(sINSERT))
                    nPrivileges |= Privilege::INSERT;
                else if (sPrivilege.equalsIgnoreAsciiCase(sUPDATE))
                    nPrivileges |= Privilege::UPDATE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sDELETE))
                    nPrivileges |= Privilege::DELETE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sREAD))
                    nPrivileges |= Privilege::READ;
                else if (sPrivilege.equalsIgnoreAsciiCase(sCREATE))
                    nPrivileges |= Privilege::CREATE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sALTER))
                    nPrivileges |= Privilege::ALTER;
                else if (sPrivilege.equalsIgnoreAsciiCase(sREFERENCE))
                    nPrivileges |= Privilege::REFERENCE;
                else if (sPrivilege.equalsIgnoreAsciiCase(sDROP))
                    nPrivileges |= Privilege::DROP;
            }
        }
        disposeComponent(xColumnPrivileges);
    }
    catch(const SQLException& e)
    {
        // some drivers don't support any privileges so we assume that we are allowed to do all we want :-)
        if(e.SQLState == "IM001")
            nPrivileges |=  Privilege::DROP         |
                            Privilege::REFERENCE    |
                            Privilege::ALTER        |
                            Privilege::CREATE       |
                            Privilege::READ         |
                            Privilege::DELETE       |
                            Privilege::UPDATE       |
                            Privilege::INSERT       |
                            Privilege::SELECT;
        else
            OSL_FAIL("Could not collect the privileges !");
    }
    return nPrivileges;
}

// we need some more information about the column
void collectColumnInformation(const Reference< XConnection>& _xConnection,
                              std::u16string_view _sComposedName,
                              std::u16string_view _rName,
                              ColumnInformationMap& _rInfo)
{
    OUString sSelect = OUString::Concat("SELECT ") + _rName +
        " FROM " + _sComposedName +
        " WHERE 0 = 1";

    try
    {
        ::utl::SharedUNOComponent< XStatement > xStmt( _xConnection->createStatement() );
        Reference< XPropertySet > xStatementProps( xStmt, UNO_QUERY_THROW );
        xStatementProps->setPropertyValue( OMetaConnection::getPropMap().getNameByIndex( PROPERTY_ID_ESCAPEPROCESSING ), Any( false ) );
        Reference< XResultSet > xResult( xStmt->executeQuery( sSelect ), UNO_SET_THROW );
        Reference< XResultSetMetaDataSupplier > xSuppMeta( xResult, UNO_QUERY_THROW );
        Reference< XResultSetMetaData > xMeta( xSuppMeta->getMetaData(), UNO_SET_THROW );

        sal_Int32 nCount = xMeta->getColumnCount();
        OSL_ENSURE( nCount != 0, "::dbtools::collectColumnInformation: result set has empty (column-less) meta data!" );
        for (sal_Int32 i=1; i <= nCount ; ++i)
        {
            _rInfo.emplace( xMeta->getColumnName(i),
                ColumnInformation(TBoolPair(xMeta->isAutoIncrement(i),xMeta->isCurrency(i)),xMeta->getColumnType(i)));
        }
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
    }
}


bool isEmbeddedInDatabase( const Reference< XInterface >& _rxComponent, Reference< XConnection >& _rxActualConnection )
{
    bool bIsEmbedded = false;
    try
    {
        Reference< XModel > xModel = lcl_getXModel( _rxComponent );

        if ( xModel.is() )
        {
            for (auto& arg : xModel->getArgs())
            {
                if (arg.Name == "ComponentData")
                {
                    Sequence<PropertyValue> aDocumentContext;
                    arg.Value >>= aDocumentContext;
                    for (auto& item : aDocumentContext)
                    {
                        if (item.Name == "ActiveConnection" && (item.Value >>= _rxActualConnection))
                        {
                            bIsEmbedded = true;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    catch(Exception&)
    {
        // not interested in
    }
    return bIsEmbedded;
}

namespace
{
    OUString lcl_getEncodingName( rtl_TextEncoding _eEncoding )
    {
        OUString sEncodingName;

        OCharsetMap aCharsets;
        OCharsetMap::CharsetIterator aEncodingPos = aCharsets.find( _eEncoding );
        OSL_ENSURE( aEncodingPos != aCharsets.end(), "lcl_getEncodingName: *which* encoding?" );
        if ( aEncodingPos != aCharsets.end() )
            sEncodingName = (*aEncodingPos).getIanaName();

        return sEncodingName;
    }
}


sal_Int32 DBTypeConversion::convertUnicodeString( const OUString& _rSource, OString& _rDest, rtl_TextEncoding _eEncoding )
{
    if ( !rtl_convertUStringToString( &_rDest.pData, _rSource.getStr(), _rSource.getLength(),
            _eEncoding,
            RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR |
            RTL_UNICODETOTEXT_FLAGS_UNDEFINED_REPLACE |
            RTL_UNICODETOTEXT_FLAGS_PRIVATE_MAPTO0 )
        )
    {
        SharedResources aResources;
        OUString sMessage = aResources.getResourceStringWithSubstitution( STR_CANNOT_CONVERT_STRING,
            "$string$", _rSource,
            "$charset$",  lcl_getEncodingName( _eEncoding )
        );

        throw SQLException(
            sMessage,
            nullptr,
            u"22018"_ustr,
            22018,
            Any()
        );
    }

    return _rDest.getLength();
}


sal_Int32 DBTypeConversion::convertUnicodeStringToLength( const OUString& _rSource, OString&  _rDest,
   sal_Int32 _nMaxLen, rtl_TextEncoding _eEncoding )
{
    sal_Int32 nLen = convertUnicodeString( _rSource, _rDest, _eEncoding );
    if ( nLen > _nMaxLen )
    {
        SharedResources aResources;
        OUString sMessage = aResources.getResourceStringWithSubstitution( STR_STRING_LENGTH_EXCEEDED,
            "$string$", _rSource,
            "$maxlen$", OUString::number( _nMaxLen ),
            "$charset$", lcl_getEncodingName( _eEncoding )
        );

        throw SQLException(
            sMessage,
            nullptr,
            u"22001"_ustr,
            22001,
            Any()
        );
    }

    return nLen;
}

OUString getDefaultReportEngineServiceName(const Reference< XComponentContext >& _rxORB)
{
    ::utl::OConfigurationTreeRoot aReportEngines = ::utl::OConfigurationTreeRoot::createWithComponentContext(
        _rxORB, u"org.openoffice.Office.DataAccess/ReportEngines"_ustr, -1, ::utl::OConfigurationTreeRoot::CM_READONLY);

    if ( aReportEngines.isValid() )
    {
        OUString sDefaultReportEngineName;
        aReportEngines.getNodeValue(u"DefaultReportEngine"_ustr) >>= sDefaultReportEngineName;
        if ( !sDefaultReportEngineName.isEmpty() )
        {
            ::utl::OConfigurationNode aReportEngineNames = aReportEngines.openNode(u"ReportEngineNames"_ustr);
            if ( aReportEngineNames.isValid() )
            {
                ::utl::OConfigurationNode aReportEngine = aReportEngineNames.openNode(sDefaultReportEngineName);
                if ( aReportEngine.isValid() )
                {
                    OUString sRet;
                    aReportEngine.getNodeValue(u"ServiceName"_ustr) >>= sRet;
                    return sRet;
                }
            }
        }
        else
            return u"org.libreoffice.report.pentaho.SOReportJobFactory"_ustr;
    }
    else
        return u"org.libreoffice.report.pentaho.SOReportJobFactory"_ustr;
    return OUString();
}

bool isAggregateColumn(const Reference< XSingleSelectQueryComposer > &_xParser, const Reference< XPropertySet > &_xField)
{
    OUString sName;
    _xField->getPropertyValue(u"Name"_ustr) >>= sName;
    Reference< XColumnsSupplier > xColumnsSupplier(_xParser, UNO_QUERY);
    Reference< css::container::XNameAccess >  xCols;
    if (xColumnsSupplier.is())
        xCols = xColumnsSupplier->getColumns();

    return isAggregateColumn(xCols, sName);
}

bool isAggregateColumn(const Reference< XNameAccess > &_xColumns, const OUString &_sName)
{
    if ( _xColumns.is() && _xColumns->hasByName(_sName) )
    {
        Reference<XPropertySet> xProp(_xColumns->getByName(_sName),UNO_QUERY);
        assert(xProp.is());
        return isAggregateColumn( xProp );
    }
    return false;
}

bool isAggregateColumn( const Reference< XPropertySet > &_xColumn )
{
    bool bAgg(false);

    static constexpr OUString sAgg = u"AggregateFunction"_ustr;
    if ( _xColumn->getPropertySetInfo()->hasPropertyByName(sAgg) )
        _xColumn->getPropertyValue(sAgg) >>= bAgg;

    return bAgg;
}


}   // namespace dbtools


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

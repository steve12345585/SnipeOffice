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

#include <string_view>

#include <ado/AResultSetMetaData.hxx>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <ado/Awrapado.hxx>
#include <connectivity/dbexception.hxx>

using namespace connectivity;
using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;

OResultSetMetaData::OResultSetMetaData( ADORecordset* _pRecordSet)
                    :   m_pRecordSet(_pRecordSet),
                        m_nColCount(-1)
{
    if ( m_pRecordSet )
        m_pRecordSet->AddRef();
}

OResultSetMetaData::~OResultSetMetaData()
{
    if ( m_pRecordSet )
        m_pRecordSet->Release();
}

sal_Int32 SAL_CALL OResultSetMetaData::getColumnDisplaySize( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid() && aField.GetActualSize() != -1)
        return aField.GetActualSize();
    return 0;
}


sal_Int32 SAL_CALL OResultSetMetaData::getColumnType( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    return ADOS::MapADOType2Jdbc(aField.GetADOType());
}


sal_Int32 SAL_CALL OResultSetMetaData::getColumnCount(  )
{
    if(m_nColCount != -1 )
        return m_nColCount;

    if ( !m_pRecordSet )
        return 0;

    WpOLEAppendCollection<ADOFields, WpADOField> pFields;
    m_pRecordSet->get_Fields(&pFields);
    m_nColCount = pFields.GetItemCount();
    return m_nColCount;
}


sal_Bool SAL_CALL OResultSetMetaData::isCaseSensitive( sal_Int32 column )
{
    bool bRet = false;
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if ( aField.IsValid() )
    {
        WpADOProperties aProps( aField.get_Properties() );
        if ( aProps.IsValid() )
            bRet = OTools::getValue(aProps, std::u16string_view(u"ISCASESENSITIVE")).getBool();
    }
    return bRet;
}


OUString SAL_CALL OResultSetMetaData::getSchemaName( sal_Int32 /*column*/ )
{
    return OUString();
}


OUString SAL_CALL OResultSetMetaData::getColumnName( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
        return aField.GetName();

    return OUString();
}

OUString SAL_CALL OResultSetMetaData::getTableName( sal_Int32 column )
{
    OUString sTableName;

    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if ( aField.IsValid() )
    {
        WpADOProperties aProps( aField.get_Properties() );
        if ( aProps.IsValid() )
            sTableName
                = OTools::getValue(aProps, std::u16string_view(u"BASETABLENAME")).getString();
    }
    return sTableName;
}

OUString SAL_CALL OResultSetMetaData::getCatalogName( sal_Int32 /*column*/ )
{
    return OUString();
}

OUString SAL_CALL OResultSetMetaData::getColumnTypeName( sal_Int32 /*column*/ )
{
    return OUString();
}

OUString SAL_CALL OResultSetMetaData::getColumnLabel( sal_Int32 column )
{
    return getColumnName(column);
}

OUString SAL_CALL OResultSetMetaData::getColumnServiceName( sal_Int32 /*column*/ )
{
    return OUString();
}


sal_Bool SAL_CALL OResultSetMetaData::isCurrency( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
    {
        return ((aField.GetAttributes() & adFldFixed) == adFldFixed) && (aField.GetADOType() == adCurrency);
    }
    return false;
}


sal_Bool SAL_CALL OResultSetMetaData::isAutoIncrement( sal_Int32 column )
{
    bool bRet = false;
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if ( aField.IsValid() )
    {
        WpADOProperties aProps( aField.get_Properties() );
        if ( aProps.IsValid() )
        {
            bRet = OTools::getValue(aProps, std::u16string_view(u"ISAUTOINCREMENT")).getBool();
        }
    }
    return bRet;
}


sal_Bool SAL_CALL OResultSetMetaData::isSigned( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
    {
        DataTypeEnum eType = aField.GetADOType();
        return !(eType == adUnsignedBigInt || eType == adUnsignedInt || eType == adUnsignedSmallInt || eType == adUnsignedTinyInt);
    }
    return false;
}

sal_Int32 SAL_CALL OResultSetMetaData::getPrecision( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
        return aField.GetPrecision();
    return 0;
}

sal_Int32 SAL_CALL OResultSetMetaData::getScale( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
        return aField.GetNumericScale();
    return 0;
}


sal_Int32 SAL_CALL OResultSetMetaData::isNullable( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
    {
        return sal_Int32((aField.GetAttributes() & adFldIsNullable) == adFldIsNullable);
    }
    return sal_Int32(false);
}


sal_Bool SAL_CALL OResultSetMetaData::isSearchable( sal_Int32 /*column*/ )
{
    return true;
}


sal_Bool SAL_CALL OResultSetMetaData::isReadOnly( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
    {
        //  return (aField.GetStatus() & adFieldReadOnly) == adFieldReadOnly;
    }
    return false;
}


sal_Bool SAL_CALL OResultSetMetaData::isDefinitelyWritable( sal_Int32 column )
{
    WpADOField aField = ADOS::getField(m_pRecordSet,column);
    if(aField.IsValid())
    {
        return (aField.GetAttributes() & adFldUpdatable) == adFldUpdatable;
    }
    return false;
;
}

sal_Bool SAL_CALL OResultSetMetaData::isWritable( sal_Int32 column )
{
    return isDefinitelyWritable(column);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

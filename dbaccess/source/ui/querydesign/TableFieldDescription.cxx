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

#include <TableFieldDescription.hxx>

#include <osl/diagnose.h>
#include <com/sun/star/sdbc/DataType.hpp>
#include <comphelper/namedvaluecollection.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace comphelper;
using namespace dbaui;

OTableFieldDesc::OTableFieldDesc()
    :m_pTabWindow(nullptr)
    ,m_eDataType(1000)
    ,m_eFunctionType( FKT_NONE )
    ,m_eFieldType(TAB_NORMAL_FIELD)
    ,m_eOrderDir( ORDER_NONE )
    ,m_nIndex(0)
    ,m_nColWidth(0)
    ,m_nColumnId(sal_uInt16(-1))
    ,m_bGroupBy(false)
    ,m_bVisible(false)
{
}

OTableFieldDesc::OTableFieldDesc(const OTableFieldDesc& rRS)
    : ::salhelper::SimpleReferenceObject()
    , m_pTabWindow(nullptr)
{
    *this = rRS;
}

OTableFieldDesc::OTableFieldDesc(const OUString& rT, const OUString& rF )
    :m_pTabWindow(nullptr)
    ,m_eDataType(1000)
    ,m_eFunctionType( FKT_NONE )
    ,m_eFieldType(TAB_NORMAL_FIELD)
    ,m_eOrderDir( ORDER_NONE )
    ,m_nIndex(0)
    ,m_nColWidth(0)
    ,m_nColumnId(sal_uInt16(-1))
    ,m_bGroupBy(false)
    ,m_bVisible(false)
{
    SetField( rF ); SetTable( rT );
}

OTableFieldDesc::~OTableFieldDesc()
{
}

OTableFieldDesc& OTableFieldDesc::operator=( const OTableFieldDesc& rRS )
{
    if (&rRS == this)
        return *this;

    m_aCriteria = rRS.GetCriteria();
    m_aTableName = rRS.GetTable();
    m_aAliasName = rRS.GetAlias();      // table range
    m_aFieldName = rRS.GetField();      // column
    m_aFieldAlias = rRS.GetFieldAlias();    // column alias
    m_aFunctionName = rRS.GetFunction();
    m_pTabWindow = rRS.GetTabWindow();
    m_eDataType = rRS.GetDataType();
    m_eFunctionType = rRS.GetFunctionType();
    m_eFieldType = rRS.GetFieldType();
    m_eOrderDir = rRS.GetOrderDir();
    m_nIndex = rRS.GetFieldIndex();
    m_nColWidth = rRS.GetColWidth();
    m_nColumnId = rRS.m_nColumnId;
    m_bGroupBy = rRS.IsGroupBy();
    m_bVisible = rRS.IsVisible();

    return *this;
}

void OTableFieldDesc::SetCriteria( sal_uInt16 nIdx, const OUString& rCrit)
{
    if (nIdx < m_aCriteria.size())
        m_aCriteria[nIdx] = rCrit;
    else
    {
        m_aCriteria.insert(m_aCriteria.end(), nIdx - m_aCriteria.size(), OUString());
        m_aCriteria.push_back(rCrit);
    }
}

OUString OTableFieldDesc::GetCriteria( sal_uInt16 nIdx ) const
{
    OUString aRetStr;
    if( nIdx < m_aCriteria.size())
        aRetStr = m_aCriteria[nIdx];

    return aRetStr;
}

namespace
{
    struct SelectPropertyValueAsString
    {
        OUString operator()( const PropertyValue& i_rPropValue ) const
        {
            OUString sValue;
            OSL_VERIFY( i_rPropValue.Value >>= sValue );
            return sValue;
        }
    };
}

void OTableFieldDesc::Load( const css::beans::PropertyValue& i_rSettings, const bool i_bIncludingCriteria )
{

    ::comphelper::NamedValueCollection aFieldDesc( i_rSettings.Value );
    m_aAliasName = aFieldDesc.getOrDefault( u"AliasName"_ustr, m_aAliasName );
    m_aTableName = aFieldDesc.getOrDefault( u"TableName"_ustr, m_aTableName );
    m_aFieldName = aFieldDesc.getOrDefault( u"FieldName"_ustr, m_aFieldName );
    m_aFieldAlias = aFieldDesc.getOrDefault( u"FieldAlias"_ustr, m_aFieldAlias );
    m_aFunctionName = aFieldDesc.getOrDefault( u"FunctionName"_ustr, m_aFunctionName );
    m_eDataType = aFieldDesc.getOrDefault( u"DataType"_ustr, m_eDataType );
    m_eFunctionType = aFieldDesc.getOrDefault( u"FunctionType"_ustr, m_eFunctionType );
    m_nColWidth = aFieldDesc.getOrDefault( u"ColWidth"_ustr, m_nColWidth );
    m_bGroupBy = aFieldDesc.getOrDefault( u"GroupBy"_ustr, m_bGroupBy );
    m_bVisible = aFieldDesc.getOrDefault( u"Visible"_ustr, m_bVisible );

    m_eFieldType = static_cast< ETableFieldType >( aFieldDesc.getOrDefault( u"FieldType"_ustr, static_cast< sal_Int32 >( m_eFieldType ) ) );
    m_eOrderDir = static_cast< EOrderDir >( aFieldDesc.getOrDefault( u"OrderDir"_ustr, static_cast< sal_Int32 >( m_eOrderDir ) ) );

    if ( i_bIncludingCriteria )
    {
        const Sequence< PropertyValue > aCriteria( aFieldDesc.getOrDefault( u"Criteria"_ustr, Sequence< PropertyValue >() ) );
        m_aCriteria.resize( aCriteria.getLength() );
        std::transform(
            aCriteria.begin(),
            aCriteria.end(),
            m_aCriteria.begin(),
            SelectPropertyValueAsString()
        );
    }
}

void OTableFieldDesc::Save( ::comphelper::NamedValueCollection& o_rSettings, const bool i_bIncludingCriteria )
{

    o_rSettings.put( u"AliasName"_ustr, m_aAliasName );
    o_rSettings.put( u"TableName"_ustr, m_aTableName );
    o_rSettings.put( u"FieldName"_ustr, m_aFieldName );
    o_rSettings.put( u"FieldAlias"_ustr, m_aFieldAlias );
    o_rSettings.put( u"FunctionName"_ustr, m_aFunctionName );
    o_rSettings.put( u"DataType"_ustr, m_eDataType );
    o_rSettings.put( u"FunctionType"_ustr, m_eFunctionType );
    o_rSettings.put( u"FieldType"_ustr, static_cast<sal_Int32>(m_eFieldType) );
    o_rSettings.put( u"OrderDir"_ustr, static_cast<sal_Int32>(m_eOrderDir) );
    o_rSettings.put( u"ColWidth"_ustr, m_nColWidth );
    o_rSettings.put( u"GroupBy"_ustr, m_bGroupBy );
    o_rSettings.put( u"Visible"_ustr, m_bVisible );

    if ( !i_bIncludingCriteria )
        return;

    if ( m_aCriteria.empty() )
        return;

    sal_Int32 c = 0;
    Sequence< PropertyValue > aCriteria( m_aCriteria.size() );
    auto pCriteria = aCriteria.getArray();
    for (auto const& criteria : m_aCriteria)
    {
        pCriteria[c].Name = "Criterion_" + OUString::number( c );
        pCriteria[c].Value <<= criteria;
        ++c;
    }

    o_rSettings.put( u"Criteria"_ustr, aCriteria );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

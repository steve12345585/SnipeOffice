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

#include <DExport.hxx>
#include <core_resource.hxx>

#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/util/NumberFormat.hpp>
#include <com/sun/star/util/XNumberFormatTypes.hpp>
#include <strings.hrc>
#include <strings.hxx>
#include <connectivity/dbconversion.hxx>
#include <sal/log.hxx>
#include <sfx2/sfxhtml.hxx>
#include <svl/numuno.hxx>
#include <connectivity/dbtools.hxx>
#include <TypeInfo.hxx>
#include <FieldDescriptions.hxx>
#include <UITools.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <com/sun/star/awt/FontDescriptor.hpp>
#include <WCopyTable.hxx>
#include <unotools/syslocale.hxx>
#include <svl/numformat.hxx>
#include <connectivity/dbexception.hxx>
#include <connectivity/FValue.hxx>
#include <com/sun/star/sdb/application/CopyTableOperation.hpp>
#include <sqlmessage.hxx>
#include "UpdateHelperImpl.hxx"
#include <cppuhelper/exc_hlp.hxx>

using namespace dbaui;
using namespace utl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::awt;

namespace CopyTableOperation = ::com::sun::star::sdb::application::CopyTableOperation;

// ODatabaseExport
ODatabaseExport::ODatabaseExport(sal_Int32 nRows,
                                 TPositions&&_rColumnPositions,
                                 const Reference< XNumberFormatter >& _rxNumberF,
                                 const Reference< css::uno::XComponentContext >& _rxContext,
                                 const TColumnVector* pList,
                                 const OTypeInfoMap* _pInfoMap,
                                 bool _bAutoIncrementEnabled,
                                 SvStream& _rInputStream)
    :m_vColumnPositions(std::move(_rColumnPositions))
    ,m_aDestColumns(comphelper::UStringMixLess(true))
    ,m_xFormatter(_rxNumberF)
    ,m_xContext(_rxContext)
    ,m_pFormatter(nullptr)
    ,m_rInputStream( _rInputStream )
    ,m_pColumnList(pList)
    ,m_pInfoMap(_pInfoMap)
    ,m_nColumnPos(0)
    ,m_nRows(1)
    ,m_nRowCount(0)
    ,m_bError(false)
    ,m_bInTbl(false)
    ,m_bHead(true)
    ,m_bDontAskAgain(false)
    ,m_bIsAutoIncrement(_bAutoIncrementEnabled)
    ,m_bFoundTable(false)
    ,m_bCheckOnly(false)
    ,m_bAppendFirstLine(false)
{
    m_nRows += nRows;
    sal_Int32 nCount = 0;
    for(const std::pair<sal_Int32,sal_Int32> & rPair : m_vColumnPositions)
        if ( rPair.first != COLUMN_POSITION_NOT_FOUND )
            ++nCount;

    m_vColumnSize.resize(nCount);
    m_vNumberFormat.resize(nCount);
    for(sal_Int32 i=0;i<nCount;++i)
    {
        m_vColumnSize[i] = 0;
        m_vNumberFormat[i] = 0;
    }

    try
    {
        SvtSysLocale aSysLocale;
        m_aLocale = aSysLocale.GetLanguageTag().getLocale();
    }
    catch(Exception&)
    {
    }

    SetColumnTypes(pList,_pInfoMap);
}

ODatabaseExport::ODatabaseExport(const SharedConnection& _rxConnection,
                                 const Reference< XNumberFormatter >& _rxNumberF,
                                 const Reference< css::uno::XComponentContext >& _rxContext,
                                 SvStream& _rInputStream)
    :m_aDestColumns(comphelper::UStringMixLess(_rxConnection->getMetaData().is() && _rxConnection->getMetaData()->supportsMixedCaseQuotedIdentifiers()))
    ,m_xConnection(_rxConnection)
    ,m_xFormatter(_rxNumberF)
    ,m_xContext(_rxContext)
    ,m_pFormatter(nullptr)
    ,m_rInputStream( _rInputStream )
    ,m_pColumnList(nullptr)
    ,m_pInfoMap(nullptr)
    ,m_nColumnPos(0)
    ,m_nRows(1)
    ,m_nRowCount(0)
    ,m_bError(false)
    ,m_bInTbl(false)
    ,m_bHead(true)
    ,m_bDontAskAgain(false)
    ,m_bIsAutoIncrement(false)
    ,m_bFoundTable(false)
    ,m_bCheckOnly(false)
    ,m_bAppendFirstLine(false)
{
    try
    {
        SvtSysLocale aSysLocale;
        m_aLocale = aSysLocale.GetLanguageTag().getLocale();
    }
    catch(Exception&)
    {
    }

    Reference<XTablesSupplier> xTablesSup(m_xConnection,UNO_QUERY);
    if(xTablesSup.is())
        m_xTables = xTablesSup->getTables();

    Reference<XDatabaseMetaData> xMeta = m_xConnection->getMetaData();
    Reference<XResultSet> xSet = xMeta.is() ? xMeta->getTypeInfo() : Reference<XResultSet>();
    if(xSet.is())
    {
        ::connectivity::ORowSetValue aValue;
        std::vector<sal_Int32> aTypes;
        std::vector<bool> aNullable;
        Reference<XResultSetMetaData> xResultSetMetaData = Reference<XResultSetMetaDataSupplier>(xSet,UNO_QUERY_THROW)->getMetaData();
        Reference<XRow> xRow(xSet,UNO_QUERY_THROW);
        while(xSet->next())
        {
            if ( aTypes.empty() )
            {
                sal_Int32 nCount = xResultSetMetaData->getColumnCount();
                if ( nCount < 1 )
                    nCount = 18;
                aTypes.reserve(nCount+1);
                aNullable.reserve(nCount+1);
                aTypes.push_back(-1);
                aNullable.push_back(false);
                for (sal_Int32 j = 1; j <= nCount ; ++j)
                {
                    aNullable.push_back(xResultSetMetaData->isNullable(j) != ColumnValue::NO_NULLS );
                    aTypes.push_back(xResultSetMetaData->getColumnType(j));
                }
            }

            sal_Int32 nPos = 1;
            OSL_ENSURE((nPos) < static_cast<sal_Int32>(aTypes.size()),"aTypes: Illegal index for vector");
            aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
            OUString sTypeName = aValue.getString();
            ++nPos;
            OSL_ENSURE((nPos) < static_cast<sal_Int32>(aTypes.size()),"aTypes: Illegal index for vector");
            aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
            sal_Int32 nType = aValue.getInt32();
            ++nPos;

            if( nType == DataType::VARCHAR )
            {
                m_pTypeInfo                 = std::make_shared<OTypeInfo>();

                m_pTypeInfo->aTypeName      = sTypeName;
                m_pTypeInfo->nType          = nType;

                OSL_ENSURE((nPos) < static_cast<sal_Int32>(aTypes.size()),"aTypes: Illegal index for vector");
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->nPrecision     = aValue.getInt32();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow); //LiteralPrefix
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow); //LiteralSuffix
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->aCreateParams  = aValue.getString();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->bNullable      = aValue.getInt32() == ColumnValue::NULLABLE;
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                // bCaseSensitive
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->nSearchType    = aValue.getInt16();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                // bUnsigned
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->bCurrency      = aValue.getBool();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->bAutoIncrement = aValue.getBool();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->aLocalTypeName = aValue.getString();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->nMinimumScale  = aValue.getInt16();
                ++nPos;
                aValue.fill(nPos,aTypes[nPos],aNullable[nPos],xRow);
                m_pTypeInfo->nMaximumScale  = aValue.getInt16();
                nPos = 18;
                aValue.fill(nPos,aTypes[nPos],xRow);
                m_pTypeInfo->nNumPrecRadix  = aValue.getInt32();

                // check if values are less than zero like it happens in a oracle jdbc driver
                if( m_pTypeInfo->nPrecision < 0)
                    m_pTypeInfo->nPrecision = 0;
                if( m_pTypeInfo->nMinimumScale < 0)
                    m_pTypeInfo->nMinimumScale = 0;
                if( m_pTypeInfo->nMaximumScale < 0)
                    m_pTypeInfo->nMaximumScale = 0;
                if( m_pTypeInfo->nNumPrecRadix <= 1)
                    m_pTypeInfo->nNumPrecRadix = 10;
                break;
            }
        }
    }
    if ( !m_pTypeInfo )
        m_pTypeInfo = std::make_shared<OTypeInfo>();
}

ODatabaseExport::~ODatabaseExport()
{
    m_pFormatter = nullptr;
    for (auto const& destColumn : m_aDestColumns)
        delete destColumn.second;
    m_vDestVector.clear();
    m_aDestColumns.clear();
}

void ODatabaseExport::insertValueIntoColumn()
{
    if(m_nColumnPos >= sal_Int32(m_vDestVector.size()))
        return;

    OFieldDescription* pField = m_vDestVector[m_nColumnPos]->second;
    if(!pField)
        return;

    sal_Int32 nNewPos = m_bIsAutoIncrement ? m_nColumnPos+1 : m_nColumnPos;
    OSL_ENSURE(nNewPos < static_cast<sal_Int32>(m_vColumnPositions.size()),"m_vColumnPositions: Illegal index for vector");

    if ( nNewPos < static_cast<sal_Int32>(m_vColumnPositions.size() ) )
    {
        sal_Int32 nPos = m_vColumnPositions[nNewPos].first;
        if ( nPos != COLUMN_POSITION_NOT_FOUND )
        {
            if ( m_sTextToken.isEmpty() && pField->IsNullable() )
                m_pUpdateHelper->updateNull(nPos,pField->GetType());
            else
            {
                OSL_ENSURE((nNewPos) < static_cast<sal_Int32>(m_vColumnTypes.size()),"Illegal index for vector");
                if (m_vColumnTypes[nNewPos] != DataType::VARCHAR && m_vColumnTypes[nNewPos] != DataType::CHAR && m_vColumnTypes[nNewPos] != DataType::LONGVARCHAR )
                {
                    SAL_INFO("dbaccess.ui", "ODatabaseExport::insertValueIntoColumn != DataType::VARCHAR" );
                    ensureFormatter();
                    sal_Int32 nNumberFormat = 0;
                    double fOutNumber = 0.0;
                    bool bNumberFormatError = false;
                    if ( m_pFormatter && !m_sNumToken.isEmpty() )
                    {
                        LanguageType eNumLang = LANGUAGE_NONE;
                        sal_uInt32 nNumberFormat2( nNumberFormat );
                        fOutNumber = SfxHTMLParser::GetTableDataOptionsValNum(nNumberFormat2,eNumLang,m_sTextToken,m_sNumToken,*m_pFormatter);
                        if ( eNumLang != LANGUAGE_NONE )
                        {
                            nNumberFormat2 = m_pFormatter->GetFormatForLanguageIfBuiltIn( nNumberFormat2, eNumLang );
                            (void)m_pFormatter->IsNumberFormat( m_sTextToken, nNumberFormat2, fOutNumber );
                        }
                        nNumberFormat = static_cast<sal_Int32>(nNumberFormat2);
                    }
                    else
                    {
                        Reference< XNumberFormatsSupplier >  xSupplier = m_xFormatter->getNumberFormatsSupplier();
                        Reference<XNumberFormatTypes> xNumType(xSupplier->getNumberFormats(),UNO_QUERY);
                        const sal_Int16 nFormats[] = {
                            NumberFormat::DATETIME
                            ,NumberFormat::DATE
                            ,NumberFormat::TIME
                            ,NumberFormat::CURRENCY
                            ,NumberFormat::NUMBER
                            ,NumberFormat::LOGICAL
                        };
                        for (short nFormat : nFormats)
                        {
                            try
                            {
                                nNumberFormat = m_xFormatter->detectNumberFormat(xNumType->getStandardFormat(nFormat,m_aLocale),m_sTextToken);
                                break;
                            }
                            catch(Exception&)
                            {
                            }
                        }
                        try
                        {
                            fOutNumber = m_xFormatter->convertStringToNumber(nNumberFormat,m_sTextToken);
                        }
                        catch(Exception&)
                        {
                            bNumberFormatError = true;
                            m_pUpdateHelper->updateString(nPos,m_sTextToken);
                        }
                    }
                    if ( !bNumberFormatError )
                    {
                        try
                        {
                            Reference< XNumberFormatsSupplier >  xSupplier = m_xFormatter->getNumberFormatsSupplier();
                            Reference< XNumberFormats >         xFormats = xSupplier->getNumberFormats();
                            Reference<XPropertySet> xProp = xFormats->getByKey(nNumberFormat);
                            sal_Int16 nType = 0;
                            xProp->getPropertyValue(PROPERTY_TYPE) >>= nType;
                            switch(nType)
                            {
                                case NumberFormat::DATE:
                                    m_pUpdateHelper->updateDate(nPos,::dbtools::DBTypeConversion::toDate(fOutNumber,m_aNullDate));
                                    break;
                                case NumberFormat::DATETIME:
                                    m_pUpdateHelper->updateTimestamp(nPos,::dbtools::DBTypeConversion::toDateTime(fOutNumber,m_aNullDate));
                                    break;
                                case NumberFormat::TIME:
                                    m_pUpdateHelper->updateTime(nPos,::dbtools::DBTypeConversion::toTime(fOutNumber));
                                    break;
                                default:
                                    m_pUpdateHelper->updateDouble(nPos,fOutNumber);
                            }
                        }
                        catch(Exception&)
                        {
                            m_pUpdateHelper->updateString(nPos,m_sTextToken);
                        }
                    }

                }
                else
                    m_pUpdateHelper->updateString(nPos,m_sTextToken);
            }
        }
    }
    eraseTokens();
}

sal_Int16 ODatabaseExport::CheckString(const OUString& aCheckToken, sal_Int16 _nOldNumberFormat)
{
    sal_Int16 nNumberFormat = 0;

    try
    {
        Reference< XNumberFormatsSupplier > xSupplier = m_xFormatter->getNumberFormatsSupplier();
        Reference< XNumberFormats >         xFormats = xSupplier->getNumberFormats();

        ensureFormatter();
        if ( m_pFormatter && !m_sNumToken.isEmpty() )
        {
            LanguageType eNumLang;
            sal_uInt32 nFormatKey(0);
            double fOutNumber = SfxHTMLParser::GetTableDataOptionsValNum(nFormatKey,eNumLang,m_sTextToken,m_sNumToken,*m_pFormatter);
            if ( eNumLang != LANGUAGE_NONE )
            {
                nFormatKey = m_pFormatter->GetFormatForLanguageIfBuiltIn( nFormatKey, eNumLang );
                if ( !m_pFormatter->IsNumberFormat( m_sTextToken, nFormatKey, fOutNumber ) )
                    return NumberFormat::TEXT;
            }
            Reference<XPropertySet> xProp = xFormats->getByKey(nFormatKey);
            xProp->getPropertyValue(PROPERTY_TYPE) >>= nNumberFormat;
        }
        else
        {
            Reference<XNumberFormatTypes> xNumType(xFormats,UNO_QUERY);
            sal_Int32 nFormatKey = m_xFormatter->detectNumberFormat(xNumType->getStandardFormat(NumberFormat::ALL,m_aLocale),aCheckToken);
            m_xFormatter->convertStringToNumber(nFormatKey,aCheckToken);

            Reference<XPropertySet> xProp = xFormats->getByKey(nFormatKey);
            sal_Int16 nType = 0;
            xProp->getPropertyValue(PROPERTY_TYPE) >>= nType;

            switch(nType)
            {
                case NumberFormat::ALL:
                    nNumberFormat = NumberFormat::ALL;
                    break;
                case NumberFormat::DATE:
                    switch(_nOldNumberFormat)
                    {
                        case NumberFormat::DATETIME:
                        case NumberFormat::TEXT:
                        case NumberFormat::DATE:
                            nNumberFormat = _nOldNumberFormat;
                            break;
                        case NumberFormat::ALL:
                            nNumberFormat = NumberFormat::DATE;
                            break;
                        default:
                            nNumberFormat = NumberFormat::TEXT;

                    }
                    break;
                case NumberFormat::TIME:
                    switch(_nOldNumberFormat)
                    {
                        case NumberFormat::DATETIME:
                        case NumberFormat::TEXT:
                        case NumberFormat::TIME:
                            nNumberFormat = _nOldNumberFormat;
                            break;
                        case NumberFormat::ALL:
                            nNumberFormat = NumberFormat::TIME;
                            break;
                        default:
                            nNumberFormat = NumberFormat::TEXT;
                            break;
                    }
                    break;
                case NumberFormat::CURRENCY:
                    switch(_nOldNumberFormat)
                    {
                        case NumberFormat::CURRENCY:
                            nNumberFormat = _nOldNumberFormat;
                            break;
                        case NumberFormat::NUMBER:
                        case NumberFormat::ALL:
                            nNumberFormat = NumberFormat::CURRENCY;
                            break;
                        default:
                            nNumberFormat = NumberFormat::TEXT;
                            break;
                    }
                    break;
                case NumberFormat::NUMBER:
                case NumberFormat::SCIENTIFIC:
                case NumberFormat::FRACTION:
                case NumberFormat::PERCENT:
                    switch(_nOldNumberFormat)
                    {
                        case NumberFormat::NUMBER:
                            nNumberFormat = _nOldNumberFormat;
                            break;
                        case NumberFormat::CURRENCY:
                            nNumberFormat = NumberFormat::CURRENCY;
                            break;
                        case NumberFormat::ALL:
                            nNumberFormat = nType;
                            break;
                        default:
                            nNumberFormat = NumberFormat::TEXT;
                            break;
                    }
                    break;
                case NumberFormat::TEXT:
                case NumberFormat::UNDEFINED:
                case NumberFormat::LOGICAL:
                case NumberFormat::DEFINED:
                    nNumberFormat = NumberFormat::TEXT; // Text overwrites everything
                    break;
                case NumberFormat::DATETIME:
                    switch(_nOldNumberFormat)
                    {
                        case NumberFormat::DATETIME:
                        case NumberFormat::TEXT:
                        case NumberFormat::TIME:
                            nNumberFormat = _nOldNumberFormat;
                            break;
                        case NumberFormat::ALL:
                            nNumberFormat = NumberFormat::DATETIME;
                            break;
                        default:
                            nNumberFormat = NumberFormat::TEXT;
                            break;
                    }
                    break;
                default:
                    SAL_WARN("dbaccess.ui", "ODatabaseExport: Unknown NumberFormat");
            }
        }
    }
    catch(Exception&)
    {
        nNumberFormat = NumberFormat::TEXT; // Text overwrites everything
    }

    return nNumberFormat;
}

void ODatabaseExport::SetColumnTypes(const TColumnVector* _pList,const OTypeInfoMap* _pInfoMap)
{
    if(!(_pList && _pInfoMap))
        return;

    OSL_ENSURE(m_vNumberFormat.size() == m_vColumnSize.size() && m_vColumnSize.size() == _pList->size(),"Illegal columns in list");
    Reference< XNumberFormatsSupplier > xSupplier = m_xFormatter->getNumberFormatsSupplier();
    Reference< XNumberFormats >         xFormats = xSupplier->getNumberFormats();
    sal_Int32 minBothSize = std::min<sal_Int32>(m_vNumberFormat.size(), m_vColumnSize.size());
    sal_Int32 i = 0;
    for (auto const& elem : *_pList)
    {
        if (i >= minBothSize)
            break;

        sal_Int32 nDataType;
        sal_Int32 nLength(0),nScale(0);
        sal_Int16 nType = m_vNumberFormat[i] & ~NumberFormat::DEFINED;

        switch ( nType )
        {
            case NumberFormat::DATE:
                nDataType  = DataType::DATE;
                break;
            case NumberFormat::TIME:
                nDataType  = DataType::TIME;
                break;
            case NumberFormat::DATETIME:
                nDataType  = DataType::TIMESTAMP;
                break;
            case NumberFormat::CURRENCY:
                nDataType  = DataType::NUMERIC;
                nScale      = 4;
                nLength     = 19;
                break;
            case NumberFormat::ALL:
            case NumberFormat::NUMBER:
            case NumberFormat::SCIENTIFIC:
            case NumberFormat::FRACTION:
            case NumberFormat::PERCENT:
                nDataType  = DataType::DOUBLE;
                break;
            case NumberFormat::DEFINED:
            case NumberFormat::TEXT:
            case NumberFormat::UNDEFINED:
            case NumberFormat::LOGICAL:
            default:
                nDataType  = DataType::VARCHAR;
                nLength     = ((m_vColumnSize[i] % 10 ) ? m_vColumnSize[i]/ 10 + 1: m_vColumnSize[i]/ 10) * 10;
                break;
        }
        OTypeInfoMap::const_iterator aFind = _pInfoMap->find(nDataType);
        if(aFind != _pInfoMap->end())
        {
            elem->second->SetType(aFind->second);
            elem->second->SetPrecision(std::min<sal_Int32>(aFind->second->nPrecision,nLength));
            elem->second->SetScale(std::min<sal_Int32>(aFind->second->nMaximumScale,nScale));

            sal_Int32 nFormatKey = ::dbtools::getDefaultNumberFormat( nDataType,
                                elem->second->GetScale(),
                                elem->second->IsCurrency(),
                                Reference< XNumberFormatTypes>(xFormats,UNO_QUERY),
                                m_aLocale);

            elem->second->SetFormatKey(nFormatKey);
        }
        ++i;
    }
}

void ODatabaseExport::CreateDefaultColumn(const OUString& _rColumnName)
{
    Reference< XDatabaseMetaData>  xDestMetaData(m_xConnection->getMetaData());
    sal_Int32 nMaxNameLen(xDestMetaData->getMaxColumnNameLength());
    OUString aAlias = _rColumnName;
    if ( isSQL92CheckEnabled(m_xConnection) )
        aAlias = ::dbtools::convertName2SQLName(_rColumnName,xDestMetaData->getExtraNameCharacters());

    if(nMaxNameLen && aAlias.getLength() > nMaxNameLen)
        aAlias = aAlias.copy(0, std::min<sal_Int32>( nMaxNameLen-1, aAlias.getLength() ) );

    OUString sName(aAlias);
    if(m_aDestColumns.find(sName) != m_aDestColumns.end())
    {
        sal_Int32 nPos = 0;
        sal_Int32 nCount = 2;
        while(m_aDestColumns.find(sName) != m_aDestColumns.end())
        {
            sName = aAlias
                  + OUString::number(++nPos);
            if(nMaxNameLen && sName.getLength() > nMaxNameLen)
            {
                aAlias = aAlias.copy(0,std::min<sal_Int32>( nMaxNameLen-nCount, aAlias.getLength() ));
                sName = aAlias
                      + OUString::number(nPos);
                ++nCount;
            }
        }
    }
    aAlias = sName;
    // now create a column
    OFieldDescription* pField = new OFieldDescription();
    pField->SetType(m_pTypeInfo);
    pField->SetName(aAlias);
    pField->SetPrecision(std::min<sal_Int32>(sal_Int32(255),m_pTypeInfo->nPrecision));
    pField->SetScale(0);
    pField->SetIsNullable(ColumnValue::NULLABLE);
    pField->SetAutoIncrement(false);
    pField->SetPrimaryKey(false);
    pField->SetCurrency(false);

    TColumns::const_iterator aFind = m_aDestColumns.find( aAlias );
    if ( aFind != m_aDestColumns.end() )
    {
        delete aFind->second;
        m_aDestColumns.erase(aFind);
    }

    m_vDestVector.emplace_back(m_aDestColumns.emplace(aAlias,pField).first);
}

void ODatabaseExport::createRowSet()
{
    m_pUpdateHelper = std::make_shared<OParameterUpdateHelper>(createPreparedStatement(m_xConnection->getMetaData(),m_xTable,m_vColumnPositions));
}

bool ODatabaseExport::executeWizard(const OUString& _rTableName, const Any& _aTextColor, const FontDescriptor& _rFont)
{
    bool bHaveDefaultTable =  !m_sDefaultTableName.isEmpty();
    const OUString& rTableName(bHaveDefaultTable ? m_sDefaultTableName : _rTableName);
    OCopyTableWizard aWizard(
        nullptr,
        rTableName,
        bHaveDefaultTable ? CopyTableOperation::AppendData : CopyTableOperation::CopyDefinitionAndData,
        ODatabaseExport::TColumns(m_aDestColumns),
        m_vDestVector,
        m_xConnection,
        m_xFormatter,
        getTypeSelectionPageFactory(),
        m_rInputStream,
        m_xContext
    );

    bool bError = false;
    try
    {
        if (aWizard.run())
        {
            switch(aWizard.getOperation())
            {
                case CopyTableOperation::CopyDefinitionAndData:
                case CopyTableOperation::AppendData:
                    {
                        m_xTable = aWizard.returnTable();
                        bError = !m_xTable.is();
                        if(m_xTable.is())
                        {
                            m_xTable->setPropertyValue(PROPERTY_FONT,Any(_rFont));
                            if(_aTextColor.hasValue())
                                m_xTable->setPropertyValue(PROPERTY_TEXTCOLOR,_aTextColor);
                        }
                        m_bIsAutoIncrement  = aWizard.shouldCreatePrimaryKey();
                        m_vColumnPositions  = aWizard.GetColumnPositions();
                        m_vColumnTypes      = aWizard.GetColumnTypes();
                        m_bAppendFirstLine  = !aWizard.UseHeaderLine();
                    }
                    break;
                default:
                    bError = true; // there is no error but I have nothing more to do
            }
        }
        else
            bError = true;

        if(!bError)
            createRowSet();
    }
    catch( const SQLException&)
    {
        ::dbtools::showError( ::dbtools::SQLExceptionInfo( ::cppu::getCaughtException() ), aWizard.getDialog()->GetXWindow(), m_xContext );
        bError = true;
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("dbaccess");
    }

    return bError;
}

void ODatabaseExport::showErrorDialog(const css::sdbc::SQLException& e)
{
    if(!m_bDontAskAgain)
    {
        OUString aMsg = e.Message
                      + "\n"
                      + DBA_RES( STR_QRY_CONTINUE );
        OSQLWarningBox aBox(nullptr, aMsg, MessBoxStyle::YesNo | MessBoxStyle::DefaultNo);

        if (aBox.run() == RET_YES)
            m_bDontAskAgain = true;
        else
            m_bError = true;
    }
}

void ODatabaseExport::adjustFormat()
{
    if ( m_sTextToken.isEmpty() )
        return;

    sal_Int32 nNewPos = m_bIsAutoIncrement ? m_nColumnPos+1 : m_nColumnPos;
    OSL_ENSURE(nNewPos < static_cast<sal_Int32>(m_vColumnPositions.size()),"Illegal index for vector");
    if ( nNewPos < static_cast<sal_Int32>(m_vColumnPositions.size()) )
    {
        sal_Int32 nColPos = m_vColumnPositions[nNewPos].first;
        if( nColPos != COLUMN_POSITION_NOT_FOUND)
        {
            --nColPos;
            OSL_ENSURE((nColPos) < static_cast<sal_Int32>(m_vNumberFormat.size()),"m_vFormatKey: Illegal index for vector");
            OSL_ENSURE((nColPos) < static_cast<sal_Int32>(m_vColumnSize.size()),"m_vColumnSize: Illegal index for vector");
            m_vNumberFormat[nColPos] = CheckString(m_sTextToken,m_vNumberFormat[nColPos]);
            m_vColumnSize[nColPos] = std::max<sal_Int32>(static_cast<sal_Int32>(m_vColumnSize[nColPos]), m_sTextToken.getLength());
        }
    }
    eraseTokens();
}

void ODatabaseExport::eraseTokens()
{
    m_sTextToken.clear();
    m_sNumToken.clear();
}

void ODatabaseExport::ensureFormatter()
{
    if ( !m_pFormatter )
    {
        Reference< XNumberFormatsSupplier >  xSupplier = m_xFormatter->getNumberFormatsSupplier();
        auto pSupplierImpl = comphelper::getFromUnoTunnel<SvNumberFormatsSupplierObj>(xSupplier);
        m_pFormatter = pSupplierImpl ? pSupplierImpl->GetNumberFormatter() : nullptr;
        Reference<XPropertySet> xNumberFormatSettings = xSupplier->getNumberFormatSettings();
        xNumberFormatSettings->getPropertyValue(u"NullDate"_ustr) >>= m_aNullDate;
    }
}

Reference< XPreparedStatement > ODatabaseExport::createPreparedStatement( const Reference<XDatabaseMetaData>& _xMetaData
                                                       ,const Reference<XPropertySet>& _xDestTable
                                                       ,const TPositions& _rvColumns)
{
    OUString sComposedTableName = ::dbtools::composeTableName( _xMetaData, _xDestTable, ::dbtools::EComposeRule::InDataManipulation, true );

    OUStringBuffer aSql = "INSERT INTO "
                  + sComposedTableName
                  + " ( ";

    // set values and column names
    OUStringBuffer aValues(" VALUES ( ");

    OUString aQuote;
    if ( _xMetaData.is() )
        aQuote = _xMetaData->getIdentifierQuoteString();

    Reference<XColumnsSupplier> xDestColsSup(_xDestTable,UNO_QUERY_THROW);

    // create sql string and set column types
    Sequence< OUString> aDestColumnNames = xDestColsSup->getColumns()->getElementNames();
    if ( !aDestColumnNames.hasElements() )
    {
        return Reference< XPreparedStatement > ();
    }

    std::vector<OUString> aInsertList;
    auto sortedColumns = _rvColumns;
    std::sort(sortedColumns.begin(), sortedColumns.end());
    aInsertList.reserve(_rvColumns.size());
    for (const auto& [nSrc, nDest] : sortedColumns)
    {
        if (nSrc == COLUMN_POSITION_NOT_FOUND || nDest == COLUMN_POSITION_NOT_FOUND)
            continue;
        assert(nDest > 0 && nDest <= aDestColumnNames.getLength());
        aInsertList.push_back(dbtools::quoteName(aQuote, aDestColumnNames[nDest - 1]));
    }

    // create the sql string
    for (auto const& elem : aInsertList)
    {
        if ( !elem.isEmpty() )
        {
            aSql.append(elem + ",");
            aValues.append("?,");
        }
    }

    aSql[aSql.getLength()-1] = ')';
    aValues[aValues.getLength()-1] = ')';

    aSql.append(aValues);
    // now create,fill and execute the prepared statement
    return _xMetaData->getConnection()->prepareStatement(aSql.makeStringAndClear());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

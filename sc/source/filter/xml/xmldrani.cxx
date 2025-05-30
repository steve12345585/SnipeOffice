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

#include "xmldrani.hxx"
#include "xmlimprt.hxx"
#include "xmlfilti.hxx"
#include "xmlsorti.hxx"
#include <document.hxx>
#include <globalnames.hxx>
#include <dbdata.hxx>
#include <datauno.hxx>
#include <attrib.hxx>
#include <unonames.hxx>
#include "XMLConverter.hxx"
#include <rangeutl.hxx>
#include <dputil.hxx>
#include <sortparam.hxx>

#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>

#include <sax/tools/converter.hxx>

#include <com/sun/star/sheet/DataImportMode.hpp>
#include <com/sun/star/table/TableOrientation.hpp>
#include <osl/diagnose.h>
#include <o3tl/string_view.hxx>

#include <memory>

using namespace com::sun::star;
using namespace xmloff::token;

ScXMLDatabaseRangesContext::ScXMLDatabaseRangesContext( ScXMLImport& rImport ) :
    ScXMLImportContext( rImport )
{
    // has no attributes
    rImport.LockSolarMutex();
}

ScXMLDatabaseRangesContext::~ScXMLDatabaseRangesContext()
{
    GetScImport().UnlockSolarMutex();
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLDatabaseRangesContext::createFastChildContext(
                                      sal_Int32 nElement,
                                      const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch( nElement )
    {
        case XML_ELEMENT( TABLE, XML_DATABASE_RANGE ):
        {
            pContext = new ScXMLDatabaseRangeContext( GetScImport(), pAttribList );
        }
        break;
    }

    return pContext;
}

ScXMLDatabaseRangeContext::ScXMLDatabaseRangeContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList ) :
    ScXMLImportContext( rImport ),
    mpQueryParam(new ScQueryParam),
    sDatabaseRangeName(STR_DB_LOCAL_NONAME),
    nSourceType(sheet::DataImportMode_NONE),
    nRefresh(0),
    nSubTotalsUserListIndex(0),
    mbValidRange(true),
    bContainsSort(false),
    bContainsSubTotal(false),
    bNative(true),
    bIsSelection(false),
    bKeepFormats(false),
    bMoveCells(false),
    bStripData(false),
    bAutoFilter(false),
    bSubTotalsBindFormatsToContent(false),
    bSubTotalsIsCaseSensitive(false),
    bSubTotalsInsertPageBreaks(false),
    bSubTotalsSummaryBelow(true),
    bSubTotalsSortGroups(false),
    bSubTotalsEnabledUserList(false),
    bSubTotalsAscending(true),
    bFilterConditionSourceRange(false),
    bHasHeader(true),
    bHasFooter(false),
    bByRow(true),
    meRangeType(ScDBCollection::GlobalNamed)
{
    if( rAttrList.is() )
    {
        for( auto &aIter : *rAttrList )
        {
            switch( aIter.getToken() )
            {
                case XML_ELEMENT( TABLE, XML_NAME ):
                {
                    sDatabaseRangeName = aIter.toString();
                }
                break;
                case XML_ELEMENT( TABLE, XML_IS_SELECTION ):
                {
                    bIsSelection = IsXMLToken( aIter, XML_TRUE );
                }
                break;
                case XML_ELEMENT( TABLE, XML_ON_UPDATE_KEEP_STYLES ):
                {
                    bKeepFormats = IsXMLToken( aIter, XML_TRUE );
                }
                break;
                case XML_ELEMENT( TABLE, XML_ON_UPDATE_KEEP_SIZE ):
                {
                    bMoveCells = !IsXMLToken( aIter, XML_TRUE );
                }
                break;
                case XML_ELEMENT( TABLE, XML_HAS_PERSISTENT_DATA ):
                {
                    bStripData = !IsXMLToken( aIter, XML_TRUE );
                }
                break;
                case XML_ELEMENT( TABLE, XML_ORIENTATION ):
                {
                    bByRow = !IsXMLToken( aIter, XML_COLUMN );
                    mpQueryParam->bByRow = bByRow;
                }
                break;
                case XML_ELEMENT( TABLE, XML_CONTAINS_HEADER ):
                {
                    bHasHeader = IsXMLToken( aIter, XML_TRUE );
                    mpQueryParam->bHasHeader = bHasHeader;
                }
                break;
                case XML_ELEMENT( TABLE, XML_CONTAINS_FOOTER ):
                case XML_ELEMENT( CALC_EXT, XML_CONTAINS_FOOTER ):
                {
                    bHasFooter = IsXMLToken( aIter, XML_TRUE );
                    mpQueryParam->bHasTotals = bHasFooter;
                }
                break;
                case XML_ELEMENT( TABLE, XML_DISPLAY_FILTER_BUTTONS ):
                {
                    bAutoFilter = IsXMLToken( aIter, XML_TRUE );
                }
                break;
                case XML_ELEMENT( TABLE, XML_TARGET_RANGE_ADDRESS ):
                {
                    ScDocument* pDoc = GetScImport().GetDocument();
                    assert(pDoc);
                    sal_Int32 nOffset = 0;
                    if (!ScRangeStringConverter::GetRangeFromString(
                        maRange, aIter.toString(), *pDoc, ::formula::FormulaGrammar::CONV_OOO, nOffset))
                        mbValidRange = false;
                }
                break;
                case XML_ELEMENT( TABLE, XML_REFRESH_DELAY ):
                {
                    double fTime;
                    if (::sax::Converter::convertDuration( fTime, aIter.toView() ))
                        nRefresh = std::max( static_cast<sal_Int32>(fTime * 86400.0), sal_Int32(0) );
                }
                break;
            }
        }
    }

    mpQueryParam->nTab = maRange.aStart.Tab();
    mpQueryParam->nCol1 = maRange.aStart.Col();
    mpQueryParam->nRow1 = maRange.aStart.Row();
    mpQueryParam->nCol2 = maRange.aEnd.Col();
    mpQueryParam->nRow2 = maRange.aEnd.Row();

    if (sDatabaseRangeName.startsWith(STR_DB_LOCAL_NONAME))
        meRangeType = ScDBCollection::SheetAnonymous;
    else if (sDatabaseRangeName.startsWith(STR_DB_GLOBAL_NONAME))
        meRangeType = ScDBCollection::GlobalAnonymous;
}

ScXMLDatabaseRangeContext::~ScXMLDatabaseRangeContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLDatabaseRangeContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
        case XML_ELEMENT( TABLE, XML_DATABASE_SOURCE_SQL ):
        {
            pContext = new ScXMLSourceSQLContext( GetScImport(), pAttribList, this);
        }
        break;
        case XML_ELEMENT( TABLE, XML_DATABASE_SOURCE_TABLE ):
        {
            pContext = new ScXMLSourceTableContext( GetScImport(), pAttribList, this);
        }
        break;
        case XML_ELEMENT( TABLE, XML_DATABASE_SOURCE_QUERY ):
        {
            pContext = new ScXMLSourceQueryContext( GetScImport(), pAttribList, this);
        }
        break;
        case XML_ELEMENT( TABLE, XML_FILTER ):
        {
            pContext = new ScXMLFilterContext(
                GetScImport(), pAttribList, *mpQueryParam, this);
        }
        break;
        case XML_ELEMENT( TABLE, XML_SORT ):
        {
            bContainsSort = true;
            pContext = new ScXMLSortContext( GetScImport(), pAttribList, this);
        }
        break;
        case XML_ELEMENT( TABLE, XML_SUBTOTAL_RULES ):
        {
            bContainsSubTotal = true;
            pContext = new ScXMLSubTotalRulesContext( GetScImport(), pAttribList, this);
        }
        break;
    }

    return pContext;
}

std::unique_ptr<ScDBData> ScXMLDatabaseRangeContext::ConvertToDBData(const OUString& rName)
{
    if (!mbValidRange)
        return nullptr;

    ScDocument* pDoc = GetScImport().GetDocument();

    ::std::unique_ptr<ScDBData> pData(
        new ScDBData(rName, maRange.aStart.Tab(), maRange.aStart.Col(), maRange.aStart.Row(), maRange.aEnd.Col(), maRange.aEnd.Row(), bByRow, bHasHeader, bHasFooter));

    pData->SetAutoFilter(bAutoFilter);
    pData->SetKeepFmt(bKeepFormats);
    pData->SetDoSize(bMoveCells);
    pData->SetStripData(bStripData);

    pDoc->PrepareQuery(mpQueryParam->nTab, *mpQueryParam);

    pData->SetQueryParam(*mpQueryParam);

    if (bFilterConditionSourceRange)
    {
        pData->SetAdvancedQuerySource( &aFilterConditionSourceRangeAddress );
    }

    {
        ScImportParam aParam;
        aParam.bNative = bNative;
        aParam.aDBName = sDatabaseName.isEmpty() ? sConnectionResource : sDatabaseName;
        aParam.aStatement = sSourceObject;
        switch (nSourceType)
        {
            case sheet::DataImportMode_NONE:
                aParam.bImport = false;
                break;
            case sheet::DataImportMode_SQL:
                aParam.bImport = true;
                aParam.bSql    = true;
                break;
            case sheet::DataImportMode_TABLE:
                aParam.bImport = true;
                aParam.bSql    = false;
                aParam.nType   = ScDbTable;
                break;
            case sheet::DataImportMode_QUERY:
                aParam.bImport = true;
                aParam.bSql    = false;
                aParam.nType   = ScDbQuery;
                break;
            default:
                OSL_FAIL("Unknown data import mode");
                aParam.bImport = false;
        }
        pData->SetImportParam(aParam);
    }

    if (bContainsSort)
    {
        size_t nOldSize = aSortSequence.getLength();
        aSortSequence.realloc(nOldSize + 1);
        beans::PropertyValue aProperty;
        aProperty.Name = SC_UNONAME_ORIENT;
        table::TableOrientation eOrient = mpQueryParam->bByRow ?
            table::TableOrientation_ROWS : table::TableOrientation_COLUMNS;
        aProperty.Value <<= eOrient;
        aSortSequence.getArray()[nOldSize] = std::move(aProperty);
        ScSortParam aParam;
        ScSortDescriptor::FillSortParam(aParam, aSortSequence);

        SCCOLROW nStartPos = aParam.bByRow ? maRange.aStart.Col() : maRange.aStart.Row();
        for (size_t i = 0; i < aParam.GetSortKeyCount(); ++i)
        {
            if (!aParam.maKeyState[i].bDoSort)
                break;
            aParam.maKeyState[i].nField += nStartPos;
        }

        pData->SetSortParam(aParam);
    }

    if (bContainsSubTotal)
    {
        ScSubTotalParam aParam;
        aParam.bIncludePattern = bSubTotalsBindFormatsToContent;
        aParam.bUserDef = bSubTotalsEnabledUserList;
        aParam.nUserIndex = nSubTotalsUserListIndex;
        aParam.bPagebreak = bSubTotalsInsertPageBreaks;
        aParam.bSummaryBelow = bSubTotalsSummaryBelow;
        aParam.bCaseSens = bSubTotalsIsCaseSensitive;
        aParam.bDoSort = bSubTotalsSortGroups;
        aParam.bAscending = bSubTotalsAscending;
        size_t nPos = 0;
        for (const auto& rSubTotalRule : aSubTotalRules)
        {
            if (nPos >= MAXSUBTOTAL)
                break;
            auto& group = aParam.aGroups[nPos];

            const uno::Sequence<sheet::SubTotalColumn>& rColumns = rSubTotalRule.aSubTotalColumns;
            sal_Int16 nGroupColumn = rSubTotalRule.nSubTotalRuleGroupFieldNumber;
            group.bActive = true;
            group.nField = static_cast<SCCOL>(nGroupColumn);

            group.SetSubtotals(rColumns);
            ++nPos;
        }

        pData->SetSubTotalParam(aParam);
    }

    if (pData->HasImportParam() && !pData->HasImportSelection())
    {
        pData->SetRefreshDelay(nRefresh);
        pData->SetRefreshHandler(pDoc->GetDBCollection()->GetRefreshHandler());
        pData->SetRefreshControl(&pDoc->GetRefreshTimerControlAddress());
    }

    return pData;
}

namespace {

bool setAutoFilterFlags(ScDocument& rDoc, const ScDBData& rData)
{
    if (!rData.HasAutoFilter())
        return false;

    // Set autofilter flags so that the buttons get displayed.
    ScRange aRange;
    rData.GetArea(aRange);
    rDoc.ApplyFlagsTab(
        aRange.aStart.Col(), aRange.aStart.Row(), aRange.aEnd.Col(), aRange.aStart.Row(),
        aRange.aStart.Tab(), ScMF::Auto);
    return false;
}

}

void SAL_CALL ScXMLDatabaseRangeContext::endFastElement( sal_Int32 /*nElement*/ )
{
    ScDocument* pDoc = GetScImport().GetDocument();
    if (!pDoc)
        return;

    if (meRangeType == ScDBCollection::SheetAnonymous)
    {
        ::std::unique_ptr<ScDBData> pData(ConvertToDBData(STR_DB_LOCAL_NONAME));

        if (pData)
        {
            ScRange aRange;
            pData->GetArea(aRange);

            setAutoFilterFlags(*pDoc, *pData);
            pDoc->SetAnonymousDBData(aRange.aStart.Tab(), std::move(pData));
        }
        return;
    }
    else if (meRangeType == ScDBCollection::GlobalAnonymous)
    {
        ::std::unique_ptr<ScDBData> pData(ConvertToDBData(STR_DB_GLOBAL_NONAME));

        if (pData)
        {
            ScRange aRange;
            pData->GetArea(aRange);

            if (setAutoFilterFlags(*pDoc, *pData))
                pDoc->SetAnonymousDBData(aRange.aStart.Tab(), std::move(pData));
            else
                pDoc->GetDBCollection()->getAnonDBs().insert(pData.release());
        }
        return;
    }
    else if (meRangeType == ScDBCollection::GlobalNamed)
    {
        ::std::unique_ptr<ScDBData> pData(ConvertToDBData(sDatabaseRangeName));

        if (pData)
        {
            setAutoFilterFlags(*pDoc, *pData);
            (void)pDoc->GetDBCollection()->getNamedDBs().insert(std::move(pData));
        }
    }
}

ScXMLSourceSQLContext::ScXMLSourceSQLContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
    ScXMLImportContext( rImport ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_DATABASE_NAME ):
                    sDBName = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_SQL_STATEMENT ):
                    pDatabaseRangeContext->SetSourceObject(aIter.toString());
                    break;
                case XML_ELEMENT( TABLE, XML_PARSE_SQL_STATEMENT ):
                    pDatabaseRangeContext->SetNative(IsXMLToken(aIter, XML_TRUE));
                    break;
            }
        }
    }
    pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_SQL);
}

ScXMLSourceSQLContext::~ScXMLSourceSQLContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLSourceSQLContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    if ( nElement == XML_ELEMENT( FORM, XML_CONNECTION_RESOURCE ) && sDBName.isEmpty() )
    {
        pContext = new ScXMLConResContext( GetScImport(), pAttribList, pDatabaseRangeContext);
    }

    return pContext;
}

void SAL_CALL ScXMLSourceSQLContext::endFastElement( sal_Int32 /*nElement*/ )
{
    if (!sDBName.isEmpty())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLSourceTableContext::ScXMLSourceTableContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
    ScXMLImportContext( rImport ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_DATABASE_NAME ):
                    sDBName = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_TABLE_NAME ):
                case XML_ELEMENT( TABLE, XML_DATABASE_TABLE_NAME ):
                    pDatabaseRangeContext->SetSourceObject(aIter.toString());
                    break;
            }
        }
    }
    pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_TABLE);
}

ScXMLSourceTableContext::~ScXMLSourceTableContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLSourceTableContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    if ( nElement == XML_ELEMENT( FORM, XML_CONNECTION_RESOURCE ) && sDBName.isEmpty() )
    {
        pContext = new ScXMLConResContext( GetScImport(), pAttribList, pDatabaseRangeContext);
    }

    return pContext;
}

void SAL_CALL ScXMLSourceTableContext::endFastElement( sal_Int32 /*nElement*/ )
{
    if (!sDBName.isEmpty())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLSourceQueryContext::ScXMLSourceQueryContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
    ScXMLImportContext( rImport ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_DATABASE_NAME ):
                    sDBName = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_QUERY_NAME ):
                    pDatabaseRangeContext->SetSourceObject(aIter.toString());
                    break;
            }
        }
    }
    pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_QUERY);
}

ScXMLSourceQueryContext::~ScXMLSourceQueryContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLSourceQueryContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    if ( nElement == XML_ELEMENT( FORM, XML_CONNECTION_RESOURCE ) && sDBName.isEmpty() )
    {
        pContext = new ScXMLConResContext( GetScImport(), pAttribList, pDatabaseRangeContext);
    }

    return pContext;
}

void SAL_CALL ScXMLSourceQueryContext::endFastElement( sal_Int32 /*nElement*/ )
{
    if (!sDBName.isEmpty())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLConResContext::ScXMLConResContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pDatabaseRangeContext) :
    ScXMLImportContext( rImport )
{
    OUString sConRes;
    if ( rAttrList.is() )
    {
        auto aIter( rAttrList->find( XML_ELEMENT( XLINK, XML_HREF ) ) );
        if (aIter != rAttrList->end())
            sConRes = aIter.toString();
    }
    if (!sConRes.isEmpty())
        pDatabaseRangeContext->SetConnectionResource(sConRes);
}

ScXMLConResContext::~ScXMLConResContext()
{
}

ScXMLSubTotalRulesContext::ScXMLSubTotalRulesContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
    ScXMLImportContext( rImport ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
            case XML_ELEMENT( TABLE, XML_BIND_STYLES_TO_CONTENT ):
                pDatabaseRangeContext->SetSubTotalsBindFormatsToContent(IsXMLToken(aIter, XML_TRUE));
                break;
            case XML_ELEMENT( TABLE, XML_CASE_SENSITIVE ):
                pDatabaseRangeContext->SetSubTotalsIsCaseSensitive(IsXMLToken(aIter, XML_TRUE));
                break;
            case XML_ELEMENT( TABLE, XML_PAGE_BREAKS_ON_GROUP_CHANGE ):
                pDatabaseRangeContext->SetSubTotalsInsertPageBreaks(IsXMLToken(aIter, XML_TRUE));
                break;
            case XML_ELEMENT( LO_EXT, XML_SUMMARY_BELOW ):
                pDatabaseRangeContext->SetSubTotalsSummaryBelow(IsXMLToken(aIter, XML_TRUE));
                break;
        }
    }
}

ScXMLSubTotalRulesContext::~ScXMLSubTotalRulesContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLSubTotalRulesContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
        case XML_ELEMENT( TABLE, XML_SORT_GROUPS ):
        {
            pContext = new ScXMLSortGroupsContext( GetScImport(), pAttribList, pDatabaseRangeContext);
        }
        break;
        case XML_ELEMENT( TABLE, XML_SUBTOTAL_RULE ):
        {
            pContext = new ScXMLSubTotalRuleContext( GetScImport(), pAttribList, pDatabaseRangeContext);
        }
        break;
    }

    return pContext;
}

ScXMLSortGroupsContext::ScXMLSortGroupsContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pDatabaseRangeContext) :
    ScXMLImportContext( rImport )
{
    pDatabaseRangeContext->SetSubTotalsSortGroups(true);
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
            case XML_ELEMENT( TABLE, XML_DATA_TYPE ):
            {
                const OUString sValue = aIter.toString();
                if (sValue.getLength() > 8)
                {
                    std::u16string_view sTemp = sValue.subView(0, 8);
                    if (sTemp == u"UserList")
                    {
                        pDatabaseRangeContext->SetSubTotalsEnabledUserList(true);
                        sTemp = sValue.subView(8);
                        pDatabaseRangeContext->SetSubTotalsUserListIndex(static_cast<sal_Int16>(o3tl::toInt32(sTemp)));
                    }
                    else
                    {
                        //if (IsXMLToken(aIter, XML_AUTOMATIC))
                            //aSortField.FieldType = util::SortFieldType_AUTOMATIC;
                            // is not supported by StarOffice
                    }
                }
                else
                {
                    //if (IsXMLToken(aIter, XML_TEXT))
                        //aSortField.FieldType = util::SortFieldType_ALPHANUMERIC;
                        // is not supported by StarOffice
                    //else if (IsXMLToken(aIter, XML_NUMBER))
                        //aSortField.FieldType = util::SortFieldType_NUMERIC;
                        // is not supported by StarOffice
                }
            }
            break;
            case XML_ELEMENT( TABLE, XML_ORDER ):
            {
                if (IsXMLToken(aIter, XML_ASCENDING))
                    pDatabaseRangeContext->SetSubTotalsAscending(true);
                else
                    pDatabaseRangeContext->SetSubTotalsAscending(false);
            }
            break;
        }
    }
}

ScXMLSortGroupsContext::~ScXMLSortGroupsContext()
{
}

ScXMLSubTotalRuleContext::ScXMLSubTotalRuleContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
    ScXMLImportContext( rImport ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_GROUP_BY_FIELD_NUMBER ):
                    aSubTotalRule.nSubTotalRuleGroupFieldNumber = static_cast<sal_Int16>(aIter.toInt32());
                    break;
            }
        }
    }
}

ScXMLSubTotalRuleContext::~ScXMLSubTotalRuleContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLSubTotalRuleContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
        case XML_ELEMENT( TABLE, XML_SUBTOTAL_FIELD ):
        {
            pContext = new ScXMLSubTotalFieldContext( GetScImport(), pAttribList, this);
        }
        break;
    }

    return pContext;
}

void SAL_CALL ScXMLSubTotalRuleContext::endFastElement( sal_Int32 /*nElement*/ )
{
    if (pDatabaseRangeContext)
        pDatabaseRangeContext->AddSubTotalRule(aSubTotalRule);
}

ScXMLSubTotalFieldContext::ScXMLSubTotalFieldContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLSubTotalRuleContext* pTempSubTotalRuleContext) :
    ScXMLImportContext( rImport ),
    pSubTotalRuleContext(pTempSubTotalRuleContext)
{
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
            case XML_ELEMENT( TABLE, XML_FIELD_NUMBER ):
                sFieldNumber = aIter.toString();
                break;
            case XML_ELEMENT( TABLE, XML_FUNCTION ):
                sFunction = aIter.toString();
                break;
        }
    }
}

ScXMLSubTotalFieldContext::~ScXMLSubTotalFieldContext()
{
}

void SAL_CALL ScXMLSubTotalFieldContext::endFastElement( sal_Int32 /*nElement*/ )
{
    sheet::SubTotalColumn aSubTotalColumn;
    aSubTotalColumn.Column = sFieldNumber.toInt32();
    aSubTotalColumn.Function = ScXMLConverter::GetFunctionFromString( sFunction );
    pSubTotalRuleContext->AddSubTotalColumn(aSubTotalColumn);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

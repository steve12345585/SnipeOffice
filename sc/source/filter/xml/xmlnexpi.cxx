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

#include "xmlnexpi.hxx"
#include "xmlimprt.hxx"
#include <document.hxx>

#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmltoken.hxx>

using namespace com::sun::star;
using namespace xmloff::token;

ScXMLNamedExpressionsContext::GlobalInserter::GlobalInserter(ScXMLImport& rImport) : mrImport(rImport) {}

void ScXMLNamedExpressionsContext::GlobalInserter::insert(ScMyNamedExpression aExp)
{
    mrImport.AddNamedExpression(std::move(aExp));
}

ScXMLNamedExpressionsContext::SheetLocalInserter::SheetLocalInserter(ScXMLImport& rImport, SCTAB nTab) :
    mrImport(rImport), mnTab(nTab) {}

void ScXMLNamedExpressionsContext::SheetLocalInserter::insert(ScMyNamedExpression aExp)
{
    mrImport.AddNamedExpression(mnTab, std::move(aExp));
}

ScXMLNamedExpressionsContext::ScXMLNamedExpressionsContext(
    ScXMLImport& rImport,
    std::shared_ptr<Inserter> pInserter ) :
    ScXMLImportContext( rImport ),
    mpInserter(std::move(pInserter))
{
    rImport.LockSolarMutex();
}

ScXMLNamedExpressionsContext::~ScXMLNamedExpressionsContext()
{
    GetScImport().UnlockSolarMutex();
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLNamedExpressionsContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext(nullptr);
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
        case XML_ELEMENT( TABLE, XML_NAMED_RANGE ):
            pContext = new ScXMLNamedRangeContext(
                GetScImport(), pAttribList, mpInserter.get() );
            break;
        case XML_ELEMENT( TABLE, XML_NAMED_EXPRESSION ):
            pContext = new ScXMLNamedExpressionContext(
                GetScImport(), pAttribList, mpInserter.get() );
            break;
    }

    return pContext;
}

ScXMLNamedRangeContext::ScXMLNamedRangeContext(
    ScXMLImport& rImport,
    const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
    ScXMLNamedExpressionsContext::Inserter* pInserter ) :
    ScXMLImportContext( rImport )
{
    if (!pInserter)
        return;

    ScDocument* pDoc = GetScImport().GetDocument();
    if (!pDoc)
        return;
    ScMyNamedExpression aNamedExpression;
    // A simple table:cell-range-address is not a formula expression, stored
    // without [] brackets but with dot, .A1
    aNamedExpression.eGrammar = formula::FormulaGrammar::mergeToGrammar(
            pDoc->GetStorageGrammar(),
            formula::FormulaGrammar::CONV_OOO);

    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_NAME ):
                    aNamedExpression.sName = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_CELL_RANGE_ADDRESS ):
                    aNamedExpression.sContent = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_BASE_CELL_ADDRESS ):
                    aNamedExpression.sBaseCellAddress = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_RANGE_USABLE_AS ):
                    aNamedExpression.sRangeType = aIter.toString();
                    break;
            }
        }
    }
    aNamedExpression.bIsExpression = false;
    pInserter->insert(std::move(aNamedExpression));
}

ScXMLNamedRangeContext::~ScXMLNamedRangeContext()
{
}

ScXMLNamedExpressionContext::ScXMLNamedExpressionContext(
    ScXMLImport& rImport,
    const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
    ScXMLNamedExpressionsContext::Inserter* pInserter ) :
    ScXMLImportContext( rImport )
{
    if (!pInserter)
        return;

    ScMyNamedExpression aNamedExpression;

    if ( rAttrList.is() )
    {
        for (auto &aIter : *rAttrList)
        {
            switch (aIter.getToken())
            {
                case XML_ELEMENT( TABLE, XML_NAME ):
                    aNamedExpression.sName = aIter.toString();
                    break;
                case XML_ELEMENT( TABLE, XML_EXPRESSION ):
                    GetScImport().ExtractFormulaNamespaceGrammar(
                        aNamedExpression.sContent, aNamedExpression.sContentNmsp,
                        aNamedExpression.eGrammar, aIter.toString() );
                    break;
                case XML_ELEMENT( TABLE, XML_BASE_CELL_ADDRESS ):
                    aNamedExpression.sBaseCellAddress = aIter.toString();
                    break;
                case XML_ELEMENT(LO_EXT, XML_HIDDEN):
                    if (aIter.toString() == GetXMLToken(XML_TRUE))
                        aNamedExpression.sRangeType = GetXMLToken(XML_HIDDEN);
                    break;
            }
        }
    }
    aNamedExpression.bIsExpression = true;
    pInserter->insert(std::move(aNamedExpression));
}

ScXMLNamedExpressionContext::~ScXMLNamedExpressionContext()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

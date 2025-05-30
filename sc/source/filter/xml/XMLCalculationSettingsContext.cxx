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

#include "XMLCalculationSettingsContext.hxx"
#include "xmlimprt.hxx"
#include <unonames.hxx>
#include <docoptio.hxx>
#include <document.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <sax/tools/converter.hxx>
#include <docuno.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XModel.hpp>

using namespace com::sun::star;
using namespace xmloff::token;

ScXMLCalculationSettingsContext::ScXMLCalculationSettingsContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList ) :
    ScXMLImportContext( rImport ),
    fIterationEpsilon(0.001),
    nIterationCount(100),
    nYear2000(1930),
    eSearchType(utl::SearchParam::SearchType::Regexp),
    bIsIterationEnabled(false),
    bCalcAsShown(false),
    bIgnoreCase(false),
    bLookUpLabels(true),
    bMatchWholeCell(true)
{
    aNullDate.Day = 30;
    aNullDate.Month = 12;
    aNullDate.Year = 1899;
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_CASE_SENSITIVE ):
            if( IsXMLToken( aIter, XML_FALSE ) )
                bIgnoreCase = true;
            break;
        case XML_ELEMENT( TABLE, XML_PRECISION_AS_SHOWN ):
            if( IsXMLToken( aIter, XML_TRUE ) )
                bCalcAsShown = true;
            break;
        case XML_ELEMENT( TABLE, XML_SEARCH_CRITERIA_MUST_APPLY_TO_WHOLE_CELL ):
            if( IsXMLToken( aIter, XML_FALSE ) )
                bMatchWholeCell = false;
            break;
        case XML_ELEMENT( TABLE, XML_AUTOMATIC_FIND_LABELS ):
            if( IsXMLToken( aIter, XML_FALSE ) )
                bLookUpLabels = false;
            break;
        case XML_ELEMENT( TABLE, XML_NULL_YEAR ):
            sal_Int32 nTemp;
            ::sax::Converter::convertNumber( nTemp, aIter.toView() );
            nYear2000 = static_cast<sal_uInt16>(nTemp);
            break;
        case XML_ELEMENT( TABLE, XML_USE_REGULAR_EXPRESSIONS ):
            // Overwrite only the default (regex true) value, not wildcard.
            if( eSearchType == utl::SearchParam::SearchType::Regexp && IsXMLToken( aIter, XML_FALSE ) )
                eSearchType = utl::SearchParam::SearchType::Normal;
            break;
        case XML_ELEMENT( TABLE, XML_USE_WILDCARDS ):
            if( IsXMLToken( aIter, XML_TRUE ) )
                eSearchType = utl::SearchParam::SearchType::Wildcard;
            break;
        }
    }
}

ScXMLCalculationSettingsContext::~ScXMLCalculationSettingsContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLCalculationSettingsContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    if (nElement == XML_ELEMENT( TABLE, XML_NULL_DATE ))
        pContext = new ScXMLNullDateContext(GetScImport(), pAttribList, this);
    else if (nElement == XML_ELEMENT( TABLE, XML_ITERATION ))
        pContext = new ScXMLIterationContext(GetScImport(), pAttribList, this);

    return pContext;
}

void SAL_CALL ScXMLCalculationSettingsContext::endFastElement( sal_Int32 /*nElement*/ )
{
    ScModelObj* xPropertySet(GetScImport().GetScModel());
    if (!xPropertySet)
        return;

    xPropertySet->setPropertyValue( SC_UNO_CALCASSHOWN, uno::Any(bCalcAsShown) );
    xPropertySet->setPropertyValue( SC_UNO_IGNORECASE, uno::Any(bIgnoreCase) );
    xPropertySet->setPropertyValue( SC_UNO_LOOKUPLABELS, uno::Any(bLookUpLabels) );
    xPropertySet->setPropertyValue( SC_UNO_MATCHWHOLE, uno::Any(bMatchWholeCell) );
    bool bWildcards, bRegex;
    utl::SearchParam::ConvertToBool( eSearchType, bWildcards, bRegex);
    xPropertySet->setPropertyValue( SC_UNO_REGEXENABLED, uno::Any(bRegex) );
    xPropertySet->setPropertyValue( SC_UNO_WILDCARDSENABLED, uno::Any(bWildcards) );
    xPropertySet->setPropertyValue( SC_UNO_ITERENABLED, uno::Any(bIsIterationEnabled) );
    xPropertySet->setPropertyValue( SC_UNO_ITERCOUNT, uno::Any(nIterationCount) );
    xPropertySet->setPropertyValue( SC_UNO_ITEREPSILON, uno::Any(fIterationEpsilon) );
    xPropertySet->setPropertyValue( SC_UNO_NULLDATE, uno::Any(aNullDate) );
    if (ScDocument* pDoc = GetScImport().GetDocument())
    {
        ScXMLImport::MutexGuard aGuard(GetScImport());
        ScDocOptions aDocOptions (pDoc->GetDocOptions());
        aDocOptions.SetYear2000(nYear2000);
        GetScImport().GetDocument()->SetDocOptions(aDocOptions);
    }
}

ScXMLNullDateContext::ScXMLNullDateContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLCalculationSettingsContext* pCalcSet) :
    ScXMLImportContext( rImport )
{
    if ( !rAttrList.is() )
        return;

    auto aIter( rAttrList->find( XML_ELEMENT( TABLE, XML_DATE_VALUE ) ) );
    if (aIter != rAttrList->end())
    {
        util::DateTime aDateTime;
        if (::sax::Converter::parseDateTime(aDateTime, aIter.toView()))
        {
            util::Date aDate;
            aDate.Day = aDateTime.Day;
            aDate.Month = aDateTime.Month;
            aDate.Year = aDateTime.Year;
            pCalcSet->SetNullDate(aDate);
        }
        else
        {
            SAL_WARN("sc.filter","ignoring invalid NullDate '" << aIter.toView() << "'");
        }
    }
}

ScXMLNullDateContext::~ScXMLNullDateContext()
{
}

ScXMLIterationContext::ScXMLIterationContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLCalculationSettingsContext* pCalcSet) :
    ScXMLImportContext( rImport )
{
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_STATUS ):
            if (IsXMLToken(aIter, XML_ENABLE))
                pCalcSet->SetIterationStatus(true);
            break;
        case XML_ELEMENT( TABLE, XML_STEPS ):
            pCalcSet->SetIterationCount(aIter.toInt32());
            break;
        case XML_ELEMENT( TABLE, XML_MAXIMUM_DIFFERENCE ):
            pCalcSet->SetIterationEpsilon( aIter.toDouble() );
            break;
        }
    }
}

ScXMLIterationContext::~ScXMLIterationContext()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

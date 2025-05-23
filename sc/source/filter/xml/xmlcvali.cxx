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

#include "xmlcvali.hxx"
#include "xmlimprt.hxx"
#include "xmlconti.hxx"
#include <document.hxx>
#include "XMLConverter.hxx"

#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/XMLEventsImportContext.hxx>
#include <com/sun/star/sheet/TableValidationVisibility.hpp>

using namespace com::sun::star;
using namespace xmloff::token;
using namespace ::formula;

namespace {

class ScXMLContentValidationContext : public ScXMLImportContext
{
    OUString      sName;
    OUString      sHelpTitle;
    OUString      sHelpMessage;
    OUString      sErrorTitle;
    OUString      sErrorMessage;
    OUString      sErrorMessageType;
    OUString      sBaseCellAddress;
    OUString      sCondition;
    sal_Int16          nShowList;
    bool           bAllowEmptyCell;
    bool           bIsCaseSensitive;
    bool           bDisplayHelp;
    bool           bDisplayError;

    rtl::Reference<XMLEventsImportContext> xEventContext;

    css::sheet::ValidationAlertStyle GetAlertStyle() const;
    void SetFormula( OUString& rFormula, OUString& rFormulaNmsp, FormulaGrammar::Grammar& reGrammar,
        const OUString& rCondition, const OUString& rGlobNmsp, FormulaGrammar::Grammar eGlobGrammar, bool bHasNmsp ) const;
    void GetCondition( ScMyImportValidation& rValidation ) const;

public:

    ScXMLContentValidationContext( ScXMLImport& rImport,
                        const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList );

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;

    virtual void SAL_CALL endFastElement( sal_Int32 nElement ) override;

    void SetHelpMessage(const OUString& sTitle, const OUString& sMessage, const bool bDisplay);
    void SetErrorMessage(const OUString& sTitle, const OUString& sMessage, const OUString& sMessageType, const bool bDisplay);
    void SetErrorMacro(const bool bExecute);
};

class ScXMLHelpMessageContext : public ScXMLImportContext
{
    OUString   sTitle;
    OUStringBuffer sMessage;
    sal_Int32       nParagraphCount;
    bool        bDisplay;

    ScXMLContentValidationContext* pValidationContext;

public:

    ScXMLHelpMessageContext( ScXMLImport& rImport,
                        const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                        ScXMLContentValidationContext* pValidationContext);

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;

    virtual void SAL_CALL endFastElement( sal_Int32 nElement ) override;
};

class ScXMLErrorMessageContext : public ScXMLImportContext
{
    OUString   sTitle;
    OUStringBuffer sMessage;
    OUString   sMessageType;
    sal_Int32       nParagraphCount;
    bool        bDisplay;

    ScXMLContentValidationContext* pValidationContext;

public:

    ScXMLErrorMessageContext( ScXMLImport& rImport,
                        const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                        ScXMLContentValidationContext* pValidationContext);

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;

    virtual void SAL_CALL endFastElement( sal_Int32 nElement ) override;
};

class ScXMLErrorMacroContext : public ScXMLImportContext
{
    bool        bExecute;
    ScXMLContentValidationContext*  pValidationContext;

public:

    ScXMLErrorMacroContext( ScXMLImport& rImport,
                        const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                        ScXMLContentValidationContext* pValidationContext);

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
    virtual void SAL_CALL endFastElement( sal_Int32 nElement ) override;
};

}

ScXMLContentValidationsContext::ScXMLContentValidationsContext( ScXMLImport& rImport ) :
    ScXMLImportContext( rImport )
{
    // here are no attributes
}

ScXMLContentValidationsContext::~ScXMLContentValidationsContext()
{
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLContentValidationsContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
        case XML_ELEMENT( TABLE, XML_CONTENT_VALIDATION ):
            pContext = new ScXMLContentValidationContext( GetScImport(), pAttribList );
        break;
    }

    return pContext;
}

ScXMLContentValidationContext::ScXMLContentValidationContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList ) :
    ScXMLImportContext( rImport ),
    nShowList(sheet::TableValidationVisibility::UNSORTED),
    bAllowEmptyCell(true),
    bIsCaseSensitive(false),
    bDisplayHelp(false),
    bDisplayError(false)
{
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_NAME ):
            sName = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_CONDITION ):
            sCondition = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_BASE_CELL_ADDRESS ):
            sBaseCellAddress = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_ALLOW_EMPTY_CELL ):
            if (IsXMLToken(aIter, XML_FALSE))
                bAllowEmptyCell = false;
            break;
        case XML_ELEMENT( TABLE, XML_CASE_SENSITIVE ):
            if (IsXMLToken(aIter, XML_TRUE))
                bIsCaseSensitive = true;
            break;
        case XML_ELEMENT( TABLE, XML_DISPLAY_LIST ):
            if (IsXMLToken(aIter, XML_NO))
            {
                nShowList = sheet::TableValidationVisibility::INVISIBLE;
            }
            else if (IsXMLToken(aIter, XML_UNSORTED))
            {
                nShowList = sheet::TableValidationVisibility::UNSORTED;
            }
            else if (IsXMLToken(aIter, XML_SORT_ASCENDING))
            {
                nShowList = sheet::TableValidationVisibility::SORTEDASCENDING;
            }
            else if (IsXMLToken(aIter, XML_SORTED_ASCENDING))
            {
                // Read old wrong value, fdo#72548
                nShowList = sheet::TableValidationVisibility::SORTEDASCENDING;
            }
            break;
        }
    }
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL ScXMLContentValidationContext::createFastChildContext(
    sal_Int32 nElement, const uno::Reference< xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    sax_fastparser::FastAttributeList *pAttribList =
        &sax_fastparser::castToFastAttributeList( xAttrList );

    switch (nElement)
    {
    case XML_ELEMENT( TABLE, XML_HELP_MESSAGE ):
        pContext = new ScXMLHelpMessageContext( GetScImport(), pAttribList, this);
        break;
    case XML_ELEMENT( TABLE, XML_ERROR_MESSAGE ):
        pContext = new ScXMLErrorMessageContext( GetScImport(), pAttribList, this);
        break;
    case XML_ELEMENT( TABLE, XML_ERROR_MACRO ):
        pContext = new ScXMLErrorMacroContext( GetScImport(), pAttribList, this);
        break;
    case XML_ELEMENT(OFFICE, XML_EVENT_LISTENERS):
        xEventContext = new XMLEventsImportContext( GetImport() );
        pContext = xEventContext.get();
    }

    return pContext;
}

sheet::ValidationAlertStyle ScXMLContentValidationContext::GetAlertStyle() const
{
    if (IsXMLToken(sErrorMessageType, XML_MACRO))
        return sheet::ValidationAlertStyle_MACRO;
    if (IsXMLToken(sErrorMessageType, XML_STOP))
        return sheet::ValidationAlertStyle_STOP;
    if (IsXMLToken(sErrorMessageType, XML_WARNING))
        return sheet::ValidationAlertStyle_WARNING;
    if (IsXMLToken(sErrorMessageType, XML_INFORMATION))
        return sheet::ValidationAlertStyle_INFO;
    // default for unknown
    return sheet::ValidationAlertStyle_STOP;
}

void ScXMLContentValidationContext::SetFormula( OUString& rFormula, OUString& rFormulaNmsp, FormulaGrammar::Grammar& reGrammar,
    const OUString& rCondition, const OUString& rGlobNmsp, FormulaGrammar::Grammar eGlobGrammar, bool bHasNmsp ) const
{
    reGrammar = FormulaGrammar::GRAM_UNSPECIFIED;
    if( bHasNmsp )
    {
        // the entire attribute contains a namespace: internal namespace not allowed
        rFormula = rCondition;
        rFormulaNmsp = rGlobNmsp;
        reGrammar = eGlobGrammar;
    }
    else
    {
        // the attribute does not contain a namespace: try to find a namespace of an external grammar
        GetScImport().ExtractFormulaNamespaceGrammar( rFormula, rFormulaNmsp, reGrammar, rCondition, true );
        if( reGrammar != FormulaGrammar::GRAM_EXTERNAL )
            reGrammar = eGlobGrammar;
    }
}

void ScXMLContentValidationContext::GetCondition( ScMyImportValidation& rValidation ) const
{
    rValidation.aValidationType = sheet::ValidationType_ANY;    // default if no condition is given
    rValidation.aOperator = sheet::ConditionOperator_NONE;

    if( sCondition.isEmpty() )
        return;

    // extract leading namespace from condition string
    OUString aCondition, aConditionNmsp;
    FormulaGrammar::Grammar eGrammar = FormulaGrammar::GRAM_UNSPECIFIED;
    GetScImport().ExtractFormulaNamespaceGrammar( aCondition, aConditionNmsp, eGrammar, sCondition );
    bool bHasNmsp = aCondition.getLength() < sCondition.getLength();

    // parse a condition from the attribute string
    ScXMLConditionParseResult aParseResult;
    ScXMLConditionHelper::parseCondition( aParseResult, aCondition, 0 );

    /*  Check the result. A valid value in aParseResult.meToken implies
        that the other members of aParseResult are filled with valid data
        for that token. */
    bool bSecondaryPart = false;
    switch( aParseResult.meToken )
    {
        case XML_COND_TEXTLENGTH:               // condition is 'cell-content-text-length()<operator><expression>'
        case XML_COND_TEXTLENGTH_ISBETWEEN:     // condition is 'cell-content-text-length-is-between(<expression1>,<expression2>)'
        case XML_COND_TEXTLENGTH_ISNOTBETWEEN:  // condition is 'cell-content-text-length-is-not-between(<expression1>,<expression2>)'
        case XML_COND_ISINLIST:                 // condition is 'cell-content-is-in-list(<expression>)'
        case XML_COND_ISTRUEFORMULA:            // condition is 'is-true-formula(<expression>)'
            rValidation.aValidationType = aParseResult.meValidation;
            rValidation.aOperator = aParseResult.meOperator;
        break;

        case XML_COND_ISWHOLENUMBER:            // condition is 'cell-content-is-whole-number() and <condition>'
        case XML_COND_ISDECIMALNUMBER:          // condition is 'cell-content-is-decimal-number() and <condition>'
        case XML_COND_ISDATE:                   // condition is 'cell-content-is-date() and <condition>'
        case XML_COND_ISTIME:                   // condition is 'cell-content-is-time() and <condition>'
            rValidation.aValidationType = aParseResult.meValidation;
            bSecondaryPart = true;
        break;

        default:;   // unacceptable or unknown condition
    }

    /*  Parse the following 'and <condition>' part of some conditions. This
        updates the members of aParseResult that will contain the operands
        and comparison operator then. */
    if( bSecondaryPart )
    {
        ScXMLConditionHelper::parseCondition( aParseResult, aCondition, aParseResult.mnEndIndex );
        if( aParseResult.meToken == XML_COND_AND )
        {
            ScXMLConditionHelper::parseCondition( aParseResult, aCondition, aParseResult.mnEndIndex );
            switch( aParseResult.meToken )
            {
                case XML_COND_CELLCONTENT:  // condition is 'and cell-content()<operator><expression>'
                case XML_COND_ISBETWEEN:    // condition is 'and cell-content-is-between(<expression1>,<expression2>)'
                case XML_COND_ISNOTBETWEEN: // condition is 'and cell-content-is-not-between(<expression1>,<expression2>)'
                    rValidation.aOperator = aParseResult.meOperator;
                break;
                default:;   // unacceptable or unknown condition
            }
        }
    }

    // a validation type (date, integer) without a condition isn't possible
    if( rValidation.aOperator == sheet::ConditionOperator_NONE )
        rValidation.aValidationType = sheet::ValidationType_ANY;

    // parse the formulas
    if( rValidation.aValidationType != sheet::ValidationType_ANY )
    {
        SetFormula( rValidation.sFormula1, rValidation.sFormulaNmsp1, rValidation.eGrammar1,
            aParseResult.maOperand1, aConditionNmsp, eGrammar, bHasNmsp );
        SetFormula( rValidation.sFormula2, rValidation.sFormulaNmsp2, rValidation.eGrammar2,
            aParseResult.maOperand2, aConditionNmsp, eGrammar, bHasNmsp );
    }
}

void SAL_CALL ScXMLContentValidationContext::endFastElement( sal_Int32 /*nElement*/ )
{
    // #i36650# event-listeners element moved up one level
    if (xEventContext.is())
    {
        uno::Sequence<beans::PropertyValue> aValues;
        xEventContext->GetEventSequence( u"OnError"_ustr, aValues );

        auto pValue = std::find_if(std::cbegin(aValues), std::cend(aValues),
            [](const beans::PropertyValue& rValue) {
                return rValue.Name == "MacroName" || rValue.Name == "Script"; });
        if (pValue != std::cend(aValues))
            pValue->Value >>= sErrorTitle;
    }

    ScMyImportValidation aValidation;
    if (ScDocument* pDoc = GetScImport().GetDocument())
        aValidation.eGrammar1 = aValidation.eGrammar2 = pDoc->GetStorageGrammar();
    aValidation.sName = sName;
    aValidation.sBaseCellAddress = sBaseCellAddress;
    aValidation.sInputTitle = sHelpTitle;
    aValidation.sInputMessage = sHelpMessage;
    aValidation.sErrorTitle = sErrorTitle;
    aValidation.sErrorMessage = sErrorMessage;
    GetCondition( aValidation );
    aValidation.aAlertStyle = GetAlertStyle();
    aValidation.bShowErrorMessage = bDisplayError;
    aValidation.bShowInputMessage = bDisplayHelp;
    aValidation.bIgnoreBlanks = bAllowEmptyCell;
    aValidation.bCaseSensitive = bIsCaseSensitive;
    aValidation.nShowList = nShowList;
    GetScImport().AddValidation(aValidation);
}

void ScXMLContentValidationContext::SetHelpMessage(const OUString& sTitle, const OUString& sMessage, const bool bDisplay)
{
    sHelpTitle = sTitle;
    sHelpMessage = sMessage;
    bDisplayHelp = bDisplay;
}

void ScXMLContentValidationContext::SetErrorMessage(const OUString& sTitle, const OUString& sMessage,
    const OUString& sMessageType, const bool bDisplay)
{
    sErrorTitle = sTitle;
    sErrorMessage = sMessage;
    sErrorMessageType = sMessageType;
    bDisplayError = bDisplay;
}

void ScXMLContentValidationContext::SetErrorMacro(const bool bExecute)
{
    sErrorMessageType = "macro";
    bDisplayError = bExecute;
}

ScXMLHelpMessageContext::ScXMLHelpMessageContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLContentValidationContext* pTempValidationContext) :
    ScXMLImportContext( rImport ),
    nParagraphCount(0),
    bDisplay(false)
{
    pValidationContext = pTempValidationContext;
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_TITLE ):
            sTitle = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_DISPLAY ):
            bDisplay = IsXMLToken(aIter, XML_TRUE);
            break;
        }
    }
}

css::uno::Reference< css::xml::sax::XFastContextHandler > ScXMLHelpMessageContext::createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& /*xAttrList*/ )
{
    SvXMLImportContext *pContext = nullptr;

    switch( nElement )
    {
        case XML_ELEMENT(TEXT, XML_P):
        {
            if(nParagraphCount)
                sMessage.append('\n');
            ++nParagraphCount;
            pContext = new ScXMLContentContext( GetScImport(), sMessage );
        }
        break;
    }

    return pContext;
}

void SAL_CALL ScXMLHelpMessageContext::endFastElement( sal_Int32 /*nElement*/ )
{
    pValidationContext->SetHelpMessage(sTitle, sMessage.makeStringAndClear(), bDisplay);
}

ScXMLErrorMessageContext::ScXMLErrorMessageContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLContentValidationContext* pTempValidationContext) :
    ScXMLImportContext( rImport ),
    nParagraphCount(0),
    bDisplay(false)
{
    pValidationContext = pTempValidationContext;
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_TITLE ):
            sTitle = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_MESSAGE_TYPE ):
            sMessageType = aIter.toString();
            break;
        case XML_ELEMENT( TABLE, XML_DISPLAY ):
            bDisplay = IsXMLToken(aIter, XML_TRUE);
            break;
        }
    }
}

css::uno::Reference< css::xml::sax::XFastContextHandler > ScXMLErrorMessageContext::createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& /*xAttrList*/ )
{
    SvXMLImportContext *pContext = nullptr;

    switch( nElement )
    {
        case XML_ELEMENT(TEXT, XML_P):
        {
            if(nParagraphCount)
                sMessage.append('\n');
            ++nParagraphCount;
            pContext = new ScXMLContentContext( GetScImport(), sMessage);
        }
        break;
    }

    return pContext;
}

void SAL_CALL ScXMLErrorMessageContext::endFastElement( sal_Int32 /*nElement*/ )
{
    pValidationContext->SetErrorMessage(sTitle, sMessage.makeStringAndClear(), sMessageType, bDisplay);
}

ScXMLErrorMacroContext::ScXMLErrorMacroContext( ScXMLImport& rImport,
                                      const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList,
                                      ScXMLContentValidationContext* pTempValidationContext) :
    ScXMLImportContext( rImport ),
    bExecute(false)
{
    pValidationContext = pTempValidationContext;
    if ( !rAttrList.is() )
        return;

    for (auto &aIter : *rAttrList)
    {
        switch (aIter.getToken())
        {
        case XML_ELEMENT( TABLE, XML_NAME ):
            break;
        case XML_ELEMENT( TABLE, XML_EXECUTE ):
            bExecute = IsXMLToken(aIter, XML_TRUE);
            break;
        }
    }
}

css::uno::Reference< css::xml::sax::XFastContextHandler >  ScXMLErrorMacroContext::createFastChildContext(
    sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& /*xAttrList*/ )
{
    SvXMLImportContext *pContext = nullptr;

    if (nElement == XML_ELEMENT(SCRIPT, XML_EVENTS))
    {
        pContext = new XMLEventsImportContext(GetImport());
    }

    return pContext;
}

void SAL_CALL ScXMLErrorMacroContext::endFastElement( sal_Int32 /*nElement*/ )
{
    pValidationContext->SetErrorMacro( bExecute );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

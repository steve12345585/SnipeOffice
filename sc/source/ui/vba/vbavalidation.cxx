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

#include "vbavalidation.hxx"
#include "vbaformatcondition.hxx"
#include <com/sun/star/sheet/XSheetCondition.hpp>
#include <com/sun/star/sheet/ValidationType.hpp>
#include <com/sun/star/sheet/ValidationAlertStyle.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <ooo/vba/excel/XlDVType.hpp>
#include <ooo/vba/excel/XlDVAlertStyle.hpp>

#include <unonames.hxx>
#include <rangelst.hxx>
#include "excelvbahelper.hxx"
#include "vbarange.hxx"

using namespace ::ooo::vba;
using namespace ::com::sun::star;

static void
lcl_setValidationProps( const uno::Reference< table::XCellRange >& xRange, const uno::Reference< beans::XPropertySet >& xProps )
{
    uno::Reference< beans::XPropertySet > xRangeProps( xRange, uno::UNO_QUERY_THROW );
    xRangeProps->setPropertyValue( SC_UNONAME_VALIDAT , uno::Any( xProps ) );
}

static uno::Reference< beans::XPropertySet >
lcl_getValidationProps( const uno::Reference< table::XCellRange >& xRange )
{
    uno::Reference< beans::XPropertySet > xProps( xRange, uno::UNO_QUERY_THROW );
    uno::Reference< beans::XPropertySet > xValProps;
    xValProps.set( xProps->getPropertyValue( SC_UNONAME_VALIDAT ), uno::UNO_QUERY_THROW );
    return xValProps;
}

sal_Bool SAL_CALL
ScVbaValidation::getIgnoreBlank()
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    bool bBlank = false;
    xProps->getPropertyValue( SC_UNONAME_IGNOREBL )  >>= bBlank;
    return bBlank;
}

void SAL_CALL
ScVbaValidation::setIgnoreBlank( sal_Bool _ignoreblank )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_IGNOREBL, uno::Any( _ignoreblank ) );
    lcl_setValidationProps( m_xRange, xProps );
}

sal_Bool SAL_CALL
ScVbaValidation::getCaseSensitive()
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    bool bCase = false;
    xProps->getPropertyValue( SC_UNONAME_ISCASE )  >>= bCase;
    return bCase;
}

void SAL_CALL
ScVbaValidation::setCaseSensitive( sal_Bool _bCase )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_ISCASE, uno::Any( _bCase ) );
    lcl_setValidationProps( m_xRange, xProps );
}

sal_Bool SAL_CALL
ScVbaValidation::getInCellDropdown()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    sal_Int32 nShowList = 0;
    xProps->getPropertyValue( SC_UNONAME_SHOWLIST )  >>= nShowList;
    return nShowList != 0;
}

void SAL_CALL
ScVbaValidation::setInCellDropdown( sal_Bool  _incelldropdown  )
{
    sal_Int32 nDropDown = 0;
    if ( _incelldropdown )
        nDropDown = 1;
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps(m_xRange) );
    xProps->setPropertyValue( SC_UNONAME_SHOWLIST, uno::Any( nDropDown ) );
    lcl_setValidationProps( m_xRange, xProps );
}

sal_Bool SAL_CALL
ScVbaValidation::getShowInput()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    bool bShowInput = false;
    xProps->getPropertyValue( SC_UNONAME_SHOWINP )  >>= bShowInput;
    return bShowInput;
}

void SAL_CALL
ScVbaValidation:: setShowInput( sal_Bool _showinput )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps(m_xRange) );
    xProps->setPropertyValue( SC_UNONAME_IGNOREBL, uno::Any( _showinput ) );
    lcl_setValidationProps( m_xRange, xProps );
}

sal_Bool SAL_CALL
ScVbaValidation::getShowError()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    bool bShowError = false;
    xProps->getPropertyValue( SC_UNONAME_SHOWERR )  >>= bShowError;
    return bShowError;
}

void SAL_CALL
ScVbaValidation::setShowError( sal_Bool _showerror )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_SHOWERR, uno::Any( _showerror ) );
    lcl_setValidationProps( m_xRange, xProps );
}

OUString SAL_CALL
ScVbaValidation::getErrorTitle()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    OUString sErrorTitle;
    xProps->getPropertyValue( SC_UNONAME_ERRTITLE )  >>= sErrorTitle;
    return sErrorTitle;
}

void
ScVbaValidation::setErrorTitle( const OUString& _errormessage )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_ERRTITLE, uno::Any( _errormessage ) );
    lcl_setValidationProps( m_xRange, xProps );
}

OUString SAL_CALL
ScVbaValidation::getInputMessage()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    OUString sMsg;
    xProps->getPropertyValue( SC_UNONAME_INPMESS )  >>= sMsg;
    return sMsg;
}

void SAL_CALL
ScVbaValidation::setInputMessage( const OUString& _inputmessage )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_INPMESS, uno::Any( _inputmessage ) );
    lcl_setValidationProps( m_xRange, xProps );
}

OUString SAL_CALL
ScVbaValidation::getInputTitle()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    OUString sString;
    xProps->getPropertyValue( SC_UNONAME_INPTITLE )  >>= sString;
    return sString;
}

void SAL_CALL
ScVbaValidation::setInputTitle( const OUString& _inputtitle )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_INPTITLE, uno::Any( _inputtitle ) );
    lcl_setValidationProps( m_xRange, xProps );
}

OUString SAL_CALL
ScVbaValidation::getErrorMessage()
{
    uno::Reference< beans::XPropertySet > xProps = lcl_getValidationProps( m_xRange );
    OUString sString;
    xProps->getPropertyValue( SC_UNONAME_ERRMESS )  >>= sString;
    return sString;
}

void SAL_CALL
ScVbaValidation::setErrorMessage( const OUString& _errormessage )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    xProps->setPropertyValue( SC_UNONAME_ERRMESS, uno::Any( _errormessage ) );
    lcl_setValidationProps( m_xRange, xProps );
}

void SAL_CALL
ScVbaValidation::Delete(  )
{
    OUString sBlank;
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    uno::Reference< sheet::XSheetCondition > xCond( xProps, uno::UNO_QUERY_THROW );
    xProps->setPropertyValue( SC_UNONAME_IGNOREBL, uno::Any( true ) );
    xProps->setPropertyValue( SC_UNONAME_ISCASE, uno::Any( false ) );
    xProps->setPropertyValue( SC_UNONAME_SHOWINP, uno::Any( true ) );
    xProps->setPropertyValue( SC_UNONAME_SHOWERR, uno::Any( true ) );
    xProps->setPropertyValue( SC_UNONAME_ERRTITLE, uno::Any( sBlank ) );
    xProps->setPropertyValue( SC_UNONAME_INPMESS, uno::Any( sBlank) );
    xProps->setPropertyValue( SC_UNONAME_ERRALSTY, uno::Any( sheet::ValidationAlertStyle_STOP) );
    xProps->setPropertyValue( SC_UNONAME_TYPE, uno::Any( sheet::ValidationType_ANY ) );
    xCond->setFormula1( sBlank );
    xCond->setFormula2( sBlank );
    xCond->setOperator( sheet::ConditionOperator_NONE );

    lcl_setValidationProps( m_xRange, xProps );
}

// Fix the defect that validation cannot work when the input should be limited between a lower bound and an upper bound
void SAL_CALL
ScVbaValidation::Add( const uno::Any& Type, const uno::Any& AlertStyle, const uno::Any& Operator, const uno::Any& Formula1, const uno::Any& Formula2 )
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    uno::Reference< sheet::XSheetCondition > xCond( xProps, uno::UNO_QUERY_THROW );

    sheet::ValidationType nValType = sheet::ValidationType_ANY;
    xProps->getPropertyValue( SC_UNONAME_TYPE )  >>= nValType;
    if ( nValType  != sheet::ValidationType_ANY  )
        throw uno::RuntimeException(u"validation object already exists"_ustr );
    sal_Int32 nType = -1;
    if ( !Type.hasValue()  || !( Type >>= nType ) )
        throw uno::RuntimeException(u"missing required param"_ustr );

    Delete(); // set up defaults
    OUString sFormula1;
    Formula1 >>= sFormula1;
    OUString sFormula2;
    Formula2 >>= sFormula2;
    switch ( nType )
    {
        case excel::XlDVType::xlValidateList:
            {
                // for validate list
                // at least formula1 is required
                if ( !Formula1.hasValue() )
                    throw uno::RuntimeException(u"missing param"_ustr );
                nValType = sheet::ValidationType_LIST;
                xProps->setPropertyValue( SC_UNONAME_TYPE, uno::Any(nValType ));
                // #TODO validate required params
                // #TODO need to correct the ';' delimited formula on get/set
                break;
            }
        case excel::XlDVType::xlValidateWholeNumber:
            nValType = sheet::ValidationType_WHOLE;
            xProps->setPropertyValue( SC_UNONAME_TYPE, uno::Any(nValType ));
            break;
        default:
            throw uno::RuntimeException(u"unsupported operation..."_ustr );
    }

    sheet::ValidationAlertStyle eStyle = sheet::ValidationAlertStyle_STOP;
    sal_Int32 nVbaAlertStyle = excel::XlDVAlertStyle::xlValidAlertStop;
    if ( AlertStyle.hasValue() && ( AlertStyle >>= nVbaAlertStyle ) )
    {
        switch( nVbaAlertStyle )
        {
            case excel::XlDVAlertStyle::xlValidAlertStop:
                // yes I know it's already defaulted but safer to assume
                // someone probably could change the code above
                eStyle = sheet::ValidationAlertStyle_STOP;
                break;
            case excel::XlDVAlertStyle::xlValidAlertWarning:
                eStyle = sheet::ValidationAlertStyle_WARNING;
                break;
            case excel::XlDVAlertStyle::xlValidAlertInformation:
                eStyle = sheet::ValidationAlertStyle_INFO;
                break;
            default:
            throw uno::RuntimeException(u"bad param..."_ustr );

        }
    }

    xProps->setPropertyValue( SC_UNONAME_ERRALSTY, uno::Any( eStyle ) );

    // i#108860: fix the defect that validation cannot work when the input
    // should be limited between a lower bound and an upper bound
    if ( Operator.hasValue() )
    {
        css::sheet::ConditionOperator conOperator = ScVbaFormatCondition::retrieveAPIOperator( Operator );
        xCond->setOperator( conOperator );
    }

    if ( !sFormula1.isEmpty() )
        xCond->setFormula1( sFormula1 );
    if ( !sFormula2.isEmpty() )
        xCond->setFormula2( sFormula2 );

    lcl_setValidationProps( m_xRange, xProps );
}

OUString SAL_CALL
ScVbaValidation::getFormula1()
{
    uno::Reference< sheet::XSheetCondition > xCond( lcl_getValidationProps( m_xRange ), uno::UNO_QUERY_THROW );
    OUString sString = xCond->getFormula1();

    ScRefFlags nFlags = ScRefFlags::ZERO;
    ScRangeList aCellRanges;

    ScDocShell* pDocSh = excel::GetDocShellFromRange( m_xRange );
    // in calc validation formula is either a range or formula
    // that results in range.
    // In VBA both formula and address can have a leading '='
    // in result of getFormula1, however it *seems* that a named range or
    // real formula has to (or is expected to) have the '='
    if ( pDocSh && !ScVbaRange::getCellRangesForAddress(  nFlags, sString, pDocSh, aCellRanges, formula::FormulaGrammar::CONV_XL_A1, 0 ) )
        sString = "=" + sString;
    return sString;
}

OUString SAL_CALL
ScVbaValidation::getFormula2()
{
    uno::Reference< sheet::XSheetCondition > xCond( lcl_getValidationProps( m_xRange ), uno::UNO_QUERY_THROW );
    return xCond->getFormula2();
}

sal_Int32 SAL_CALL
ScVbaValidation::getType()
{
    uno::Reference< beans::XPropertySet > xProps( lcl_getValidationProps( m_xRange ) );
    sheet::ValidationType nValType = sheet::ValidationType_ANY;
    xProps->getPropertyValue( SC_UNONAME_TYPE )  >>= nValType;
    sal_Int32 nExcelType = excel::XlDVType::xlValidateList; // pick a default
    if ( xProps.is() )
    {
        switch ( nValType )
        {
            case sheet::ValidationType_LIST:
                nExcelType = excel::XlDVType::xlValidateList;
                break;
            case sheet::ValidationType_ANY: // not ANY not really a great match for anything I fear:-(
                nExcelType = excel::XlDVType::xlValidateInputOnly;
                break;
            case sheet::ValidationType_CUSTOM:
                nExcelType = excel::XlDVType::xlValidateCustom;
                break;
            case sheet::ValidationType_WHOLE:
                nExcelType = excel::XlDVType::xlValidateWholeNumber;
                break;
            case sheet::ValidationType_DECIMAL:
                nExcelType = excel::XlDVType::xlValidateDecimal;
                break;
            case sheet::ValidationType_DATE:
                nExcelType = excel::XlDVType::xlValidateDate;
                break;
            case sheet::ValidationType_TIME:
                nExcelType = excel::XlDVType::xlValidateTime;
                break;
            case sheet::ValidationType_TEXT_LEN:
                nExcelType = excel::XlDVType::xlValidateTextLength;
                break;
            case sheet::ValidationType::ValidationType_MAKE_FIXED_SIZE:
            default:
                break;
        }
    }
    return nExcelType;
}

OUString
ScVbaValidation::getServiceImplName()
{
    return u"ScVbaValidation"_ustr;
}

uno::Sequence< OUString >
ScVbaValidation::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.excel.Validation"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

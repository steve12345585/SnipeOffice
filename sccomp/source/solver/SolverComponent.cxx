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

#include "SolverComponent.hxx"
#include <strings.hrc>

#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/table/CellAddress.hpp>

#include <cppuhelper/supportsservice.hxx>

#include <unotools/resmgr.hxx>

using namespace com::sun::star;


constexpr OUStringLiteral STR_NONNEGATIVE = u"NonNegative";
constexpr OUStringLiteral STR_INTEGER = u"Integer";
constexpr OUStringLiteral STR_TIMEOUT = u"Timeout";
constexpr OUStringLiteral STR_EPSILONLEVEL = u"EpsilonLevel";
constexpr OUStringLiteral STR_LIMITBBDEPTH = u"LimitBBDepth";
constexpr OUStringLiteral STR_GEN_SENSITIVITY = u"GenSensitivityReport";
constexpr OUStringLiteral STR_SENSITIVITY_REPORT = u"SensitivityReport";


//  Resources from tools are used for translated strings

OUString SolverComponent::GetResourceString(TranslateId aId)
{
    return Translate::get(aId, Translate::Create("scc"));
}

size_t ScSolverCellHash::operator()( const css::table::CellAddress& rAddress ) const
{
    return ( rAddress.Sheet << 24 ) | ( rAddress.Column << 16 ) | rAddress.Row;
}

bool ScSolverCellEqual::operator()( const css::table::CellAddress& rAddr1, const css::table::CellAddress& rAddr2 ) const
{
    return AddressEqual( rAddr1, rAddr2 );
}

namespace
{
    enum
    {
        PROP_NONNEGATIVE,
        PROP_INTEGER,
        PROP_TIMEOUT,
        PROP_EPSILONLEVEL,
        PROP_LIMITBBDEPTH,
        PROP_GEN_SENSITIVITY,
        PROP_SENSITIVITY_REPORT
    };
}

uno::Reference<table::XCell> SolverComponent::GetCell( const uno::Reference<sheet::XSpreadsheetDocument>& xDoc,
                                          const table::CellAddress& rPos )
{
    uno::Reference<container::XIndexAccess> xSheets( xDoc->getSheets(), uno::UNO_QUERY );
    uno::Reference<sheet::XSpreadsheet> xSheet( xSheets->getByIndex( rPos.Sheet ), uno::UNO_QUERY );
    return xSheet->getCellByPosition( rPos.Column, rPos.Row );
}

void SolverComponent::SetValue( const uno::Reference<sheet::XSpreadsheetDocument>& xDoc,
                   const table::CellAddress& rPos, double fValue )
{
    SolverComponent::GetCell( xDoc, rPos )->setValue( fValue );
}

double SolverComponent::GetValue( const uno::Reference<sheet::XSpreadsheetDocument>& xDoc,
                     const table::CellAddress& rPos )
{
    return SolverComponent::GetCell( xDoc, rPos )->getValue();
}

SolverComponent::SolverComponent() :
    mbMaximize( true ),
    mbNonNegative( false ),
    mbInteger( false ),
    mnTimeout( 100 ),
    mnEpsilonLevel( 0 ),
    mbLimitBBDepth( true ),
    mbGenSensitivity(false),
    mbSuccess( false ),
    mfResultValue( 0.0 )
{
    // for XPropertySet implementation:
    registerProperty(STR_NONNEGATIVE,  PROP_NONNEGATIVE,  0, &mbNonNegative,    cppu::UnoType<decltype(mbNonNegative)>::get());
    registerProperty(STR_INTEGER,      PROP_INTEGER,      0, &mbInteger,        cppu::UnoType<decltype(mbInteger)>::get());
    registerProperty(STR_TIMEOUT,      PROP_TIMEOUT,      0, &mnTimeout,        cppu::UnoType<decltype(mnTimeout)>::get());
    registerProperty(STR_EPSILONLEVEL, PROP_EPSILONLEVEL, 0, &mnEpsilonLevel,   cppu::UnoType<decltype(mnEpsilonLevel)>::get());
    registerProperty(STR_LIMITBBDEPTH, PROP_LIMITBBDEPTH, 0, &mbLimitBBDepth,   cppu::UnoType<decltype(mbLimitBBDepth)>::get());
    registerProperty(STR_GEN_SENSITIVITY, PROP_GEN_SENSITIVITY, 0, &mbGenSensitivity, cppu::UnoType<decltype(mbGenSensitivity)>::get());

    // Sensitivity report
    registerProperty(STR_SENSITIVITY_REPORT, PROP_SENSITIVITY_REPORT, 0, &m_aSensitivityReport, cppu::UnoType<decltype(m_aSensitivityReport)>::get());
}

SolverComponent::~SolverComponent()
{
}

IMPLEMENT_FORWARD_XINTERFACE2( SolverComponent, SolverComponent_Base, comphelper::OPropertyContainer2 )
IMPLEMENT_FORWARD_XTYPEPROVIDER2( SolverComponent, SolverComponent_Base, comphelper::OPropertyContainer2 )

cppu::IPropertyArrayHelper* SolverComponent::createArrayHelper() const
{
    uno::Sequence<beans::Property> aProps;
    describeProperties( aProps );
    return new cppu::OPropertyArrayHelper( aProps );
}

cppu::IPropertyArrayHelper& SolverComponent::getInfoHelper()
{
    return *getArrayHelper();
}

uno::Reference<beans::XPropertySetInfo> SAL_CALL SolverComponent::getPropertySetInfo()
{
    return createPropertySetInfo( getInfoHelper() );
}

// XSolverDescription

OUString SAL_CALL SolverComponent::getStatusDescription()
{
    return maStatus;
}

OUString SAL_CALL SolverComponent::getPropertyDescription( const OUString& rPropertyName )
{
    TranslateId pResId;
    sal_Int32 nHandle = getInfoHelper().getHandleByName( rPropertyName );
    switch (nHandle)
    {
        case PROP_NONNEGATIVE:
            pResId = RID_PROPERTY_NONNEGATIVE;
            break;
        case PROP_INTEGER:
            pResId = RID_PROPERTY_INTEGER;
            break;
        case PROP_TIMEOUT:
            pResId = RID_PROPERTY_TIMEOUT;
            break;
        case PROP_EPSILONLEVEL:
            pResId = RID_PROPERTY_EPSILONLEVEL;
            break;
        case PROP_LIMITBBDEPTH:
            pResId = RID_PROPERTY_LIMITBBDEPTH;
            break;
        case PROP_GEN_SENSITIVITY:
            pResId = RID_PROPERTY_SENSITIVITY;
            break;
        default:
            {
                // unknown - leave empty
            }
    }
    OUString aRet;
    if (pResId)
        aRet = SolverComponent::GetResourceString(pResId);
    return aRet;
}

// XSolver: settings

uno::Reference<sheet::XSpreadsheetDocument> SAL_CALL SolverComponent::getDocument()
{
    return mxDoc;
}

void SAL_CALL SolverComponent::setDocument( const uno::Reference<sheet::XSpreadsheetDocument>& _document )
{
    mxDoc = _document;
}

table::CellAddress SAL_CALL SolverComponent::getObjective()
{
    return maObjective;
}

void SAL_CALL SolverComponent::setObjective( const table::CellAddress& _objective )
{
    maObjective = _objective;
}

uno::Sequence<table::CellAddress> SAL_CALL SolverComponent::getVariables()
{
    return maVariables;
}

void SAL_CALL SolverComponent::setVariables( const uno::Sequence<table::CellAddress>& _variables )
{
    maVariables = _variables;
}

uno::Sequence<sheet::SolverConstraint> SAL_CALL SolverComponent::getConstraints()
{
    return maConstraints;
}

void SAL_CALL SolverComponent::setConstraints( const uno::Sequence<sheet::SolverConstraint>& _constraints )
{
    maConstraints = _constraints;
}

sal_Bool SAL_CALL SolverComponent::getMaximize()
{
    return mbMaximize;
}

void SAL_CALL SolverComponent::setMaximize( sal_Bool _maximize )
{
    mbMaximize = _maximize;
}

// XSolver: get results

sal_Bool SAL_CALL SolverComponent::getSuccess()
{
    return mbSuccess;
}

double SAL_CALL SolverComponent::getResultValue()
{
    return mfResultValue;
}

uno::Sequence<double> SAL_CALL SolverComponent::getSolution()
{
    return maSolution;
}

// XServiceInfo

sal_Bool SAL_CALL SolverComponent::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService(this, rServiceName);
}

uno::Sequence<OUString> SAL_CALL SolverComponent::getSupportedServiceNames()
{
    return { u"com.sun.star.sheet.Solver"_ustr };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

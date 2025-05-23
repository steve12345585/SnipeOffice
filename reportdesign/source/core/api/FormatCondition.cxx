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
#include <FormatCondition.hxx>
#include <strings.hxx>
#include <tools/color.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <ReportHelperImpl.hxx>

namespace reportdesign
{

    using namespace com::sun::star;

uno::Reference< uno::XInterface > OFormatCondition::create(uno::Reference< uno::XComponentContext > const & xContext)
{
    return *(new OFormatCondition(xContext));
}


OFormatCondition::OFormatCondition(uno::Reference< uno::XComponentContext > const & _xContext)
:FormatConditionBase(m_aMutex)
,FormatConditionPropertySet(_xContext,IMPLEMENTS_PROPERTY_SET,uno::Sequence< OUString >())
,m_bEnabled(true)
{
}

OFormatCondition::~OFormatCondition()
{
}

IMPLEMENT_FORWARD_XINTERFACE2(OFormatCondition,FormatConditionBase,FormatConditionPropertySet)

void SAL_CALL OFormatCondition::dispose()
{
    FormatConditionPropertySet::dispose();
    cppu::WeakComponentImplHelperBase::dispose();
}

OUString OFormatCondition::getImplementationName_Static(  )
{
    return u"com.sun.star.comp.report.OFormatCondition"_ustr;
}


OUString SAL_CALL OFormatCondition::getImplementationName(  )
{
    return getImplementationName_Static();
}

uno::Sequence< OUString > OFormatCondition::getSupportedServiceNames_Static(  )
{
    uno::Sequence< OUString > aServices { SERVICE_FORMATCONDITION };

    return aServices;
}

uno::Sequence< OUString > SAL_CALL OFormatCondition::getSupportedServiceNames(  )
{
    return getSupportedServiceNames_Static();
}

sal_Bool SAL_CALL OFormatCondition::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

uno::Reference< beans::XPropertySetInfo > SAL_CALL OFormatCondition::getPropertySetInfo(  )
{
    return FormatConditionPropertySet::getPropertySetInfo();
}

void SAL_CALL OFormatCondition::setPropertyValue( const OUString& aPropertyName, const uno::Any& aValue )
{
    FormatConditionPropertySet::setPropertyValue( aPropertyName, aValue );
}

uno::Any SAL_CALL OFormatCondition::getPropertyValue( const OUString& PropertyName )
{
    return FormatConditionPropertySet::getPropertyValue( PropertyName);
}

void SAL_CALL OFormatCondition::addPropertyChangeListener( const OUString& aPropertyName, const uno::Reference< beans::XPropertyChangeListener >& xListener )
{
    FormatConditionPropertySet::addPropertyChangeListener( aPropertyName, xListener );
}

void SAL_CALL OFormatCondition::removePropertyChangeListener( const OUString& aPropertyName, const uno::Reference< beans::XPropertyChangeListener >& aListener )
{
    FormatConditionPropertySet::removePropertyChangeListener( aPropertyName, aListener );
}

void SAL_CALL OFormatCondition::addVetoableChangeListener( const OUString& PropertyName, const uno::Reference< beans::XVetoableChangeListener >& aListener )
{
    FormatConditionPropertySet::addVetoableChangeListener( PropertyName, aListener );
}

void SAL_CALL OFormatCondition::removeVetoableChangeListener( const OUString& PropertyName, const uno::Reference< beans::XVetoableChangeListener >& aListener )
{
    FormatConditionPropertySet::removeVetoableChangeListener( PropertyName, aListener );
}

// XFormatCondition
sal_Bool SAL_CALL OFormatCondition::getEnabled()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_bEnabled;
}

void SAL_CALL OFormatCondition::setEnabled( sal_Bool _enabled )
{
    set(PROPERTY_ENABLED,_enabled,m_bEnabled);
}

OUString SAL_CALL OFormatCondition::getFormula()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_sFormula;
}

void SAL_CALL OFormatCondition::setFormula( const OUString& _formula )
{
    set(PROPERTY_FORMULA,_formula,m_sFormula);
}

// XReportControlFormat
REPORTCONTROLFORMAT_IMPL(OFormatCondition,m_aFormatProperties)

} // namespace reportdesign


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

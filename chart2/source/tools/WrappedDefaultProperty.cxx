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

#include <WrappedDefaultProperty.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyState.hpp>
#include <utility>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;

namespace chart
{

WrappedDefaultProperty::WrappedDefaultProperty(
    const OUString& rOuterName, const OUString& rInnerName,
    uno::Any aNewOuterDefault ) :
        WrappedProperty( rOuterName, rInnerName ),
        m_aOuterDefaultValue(std::move( aNewOuterDefault ))
{}

WrappedDefaultProperty::~WrappedDefaultProperty()
{}

void WrappedDefaultProperty::setPropertyToDefault(
    const Reference< beans::XPropertyState >& xInnerPropertyState ) const
{
    Reference< beans::XPropertySet > xInnerPropSet( xInnerPropertyState, uno::UNO_QUERY );
    if( xInnerPropSet.is())
        setPropertyValue( m_aOuterDefaultValue, xInnerPropSet );
}

uno::Any WrappedDefaultProperty::getPropertyDefault(
    const Reference< beans::XPropertyState >& /* xInnerPropertyState */ ) const
{
    return m_aOuterDefaultValue;
}

beans::PropertyState WrappedDefaultProperty::getPropertyState(
    const Reference< beans::XPropertyState >& xInnerPropertyState ) const
{
    beans::PropertyState aState = beans::PropertyState_DIRECT_VALUE;
    try
    {
        Reference< beans::XPropertySet > xInnerProp( xInnerPropertyState, uno::UNO_QUERY_THROW );
        uno::Any aValue = getPropertyValue( xInnerProp );
        if( m_aOuterDefaultValue == convertInnerToOuterValue( aValue ))
            aState = beans::PropertyState_DEFAULT_VALUE;
    }
    catch( const beans::UnknownPropertyException& )
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
    return aState;
}

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

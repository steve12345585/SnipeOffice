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

#include "atkwrapper.hxx"

#include <com/sun/star/accessibility/XAccessibleValue.hpp>

#include <cmath>
#include <string.h>

using namespace ::com::sun::star;

/// @throws uno::RuntimeException
static css::uno::Reference<css::accessibility::XAccessibleValue>
    getValue( AtkValue *pValue )
{
    AtkObjectWrapper *pWrap = ATK_OBJECT_WRAPPER( pValue );
    if( pWrap )
    {
        if( !pWrap->mpValue.is() )
        {
            pWrap->mpValue.set(pWrap->mpContext, css::uno::UNO_QUERY);
        }

        return pWrap->mpValue;
    }

    return css::uno::Reference<css::accessibility::XAccessibleValue>();
}

static void anyToGValue( const uno::Any& aAny, GValue *pValue )
{
    // FIXME: expand to lots of types etc.
    double aDouble=0;
    aAny >>= aDouble;

    memset( pValue,  0, sizeof( GValue ) );
    g_value_init( pValue, G_TYPE_DOUBLE );
    g_value_set_double( pValue, aDouble );
}

extern "C" {

static void
value_wrapper_get_current_value( AtkValue *value,
                                 GValue   *gval )
{
    try {
        css::uno::Reference<css::accessibility::XAccessibleValue> pValue
            = getValue( value );
        if( pValue.is() )
            anyToGValue( pValue->getCurrentValue(), gval );
    }
    catch(const uno::Exception&) {
        g_warning( "Exception in getCurrentValue()" );
    }
}

static void
value_wrapper_get_maximum_value( AtkValue *value,
                                 GValue   *gval )
{
    try {
        css::uno::Reference<css::accessibility::XAccessibleValue> pValue
            = getValue( value );
        if( pValue.is() )
            anyToGValue( pValue->getMaximumValue(), gval );
    }
    catch(const uno::Exception&) {
        g_warning( "Exception in getCurrentValue()" );
    }
}

static void
value_wrapper_get_minimum_value( AtkValue *value,
                                 GValue   *gval )
{
    try {
        css::uno::Reference<css::accessibility::XAccessibleValue> pValue
            = getValue( value );
        if( pValue.is() )
            anyToGValue( pValue->getMinimumValue(), gval );
    }
    catch(const uno::Exception&) {
        g_warning( "Exception in getCurrentValue()" );
    }
}

static gboolean
value_wrapper_set_current_value( AtkValue     *value,
                                 const GValue *gval )
{
    try {
        css::uno::Reference<css::accessibility::XAccessibleValue> pValue
            = getValue( value );
        if( pValue.is() )
        {
            double aDouble = g_value_get_double( gval );

            // Different types of numerical values for XAccessibleValue are possible.
            // If current value has an integer type, also use that for the new value, to make
            // sure underlying implementations expecting that can handle the value properly.
            const css::uno::Any aCurrentValue = pValue->getCurrentValue();
            if (aCurrentValue.getValueTypeClass() == css::uno::TypeClass::TypeClass_LONG)
            {
                const sal_Int32 nValue = std::round<sal_Int32>(aDouble);
                return pValue->setCurrentValue(css::uno::Any(nValue));
            }
            else if (aCurrentValue.getValueTypeClass() == css::uno::TypeClass::TypeClass_HYPER)
            {
                const sal_Int64 nValue = std::round<sal_Int64>(aDouble);
                return pValue->setCurrentValue(css::uno::Any(nValue));
            }

            return pValue->setCurrentValue( uno::Any(aDouble) );
        }
    }
    catch(const uno::Exception&) {
        g_warning( "Exception in getCurrentValue()" );
    }

    return FALSE;
}

} // extern "C"

void
valueIfaceInit (gpointer iface_, gpointer)
{
  auto const iface = static_cast<AtkValueIface *>(iface_);
  g_return_if_fail (iface != nullptr);

  iface->get_current_value = value_wrapper_get_current_value;
  iface->get_maximum_value = value_wrapper_get_maximum_value;
  iface->get_minimum_value = value_wrapper_get_minimum_value;
  iface->set_current_value = value_wrapper_set_current_value;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

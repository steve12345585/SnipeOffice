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

#include "Time.hxx"
#include <property.hxx>
#include <services.hxx>
#include <connectivity/dbconversion.hxx>
#include <tools/debug.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/util/DateTime.hpp>
#include <com/sun/star/form/FormComponentType.hpp>

using namespace dbtools;

namespace frm
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::util;


//=

OTimeControl::OTimeControl(const Reference<XComponentContext>& _rxFactory)
               :OBoundControl(_rxFactory, VCL_CONTROL_TIMEFIELD)
{
}


Sequence<Type> OTimeControl::_getTypes()
{
    return OBoundControl::_getTypes();
}


css::uno::Sequence<OUString> SAL_CALL OTimeControl::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> aSupported = OBoundControl::getSupportedServiceNames();
    aSupported.realloc(aSupported.getLength() + 2);

    OUString*pArray = aSupported.getArray();
    pArray[aSupported.getLength()-2] = FRM_SUN_CONTROL_TIMEFIELD;
    pArray[aSupported.getLength()-1] = STARDIV_ONE_FORM_CONTROL_TIMEFIELD;
    return aSupported;
}


//= OTimeModel

// XServiceInfo

css::uno::Sequence<OUString> SAL_CALL OTimeModel::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> aSupported = OBoundControlModel::getSupportedServiceNames();

    sal_Int32 nOldLen = aSupported.getLength();
    aSupported.realloc( nOldLen + 9 );
    OUString* pStoreTo = aSupported.getArray() + nOldLen;

    *pStoreTo++ = BINDABLE_CONTROL_MODEL;
    *pStoreTo++ = DATA_AWARE_CONTROL_MODEL;
    *pStoreTo++ = VALIDATABLE_CONTROL_MODEL;

    *pStoreTo++ = BINDABLE_DATA_AWARE_CONTROL_MODEL;
    *pStoreTo++ = VALIDATABLE_BINDABLE_CONTROL_MODEL;

    *pStoreTo++ = FRM_SUN_COMPONENT_TIMEFIELD;
    *pStoreTo++ = FRM_SUN_COMPONENT_DATABASE_TIMEFIELD;
    *pStoreTo++ = BINDABLE_DATABASE_TIME_FIELD;

    *pStoreTo++ = FRM_COMPONENT_TIMEFIELD;

    return aSupported;
}


Sequence<Type> OTimeModel::_getTypes()
{
    return OBoundControlModel::_getTypes();
}


OTimeModel::OTimeModel(const Reference<XComponentContext>& _rxFactory)
    : OEditBaseModel(_rxFactory, VCL_CONTROLMODEL_TIMEFIELD,
        FRM_SUN_CONTROL_TIMEFIELD, true, true)
      // use the old control name for compatibility reasons
    , OLimitedFormats(_rxFactory, FormComponentType::TIMEFIELD)
    , m_bDateTimeField(false)
{
    m_nClassId = FormComponentType::TIMEFIELD;
    initValueProperty( PROPERTY_TIME, PROPERTY_ID_TIME );

    setAggregateSet(m_xAggregateFastSet, getOriginalHandle(PROPERTY_ID_TIMEFORMAT));
}


OTimeModel::OTimeModel(const OTimeModel* _pOriginal, const Reference<XComponentContext>& _rxFactory)
    : OEditBaseModel(_pOriginal, _rxFactory)
    , OLimitedFormats(_rxFactory, FormComponentType::TIMEFIELD)
    , m_bDateTimeField(false)
{
    setAggregateSet( m_xAggregateFastSet, getOriginalHandle( PROPERTY_ID_TIMEFORMAT ) );
}


OTimeModel::~OTimeModel( )
{
    setAggregateSet(Reference< XFastPropertySet >(), -1);
}

// XCloneable

css::uno::Reference< css::util::XCloneable > SAL_CALL OTimeModel::createClone()
{
    rtl::Reference<OTimeModel> pClone = new OTimeModel(this, getContext());
    pClone->clonedFrom(this);
    return pClone;
}


OUString SAL_CALL OTimeModel::getServiceName()
{
    return FRM_COMPONENT_TIMEFIELD; // old (non-sun) name for compatibility !
}

// XPropertySet

void OTimeModel::describeFixedProperties( Sequence< Property >& _rProps ) const
{
    OEditBaseModel::describeFixedProperties( _rProps );
    sal_Int32 nOldCount = _rProps.getLength();
    _rProps.realloc( nOldCount + 4);
    css::beans::Property* pProperties = _rProps.getArray() + nOldCount;
    *pProperties++ = css::beans::Property(PROPERTY_DEFAULT_TIME, PROPERTY_ID_DEFAULT_TIME, cppu::UnoType<util::Time>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_TABINDEX, PROPERTY_ID_TABINDEX, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_FORMATKEY, PROPERTY_ID_FORMATKEY, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::TRANSIENT);
    *pProperties++ = css::beans::Property(PROPERTY_FORMATSSUPPLIER, PROPERTY_ID_FORMATSSUPPLIER, cppu::UnoType<XNumberFormatsSupplier>::get(),
                                          css::beans::PropertyAttribute::READONLY | css::beans::PropertyAttribute::TRANSIENT);
    DBG_ASSERT( pProperties == _rProps.getArray() + _rProps.getLength(), "<...>::describeFixedProperties/getInfoHelper: forgot to adjust the count ?");
}


void SAL_CALL OTimeModel::getFastPropertyValue(Any& _rValue, sal_Int32 _nHandle ) const
{
    switch (_nHandle)
    {
        case PROPERTY_ID_FORMATKEY:
            getFormatKeyPropertyValue(_rValue);
            break;
        case PROPERTY_ID_FORMATSSUPPLIER:
            _rValue <<= getFormatsSupplier();
            break;
        default:
            OEditBaseModel::getFastPropertyValue(_rValue, _nHandle);
            break;
    }
}


sal_Bool SAL_CALL OTimeModel::convertFastPropertyValue(Any& _rConvertedValue, Any& _rOldValue,
        sal_Int32 _nHandle, const Any& _rValue )
{
    if (PROPERTY_ID_FORMATKEY == _nHandle)
        return convertFormatKeyPropertyValue(_rConvertedValue, _rOldValue, _rValue);
    else
        return OEditBaseModel::convertFastPropertyValue(_rConvertedValue, _rOldValue, _nHandle, _rValue );
}


void SAL_CALL OTimeModel::setFastPropertyValue_NoBroadcast(sal_Int32 _nHandle, const Any& _rValue)
{
    if (PROPERTY_ID_FORMATKEY == _nHandle)
        setFormatKeyPropertyValue(_rValue);
    else
        OEditBaseModel::setFastPropertyValue_NoBroadcast(_nHandle, _rValue);
}

// XLoadListener

void OTimeModel::onConnectedDbColumn( const Reference< XInterface >& _rxForm )
{
    OBoundControlModel::onConnectedDbColumn( _rxForm );
    Reference<XPropertySet> xField = getField();
    if (!xField.is())
        return;

    m_bDateTimeField = false;
    try
    {
        sal_Int32 nFieldType = 0;
        xField->getPropertyValue(PROPERTY_FIELDTYPE) >>= nFieldType;
        m_bDateTimeField = (nFieldType == DataType::TIMESTAMP);
    }
    catch(const Exception&)
    {
    }
}


bool OTimeModel::commitControlValueToDbColumn( bool /*_bPostReset*/ )
{
    Any aControlValue( m_xAggregateFastSet->getFastPropertyValue( getValuePropertyAggHandle() ) );
    if ( aControlValue == m_aSaveValue )
        return true;

    if ( !aControlValue.hasValue() )
        m_xColumnUpdate->updateNull();
    else
    {
        try
        {
            util::Time aTime;
            if ( !( aControlValue >>= aTime ) )
            {
                sal_Int64 nAsInt(0);
                aControlValue >>= nAsInt;
                aTime = DBTypeConversion::toTime(nAsInt);
            }

            if (!m_bDateTimeField)
                m_xColumnUpdate->updateTime(aTime);
            else
            {
                util::DateTime aDateTime = m_xColumn->getTimestamp();
                if (aDateTime.Year == 0 && aDateTime.Month == 0 && aDateTime.Day == 0)
                    aDateTime = css::util::DateTime(0,0,0,0,30,12,1899, false);
                aDateTime.NanoSeconds = aTime.NanoSeconds;
                aDateTime.Seconds = aTime.Seconds;
                aDateTime.Minutes = aTime.Minutes;
                aDateTime.Hours = aTime.Hours;
                m_xColumnUpdate->updateTimestamp(aDateTime);
            }
        }
        catch(const Exception&)
        {
            return false;
        }
    }
    m_aSaveValue = std::move(aControlValue);
    return true;
}


Any OTimeModel::translateControlValueToExternalValue( ) const
{
    return getControlValue();
}


Any OTimeModel::translateExternalValueToControlValue( const Any& _rExternalValue ) const
{
    return _rExternalValue;
}


Any OTimeModel::translateControlValueToValidatableValue( ) const
{
    return getControlValue();
}


Any OTimeModel::translateDbColumnToControlValue()
{
    util::Time aTime = m_xColumn->getTime();
    if ( m_xColumn->wasNull() )
        m_aSaveValue.clear();
    else
        m_aSaveValue <<= aTime;

    return m_aSaveValue;
}


Any OTimeModel::getDefaultForReset() const
{
    return m_aDefault;
}


void OTimeModel::resetNoBroadcast()
{
    OEditBaseModel::resetNoBroadcast();
    m_aSaveValue.clear();
}


Sequence< Type > OTimeModel::getSupportedBindingTypes()
{
    return Sequence< Type >( & cppu::UnoType<util::Time>::get(), 1 );
}

}   // namespace frm

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OTimeModel_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OTimeModel(component));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OTimeControl_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OTimeControl(component));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

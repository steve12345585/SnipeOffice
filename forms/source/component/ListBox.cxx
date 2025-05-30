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

#include <config_features.h>
#include <config_fuzzers.h>

#include "ListBox.hxx"
#include <property.hxx>
#include <services.hxx>
#include <frm_resource.hxx>
#include <strings.hrc>
#include "BaseListBox.hxx"
#include <componenttools.hxx>

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>

#include <comphelper/basicio.hxx>
#include <comphelper/property.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/string.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/formattedcolumnvalue.hxx>
#include <o3tl/any.hxx>
#include <o3tl/safeint.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>
#include <unotools/sharedunocomponent.hxx>

#include <optional>

#include <algorithm>
#include <iterator>
#include <climits>

namespace frm
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::form;
    using namespace ::com::sun::star::awt;
    using namespace ::com::sun::star::io;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::util;
    using namespace ::com::sun::star::form::binding;
    using namespace ::dbtools;

    using ::connectivity::ORowSetValue;

    const ::connectivity::ORowSetValue OListBoxModel::s_aEmptyValue;
    const ::connectivity::ORowSetValue OListBoxModel::s_aEmptyStringValue = OUString();

    //= helper

    namespace
    {

        struct RowSetValueToString
        {
            OUString operator()( const ORowSetValue& _value ) const
            {
                return _value.getString();
            }
        };


        struct AppendRowSetValueString
        {
            explicit AppendRowSetValueString( OUString& _string )
                :m_string( _string )
            {
            }

            void operator()( const ORowSetValue& _append )
            {
                m_string += _append.getString();
            }

        private:
            OUString&    m_string;
        };


        Sequence< OUString > lcl_convertToStringSequence( const ValueList& _values )
        {
            Sequence< OUString > aStrings( _values.size() );
            ::std::transform(
                _values.begin(),
                _values.end(),
                aStrings.getArray(),
                RowSetValueToString()
            );
            return aStrings;
        }
    }


    //= ItemEventDescription

    typedef ::comphelper::EventHolder< ItemEvent >    ItemEventDescription;


    //= OListBoxModel

    Sequence< Type> OListBoxModel::_getTypes()
    {
        return TypeBag(
            OBoundControlModel::_getTypes(),
            OEntryListHelper::getTypes(),
            OErrorBroadcaster::getTypes()
        ).getTypes();
    }

    // stuff common to all constructors
    void OListBoxModel::init()
    {
        startAggregatePropertyListening( PROPERTY_STRINGITEMLIST );
        startAggregatePropertyListening( PROPERTY_TYPEDITEMLIST );
    }


    OListBoxModel::OListBoxModel(const Reference<XComponentContext>& _rxFactory)
        :OBoundControlModel( _rxFactory, VCL_CONTROLMODEL_LISTBOX, FRM_SUN_CONTROL_LISTBOX, true, true, true )
        // use the old control name for compatibility reasons
        ,OEntryListHelper( static_cast<OControlModel&>(*this) )
        ,OErrorBroadcaster( OComponentHelper::rBHelper )
        ,m_nConvertedBoundValuesType(0)
        ,m_nNULLPos(-1)
        ,m_nBoundColumnType( DataType::SQLNULL )
    {

        m_nClassId = FormComponentType::LISTBOX;
        m_eListSourceType = ListSourceType_VALUELIST;
        m_aBoundColumn <<= sal_Int16(1);
        initValueProperty( PROPERTY_SELECT_SEQ, PROPERTY_ID_SELECT_SEQ);

        init();
    }


    OListBoxModel::OListBoxModel( const OListBoxModel* _pOriginal, const Reference<XComponentContext>& _rxFactory )
        :OBoundControlModel( _pOriginal, _rxFactory )
        ,OEntryListHelper( *_pOriginal, static_cast<OControlModel&>(*this) )
        ,OErrorBroadcaster( OComponentHelper::rBHelper )
        ,m_eListSourceType( _pOriginal->m_eListSourceType )
        ,m_aBoundColumn( _pOriginal->m_aBoundColumn )
        ,m_aListSourceValues( _pOriginal->m_aListSourceValues )
        ,m_aBoundValues( _pOriginal->m_aBoundValues )
        ,m_nConvertedBoundValuesType(0)
        ,m_aDefaultSelectSeq( _pOriginal->m_aDefaultSelectSeq )
        ,m_nNULLPos(-1)
        ,m_nBoundColumnType( DataType::SQLNULL )
    {

        init();
    }


    OListBoxModel::~OListBoxModel()
    {
        if (!OComponentHelper::rBHelper.bDisposed)
        {
            acquire();
            dispose();
        }

    }

    // XCloneable

    css::uno::Reference< css::util::XCloneable > SAL_CALL OListBoxModel::createClone()
{
    rtl::Reference<OListBoxModel> pClone = new OListBoxModel(this, getContext());
    pClone->clonedFrom(this);
    return pClone;
}

    // XServiceInfo

    css::uno::Sequence<OUString> SAL_CALL OListBoxModel::getSupportedServiceNames()
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

        *pStoreTo++ = FRM_SUN_COMPONENT_LISTBOX;
        *pStoreTo++ = FRM_SUN_COMPONENT_DATABASE_LISTBOX;
        *pStoreTo++ = BINDABLE_DATABASE_LIST_BOX;

        *pStoreTo++ = FRM_COMPONENT_LISTBOX;

        return aSupported;
    }


    Any SAL_CALL OListBoxModel::queryAggregation(const Type& _rType)
    {
        Any aReturn = OBoundControlModel::queryAggregation( _rType );
        if ( !aReturn.hasValue() )
            aReturn = OEntryListHelper::queryInterface( _rType );
        if ( !aReturn.hasValue() )
            aReturn = OErrorBroadcaster::queryInterface( _rType );
        return aReturn;
    }

    // OComponentHelper

    void OListBoxModel::disposing()
    {
        OBoundControlModel::disposing();
        OEntryListHelper::disposing();
        OErrorBroadcaster::disposing();
    }


    void OListBoxModel::getFastPropertyValue(Any& _rValue, sal_Int32 _nHandle) const
    {
        switch (_nHandle)
        {
        case PROPERTY_ID_BOUNDCOLUMN:
            _rValue = m_aBoundColumn;
            break;

        case PROPERTY_ID_LISTSOURCETYPE:
            _rValue <<= m_eListSourceType;
            break;

        case PROPERTY_ID_LISTSOURCE:
            _rValue <<= lcl_convertToStringSequence( m_aListSourceValues );
            break;

        case PROPERTY_ID_VALUE_SEQ:
            _rValue <<= lcl_convertToStringSequence( m_aBoundValues );
            break;

        case PROPERTY_ID_SELECT_VALUE_SEQ:
            _rValue <<= getCurrentMultiValue();
            break;

        case PROPERTY_ID_SELECT_VALUE:
           _rValue = getCurrentSingleValue();
            break;

        case PROPERTY_ID_DEFAULT_SELECT_SEQ:
            _rValue <<= m_aDefaultSelectSeq;
            break;

        case PROPERTY_ID_STRINGITEMLIST:
            _rValue <<= comphelper::containerToSequence(getStringItemList());
            break;

        case PROPERTY_ID_TYPEDITEMLIST:
            _rValue <<= getTypedItemList();
            break;

        default:
            OBoundControlModel::getFastPropertyValue(_rValue, _nHandle);
        }
    }


    void OListBoxModel::setFastPropertyValue_NoBroadcast(sal_Int32 _nHandle, const Any& _rValue)
    {
        switch (_nHandle)
        {
        case PROPERTY_ID_BOUNDCOLUMN :
            DBG_ASSERT((_rValue.getValueTypeClass() == TypeClass_SHORT) || (_rValue.getValueTypeClass() == TypeClass_VOID),
                "OListBoxModel::setFastPropertyValue_NoBroadcast : invalid type !" );
            m_aBoundColumn = _rValue;
            break;

        case PROPERTY_ID_LISTSOURCETYPE :
            DBG_ASSERT(_rValue.getValueType().equals(::cppu::UnoType<ListSourceType>::get()),
                "OComboBoxModel::setFastPropertyValue_NoBroadcast : invalid type !" );
            _rValue >>= m_eListSourceType;
            break;

        case PROPERTY_ID_LISTSOURCE:
        {
            // extract
            Sequence< OUString > aListSource;
            OSL_VERIFY( _rValue >>= aListSource );

            // copy to member
            ValueList().swap(m_aListSourceValues);
            ::std::copy(
                std::cbegin(aListSource),
                std::cend(aListSource),
                ::std::insert_iterator< ValueList >( m_aListSourceValues, m_aListSourceValues.end() )
            );

            // propagate
            if ( m_eListSourceType == ListSourceType_VALUELIST )
            {
                setBoundValues(std::vector(m_aListSourceValues));
            }
            else
            {
                if ( m_xCursor.is() && !hasField() && !hasExternalListSource() )
                    // listbox is already connected to a database, and no external list source
                    // data source changed -> refresh
                    loadData( false );
            }
        }
        break;

        case PROPERTY_ID_VALUE_SEQ :
            SAL_WARN( "forms.component", "ValueItemList is read-only!" );
            throw PropertyVetoException();

        case PROPERTY_ID_SELECT_VALUE_SEQ :
        {
            Sequence< const Any > v;
            _rValue >>= v;
            Any newSelectSeq(translateBindingValuesToControlValue(v));
            setControlValue( newSelectSeq, eOther );
        }
        break;

#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        case PROPERTY_ID_SELECT_VALUE :
        {
            ORowSetValue v;
            v.fill(_rValue);
            Any newSelectSeq(translateDbValueToControlValue(v));
            setControlValue( newSelectSeq, eOther );
        }
        break;
#endif
        case PROPERTY_ID_DEFAULT_SELECT_SEQ :
            DBG_ASSERT(_rValue.getValueType().equals(cppu::UnoType<Sequence<sal_Int16>>::get()),
                "OListBoxModel::setFastPropertyValue_NoBroadcast : invalid type !" );
            _rValue >>= m_aDefaultSelectSeq;

            DBG_ASSERT(m_xAggregateFastSet.is(), "OListBoxModel::setFastPropertyValue_NoBroadcast(DEFAULT_SELECT_SEQ) : invalid aggregate !");
            if ( m_xAggregateFastSet.is() )
                setControlValue( _rValue, eOther );
            break;

        case PROPERTY_ID_STRINGITEMLIST:
        {
            ControlModelLock aLock( *this );
            setNewStringItemList( _rValue, aLock );
                // TODO: this is bogus. setNewStringItemList expects a guard which has the *only*
                // lock to the mutex, but setFastPropertyValue_NoBroadcast is already called with
                // a lock - so we effectively has two locks here, of which setNewStringItemList can
                // only control one.
        }
        resetNoBroadcast();
        break;

        case PROPERTY_ID_TYPEDITEMLIST:
        {
            ControlModelLock aLock( *this );
            setNewTypedItemList( _rValue, aLock );
            // Same TODO as above.
        }
        resetNoBroadcast();
        break;

        default:
            OBoundControlModel::setFastPropertyValue_NoBroadcast(_nHandle, _rValue);
        }
    }


    sal_Bool OListBoxModel::convertFastPropertyValue(
        Any& _rConvertedValue, Any& _rOldValue, sal_Int32 _nHandle, const Any& _rValue)
    {
        bool bModified(false);
        switch (_nHandle)
        {
        case PROPERTY_ID_BOUNDCOLUMN :
            bModified = tryPropertyValue(_rConvertedValue, _rOldValue, _rValue, m_aBoundColumn, ::cppu::UnoType<sal_Int16>::get());
            break;

        case PROPERTY_ID_LISTSOURCETYPE:
            bModified = tryPropertyValueEnum(_rConvertedValue, _rOldValue, _rValue, m_eListSourceType);
            break;

        case PROPERTY_ID_LISTSOURCE:
            bModified = tryPropertyValue(_rConvertedValue, _rOldValue, _rValue, lcl_convertToStringSequence( m_aListSourceValues ) );
            break;

        case PROPERTY_ID_VALUE_SEQ :
            SAL_WARN( "forms.component", "ValueItemList is read-only!" );
            throw IllegalArgumentException();

        case PROPERTY_ID_SELECT_VALUE_SEQ :
            bModified = tryPropertyValue(_rConvertedValue, _rOldValue, _rValue, getCurrentMultiValue());
            break;

        case PROPERTY_ID_SELECT_VALUE :
        {
            // Any from connectivity::ORowSetValue
            Any aCurrentValue = getCurrentSingleValue();
            if (aCurrentValue != _rValue)
            {
                _rOldValue = std::move(aCurrentValue);
                _rConvertedValue = _rValue;
                bModified = true;
            }
            break;
        }
        case PROPERTY_ID_DEFAULT_SELECT_SEQ :
            bModified = tryPropertyValue(_rConvertedValue, _rOldValue, _rValue, m_aDefaultSelectSeq);
            break;

        case PROPERTY_ID_STRINGITEMLIST:
            bModified = convertNewListSourceProperty( _rConvertedValue, _rOldValue, _rValue );
            break;

        case PROPERTY_ID_TYPEDITEMLIST :
            if (hasExternalListSource())
                throw IllegalArgumentException();
            bModified = tryPropertyValue( _rConvertedValue, _rOldValue, _rValue, getTypedItemList());
            break;

        default:
            return OBoundControlModel::convertFastPropertyValue(_rConvertedValue, _rOldValue, _nHandle, _rValue);
        }
        return bModified;
    }


    void SAL_CALL OListBoxModel::setPropertyValues( const Sequence< OUString >& _rPropertyNames, const Sequence< Any >& _rValues )
    {
        // if both SelectedItems and StringItemList are set, care for this
        // #i27024#
        const Any* pSelectSequenceValue = nullptr;

        const OUString* pSelectedItemsPos = std::find(
            _rPropertyNames.begin(), _rPropertyNames.end(), PROPERTY_SELECT_SEQ
        );
        auto aStringItemListExists = std::any_of(
            _rPropertyNames.begin(), _rPropertyNames.end(),
             [](OUString const & s) { return s == PROPERTY_STRINGITEMLIST; }
        );
        if ( ( pSelectedItemsPos != _rPropertyNames.end() ) && aStringItemListExists )
        {
            if (_rPropertyNames.getLength() != _rValues.getLength())
                throw css::lang::IllegalArgumentException(u"lengths do not match"_ustr,
                                                          static_cast<cppu::OWeakObject*>(this), -1);

            // both properties are present
            // -> remember the value for the select sequence
            pSelectSequenceValue = _rValues.getConstArray() + ( pSelectedItemsPos - _rPropertyNames.begin() );
        }

        OBoundControlModel::setPropertyValues( _rPropertyNames, _rValues );

        if ( pSelectSequenceValue )
        {
            setPropertyValue( PROPERTY_SELECT_SEQ, *pSelectSequenceValue );
            // Note that this is the only reliable way, since one of the properties is implemented
            // by ourself, and one is implemented by the aggregate, we cannot rely on any particular
            // results when setting them both - too many undocumented behavior in all the involved

        }
    }


    void OListBoxModel::describeFixedProperties( Sequence< Property >& _rProps ) const
    {
        OBoundControlModel::describeFixedProperties( _rProps );
        sal_Int32 nOldCount = _rProps.getLength();
        _rProps.realloc( nOldCount + 10);
        css::beans::Property* pProperties = _rProps.getArray() + nOldCount;
        *pProperties++ = css::beans::Property(PROPERTY_TABINDEX, PROPERTY_ID_TABINDEX, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND);
        *pProperties++ = css::beans::Property(PROPERTY_BOUNDCOLUMN, PROPERTY_ID_BOUNDCOLUMN, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEVOID);
        *pProperties++ = css::beans::Property(PROPERTY_LISTSOURCETYPE, PROPERTY_ID_LISTSOURCETYPE, cppu::UnoType<ListSourceType>::get(), css::beans::PropertyAttribute::BOUND);
        *pProperties++ = css::beans::Property(PROPERTY_LISTSOURCE, PROPERTY_ID_LISTSOURCE, cppu::UnoType<css::uno::Sequence<OUString>>::get(), css::beans::PropertyAttribute::BOUND);
        *pProperties++ = css::beans::Property(PROPERTY_VALUE_SEQ, PROPERTY_ID_VALUE_SEQ, cppu::UnoType<css::uno::Sequence<OUString>>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::READONLY | css::beans::PropertyAttribute::TRANSIENT);
        *pProperties++ = css::beans::Property(PROPERTY_SELECT_VALUE_SEQ, PROPERTY_ID_SELECT_VALUE_SEQ, cppu::UnoType<Sequence< Any >>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::TRANSIENT);
        *pProperties++ = css::beans::Property(PROPERTY_SELECT_VALUE, PROPERTY_ID_SELECT_VALUE, cppu::UnoType<Any>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::TRANSIENT);
        *pProperties++ = css::beans::Property(PROPERTY_DEFAULT_SELECT_SEQ, PROPERTY_ID_DEFAULT_SELECT_SEQ, cppu::UnoType<Sequence<sal_Int16>>::get(), css::beans::PropertyAttribute::BOUND);
        *pProperties++ = css::beans::Property(PROPERTY_STRINGITEMLIST, PROPERTY_ID_STRINGITEMLIST, cppu::UnoType<Sequence< OUString >>::get(), css::beans::PropertyAttribute::BOUND);
        *pProperties++ = css::beans::Property(PROPERTY_TYPEDITEMLIST, PROPERTY_ID_TYPEDITEMLIST, cppu::UnoType<Sequence< Any >>::get(), css::beans::PropertyAttribute::OPTIONAL);
        DBG_ASSERT( pProperties == _rProps.getArray() + _rProps.getLength(), "<...>::describeFixedProperties/getInfoHelper: forgot to adjust the count ?");
    }


    void OListBoxModel::_propertyChanged( const PropertyChangeEvent& i_rEvent )
    {
        if ( i_rEvent.PropertyName == PROPERTY_STRINGITEMLIST )
        {
            ControlModelLock aLock( *this );
            // SYNCHRONIZED ----->
            // our aggregate internally changed its StringItemList property - reflect this in our "overridden"
            // version of the property
            setNewStringItemList( i_rEvent.NewValue, aLock );
            // <----- SYNCHRONIZED
            return;
        }
        else if ( i_rEvent.PropertyName == PROPERTY_TYPEDITEMLIST )
        {
            ControlModelLock aLock( *this );
            // SYNCHRONIZED ----->
            // our aggregate internally changed its TypedItemList property - reflect this in our "overridden"
            // version of the property
            setNewTypedItemList( i_rEvent.NewValue, aLock );
            // <----- SYNCHRONIZED
            return;
        }
        OBoundControlModel::_propertyChanged( i_rEvent );
    }


    void OListBoxModel::describeAggregateProperties( Sequence< Property >& _rAggregateProps ) const
    {
        OBoundControlModel::describeAggregateProperties( _rAggregateProps );

        // superseded properties:
        RemoveProperty( _rAggregateProps, PROPERTY_STRINGITEMLIST );
        RemoveProperty( _rAggregateProps, PROPERTY_TYPEDITEMLIST );
    }


    OUString SAL_CALL OListBoxModel::getServiceName()
    {
        return FRM_COMPONENT_LISTBOX;   // old (non-sun) name for compatibility !
    }


    void SAL_CALL OListBoxModel::write(const Reference<XObjectOutputStream>& _rxOutStream)
    {
        OBoundControlModel::write(_rxOutStream);

        // Dummy sequence, to stay compatible if SelectSeq is not saved anymore
        Sequence<sal_Int16> aDummySeq;

        // Version
        // Version 0x0002: ListSource becomes StringSeq
        _rxOutStream->writeShort(0x0004);

        // Masking for any
        sal_uInt16 nAnyMask = 0;
        if (m_aBoundColumn.getValueTypeClass() != TypeClass_VOID)
            nAnyMask |= BOUNDCOLUMN;

        _rxOutStream << nAnyMask;

        _rxOutStream << lcl_convertToStringSequence( m_aListSourceValues );
        _rxOutStream << static_cast<sal_Int16>(m_eListSourceType);
        _rxOutStream << aDummySeq;
        _rxOutStream << m_aDefaultSelectSeq;

        if ((nAnyMask & BOUNDCOLUMN) == BOUNDCOLUMN)
        {
            sal_Int16 nBoundColumn = 0;
            m_aBoundColumn >>= nBoundColumn;
            _rxOutStream << nBoundColumn;
        }
        writeHelpTextCompatibly(_rxOutStream);

        // from version 0x0004 : common properties
        writeCommonProperties(_rxOutStream);
    }


    void SAL_CALL OListBoxModel::read(const Reference<XObjectInputStream>& _rxInStream)
    {
        // We need to respect dependencies for certain variables.
        // Therefore, we need to set them explicitly via setPropertyValue().

        OBoundControlModel::read(_rxInStream);
        ControlModelLock aLock( *this );

        // since we are "overwriting" the StringItemList of our aggregate (means we have
        // an own place to store the value, instead of relying on our aggregate storing it),
        // we need to respect what the aggregate just read for the StringItemList property.
        try
        {
            if ( m_xAggregateSet.is() )
                setNewStringItemList( m_xAggregateSet->getPropertyValue( PROPERTY_STRINGITEMLIST ), aLock );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "forms.component", "OComboBoxModel::read: caught an exception while examining the aggregate's string item list" );
        }

        // Version
        sal_uInt16 nVersion = _rxInStream->readShort();
        DBG_ASSERT(nVersion > 0, "OListBoxModel::read : version 0 ? this should never have been written !");

        if (nVersion > 0x0004)
        {
            SAL_WARN( "forms.component", "OListBoxModel::read : invalid (means unknown) version !");
            ValueList().swap(m_aListSourceValues);
            m_aBoundColumn <<= sal_Int16(0);
            clearBoundValues();
            m_eListSourceType = ListSourceType_VALUELIST;
            m_aDefaultSelectSeq.realloc(0);
            defaultCommonProperties();
            return;
        }

        // Masking for any
        sal_uInt16 nAnyMask;
        _rxInStream >> nAnyMask;

        // ListSourceSeq
        css::uno::Sequence<OUString> aListSourceSeq;
        if (nVersion == 0x0001)
        {
            // Create ListSourceSeq from String
            OUString sListSource;
            _rxInStream >> sListSource;

            const sal_Int32 nTokens{ comphelper::string::getTokenCount(sListSource, ';') };
            aListSourceSeq.realloc( nTokens );
            sal_Int32 nIdx{ 0 };
            for (sal_Int32 i=0; i<nTokens; ++i)
            {
                aListSourceSeq.getArray()[i] = sListSource.getToken(0, ';', nIdx);
            }
        }
        else
            _rxInStream >> aListSourceSeq;

        sal_Int16 nListSourceType;
        _rxInStream >> nListSourceType;
        m_eListSourceType = static_cast<ListSourceType>(nListSourceType);
        Any aListSourceSeqAny;
        aListSourceSeqAny <<= aListSourceSeq;

        setFastPropertyValue(PROPERTY_ID_LISTSOURCE, aListSourceSeqAny );

        // Dummy sequence, to stay compatible if SelectSeq is not saved anymore
        Sequence<sal_Int16> aDummySeq;
        _rxInStream >> aDummySeq;

        // DefaultSelectSeq
        Sequence<sal_Int16> aDefaultSelectSeq;
        _rxInStream >> aDefaultSelectSeq;
        Any aDefaultSelectSeqAny;
        aDefaultSelectSeqAny <<= aDefaultSelectSeq;
        setFastPropertyValue(PROPERTY_ID_DEFAULT_SELECT_SEQ, aDefaultSelectSeqAny);

        // BoundColumn
        if ((nAnyMask & BOUNDCOLUMN) == BOUNDCOLUMN)
        {
            sal_Int16 nValue;
            _rxInStream >> nValue;
            m_aBoundColumn <<= nValue;
        }
        else // the constructor initialises to 1, so if it is empty,
             // we must explicitly set to empty
        {
            m_aBoundColumn = Any();
        }

        if (nVersion > 2)
            readHelpTextCompatibly(_rxInStream);

        // if our string list is not filled from the value list, we must empty it
        // this can be the case when somebody saves in alive mode
        if  (   ( m_eListSourceType != ListSourceType_VALUELIST )
            &&  !hasExternalListSource()
            )
        {
            setFastPropertyValue( PROPERTY_ID_STRINGITEMLIST, Any( css::uno::Sequence<OUString>() ) );
            setFastPropertyValue( PROPERTY_ID_TYPEDITEMLIST, Any( css::uno::Sequence<css::uno::Any>() ) );
        }

        if (nVersion > 3)
            readCommonProperties(_rxInStream);

        // Display the default values after reading
        if ( !getControlSource().isEmpty() )
            // (not if we don't have a control source - the "State" property acts like it is persistent, then
            resetNoBroadcast();
    }


    void OListBoxModel::loadData( bool _bForce )
    {
        SAL_INFO( "forms.component", "OListBoxModel::loadData" );
        DBG_ASSERT( m_eListSourceType != ListSourceType_VALUELIST, "OListBoxModel::loadData: cannot load value list from DB!" );
        DBG_ASSERT( !hasExternalListSource(), "OListBoxModel::loadData: cannot load from DB when I have an external list source!" );

        const sal_Int16 nNULLPosBackup( m_nNULLPos );
        const sal_Int32 nBoundColumnTypeBackup( m_nBoundColumnType );
        m_nNULLPos = -1;
        m_nBoundColumnType = DataType::SQLNULL;

        // pre-requisites:
        // PRE1: connection
        Reference< XConnection > xConnection;
        // is the active connection of our form
        Reference< XPropertySet > xFormProps( m_xCursor, UNO_QUERY );
        if ( xFormProps.is() )
            xFormProps->getPropertyValue( PROPERTY_ACTIVE_CONNECTION ) >>= xConnection;

        // PRE2: list source
        OUString sListSource;
        // if our list source type is no value list, we need to concatenate
        // the single list source elements
        ::std::for_each(
            m_aListSourceValues.begin(),
            m_aListSourceValues.end(),
            AppendRowSetValueString( sListSource )
        );

        // outta here if we don't have all pre-requisites
        if ( !xConnection.is() || sListSource.isEmpty() )
        {
            clearBoundValues();
            return;
        }

        ::std::optional< sal_Int16 > aBoundColumn(std::nullopt);
        if ( m_aBoundColumn.getValueTypeClass() == TypeClass_SHORT )
        {
            sal_Int16 nBoundColumn( 0 );
            m_aBoundColumn >>= nBoundColumn;
            aBoundColumn = nBoundColumn;
        }

        ::utl::SharedUNOComponent< XResultSet > xListCursor;
        try
        {
            m_aListRowSet.setConnection( xConnection );

            bool bExecute = false;
            switch (m_eListSourceType)
            {
            case ListSourceType_TABLEFIELDS:
                // don't work with a statement here, the fields will be collected below
                break;

            case ListSourceType_TABLE:
                {
                    Reference<XNameAccess> xFieldsByName = getTableFields(xConnection, sListSource);
                    Reference<XIndexAccess> xFieldsByIndex(xFieldsByName, UNO_QUERY);

                    // do we have a bound column if yes we have to select it
                    // and the displayed column is the first column otherwise we act as a combobox
                    OUString aFieldName;
                    OUString aBoundFieldName;

                    if ( !!aBoundColumn && ( *aBoundColumn >= 0 ) && xFieldsByIndex.is() )
                    {
                        if ( *aBoundColumn >= xFieldsByIndex->getCount() )
                            break;

                        Reference<XPropertySet> xFieldAsSet(xFieldsByIndex->getByIndex( *aBoundColumn ),UNO_QUERY);
                        assert(xFieldAsSet.is());
                        xFieldAsSet->getPropertyValue(PROPERTY_NAME) >>= aBoundFieldName;
                        aBoundColumn = 1;

                        xFieldAsSet.set(xFieldsByIndex->getByIndex(0),UNO_QUERY);
                        xFieldAsSet->getPropertyValue(PROPERTY_NAME) >>= aFieldName;
                    }
                    else if (xFieldsByName.is())
                    {
                        if ( xFieldsByName->hasByName( getControlSource() ) )
                            aFieldName = getControlSource();
                        else
                        {
                            // otherwise look for the alias
                            Reference< XColumnsSupplier > xSupplyFields;
                            xFormProps->getPropertyValue(u"SingleSelectQueryComposer"_ustr) >>= xSupplyFields;

                            // search the field
                            DBG_ASSERT(xSupplyFields.is(), "OListBoxModel::loadData : invalid query composer !");

                            Reference<XNameAccess> xFieldNames = xSupplyFields->getColumns();
                            if ( xFieldNames->hasByName( getControlSource() ) )
                            {
                                Reference<XPropertySet> xComposerFieldAsSet;
                                xFieldNames->getByName( getControlSource() ) >>= xComposerFieldAsSet;
                                if (hasProperty(PROPERTY_FIELDSOURCE, xComposerFieldAsSet))
                                    xComposerFieldAsSet->getPropertyValue(PROPERTY_FIELDSOURCE) >>= aFieldName;
                            }
                        }
                    }
                    if (aFieldName.isEmpty())
                        break;

                    Reference<XDatabaseMetaData> xMeta = xConnection->getMetaData();
                    OUString aQuote = xMeta->getIdentifierQuoteString();
                    OUString aStatement(u"SELECT "_ustr);
                    if (aBoundFieldName.isEmpty())   // act like a combobox
                        aStatement += "DISTINCT ";

                    aStatement += quoteName(aQuote,aFieldName);
                    if (!aBoundFieldName.isEmpty())
                    {
                        aStatement += ", " + quoteName(aQuote, aBoundFieldName);
                    }
                    aStatement += " FROM ";

                    OUString sCatalog, sSchema, sTable;
                    qualifiedNameComponents( xMeta, sListSource, sCatalog, sSchema, sTable, EComposeRule::InDataManipulation );
                    aStatement += composeTableNameForSelect( xConnection, sCatalog, sSchema, sTable );

                    m_aListRowSet.setEscapeProcessing( false );
                    m_aListRowSet.setCommand( aStatement );
                    bExecute = true;
                }
                break;

            case ListSourceType_QUERY:
                m_aListRowSet.setCommandFromQuery( sListSource );
                bExecute = true;
                break;

            default:
                m_aListRowSet.setEscapeProcessing( ListSourceType_SQLPASSTHROUGH != m_eListSourceType );
                m_aListRowSet.setCommand( sListSource );
                bExecute = true;
                break;
            }

            if (bExecute)
            {
                if ( !_bForce && !m_aListRowSet.isDirty() )
                {
                    // if none of the settings of the row set changed, compared to the last
                    // invocation of loadData, then don't re-fill the list. Instead, assume
                    // the list entries are the same.
                    m_nNULLPos = nNULLPosBackup;
                    m_nBoundColumnType = nBoundColumnTypeBackup;
                    return;
                }
                xListCursor.reset( m_aListRowSet.execute() );
            }
        }
        catch(const SQLException& eSQL)
        {
            onError(eSQL, ResourceManager::loadString(RID_BASELISTBOX_ERROR_FILLLIST));
            return;
        }
        catch(const Exception&)
        {
            return;
        }

        // Fill display and value lists
        ValueList aDisplayList, aValueList;
        bool bUseNULL = hasField() && !isRequired();

        // empty BoundColumn is treated as BoundColumn==0,
        if(!aBoundColumn)
            aBoundColumn = 0;

        try
        {
            OSL_ENSURE( xListCursor.is() || ( ListSourceType_TABLEFIELDS == m_eListSourceType ),
                "OListBoxModel::loadData: logic error!" );
            if ( !xListCursor.is() && ( ListSourceType_TABLEFIELDS != m_eListSourceType ) )
                return;

            switch (m_eListSourceType)
            {
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
            case ListSourceType_SQL:
            case ListSourceType_SQLPASSTHROUGH:
            case ListSourceType_TABLE:
            case ListSourceType_QUERY:
                {
                    // Get field of the ResultSet's 1st column
                    Reference<XColumnsSupplier> xSupplyCols(xListCursor, UNO_QUERY);
                    DBG_ASSERT(xSupplyCols.is(), "OListBoxModel::loadData : cursor supports the row set service but is no column supplier?!");
                    Reference<XIndexAccess> xColumns;
                    if (xSupplyCols.is())
                    {
                        xColumns.set(xSupplyCols->getColumns(), UNO_QUERY);
                        DBG_ASSERT(xColumns.is(), "OListBoxModel::loadData : no columns supplied by the row set !");
                    }

                    Reference< XPropertySet > xDataField;
                    if ( xColumns.is() )
                        xColumns->getByIndex(0) >>= xDataField;
                    if ( !xDataField.is() )
                        return;

                    ::dbtools::FormattedColumnValue aValueFormatter( getContext(), m_xCursor, xDataField );

                    // Get the field of BoundColumn of the ResultSet
                    m_nBoundColumnType = DataType::SQLNULL;
                    if ( *aBoundColumn >= 0 )
                    {
                        try
                        {
                            Reference< XPropertySet > xBoundField( xColumns->getByIndex( *aBoundColumn ), UNO_QUERY_THROW );
                            OSL_VERIFY( xBoundField->getPropertyValue(u"Type"_ustr) >>= m_nBoundColumnType );
                        }
                        catch( const Exception& )
                        {
                            DBG_UNHANDLED_EXCEPTION("forms.component");
                        }
                    }
                    else if ( *aBoundColumn == -1)
                        m_nBoundColumnType = DataType::SMALLINT;

                    //  If the LB is bound to a field and empty entries are valid, we remember the position
                    //  for an empty entry
                    SAL_INFO( "forms.component", "OListBoxModel::loadData: string collection" );
                    OUString aStr;
                    sal_Int16 entryPos = 0;
                    ORowSetValue aBoundValue;
                    Reference< XRow > xCursorRow( xListCursor, UNO_QUERY_THROW );
                    while ( xListCursor->next() && ( entryPos++ < SHRT_MAX ) ) // SHRT_MAX is the maximum number of entries
                    {
                        aStr = aValueFormatter.getFormattedValue();
                        aDisplayList.emplace_back(aStr );

                        if(*aBoundColumn >= 0)
                            aBoundValue.fill( *aBoundColumn + 1, m_nBoundColumnType, xCursorRow );
                        else
                            // -1 because getRow() is 1-indexed, but ListBox positions are 0-indexed
                            aBoundValue = static_cast<sal_Int16>(xListCursor->getRow()-1);
                        aValueList.push_back( aBoundValue );

                        if ( m_nNULLPos == -1 && aBoundValue.isNull() )
                            m_nNULLPos = sal_Int16( aDisplayList.size() - 1 );
                        if ( bUseNULL && ( m_nNULLPos == -1 ) && aStr.isEmpty() )
                            // There is already a non-NULL entry with empty display string;
                            // adding another one for NULL would make things confusing,
                            // so back off.
                            bUseNULL = false;
                    }
                }
                break;
#endif
            case ListSourceType_TABLEFIELDS:
                {
                    Reference<XNameAccess> xFieldNames = getTableFields(xConnection, sListSource);
                    if (xFieldNames.is())
                    {
                        const css::uno::Sequence<OUString> seqNames = xFieldNames->getElementNames();
                        ::std::copy(
                            seqNames.begin(),
                            seqNames.end(),
                            ::std::insert_iterator< ValueList >( aDisplayList, aDisplayList.end() )
                        );
                        if(*aBoundColumn == -1)
                        {
                            // the type of i matters! It will be the type of the ORowSetValue pushed to aValueList!
                            for(size_t i=0; i < aDisplayList.size(); ++i)
                            {
                                aValueList.emplace_back(sal_Int16(i));
                            }
                        }
                        else
                        {
                            aValueList = aDisplayList;
                        }
                    }
                }
                break;
            default:
                SAL_WARN( "forms.component", "OListBoxModel::loadData: unreachable!" );
                break;
            }
        }
        catch(const SQLException& eSQL)
        {
            onError(eSQL, ResourceManager::loadString(RID_BASELISTBOX_ERROR_FILLLIST));
            return;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
            return;
        }


        // Create Values sequence
        // Add NULL entry
        if (bUseNULL && m_nNULLPos == -1)
        {
            aValueList.insert( aValueList.begin(), ORowSetValue() );

            aDisplayList.insert( aDisplayList.begin(), ORowSetValue( OUString() ) );
            m_nNULLPos = 0;
        }

        setBoundValues(std::move(aValueList));

        setFastPropertyValue( PROPERTY_ID_STRINGITEMLIST, Any( lcl_convertToStringSequence( aDisplayList ) ) );
        setFastPropertyValue( PROPERTY_ID_TYPEDITEMLIST, Any( css::uno::Sequence<css::uno::Any>() ) );
    }


    void OListBoxModel::onConnectedDbColumn( const Reference< XInterface >& /*_rxForm*/ )
    {
        // list boxes which are bound to a db column don't have multi selection
        // - this would be unable to reflect in the db column
        if ( hasField() )
        {
            setFastPropertyValue( PROPERTY_ID_MULTISELECTION, css::uno::Any(false) );
        }

        if ( !hasExternalListSource() )
            impl_refreshDbEntryList( false );
    }


    void OListBoxModel::onDisconnectedDbColumn()
    {
        clearBoundValues();
        m_nNULLPos = -1;
        m_nBoundColumnType = DataType::SQLNULL;

        if ( m_eListSourceType != ListSourceType_VALUELIST )
        {
            if ( !hasExternalListSource() )
                setFastPropertyValue( PROPERTY_ID_STRINGITEMLIST, Any( css::uno::Sequence<OUString>() ) );

            m_aListRowSet.dispose();
        }
    }


    void OListBoxModel::setBoundValues(ValueList && l)
    {
        m_aConvertedBoundValues.clear();
        m_aBoundValues = std::move(l);
    }


    void OListBoxModel::clearBoundValues()
    {
        ValueList().swap(m_aConvertedBoundValues);
        ValueList().swap(m_aBoundValues);
    }


    void OListBoxModel::convertBoundValues(const sal_Int32 nFieldType) const
    {
        assert(s_aEmptyValue.isNull());
        m_nNULLPos = -1;
        m_aConvertedBoundValues.resize(m_aBoundValues.size());
        ValueList::iterator dst = m_aConvertedBoundValues.begin();
        sal_Int16 nPos = 0;
        for (auto const& src : m_aBoundValues)
        {
            if(m_nNULLPos == -1 &&
               !isRequired()    &&
               (src == s_aEmptyStringValue || src == s_aEmptyValue || src.isNull()) )
            {
                m_nNULLPos = nPos;
                dst->setNull();
            }
            else
            {
                *dst = src;
            }
            dst->setTypeKind(nFieldType);
            ++dst;
            ++nPos;
        }
        m_nConvertedBoundValuesType = nFieldType;
        OSL_ENSURE(dst == m_aConvertedBoundValues.end(), "OListBoxModel::convertBoundValues expected to have overwritten all of m_aConvertedBoundValues, but did not.");
        assert(dst == m_aConvertedBoundValues.end());
    }

    sal_Int32 OListBoxModel::getValueType() const
    {
        return (m_nBoundColumnType != css::sdbc::DataType::SQLNULL) ?
            m_nBoundColumnType :
            ( hasField() ? getFieldType() : DataType::VARCHAR);
    }

    ValueList OListBoxModel::impl_getValues() const
    {
        const sal_Int32 nFieldType = getValueType();

        if ( !m_aConvertedBoundValues.empty() && m_nConvertedBoundValuesType == nFieldType )
            return m_aConvertedBoundValues;

        if ( !m_aBoundValues.empty() )
        {
            convertBoundValues(nFieldType);
            return m_aConvertedBoundValues;
        }

        const std::vector< OUString >& aStringItems( getStringItemList() );
        ValueList aValues( aStringItems.size() );
        ValueList::iterator dst = aValues.begin();
        for (auto const& src : aStringItems)
        {
            *dst = src;
            dst->setTypeKind(nFieldType);
            ++dst;
        }
        m_nConvertedBoundValuesType = nFieldType;
        OSL_ENSURE(dst == aValues.end(), "OListBoxModel::impl_getValues expected to have set all of aValues, but did not.");
        assert(dst == aValues.end());
        return aValues;
    }

    ORowSetValue OListBoxModel::getFirstSelectedValue() const
    {
        DBG_ASSERT( m_xAggregateFastSet.is(), "OListBoxModel::getFirstSelectedValue: invalid aggregate!" );
        if ( !m_xAggregateFastSet.is() )
            return s_aEmptyValue;

        Sequence< sal_Int16 > aSelectedIndices;
        OSL_VERIFY( m_xAggregateFastSet->getFastPropertyValue( getValuePropertyAggHandle() ) >>= aSelectedIndices );
        if ( !aSelectedIndices.hasElements() )
            // nothing selected at all
            return s_aEmptyValue;

        if ( ( m_nNULLPos != -1 ) && ( aSelectedIndices[0] == m_nNULLPos ) )
            // the dedicated "NULL" entry is selected
            return s_aEmptyValue;

        ValueList aValues( impl_getValues() );

        size_t selectedValue = aSelectedIndices[0];
        if ( selectedValue >= aValues.size() )
        {
            SAL_WARN( "forms.component", "OListBoxModel::getFirstSelectedValue: inconsistent selection/valuelist!" );
            return s_aEmptyValue;
        }

        return aValues[ selectedValue ];
    }


    bool OListBoxModel::commitControlValueToDbColumn( bool /*_bPostReset*/ )
    {
        // current selection list
        const ORowSetValue aCurrentValue( getFirstSelectedValue() );
        if ( aCurrentValue != m_aSaveValue )
        {
            if ( aCurrentValue.isNull() )
                m_xColumnUpdate->updateNull();
            else
            {
                try
                {
                    m_xColumnUpdate->updateObject( aCurrentValue.makeAny() );
                }
                catch ( const Exception& )
                {
                    return false;
                }
            }
            m_aSaveValue = aCurrentValue;
        }
        return true;
    }


    Sequence< sal_Int16 > OListBoxModel::translateDbValueToControlValue(const ORowSetValue &i_aValue) const
    {
        Sequence< sal_Int16 > aSelectionIndicies;

        // reset selection for NULL values
        if ( i_aValue.isNull() )
        {
            if ( m_nNULLPos != -1 )
            {
                aSelectionIndicies = { m_nNULLPos };
            }
        }
        else
        {
            ValueList aValues( impl_getValues() );
            assert( m_nConvertedBoundValuesType == getValueType());
            ORowSetValue v(i_aValue);
            v.setTypeKind( m_nConvertedBoundValuesType );
            ValueList::const_iterator curValuePos = ::std::find( aValues.begin(), aValues.end(), v );
            if ( curValuePos != aValues.end() )
            {
                aSelectionIndicies = { o3tl::narrowing<sal_Int16>(curValuePos - aValues.begin()) };
            }
        }

        return aSelectionIndicies;
    }

    Sequence< sal_Int16 > OListBoxModel::translateBindingValuesToControlValue(const Sequence< const Any > &i_aValues) const
    {
        const ValueList aValues( impl_getValues() );
        assert( m_nConvertedBoundValuesType == getValueType());
        Sequence< sal_Int16 > aSelectionIndicies(i_aValues.getLength());

        sal_Int32 nCount(0);

#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        sal_Int16 *pIndex = aSelectionIndicies.getArray();
        for ( auto const & value : i_aValues)
        {
            if ( value.hasValue() )
            {
                ORowSetValue v;
                v.fill(value);
                v.setTypeKind( m_nConvertedBoundValuesType );
                ValueList::const_iterator curValuePos = ::std::find( aValues.begin(), aValues.end(), v );
                if ( curValuePos != aValues.end() )
                {
                    *pIndex = curValuePos - aValues.begin();
                    ++pIndex;
                    ++nCount;
                }
            }
            else
            {
                if ( m_nNULLPos != -1 )
                {
                    *pIndex = m_nNULLPos;
                    ++pIndex;
                    ++nCount;
                }
            }
        }
        assert(aSelectionIndicies.getArray() + nCount == pIndex);
#endif
        aSelectionIndicies.realloc(nCount);
        return aSelectionIndicies;
    }

    Any OListBoxModel::translateDbColumnToControlValue()
    {
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        Reference< XPropertySet > xBoundField( getField() );
        if ( !xBoundField.is() )
        {
            SAL_WARN( "forms.component", "OListBoxModel::translateDbColumnToControlValue: no field? How could that happen?!" );
            return Any();
        }

        ORowSetValue aCurrentValue;
        aCurrentValue.fill( getValueType(), m_xColumn );

        m_aSaveValue = aCurrentValue;

        return Any( translateDbValueToControlValue(aCurrentValue) );
#else
        return Any();
#endif
    }

    // XReset

    Any OListBoxModel::getDefaultForReset() const
    {
        Any aValue;
        if (m_aDefaultSelectSeq.hasElements())
            aValue <<= m_aDefaultSelectSeq;
        else if (m_nNULLPos != -1)  // bound Listbox
        {
            Sequence<sal_Int16> aSeq { m_nNULLPos };
            aValue <<= aSeq;
        }
        else
        {
            Sequence<sal_Int16> aSeq;
            aValue <<= aSeq;
        }

        return aValue;
    }


    void OListBoxModel::resetNoBroadcast()
    {
        OBoundControlModel::resetNoBroadcast();
        m_aSaveValue.setNull();
    }


    void SAL_CALL OListBoxModel::disposing( const EventObject& _rSource )
    {
        if ( !OEntryListHelper::handleDisposing( _rSource ) )
            OBoundControlModel::disposing( _rSource );
    }


    namespace
    {
        // The type of how we should transfer our selection to external value bindings
        enum ExchangeType
        {
            eIndexList,     /// as list of indexes of selected entries
            eIndex,         /// as index of the selected entry
            eEntryList,     /// as list of string representations of selected *display* entries
            eEntry,         /// as string representation of the selected *display* entry
            eValueList,     /// as list of string representations of selected values
            eValue          /// as string representation of the selected value
        };


        ExchangeType lcl_getCurrentExchangeType( const Type& _rExchangeType )
        {
            switch ( _rExchangeType.getTypeClass() )
            {
            case TypeClass_ANY:
                return eValue;
            case TypeClass_STRING:
                return eEntry;
            case TypeClass_LONG:
                return eIndex;
            case TypeClass_SEQUENCE:
            {
                Type aElementType = ::comphelper::getSequenceElementType( _rExchangeType );
                switch ( aElementType.getTypeClass() )
                {
                case TypeClass_ANY:
                    return eValueList;
                case TypeClass_STRING:
                    return eEntryList;
                case TypeClass_LONG:
                    return eIndexList;
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
            SAL_WARN( "forms.component", "lcl_getCurrentExchangeType: unsupported (unexpected) exchange type!" );
            return eEntry;
        }
    }


    Any OListBoxModel::translateExternalValueToControlValue( const Any& _rExternalValue ) const
    {
        Sequence< sal_Int16 > aSelectIndexes;

        switch ( lcl_getCurrentExchangeType( getExternalValueType() ) )
        {
        case eValueList:
        {
            Sequence< const Any > aExternalValues;
            OSL_VERIFY( _rExternalValue >>= aExternalValues );
            aSelectIndexes = translateBindingValuesToControlValue( aExternalValues );
        }
        break;

        case eValue:
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
        {
            ORowSetValue v;
            v.fill(_rExternalValue);
            aSelectIndexes = translateDbValueToControlValue(v);
        }
#endif
        break;

        case eIndexList:
        {
            // unfortunately, our select sequence is a sequence<short>, while the external binding
            // supplies sequence<int> only -> transform this
            Sequence< sal_Int32 > aSelectIndexesPure;
            OSL_VERIFY( _rExternalValue >>= aSelectIndexesPure );
            aSelectIndexes.realloc( aSelectIndexesPure.getLength() );
            ::std::copy(
                std::cbegin(aSelectIndexesPure),
                std::cend(aSelectIndexesPure),
                aSelectIndexes.getArray()
            );
        }
        break;

        case eIndex:
        {
            sal_Int32 nSelectIndex = -1;
            OSL_VERIFY( _rExternalValue >>= nSelectIndex );
            if ( ( nSelectIndex >= 0 ) && ( o3tl::make_unsigned(nSelectIndex) < getStringItemList().size() ) )
            {
                aSelectIndexes = { o3tl::narrowing<sal_Int16>(nSelectIndex) };
            }
        }
        break;

        case eEntryList:
        {
            // we can retrieve a string list from the binding for multiple selection
            Sequence< OUString > aSelectEntries;
            OSL_VERIFY( _rExternalValue >>= aSelectEntries );

            ::std::set< sal_Int16 > aSelectionSet;

            // find the selection entries in our item list
            for (OUString const& selectEntry : aSelectEntries)
            {
                int idx = 0;
                for(const OUString& s : getStringItemList())
                {
                    if (s==selectEntry)
                        aSelectionSet.insert(idx);
                    ++idx;
                }
            }

            // copy the indexes to the sequence
            aSelectIndexes = comphelper::containerToSequence( aSelectionSet );
        }
        break;

        case eEntry:
        {
            OUString sStringToSelect;
            OSL_VERIFY( _rExternalValue >>= sStringToSelect );
            ::std::set< sal_Int16 > aSelectionSet;
            int idx = 0;
            for(const OUString& s : getStringItemList())
            {
                if (s==sStringToSelect)
                    aSelectionSet.insert(idx);
                ++idx;
            }

            aSelectIndexes = comphelper::containerToSequence( aSelectionSet );
        }
        break;
        }

        return Any( aSelectIndexes );
    }


    namespace
    {

        struct ExtractStringFromSequence_Safe
        {
        protected:
            const std::vector< OUString >&  m_rList;

        public:
            explicit ExtractStringFromSequence_Safe( const std::vector< OUString >& _rList ) : m_rList( _rList ) { }

            const OUString & operator ()( sal_Int16 _nIndex )
            {
                OSL_ENSURE( _nIndex < static_cast<sal_Int32>(m_rList.size()), "ExtractStringFromSequence_Safe: inconsistence!" );
                if ( _nIndex < static_cast<sal_Int32>(m_rList.size()) )
                    return m_rList[ _nIndex ];
                return EMPTY_OUSTRING;
            }
        };


        Any lcl_getSingleSelectedEntryTyped( const Sequence< sal_Int16 >& _rSelectSequence, const Sequence<Any>& _rTypedList )
        {
            Any aReturn;

            // by definition, multiple selected entries are transferred as NULL if the
            // binding does not support lists
            if ( _rSelectSequence.getLength() <= 1 )
            {
                if ( _rSelectSequence.getLength() == 1 )
                {
                    sal_Int32 nIndex = _rSelectSequence[0];
                    if (0 <= nIndex && nIndex < _rTypedList.getLength())
                        aReturn = _rTypedList[nIndex];
                }
            }

            return aReturn;
        }


        Any lcl_getSingleSelectedEntry( const Sequence< sal_Int16 >& _rSelectSequence, const std::vector< OUString >& _rStringList )
        {
            Any aReturn;

            // by definition, multiple selected entries are transferred as NULL if the
            // binding does not support string lists
            if ( _rSelectSequence.getLength() <= 1 )
            {
                OUString sSelectedEntry;

                if ( _rSelectSequence.getLength() == 1 )
                    sSelectedEntry = ExtractStringFromSequence_Safe( _rStringList )( _rSelectSequence[0] );

                aReturn <<= sSelectedEntry;
            }

            return aReturn;
        }


        Any lcl_getMultiSelectedEntries( const Sequence< sal_Int16 >& _rSelectSequence, const std::vector< OUString >& _rStringList )
        {
            Sequence< OUString > aSelectedEntriesTexts( _rSelectSequence.getLength() );
            ::std::transform(
                _rSelectSequence.begin(),
                _rSelectSequence.end(),
                aSelectedEntriesTexts.getArray(),
                ExtractStringFromSequence_Safe( _rStringList )
            );
            return Any( aSelectedEntriesTexts );
        }


        struct ExtractAnyFromValueList_Safe
        {
        protected:
            const ValueList&  m_rList;

        public:
            explicit ExtractAnyFromValueList_Safe( const ValueList& _rList ) : m_rList( _rList ) { }

            Any operator ()( sal_Int16 _nIndex )
            {
                OSL_ENSURE( o3tl::make_unsigned(_nIndex) < m_rList.size(), "ExtractAnyFromValueList: inconsistence!" );
                if ( o3tl::make_unsigned(_nIndex) < m_rList.size() )
                    return m_rList[ _nIndex ].makeAny();
                return Any();
            }
        };


        Any lcl_getSingleSelectedEntryAny( const Sequence< sal_Int16 >& _rSelectSequence, const ValueList& _rStringList )
        {
            Any aReturn;

            // by definition, multiple selected entries are transferred as NULL if the
            // binding does not support string lists
            if ( _rSelectSequence.getLength() <= 1 )
            {
                if ( _rSelectSequence.getLength() == 1 )
                    aReturn = ExtractAnyFromValueList_Safe( _rStringList )( _rSelectSequence[0] );
            }

            return aReturn;
        }


        Sequence< Any > lcl_getMultiSelectedEntriesAny( const Sequence< sal_Int16 >& _rSelectSequence, const ValueList& _rStringList )
        {
            Sequence< Any > aSelectedEntriesValues( _rSelectSequence.getLength() );
            ::std::transform(
                _rSelectSequence.begin(),
                _rSelectSequence.end(),
                aSelectedEntriesValues.getArray(),
                ExtractAnyFromValueList_Safe( _rStringList )
            );
            return aSelectedEntriesValues;
        }
    }


    Any OListBoxModel::translateControlValueToExternalValue( ) const
    {
        OSL_PRECOND( hasExternalValueBinding(), "OListBoxModel::translateControlValueToExternalValue: no binding!" );

        Sequence< sal_Int16 > aSelectSequence;
        OSL_VERIFY( getControlValue() >>= aSelectSequence );

        Any aReturn;
        switch ( lcl_getCurrentExchangeType( getExternalValueType() ) )
        {
        case eValueList:
            aReturn <<= getCurrentMultiValue();
            break;

        case eValue:
            aReturn = getCurrentSingleValue();
            break;

        case eIndexList:
        {
            // unfortunately, the select sequence is a sequence<short>, but our binding
            // expects int's
            Sequence< sal_Int32 > aTransformed( aSelectSequence.getLength() );
            ::std::copy(
                std::cbegin(aSelectSequence),
                std::cend(aSelectSequence),
                aTransformed.getArray()
            );
            aReturn <<= aTransformed;
        }
        break;

        case eIndex:
        if ( aSelectSequence.getLength() <= 1 )
        {
            sal_Int32 nIndex = -1;

            if ( aSelectSequence.getLength() == 1 )
                nIndex = aSelectSequence[0];

            aReturn <<= nIndex;
        }
        break;

        case eEntryList:
            aReturn = lcl_getMultiSelectedEntries( aSelectSequence, getStringItemList() );
            break;

        case eEntry:
            {
                const std::vector<OUString>& rStrings = getStringItemList();
                const Sequence<Any>& rValues = getTypedItemList();
                if (rStrings.size() == static_cast<size_t>(rValues.getLength()))
                    aReturn = lcl_getSingleSelectedEntryTyped( aSelectSequence, rValues );
                else
                    aReturn = lcl_getSingleSelectedEntry( aSelectSequence, rStrings );
            }
            break;
        }

        return aReturn;
    }


    Any OListBoxModel::translateControlValueToValidatableValue( ) const
    {
        OSL_PRECOND( hasValidator(), "OListBoxModel::translateControlValueToValidatableValue: no validator, so why should I?" );
        return getCurrentFormComponentValue();
    }


    Any OListBoxModel::getCurrentSingleValue() const
    {
        Any aCurrentValue;

        try
        {
            Sequence< sal_Int16 > aSelectSequence;
            OSL_VERIFY( getControlValue() >>= aSelectSequence );
            aCurrentValue = lcl_getSingleSelectedEntryAny( aSelectSequence, impl_getValues() );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }

        return aCurrentValue;
    }

    Sequence< Any > OListBoxModel::getCurrentMultiValue() const
    {
        Sequence< Any > aCurrentValue;

        try
        {
            Sequence< sal_Int16 > aSelectSequence;
            OSL_VERIFY( getControlValue() >>= aSelectSequence );
            aCurrentValue = lcl_getMultiSelectedEntriesAny( aSelectSequence, impl_getValues() );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }

        return aCurrentValue;
    }

    Any OListBoxModel::getCurrentFormComponentValue() const
    {
        {
            Reference< css::form::validation::XValidator > vtor (const_cast<OListBoxModel*>(this)->getValidator());
            Reference< XValueBinding > extBinding (const_cast<OListBoxModel*>(this)->getValueBinding());
            if ( vtor.is() && vtor == extBinding )
                return translateControlValueToExternalValue();
        }

        Any aCurrentValue;

        try
        {
            bool bMultiSelection( false );
            OSL_VERIFY( const_cast< OListBoxModel* >( this )->getPropertyValue( PROPERTY_MULTISELECTION ) >>= bMultiSelection );

            if ( bMultiSelection )
                aCurrentValue <<= getCurrentMultiValue();
            else
                aCurrentValue = getCurrentSingleValue();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }

        return aCurrentValue;
    }


    Sequence< Type > OListBoxModel::getSupportedBindingTypes()
    {
        return
        {
            cppu::UnoType<Sequence< Any >>::get(),
            cppu::UnoType<Any>::get(),
            cppu::UnoType<Sequence< sal_Int32 >>::get(),
            cppu::UnoType<sal_Int32>::get(),
            cppu::UnoType<Sequence< OUString >>::get(),
            cppu::UnoType<OUString>::get()
        };
    }


    void OListBoxModel::stringItemListChanged( ControlModelLock& _rInstanceLock )
    {
        if ( !m_xAggregateSet.is() )
            return;

        suspendValueListening();
        try
        {
            m_xAggregateSet->setPropertyValue( PROPERTY_STRINGITEMLIST, Any( comphelper::containerToSequence(getStringItemList()) ) );
            m_xAggregateSet->setPropertyValue( PROPERTY_TYPEDITEMLIST, Any( getTypedItemList() ) );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("forms.component");
        }
        resumeValueListening();

        // update the selection here
        if ( hasExternalValueBinding( ) )
            transferExternalValueToControl( _rInstanceLock );
        else
        {
            if ( hasField() )
            {
                // TODO: update the selection in case we're bound to a database column
            }
            else
            {
                if ( m_aDefaultSelectSeq.hasElements() )
                    setControlValue( Any( m_aDefaultSelectSeq ), eOther );
            }
        }
    }


    void OListBoxModel::impl_refreshDbEntryList( bool _bForce )
    {
        DBG_ASSERT( !hasExternalListSource(), "OListBoxModel::impl_refreshDbEntryList: invalid call!" );

        if  (   !hasExternalListSource( )
            &&  ( m_eListSourceType != ListSourceType_VALUELIST )
            &&  ( m_xCursor.is() )
            )
        {
            loadData( _bForce );
        }
    }


    void OListBoxModel::refreshInternalEntryList()
    {
        impl_refreshDbEntryList( true );
        if ( hasField() && m_xCursor.is() )
            initFromField( m_xCursor );
    }


    // OListBoxControl

    Sequence< Type> OListBoxControl::_getTypes()
    {
        return TypeBag(
            OBoundControl::_getTypes(),
            OListBoxControl_BASE::getTypes()
        ).getTypes();
    }


    Any SAL_CALL OListBoxControl::queryAggregation(const Type& _rType)
    {
        Any aReturn = OListBoxControl_BASE::queryInterface( _rType );

        if  (   !aReturn.hasValue()
            ||  _rType.equals( cppu::UnoType<XTypeProvider>::get() )
            )
            aReturn = OBoundControl::queryAggregation( _rType );

        return aReturn;
    }


    OListBoxControl::OListBoxControl(const Reference<XComponentContext>& _rxFactory)
        :OBoundControl( _rxFactory, VCL_CONTROL_LISTBOX, false )
        ,m_aChangeListeners( m_aMutex )
        ,m_aItemListeners( m_aMutex )
        ,m_aChangeIdle("forms OListBoxControl m_aChangedIdle")
    {

        osl_atomic_increment(&m_refCount);
        {
            // Register as FocusListener
            if (auto xComp = query_aggregation<XWindow>(m_xAggregate))
                xComp->addFocusListener(this);

            // Register as ItemListener
            if ( query_aggregation( m_xAggregate, m_xAggregateListBox ) )
                m_xAggregateListBox->addItemListener(this);
        }
        // Refcount at 2 for registered Listener
        osl_atomic_decrement(&m_refCount);

        doSetDelegator();

        m_aChangeIdle.SetPriority(TaskPriority::LOWEST);
        m_aChangeIdle.SetInvokeHandler(LINK(this,OListBoxControl,OnTimeout));
    }


    OListBoxControl::~OListBoxControl()
    {
        if (!OComponentHelper::rBHelper.bDisposed)
        {
            acquire();
            dispose();
        }

        doResetDelegator();
        m_xAggregateListBox.clear();

    }


    css::uno::Sequence<OUString> SAL_CALL OListBoxControl::getSupportedServiceNames()
    {
        css::uno::Sequence<OUString> aSupported = OBoundControl::getSupportedServiceNames();
        aSupported.realloc(aSupported.getLength() + 2);

        OUString* pArray = aSupported.getArray();
        pArray[aSupported.getLength()-2] = FRM_SUN_CONTROL_LISTBOX;
        pArray[aSupported.getLength()-1] = STARDIV_ONE_FORM_CONTROL_LISTBOX;
        return aSupported;
    }


    // XFocusListener

    void SAL_CALL OListBoxControl::focusGained(const FocusEvent& /*_rEvent*/)
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        if ( m_aChangeListeners.getLength() ) // only if there are listeners
        {
            Reference<XPropertySet> xSet(getModel(), UNO_QUERY);
            if (xSet.is())
            {
                // memorize the current selection for posting the change event
                m_aCurrentSelection = xSet->getPropertyValue(PROPERTY_SELECT_SEQ);
            }
        }
    }


    void SAL_CALL OListBoxControl::focusLost(const FocusEvent& /*_rEvent*/)
    {
        m_aCurrentSelection.clear();
    }

    // XItemListener

    void SAL_CALL OListBoxControl::itemStateChanged(const ItemEvent& _rEvent)
    {
        // forward this to our listeners
        Reference< XChild > xChild( getModel(), UNO_QUERY );
        if ( xChild.is() && xChild->getParent().is() )
        {
            ::osl::MutexGuard aGuard( m_aMutex );
            if ( m_aItemListeners.getLength() )
            {
                if ( !m_pItemBroadcaster.is() )
                {
                    m_pItemBroadcaster.set(
                        new ::comphelper::AsyncEventNotifier("ListBox"));
                    m_pItemBroadcaster->launch();
                }
                m_pItemBroadcaster->addEvent( new ItemEventDescription( _rEvent ), this );
            }
        }
        else
            m_aItemListeners.notifyEach( &XItemListener::itemStateChanged, _rEvent );

        // and do the handling for the ChangeListeners
        osl::MutexGuard aGuard(m_aMutex);
        if ( m_aChangeIdle.IsActive() )
        {
            Reference<XPropertySet> xSet(getModel(), UNO_QUERY);
            m_aCurrentSelection = xSet->getPropertyValue(PROPERTY_SELECT_SEQ);

            m_aChangeIdle.Stop();
            m_aChangeIdle.Start();
        }
        else
        {
            if ( m_aChangeListeners.getLength() && m_aCurrentSelection.hasValue() )
            {
                Reference<XPropertySet> xSet(getModel(), UNO_QUERY);
                if (xSet.is())
                {
                    // Has the selection been changed?
                    bool bModified(false);
                    Any aValue = xSet->getPropertyValue(PROPERTY_SELECT_SEQ);

                    Sequence<sal_Int16> const & rSelection = *o3tl::doAccess<Sequence<sal_Int16>>(aValue);
                    Sequence<sal_Int16> const & rOldSelection = *o3tl::doAccess<Sequence<sal_Int16>>(m_aCurrentSelection);
                    sal_Int32 nLen = rSelection.getLength();
                    if (nLen != rOldSelection.getLength())
                        bModified = true;
                    else
                    {
                        const sal_Int16* pVal = rSelection.getConstArray();
                        const sal_Int16* pCompVal = rOldSelection.getConstArray();

                        while (nLen-- && !bModified)
                            bModified = pVal[nLen] != pCompVal[nLen];
                    }

                    if (bModified)
                    {
                        m_aCurrentSelection = std::move(aValue);
                        m_aChangeIdle.Start();
                    }
                }
            }
            else if (m_aCurrentSelection.hasValue())
                m_aCurrentSelection.clear();
        }
    }

    // XEventListener

    void SAL_CALL OListBoxControl::disposing(const EventObject& _rSource)
    {
        OBoundControl::disposing(_rSource);
    }

    // XChangeBroadcaster

    void SAL_CALL OListBoxControl::addChangeListener(const Reference<XChangeListener>& _rxListener)
    {
        m_aChangeListeners.addInterface( _rxListener );
    }


    void SAL_CALL OListBoxControl::removeChangeListener(const Reference<XChangeListener>& _rxListener)
    {
        m_aChangeListeners.removeInterface( _rxListener );
    }

    // OComponentHelper

    void OListBoxControl::disposing()
    {
        if (m_aChangeIdle.IsActive())
            m_aChangeIdle.Stop();

        EventObject aEvent( *this );
        m_aChangeListeners.disposeAndClear( aEvent );
        m_aItemListeners.disposeAndClear( aEvent );

        rtl::Reference< comphelper::AsyncEventNotifier > t;
        {
            ::osl::MutexGuard aGuard( m_aMutex );
            if ( m_pItemBroadcaster.is() )
            {
                t = m_pItemBroadcaster;
                m_pItemBroadcaster->removeEventsForProcessor( this );
                m_pItemBroadcaster->terminate();
                m_pItemBroadcaster = nullptr;
            }
        }
        if (t.is()) {
            t->join();
        }

        OBoundControl::disposing();
    }


    void OListBoxControl::processEvent( const AnyEvent& _rEvent )
    {
        Reference< XListBox > xKeepAlive( this );
        {
            ::osl::MutexGuard aGuard( m_aMutex );
            if ( OComponentHelper::rBHelper.bDisposed )
                return;
        }
        const ItemEventDescription& rItemEvent = static_cast< const ItemEventDescription& >( _rEvent );
        m_aItemListeners.notifyEach( &XItemListener::itemStateChanged, rItemEvent.getEventObject() );
    }


    IMPL_LINK_NOARG(OListBoxControl, OnTimeout, Timer*, void)
    {
        m_aChangeListeners.notifyEach( &XChangeListener::changed, EventObject( *this ) );
    }


    void SAL_CALL OListBoxControl::addItemListener( const Reference< XItemListener >& l )
    {
        m_aItemListeners.addInterface( l );
    }


    void SAL_CALL OListBoxControl::removeItemListener( const Reference< XItemListener >& l )
    {
        m_aItemListeners.removeInterface( l );
    }


    void SAL_CALL OListBoxControl::addActionListener( const Reference< XActionListener >& l )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->addActionListener( l );
    }


    void SAL_CALL OListBoxControl::removeActionListener( const Reference< XActionListener >& l )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->removeActionListener( l );
    }


    void SAL_CALL OListBoxControl::addItem( const OUString& aItem, ::sal_Int16 nPos )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->addItem( aItem, nPos );
    }


    void SAL_CALL OListBoxControl::addItems( const Sequence< OUString >& aItems, ::sal_Int16 nPos )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->addItems( aItems, nPos );
    }


    void SAL_CALL OListBoxControl::removeItems( ::sal_Int16 nPos, ::sal_Int16 nCount )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->removeItems( nPos, nCount );
    }


    ::sal_Int16 SAL_CALL OListBoxControl::getItemCount(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getItemCount();
        return 0;
    }


    OUString SAL_CALL OListBoxControl::getItem( ::sal_Int16 nPos )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getItem( nPos );
        return OUString( );
    }


    Sequence< OUString > SAL_CALL OListBoxControl::getItems(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getItems();
        return Sequence< OUString >( );
    }


    ::sal_Int16 SAL_CALL OListBoxControl::getSelectedItemPos(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getSelectedItemPos();
        return -1;
    }


    Sequence< ::sal_Int16 > SAL_CALL OListBoxControl::getSelectedItemsPos(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getSelectedItemsPos();
        return Sequence< ::sal_Int16 >( );
    }


    OUString SAL_CALL OListBoxControl::getSelectedItem(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getSelectedItem();
        return OUString( );
    }


    Sequence< OUString > SAL_CALL OListBoxControl::getSelectedItems(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getSelectedItems();
        return Sequence< OUString >( );
    }


    void SAL_CALL OListBoxControl::selectItemPos( ::sal_Int16 nPos, sal_Bool bSelect )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->selectItemPos( nPos, bSelect );
    }


    void SAL_CALL OListBoxControl::selectItemsPos( const Sequence< ::sal_Int16 >& aPositions, sal_Bool bSelect )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->selectItemsPos( aPositions, bSelect );
    }


    void SAL_CALL OListBoxControl::selectItem( const OUString& aItem, sal_Bool bSelect )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->selectItem( aItem, bSelect );
    }


    sal_Bool SAL_CALL OListBoxControl::isMutipleMode(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->isMutipleMode();
        return false;
    }


    void SAL_CALL OListBoxControl::setMultipleMode( sal_Bool bMulti )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->setMultipleMode( bMulti );
    }


    ::sal_Int16 SAL_CALL OListBoxControl::getDropDownLineCount(  )
    {
        if ( m_xAggregateListBox.is() )
            return m_xAggregateListBox->getDropDownLineCount();
        return 0;
    }


    void SAL_CALL OListBoxControl::setDropDownLineCount( ::sal_Int16 nLines )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->setDropDownLineCount( nLines );
    }


    void SAL_CALL OListBoxControl::makeVisible( ::sal_Int16 nEntry )
    {
        if ( m_xAggregateListBox.is() )
            m_xAggregateListBox->makeVisible( nEntry );
    }

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OListBoxModel_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OListBoxModel(component));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OListBoxControl_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OListBoxControl(component));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

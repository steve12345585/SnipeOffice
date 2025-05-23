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

#include "Columns.hxx"
#include "findpos.hxx"
#include "Grid.hxx"
#include <property.hxx>
#include <services.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/io/XMarkableStream.hpp>
#include <com/sun/star/text/WritingMode2.hpp>
#include <comphelper/basicio.hxx>
#include <comphelper/property.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/types.hxx>
#include <vcl/unohelp.hxx>
#include <vcl/svapp.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>

using namespace ::com::sun::star::uno;

namespace frm
{
using namespace ::com::sun::star;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::view;
namespace WritingMode2 = ::com::sun::star::text::WritingMode2;
const sal_uInt16 ROWHEIGHT          =   0x0001;
const sal_uInt16 FONTTYPE           =   0x0002;
const sal_uInt16 FONTSIZE           =   0x0004;
const sal_uInt16 FONTATTRIBS        =   0x0008;
const sal_uInt16 TABSTOP            =   0x0010;
const sal_uInt16 TEXTCOLOR          =   0x0020;
const sal_uInt16 FONTDESCRIPTOR     =   0x0040;
const sal_uInt16 RECORDMARKER       =   0x0080;
const sal_uInt16 BACKGROUNDCOLOR    =   0x0100;

OGridControlModel::OGridControlModel(const Reference<XComponentContext>& _rxFactory)
    :OControlModel(_rxFactory, OUString())
    ,OInterfaceContainer(_rxFactory, m_aMutex, cppu::UnoType<XPropertySet>::get())
    ,OErrorBroadcaster( OComponentHelper::rBHelper )
    ,FontControlModel( false )
    ,m_aSelectListeners(m_aMutex)
    ,m_aResetListeners(m_aMutex)
    ,m_aRowSetChangeListeners(m_aMutex)
    ,m_aDefaultControl( FRM_SUN_CONTROL_GRIDCONTROL )
    ,m_nBorder(1)
    ,m_nWritingMode( WritingMode2::CONTEXT )
    ,m_nContextWritingMode( WritingMode2::CONTEXT )
    ,m_bEnableVisible(true)
    ,m_bEnable(true)
    ,m_bNavigation(true)
    ,m_bRecordMarker(true)
    ,m_bPrintable(true)
    ,m_bAlwaysShowCursor(false)
    ,m_bDisplaySynchron(true)
{
    m_nClassId = FormComponentType::GRIDCONTROL;
}

OGridControlModel::OGridControlModel( const OGridControlModel* _pOriginal, const Reference< XComponentContext >& _rxFactory )
    :OControlModel( _pOriginal, _rxFactory )
    ,OInterfaceContainer( _rxFactory, m_aMutex, cppu::UnoType<XPropertySet>::get() )
    ,OErrorBroadcaster( OComponentHelper::rBHelper )
    ,FontControlModel( _pOriginal )
    ,m_aSelectListeners( m_aMutex )
    ,m_aResetListeners( m_aMutex )
    ,m_aRowSetChangeListeners( m_aMutex )
{
    m_aDefaultControl = _pOriginal->m_aDefaultControl;
    m_bEnable = _pOriginal->m_bEnable;
    m_bEnableVisible = _pOriginal->m_bEnableVisible;
    m_bNavigation = _pOriginal->m_bNavigation;
    m_nBorder = _pOriginal->m_nBorder;
    m_nWritingMode = _pOriginal->m_nWritingMode;
    m_nContextWritingMode = _pOriginal->m_nContextWritingMode;
    m_bRecordMarker = _pOriginal->m_bRecordMarker;
    m_bPrintable = _pOriginal->m_bPrintable;
    m_bAlwaysShowCursor = _pOriginal->m_bAlwaysShowCursor;
    m_bDisplaySynchron = _pOriginal->m_bDisplaySynchron;
    // clone the columns
    cloneColumns( _pOriginal );
    // TODO: clone the events?
}

OGridControlModel::~OGridControlModel()
{
    if (!OComponentHelper::rBHelper.bDisposed)
    {
        acquire();
        dispose();
    }
}

// XCloneable
Reference< XCloneable > SAL_CALL OGridControlModel::createClone( )
{
    rtl::Reference<OGridControlModel> pClone = new OGridControlModel( this, getContext() );
    pClone->OControlModel::clonedFrom( this );
    // do not call OInterfaceContainer::clonedFrom, it would clone the elements aka columns, which is
    // already done in the ctor
    //pClone->OInterfaceContainer::clonedFrom( *this );
    return static_cast< XCloneable* >( static_cast< OControlModel* >( pClone.get() ) );
}

void OGridControlModel::cloneColumns( const OGridControlModel* _pOriginalContainer )
{
    try
    {
        Reference< XCloneable > xColCloneable;
        sal_Int32 nIndex = 0;
        for (auto const& column : _pOriginalContainer->m_aItems)
        {
            // ask the col for a factory for the clone
            xColCloneable.set(column, css::uno::UNO_QUERY);
            DBG_ASSERT( xColCloneable.is(), "OGridControlModel::cloneColumns: column is not cloneable!" );
            if ( xColCloneable.is() )
            {
                // create a clone of the column
                Reference< XCloneable > xColClone( xColCloneable->createClone() );
                DBG_ASSERT( xColClone.is(), "OGridControlModel::cloneColumns: invalid column clone!" );
                if ( xColClone.is() )
                {
                    // insert this clone into our own container
                    insertByIndex( nIndex, xColClone->queryInterface( m_aElementType ) );
                }
            }
            ++nIndex;
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "forms.component", "OGridControlModel::cloneColumns: caught an exception while cloning the columns!" );
    }
}

// XServiceInfo
css::uno::Sequence<OUString> OGridControlModel::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> aSupported = OControlModel::getSupportedServiceNames();
    aSupported.realloc(aSupported.getLength() + 4);
    auto pSupported = aSupported.getArray();
    pSupported[aSupported.getLength()-4] = "com.sun.star.awt.UnoControlModel";
    pSupported[aSupported.getLength()-3] = FRM_SUN_COMPONENT_GRIDCONTROL;
    pSupported[aSupported.getLength()-2] = FRM_COMPONENT_GRID;
    pSupported[aSupported.getLength()-1] = FRM_COMPONENT_GRIDCONTROL;
    return aSupported;
}
Any SAL_CALL OGridControlModel::queryAggregation( const Type& _rType )
{
    Any aReturn = OGridControlModel_BASE::queryInterface(_rType);
    if ( !aReturn.hasValue() )
    {
        aReturn = OControlModel::queryAggregation( _rType );
        if ( !aReturn.hasValue() )
        {
            aReturn = OInterfaceContainer::queryInterface( _rType );
            if ( !aReturn.hasValue() )
                aReturn = OErrorBroadcaster::queryInterface( _rType );
        }
    }
    return aReturn;
}

// XSQLErrorListener
void SAL_CALL OGridControlModel::errorOccured( const SQLErrorEvent& _rEvent )
{
    // forward the errors which happened to my columns to my own listeners
    onError( _rEvent );
}

// XRowSetSupplier
Reference< XRowSet > SAL_CALL OGridControlModel::getRowSet(  )
{
    return Reference< XRowSet >( getParent(), UNO_QUERY );
}

void SAL_CALL OGridControlModel::setRowSet( const Reference< XRowSet >& /*_rxDataSource*/ )
{
    OSL_FAIL( "OGridControlModel::setRowSet: not supported!" );
}

void SAL_CALL OGridControlModel::addRowSetChangeListener( const Reference< XRowSetChangeListener >& i_Listener )
{
    if ( i_Listener.is() )
        m_aRowSetChangeListeners.addInterface( i_Listener );
}

void SAL_CALL OGridControlModel::removeRowSetChangeListener( const Reference< XRowSetChangeListener >& i_Listener )
{
    m_aRowSetChangeListeners.removeInterface( i_Listener );
}

// XChild
void SAL_CALL OGridControlModel::setParent( const css::uno::Reference<css::uno::XInterface>& i_Parent )
{
    ::osl::ClearableMutexGuard aGuard( m_aMutex );
    if ( i_Parent == getParent() )
        return;
    OControlModel::setParent( i_Parent );
    EventObject aEvent( *this );
    aGuard.clear();
    m_aRowSetChangeListeners.notifyEach( &XRowSetChangeListener::onRowSetChanged, aEvent );
}
Sequence< Type > SAL_CALL OGridControlModel::getTypes(  )
{
    return concatSequences(
            OControlModel::getTypes(),
            OInterfaceContainer::getTypes(),
            OErrorBroadcaster::getTypes(),
        OGridControlModel_BASE::getTypes()
    );
}

// OComponentHelper
void OGridControlModel::disposing()
{
    OControlModel::disposing();
    OErrorBroadcaster::disposing();
    OInterfaceContainer::disposing();
    setParent(nullptr);
    EventObject aEvt(static_cast<XWeak*>(this));
    m_aSelectListeners.disposeAndClear(aEvt);
    m_aResetListeners.disposeAndClear(aEvt);
    m_aRowSetChangeListeners.disposeAndClear(aEvt);
}

// XEventListener
void OGridControlModel::disposing(const EventObject& _rEvent)
{
    OControlModel::disposing( _rEvent );
    OInterfaceContainer::disposing( _rEvent );
}

// XSelectionSupplier
sal_Bool SAL_CALL OGridControlModel::select(const Any& rElement)
{
    ::osl::ClearableMutexGuard aGuard( m_aMutex );
    Reference<XPropertySet> xSel;
    if (rElement.hasValue())
    {
        xSel.set(rElement, css::uno::UNO_QUERY);
        if (!xSel.is())
        {
            throw IllegalArgumentException();
        }
    }
    css::uno::Reference<css::uno::XInterface> xMe = static_cast<XWeak*>(this);
    if (xSel.is())
    {
        Reference<XChild> xAsChild(xSel, UNO_QUERY);
        if (!xAsChild.is() || (xAsChild->getParent() != xMe))
        {
            throw IllegalArgumentException();
        }
    }
    if ( xSel != m_xSelection )
    {
        m_xSelection = std::move(xSel);
        aGuard.clear();
        m_aSelectListeners.notifyEach( &XSelectionChangeListener::selectionChanged, EventObject( *this ) );
        return true;
    }
    return false;
}
Any SAL_CALL OGridControlModel::getSelection()
{
    return Any(m_xSelection);
}

void OGridControlModel::addSelectionChangeListener(const Reference< XSelectionChangeListener >& _rxListener)
{
    m_aSelectListeners.addInterface(_rxListener);
}

void OGridControlModel::removeSelectionChangeListener(const Reference< XSelectionChangeListener >& _rxListener)
{
    m_aSelectListeners.removeInterface(_rxListener);
}

// XGridColumnFactory
Reference<XPropertySet> SAL_CALL OGridControlModel::createColumn(const OUString& ColumnType)
{
    SolarMutexGuard g;
    const Sequence< OUString >& rColumnTypes = frm::getColumnTypes();
    return createColumnById( ::detail::findPos( ColumnType, rColumnTypes ) );
}
rtl::Reference<OGridColumn>  OGridControlModel::createColumnById(sal_Int32 nTypeId) const
{
    rtl::Reference<OGridColumn>  xReturn;
    switch (nTypeId)
    {
        case TYPE_CHECKBOX:         xReturn = new CheckBoxColumn( getContext() ); break;
        case TYPE_COMBOBOX:         xReturn = new ComboBoxColumn( getContext() ); break;
        case TYPE_CURRENCYFIELD:    xReturn = new CurrencyFieldColumn( getContext() ); break;
        case TYPE_DATEFIELD:        xReturn = new DateFieldColumn( getContext() ); break;
        case TYPE_LISTBOX:          xReturn = new ListBoxColumn( getContext() ); break;
        case TYPE_NUMERICFIELD:     xReturn = new NumericFieldColumn( getContext() ); break;
        case TYPE_PATTERNFIELD:     xReturn = new PatternFieldColumn( getContext() ); break;
        case TYPE_TEXTFIELD:        xReturn = new TextFieldColumn( getContext() ); break;
        case TYPE_TIMEFIELD:        xReturn = new TimeFieldColumn( getContext() ); break;
        case TYPE_FORMATTEDFIELD:   xReturn = new FormattedFieldColumn( getContext() ); break;
        default:
            OSL_FAIL("OGridControlModel::createColumn: Unknown Column");
            break;
    }
    return xReturn;
}
css::uno::Sequence<OUString> SAL_CALL OGridControlModel::getColumnTypes()
{
    return frm::getColumnTypes();
}

// XReset
void SAL_CALL OGridControlModel::reset()
{
    ::comphelper::OInterfaceIteratorHelper3 aIter(m_aResetListeners);
    EventObject aEvt(static_cast<XWeak*>(this));
    bool bContinue = true;
    while (aIter.hasMoreElements() && bContinue)
        bContinue = aIter.next()->approveReset(aEvt);
    if (bContinue)
    {
        _reset();
        m_aResetListeners.notifyEach( &XResetListener::resetted, aEvt );
    }
}
void SAL_CALL OGridControlModel::addResetListener(const Reference<XResetListener>& _rxListener)
{
    m_aResetListeners.addInterface(_rxListener);
}
void SAL_CALL OGridControlModel::removeResetListener(const Reference<XResetListener>& _rxListener)
{
    m_aResetListeners.removeInterface(_rxListener);
}
void OGridControlModel::_reset()
{
    Reference<XReset> xReset;
    sal_Int32 nCount = getCount();
    for (sal_Int32 nIndex=0; nIndex < nCount; nIndex++)
    {
        getByIndex( nIndex ) >>= xReset;
        if (xReset.is())
            xReset->reset();
    }
}

// XPropertySet
void OGridControlModel::describeFixedProperties( Sequence< Property >& _rProps ) const
{
    _rProps.realloc(37);
    css::beans::Property* pProperties = _rProps.getArray();
    *pProperties++ = css::beans::Property(PROPERTY_NAME, PROPERTY_ID_NAME, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_CLASSID, PROPERTY_ID_CLASSID, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::READONLY | css::beans::PropertyAttribute::TRANSIENT);
    *pProperties++ = css::beans::Property(PROPERTY_TAG, PROPERTY_ID_TAG, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TABINDEX, PROPERTY_ID_TABINDEX, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TABSTOP, PROPERTY_ID_TABSTOP, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_HASNAVIGATION, PROPERTY_ID_HASNAVIGATION, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_ENABLED, PROPERTY_ID_ENABLED, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_ENABLEVISIBLE, PROPERTY_ID_ENABLEVISIBLE, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_BORDER, PROPERTY_ID_BORDER, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_BORDERCOLOR, PROPERTY_ID_BORDERCOLOR, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_DEFAULTCONTROL, PROPERTY_ID_DEFAULTCONTROL, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TEXTCOLOR, PROPERTY_ID_TEXTCOLOR, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_BACKGROUNDCOLOR, PROPERTY_ID_BACKGROUNDCOLOR, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_FONT, PROPERTY_ID_FONT, cppu::UnoType<FontDescriptor>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_ROWHEIGHT, PROPERTY_ID_ROWHEIGHT, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_HELPTEXT, PROPERTY_ID_HELPTEXT, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_NAME, PROPERTY_ID_FONT_NAME, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_STYLENAME, PROPERTY_ID_FONT_STYLENAME, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_FAMILY, PROPERTY_ID_FONT_FAMILY, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_CHARSET, PROPERTY_ID_FONT_CHARSET, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_HEIGHT, PROPERTY_ID_FONT_HEIGHT, cppu::UnoType<float>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_WEIGHT, PROPERTY_ID_FONT_WEIGHT, cppu::UnoType<float>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_SLANT, PROPERTY_ID_FONT_SLANT, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_UNDERLINE, PROPERTY_ID_FONT_UNDERLINE, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_WORDLINEMODE, PROPERTY_ID_FONT_WORDLINEMODE, cppu::UnoType<bool>::get(),
                                          css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_TEXTLINECOLOR, PROPERTY_ID_TEXTLINECOLOR, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::MAYBEVOID);
    *pProperties++ = css::beans::Property(PROPERTY_FONTEMPHASISMARK, PROPERTY_ID_FONTEMPHASISMARK, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONTRELIEF, PROPERTY_ID_FONTRELIEF, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_FONT_STRIKEOUT, PROPERTY_ID_FONT_STRIKEOUT, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_RECORDMARKER, PROPERTY_ID_RECORDMARKER, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_PRINTABLE, PROPERTY_ID_PRINTABLE, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_CURSORCOLOR, PROPERTY_ID_CURSORCOLOR, cppu::UnoType<sal_Int32>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT |
                                          css::beans::PropertyAttribute::MAYBEVOID | css::beans::PropertyAttribute::TRANSIENT);
    *pProperties++ = css::beans::Property(PROPERTY_ALWAYSSHOWCURSOR, PROPERTY_ID_ALWAYSSHOWCURSOR, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::TRANSIENT);
    *pProperties++ = css::beans::Property(PROPERTY_DISPLAYSYNCHRON, PROPERTY_ID_DISPLAYSYNCHRON, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::TRANSIENT);
    *pProperties++ = css::beans::Property(PROPERTY_HELPURL, PROPERTY_ID_HELPURL, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_WRITING_MODE, PROPERTY_ID_WRITING_MODE, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT);
    *pProperties++ = css::beans::Property(PROPERTY_CONTEXT_WRITING_MODE, PROPERTY_ID_CONTEXT_WRITING_MODE, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND | css::beans::PropertyAttribute::MAYBEDEFAULT | css::beans::PropertyAttribute::TRANSIENT);
    DBG_ASSERT( pProperties == _rProps.getArray() + _rProps.getLength(), "<...>::describeFixedProperties/getInfoHelper: forgot to adjust the count ?");
}
void OGridControlModel::getFastPropertyValue(Any& rValue, sal_Int32 nHandle ) const
{
    switch (nHandle)
    {
        case PROPERTY_ID_CONTEXT_WRITING_MODE:
            rValue <<= m_nContextWritingMode;
            break;
        case PROPERTY_ID_WRITING_MODE:
            rValue <<= m_nWritingMode;
            break;
        case PROPERTY_ID_HELPTEXT:
            rValue <<= m_sHelpText;
            break;
        case PROPERTY_ID_HELPURL:
            rValue <<= m_sHelpURL;
            break;
        case PROPERTY_ID_DISPLAYSYNCHRON:
            rValue <<= m_bDisplaySynchron;
            break;
        case PROPERTY_ID_ALWAYSSHOWCURSOR:
            rValue <<= m_bAlwaysShowCursor;
            break;
        case PROPERTY_ID_CURSORCOLOR:
            rValue = m_aCursorColor;
            break;
        case PROPERTY_ID_PRINTABLE:
            rValue <<= m_bPrintable;
            break;
        case PROPERTY_ID_TABSTOP:
            rValue = m_aTabStop;
            break;
        case PROPERTY_ID_HASNAVIGATION:
            rValue <<= m_bNavigation;
            break;
        case PROPERTY_ID_RECORDMARKER:
            rValue <<= m_bRecordMarker;
            break;
        case PROPERTY_ID_ENABLED:
            rValue <<= m_bEnable;
            break;
        case PROPERTY_ID_ENABLEVISIBLE:
            rValue <<= m_bEnableVisible;
            break;
        case PROPERTY_ID_BORDER:
            rValue <<= m_nBorder;
            break;
        case PROPERTY_ID_BORDERCOLOR:
            rValue = m_aBorderColor;
            break;
        case PROPERTY_ID_DEFAULTCONTROL:
            rValue <<= m_aDefaultControl;
            break;
        case PROPERTY_ID_BACKGROUNDCOLOR:
            rValue = m_aBackgroundColor;
            break;
        case PROPERTY_ID_ROWHEIGHT:
            rValue = m_aRowHeight;
            break;
        default:
            if ( isFontRelatedProperty( nHandle ) )
                FontControlModel::getFastPropertyValue( rValue, nHandle );
            else
                OControlModel::getFastPropertyValue( rValue, nHandle );
    }
}

sal_Bool OGridControlModel::convertFastPropertyValue( Any& rConvertedValue, Any& rOldValue,
                                                    sal_Int32 nHandle, const Any& rValue )
{
    bool bModified(false);
    switch (nHandle)
    {
        case PROPERTY_ID_CONTEXT_WRITING_MODE:
            bModified = tryPropertyValue( rConvertedValue, rOldValue, rValue, m_nContextWritingMode );
            break;
        case PROPERTY_ID_WRITING_MODE:
            bModified = tryPropertyValue( rConvertedValue, rOldValue, rValue, m_nWritingMode );
            break;
        case PROPERTY_ID_HELPTEXT:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_sHelpText);
            break;
        case PROPERTY_ID_HELPURL:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_sHelpURL);
            break;
        case PROPERTY_ID_DISPLAYSYNCHRON:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bDisplaySynchron);
            break;
        case PROPERTY_ID_ALWAYSSHOWCURSOR:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bAlwaysShowCursor);
            break;
        case PROPERTY_ID_CURSORCOLOR:
            if (!rValue.hasValue() || !m_aCursorColor.hasValue())
            {
                if (rValue.hasValue() && (TypeClass_LONG != rValue.getValueTypeClass()))
                {
                    throw IllegalArgumentException();
                }
                rOldValue = m_aCursorColor;
                rConvertedValue = rValue;
                bModified = rOldValue != rConvertedValue;
            }
            else
                bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, getINT32(m_aCursorColor));
            break;
        case PROPERTY_ID_PRINTABLE:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bPrintable);
            break;
        case PROPERTY_ID_TABSTOP:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_aTabStop, cppu::UnoType<bool>::get());
            break;
        case PROPERTY_ID_HASNAVIGATION:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bNavigation);
            break;
        case PROPERTY_ID_RECORDMARKER:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bRecordMarker);
            break;
        case PROPERTY_ID_ENABLED:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bEnable);
            break;
        case PROPERTY_ID_ENABLEVISIBLE:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_bEnableVisible);
            break;
        case PROPERTY_ID_BORDER:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_nBorder);
            break;
        case PROPERTY_ID_BORDERCOLOR:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_aBorderColor, cppu::UnoType<sal_Int32>::get());
            break;
        case PROPERTY_ID_DEFAULTCONTROL:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_aDefaultControl);
            break;
        case PROPERTY_ID_BACKGROUNDCOLOR:
            bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_aBackgroundColor, cppu::UnoType<sal_Int32>::get());
            break;
        case PROPERTY_ID_ROWHEIGHT:
            {
                bModified = tryPropertyValue(rConvertedValue, rOldValue, rValue, m_aRowHeight, cppu::UnoType<sal_Int32>::get());
                sal_Int32 nNewVal( 0 );
                if ( ( rConvertedValue >>= nNewVal ) && ( nNewVal <= 0 ) )
                {
                    rConvertedValue.clear();
                    bModified = m_aRowHeight.hasValue();
                }
            }
            break;
        default:
            if ( isFontRelatedProperty( nHandle ) )
                bModified = FontControlModel::convertFastPropertyValue( rConvertedValue, rOldValue, nHandle, rValue );
            else
                bModified = OControlModel::convertFastPropertyValue( rConvertedValue, rOldValue, nHandle, rValue);
    }
    return bModified;
}
void OGridControlModel::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any& rValue )
{
    switch (nHandle)
    {
        case PROPERTY_ID_CONTEXT_WRITING_MODE:
            rValue >>= m_nContextWritingMode;
            break;
        case PROPERTY_ID_WRITING_MODE:
            rValue >>= m_nWritingMode;
            break;
        case PROPERTY_ID_HELPTEXT:
            rValue >>= m_sHelpText;
            break;
        case PROPERTY_ID_HELPURL:
            rValue >>= m_sHelpURL;
            break;
        case PROPERTY_ID_DISPLAYSYNCHRON:
            m_bDisplaySynchron = getBOOL(rValue);
            break;
        case PROPERTY_ID_ALWAYSSHOWCURSOR:
            m_bAlwaysShowCursor = getBOOL(rValue);
            break;
        case PROPERTY_ID_CURSORCOLOR:
            m_aCursorColor = rValue;
            break;
        case PROPERTY_ID_PRINTABLE:
            m_bPrintable = getBOOL(rValue);
            break;
        case PROPERTY_ID_TABSTOP:
            m_aTabStop = rValue;
            break;
        case PROPERTY_ID_HASNAVIGATION:
            m_bNavigation = getBOOL(rValue);
            break;
        case PROPERTY_ID_ENABLED:
            m_bEnable = getBOOL(rValue);
            break;
        case PROPERTY_ID_ENABLEVISIBLE:
            m_bEnableVisible = getBOOL(rValue);
            break;
        case PROPERTY_ID_RECORDMARKER:
            m_bRecordMarker = getBOOL(rValue);
            break;
        case PROPERTY_ID_BORDER:
            rValue >>= m_nBorder;
            break;
        case PROPERTY_ID_BORDERCOLOR:
            m_aBorderColor = rValue;
            break;
        case PROPERTY_ID_DEFAULTCONTROL:
            rValue >>= m_aDefaultControl;
            break;
        case PROPERTY_ID_BACKGROUNDCOLOR:
            m_aBackgroundColor = rValue;
            break;
        case PROPERTY_ID_ROWHEIGHT:
            m_aRowHeight = rValue;
            break;
        default:
            if ( isFontRelatedProperty( nHandle ) )
            {
                FontControlModel::setFastPropertyValue_NoBroadcast_impl(
                    *this, &OGridControlModel::setDependentFastPropertyValue,
                    nHandle, rValue);
            }
            else
                OControlModel::setFastPropertyValue_NoBroadcast( nHandle, rValue );
    }
}

//XPropertyState
Any OGridControlModel::getPropertyDefaultByHandle( sal_Int32 nHandle ) const
{
    Any aReturn;
    switch (nHandle)
    {
        case PROPERTY_ID_CONTEXT_WRITING_MODE:
        case PROPERTY_ID_WRITING_MODE:
            aReturn <<= WritingMode2::CONTEXT;
            break;
        case PROPERTY_ID_DEFAULTCONTROL:
            aReturn <<= STARDIV_ONE_FORM_CONTROL_GRID;
            break;
        case PROPERTY_ID_PRINTABLE:
        case PROPERTY_ID_HASNAVIGATION:
        case PROPERTY_ID_RECORDMARKER:
        case PROPERTY_ID_DISPLAYSYNCHRON:
        case PROPERTY_ID_ENABLED:
        case PROPERTY_ID_ENABLEVISIBLE:
            aReturn <<= true;
            break;
        case PROPERTY_ID_ALWAYSSHOWCURSOR:
            aReturn <<= false;
            break;
        case PROPERTY_ID_HELPURL:
        case PROPERTY_ID_HELPTEXT:
            aReturn <<= OUString();
            break;
        case PROPERTY_ID_BORDER:
            aReturn <<= sal_Int16(1);
            break;
        case PROPERTY_ID_BORDERCOLOR:
        case PROPERTY_ID_TABSTOP:
        case PROPERTY_ID_BACKGROUNDCOLOR:
        case PROPERTY_ID_ROWHEIGHT:
        case PROPERTY_ID_CURSORCOLOR:
            // void
            break;
        default:
            if ( isFontRelatedProperty( nHandle ) )
                aReturn = FontControlModel::getPropertyDefaultByHandle( nHandle );
            else
                aReturn = OControlModel::getPropertyDefaultByHandle(nHandle);
    }
    return aReturn;
}

void OGridControlModel::gotColumn( const Reference< XInterface >& _rxColumn )
{
    Reference< XSQLErrorBroadcaster > xBroadcaster( _rxColumn, UNO_QUERY );
    if ( xBroadcaster.is() )
        xBroadcaster->addSQLErrorListener( this );
}

void OGridControlModel::lostColumn(const Reference< XInterface >& _rxColumn)
{
    if ( m_xSelection == _rxColumn )
    {   // the currently selected element was replaced
        m_xSelection.clear();
        EventObject aEvt( static_cast< XWeak* >( this ) );
        m_aSelectListeners.notifyEach( &XSelectionChangeListener::selectionChanged, aEvt );
    }
    Reference< XSQLErrorBroadcaster > xBroadcaster( _rxColumn, UNO_QUERY );
    if ( xBroadcaster.is() )
        xBroadcaster->removeSQLErrorListener( this );
}

void OGridControlModel::implRemoved(const css::uno::Reference<css::uno::XInterface>& _rxObject)
{
    OInterfaceContainer::implRemoved(_rxObject);
    lostColumn(_rxObject);
}

void OGridControlModel::implInserted( const ElementDescription* _pElement )
{
    OInterfaceContainer::implInserted( _pElement );
    gotColumn( _pElement->xInterface );
}

void OGridControlModel::impl_replacedElement( const ContainerEvent& _rEvent, ::osl::ClearableMutexGuard& _rInstanceLock )
{
    Reference< XInterface > xOldColumn( _rEvent.ReplacedElement, UNO_QUERY );
    Reference< XInterface > xNewColumn( _rEvent.Element, UNO_QUERY );
    bool bNewSelection = ( xOldColumn == m_xSelection );
    lostColumn( xOldColumn );
    gotColumn( xNewColumn );
    if ( bNewSelection )
        m_xSelection.set( xNewColumn, UNO_QUERY );
    OInterfaceContainer::impl_replacedElement( _rEvent, _rInstanceLock );
    // < SYNCHRONIZED
    if ( bNewSelection )
    {
        m_aSelectListeners.notifyEach( &XSelectionChangeListener::selectionChanged, EventObject( *this ) );
    }
}

ElementDescription* OGridControlModel::createElementMetaData( )
{
    return new ElementDescription;
}

void OGridControlModel::approveNewElement( const Reference< XPropertySet >& _rxObject, ElementDescription* _pElement )
{
    OGridColumn* pCol = comphelper::getFromUnoTunnel<OGridColumn>( _rxObject );
    if ( !pCol )
        throw IllegalArgumentException();
    OInterfaceContainer::approveNewElement( _rxObject, _pElement );
}

// XPersistObject
OUString SAL_CALL OGridControlModel::getServiceName()
{
    return FRM_COMPONENT_GRID;  // old (non-sun) name for compatibility!
}

void OGridControlModel::write(const Reference<XObjectOutputStream>& _rxOutStream)
{
    OControlModel::write(_rxOutStream);
    Reference<XMarkableStream>  xMark(_rxOutStream, UNO_QUERY);
    // 1. Version
    _rxOutStream->writeShort(0x0008);
    // 2. Columns
    sal_Int32 nLen = getCount();
    _rxOutStream->writeLong(nLen);
    for (sal_Int32 i = 0; i < nLen; i++)
    {
        // first the service name for the underlying model
        OGridColumn* pCol = comphelper::getFromUnoTunnel<OGridColumn>(m_aItems[i]);
        assert(pCol != nullptr && "OGridControlModel::write : such items should never reach it into my container !");
        _rxOutStream << pCol->getModelName();
        // then the object itself
        sal_Int32 nMark = xMark->createMark();
        sal_Int32 nObjLen = 0;
        _rxOutStream->writeLong(nObjLen);
        // writing the column
        pCol->write(_rxOutStream);
        // determining the length
        nObjLen = xMark->offsetToMark(nMark) - 4;
        xMark->jumpToMark(nMark);
        _rxOutStream->writeLong(nObjLen);
        xMark->jumpToFurthest();
        xMark->deleteMark(nMark);
    }
    // 3. Events
    writeEvents(_rxOutStream);
    // 4. Attributes
    // Masking for all 'any' types
    sal_uInt16 nAnyMask = 0;
    if (m_aRowHeight.getValueTypeClass() == TypeClass_LONG)
        nAnyMask |= ROWHEIGHT;
    if ( getFont() != getDefaultFont() )
        nAnyMask |= FONTATTRIBS | FONTSIZE | FONTTYPE | FONTDESCRIPTOR;
    if (m_aTabStop.getValueTypeClass() == TypeClass_BOOLEAN)
        nAnyMask |= TABSTOP;
    if ( hasTextColor() )
        nAnyMask |= TEXTCOLOR;
    if (m_aBackgroundColor.getValueTypeClass() == TypeClass_LONG)
        nAnyMask |= BACKGROUNDCOLOR;
    if (!m_bRecordMarker)
        nAnyMask |= RECORDMARKER;
    _rxOutStream->writeShort(nAnyMask);
    if (nAnyMask & ROWHEIGHT)
        _rxOutStream->writeLong(getINT32(m_aRowHeight));
    // old structures
    const FontDescriptor& aFont = getFont();
    if ( nAnyMask & FONTDESCRIPTOR )
    {
        // Attrib
        _rxOutStream->writeShort( sal::static_int_cast< sal_Int16 >( vcl::unohelper::ConvertFontWeight( aFont.Weight ) ) );
        _rxOutStream->writeShort( sal::static_int_cast< sal_Int16 >( aFont.Slant ) );
        _rxOutStream->writeShort( aFont.Underline );
        _rxOutStream->writeShort( aFont.Strikeout );
        _rxOutStream->writeShort( sal_Int16(aFont.Orientation * 10) );
        _rxOutStream->writeBoolean( aFont.Kerning );
        _rxOutStream->writeBoolean( aFont.WordLineMode );
        // Size
        _rxOutStream->writeLong( aFont.Width );
        _rxOutStream->writeLong( aFont.Height );
        _rxOutStream->writeShort( sal::static_int_cast< sal_Int16 >( vcl::unohelper::ConvertFontWidth( aFont.CharacterWidth ) ) );
        // Type
        _rxOutStream->writeUTF( aFont.Name );
        _rxOutStream->writeUTF( aFont.StyleName );
        _rxOutStream->writeShort( aFont.Family );
        _rxOutStream->writeShort( aFont.CharSet );
        _rxOutStream->writeShort( aFont.Pitch );
    }
    _rxOutStream << m_aDefaultControl;
    _rxOutStream->writeShort(m_nBorder);
    _rxOutStream->writeBoolean(m_bEnable);
    if (nAnyMask & TABSTOP)
        _rxOutStream->writeBoolean(getBOOL(m_aTabStop));
    _rxOutStream->writeBoolean(m_bNavigation);
    if (nAnyMask & TEXTCOLOR)
        _rxOutStream->writeLong( sal_Int32(getTextColor()) );
    // new since version 6
    _rxOutStream << m_sHelpText;
    if (nAnyMask & FONTDESCRIPTOR)
        _rxOutStream << getFont();
    if (nAnyMask & RECORDMARKER)
        _rxOutStream->writeBoolean(m_bRecordMarker);
    // new since version 7
    _rxOutStream->writeBoolean(m_bPrintable);
    // new since version 8
    if (nAnyMask & BACKGROUNDCOLOR)
        _rxOutStream->writeLong(getINT32(m_aBackgroundColor));
}

void OGridControlModel::read(const Reference<XObjectInputStream>& _rxInStream)
{
    SolarMutexGuard g;
    OControlModel::read(_rxInStream);
    Reference<XMarkableStream>  xMark(_rxInStream, UNO_QUERY);
    // 1. version
    sal_Int16 nVersion = _rxInStream->readShort();
    // 2. reading the columns
    sal_Int32 nLen = _rxInStream->readLong();
    if (nLen)
    {
        for (sal_Int32 i = 0; i < nLen; i++)
        {
            // reading the model names
            OUString sModelName;
            _rxInStream >> sModelName;
            rtl::Reference<OGridColumn> xCol(createColumnById(getColumnTypeByModelName(sModelName)));
            DBG_ASSERT(xCol.is(), "OGridControlModel::read : unknown column type !");
            sal_Int32 nObjLen = _rxInStream->readLong();
            if (nObjLen)
            {
                sal_Int32 nMark = xMark->createMark();
                if (xCol.is())
                {
                    xCol->read(_rxInStream);
                }
                xMark->jumpToMark(nMark);
                _rxInStream->skipBytes(nObjLen);
                xMark->deleteMark(nMark);
            }
            if ( xCol.is() )
                implInsert( i, xCol, false, nullptr, false );
        }
    }
    // In the base implementation events are only read, elements in the container exist
    // but since before TF_ONE for the GridControl events were always written, so they
    // need to be read, too
    sal_Int32 nObjLen = _rxInStream->readLong();
    if (nObjLen)
    {
        sal_Int32 nMark = xMark->createMark();
        Reference<XPersistObject>  xObj(m_xEventAttacher, UNO_QUERY);
        if (xObj.is())
            xObj->read(_rxInStream);
        xMark->jumpToMark(nMark);
        _rxInStream->skipBytes(nObjLen);
        xMark->deleteMark(nMark);
    }
    // reading the attachment
    for (sal_Int32 i = 0; i < nLen; i++)
    {
        css::uno::Reference<css::uno::XInterface>  xIfc(m_aItems[i], UNO_QUERY);
        Reference<XPropertySet>  xSet(xIfc, UNO_QUERY);
        Any aHelper;
        aHelper <<= xSet;
        m_xEventAttacher->attach( i, xIfc, aHelper );
    }
    // 4. reading the attributes
    if (nVersion == 1)
        return;
    // Masking for any
    sal_uInt16 nAnyMask = _rxInStream->readShort();
    if (nAnyMask & ROWHEIGHT)
    {
        sal_Int32 nValue = _rxInStream->readLong();
        m_aRowHeight <<= nValue;
    }
    FontDescriptor aFont( getFont() );
    if ( nAnyMask & FONTATTRIBS )
    {
        aFont.Weight = static_cast<float>(vcl::unohelper::ConvertFontWeight( _rxInStream->readShort() ));
        aFont.Slant = static_cast<FontSlant>(_rxInStream->readShort());
        aFont.Underline = _rxInStream->readShort();
        aFont.Strikeout = _rxInStream->readShort();
        aFont.Orientation = static_cast<float>(_rxInStream->readShort()) / 10;
        aFont.Kerning = _rxInStream->readBoolean() != 0;
        aFont.WordLineMode = _rxInStream->readBoolean() != 0;
    }
    if ( nAnyMask & FONTSIZE )
    {
        aFont.Width = static_cast<sal_Int16>(_rxInStream->readLong());
        aFont.Height = static_cast<sal_Int16>(_rxInStream->readLong());
        aFont.CharacterWidth = static_cast<float>(vcl::unohelper::ConvertFontWidth( _rxInStream->readShort() ));
    }
    if ( nAnyMask & FONTTYPE )
    {
        aFont.Name = _rxInStream->readUTF();
        aFont.StyleName = _rxInStream->readUTF();
        aFont.Family = _rxInStream->readShort();
        aFont.CharSet = _rxInStream->readShort();
        aFont.Pitch = _rxInStream->readShort();
    }
    if ( nAnyMask & ( FONTATTRIBS | FONTSIZE | FONTTYPE ) )
        setFont( aFont );
    // Name
    _rxInStream >> m_aDefaultControl;
    m_nBorder = _rxInStream->readShort();
    m_bEnable = _rxInStream->readBoolean();
    if (nAnyMask & TABSTOP)
    {
        m_aTabStop <<= (_rxInStream->readBoolean() != 0);
    }
    if (nVersion > 3)
        m_bNavigation = _rxInStream->readBoolean();
    if (nAnyMask & TEXTCOLOR)
    {
        sal_Int32 nValue = _rxInStream->readLong();
        setTextColor( ::Color(ColorTransparency, nValue) );
    }
    // new since version 6
    if (nVersion > 5)
        _rxInStream >> m_sHelpText;
    if (nAnyMask & FONTDESCRIPTOR)
    {
        FontDescriptor aUNOFont;
        _rxInStream >> aUNOFont;
        setFont( aFont );
    }
    if (nAnyMask & RECORDMARKER)
        m_bRecordMarker = _rxInStream->readBoolean();
    // new since version 7
    if (nVersion > 6)
        m_bPrintable = _rxInStream->readBoolean();
    if (nAnyMask & BACKGROUNDCOLOR)
    {
        sal_Int32 nValue = _rxInStream->readLong();
        m_aBackgroundColor <<= nValue;
    }
}

}

extern "C" SAL_DLLPUBLIC_EXPORT XInterface*
com_sun_star_form_OGridControlModel_get_implementation(XComponentContext* component,
        Sequence<Any> const &)
{
    return cppu::acquire(new frm::OGridControlModel(component));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

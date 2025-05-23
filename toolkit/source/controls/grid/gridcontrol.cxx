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


#include "gridcontrol.hxx"
#include "grideventforwarder.hxx"

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/view/SelectionType.hpp>
#include <com/sun/star/awt/grid/XGridControl.hpp>
#include <com/sun/star/awt/grid/XGridDataModel.hpp>
#include <com/sun/star/awt/grid/XGridRowSelection.hpp>
#include <com/sun/star/awt/grid/XMutableGridDataModel.hpp>
#include <com/sun/star/awt/grid/DefaultGridDataModel.hpp>
#include <com/sun/star/awt/grid/SortableGridDataModel.hpp>
#include <com/sun/star/awt/grid/DefaultGridColumnModel.hpp>
#include <helper/property.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <toolkit/controls/unocontrolbase.hxx>
#include <toolkit/controls/unocontrolmodel.hxx>
#include <toolkit/helper/listenermultiplexer.hxx>

#include <memory>

#include <helper/unopropertyarrayhelper.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::awt::grid;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::view;
using namespace ::com::sun::star::util;

namespace toolkit {

namespace
{
    Reference< XGridDataModel > lcl_getDefaultDataModel_throw( const Reference<XComponentContext> & i_context )
    {
        Reference< XMutableGridDataModel > const xDelegatorModel( DefaultGridDataModel::create( i_context ), UNO_SET_THROW );
        Reference< XGridDataModel > const xDataModel( SortableGridDataModel::create( i_context, xDelegatorModel ), UNO_QUERY_THROW );
        return xDataModel;
    }

    Reference< XGridColumnModel > lcl_getDefaultColumnModel_throw( const Reference<XComponentContext> & i_context )
    {
        Reference< XGridColumnModel > const xColumnModel = DefaultGridColumnModel::create( i_context );
        return xColumnModel;
    }
}


UnoGridModel::UnoGridModel( const css::uno::Reference< css::uno::XComponentContext >& rxContext )
        :UnoControlModel( rxContext )
{
    ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
    ImplRegisterProperty( BASEPROPERTY_BORDER );
    ImplRegisterProperty( BASEPROPERTY_BORDERCOLOR );
    ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
    ImplRegisterProperty( BASEPROPERTY_ENABLED );
    ImplRegisterProperty( BASEPROPERTY_FILLCOLOR );
    ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
    ImplRegisterProperty( BASEPROPERTY_HELPURL );
    ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
    ImplRegisterProperty( BASEPROPERTY_SIZEABLE ); // resizable
    ImplRegisterProperty( BASEPROPERTY_HSCROLL );
    ImplRegisterProperty( BASEPROPERTY_VSCROLL );
    ImplRegisterProperty( BASEPROPERTY_TABSTOP );
    ImplRegisterProperty( BASEPROPERTY_GRID_SHOWROWHEADER );
    ImplRegisterProperty( BASEPROPERTY_ROW_HEADER_WIDTH );
    ImplRegisterProperty( BASEPROPERTY_GRID_SHOWCOLUMNHEADER );
    ImplRegisterProperty( BASEPROPERTY_COLUMN_HEADER_HEIGHT );
    ImplRegisterProperty( BASEPROPERTY_ROW_HEIGHT );
    ImplRegisterProperty( BASEPROPERTY_GRID_DATAMODEL, Any( lcl_getDefaultDataModel_throw( m_xContext ) ) );
    ImplRegisterProperty( BASEPROPERTY_GRID_COLUMNMODEL, Any( lcl_getDefaultColumnModel_throw( m_xContext ) ) );
    ImplRegisterProperty( BASEPROPERTY_GRID_SELECTIONMODE );
    ImplRegisterProperty( BASEPROPERTY_FONTRELIEF );
    ImplRegisterProperty( BASEPROPERTY_FONTEMPHASISMARK );
    ImplRegisterProperty( BASEPROPERTY_FONTDESCRIPTOR );
    ImplRegisterProperty( BASEPROPERTY_TEXTCOLOR );
    ImplRegisterProperty( BASEPROPERTY_TEXTLINECOLOR );
    ImplRegisterProperty( BASEPROPERTY_USE_GRID_LINES );
    ImplRegisterProperty( BASEPROPERTY_GRID_LINE_COLOR );
    ImplRegisterProperty( BASEPROPERTY_GRID_HEADER_BACKGROUND );
    ImplRegisterProperty( BASEPROPERTY_GRID_HEADER_TEXT_COLOR );
    ImplRegisterProperty( BASEPROPERTY_GRID_ROW_BACKGROUND_COLORS );
    ImplRegisterProperty( BASEPROPERTY_ACTIVE_SEL_BACKGROUND_COLOR );
    ImplRegisterProperty( BASEPROPERTY_INACTIVE_SEL_BACKGROUND_COLOR );
    ImplRegisterProperty( BASEPROPERTY_ACTIVE_SEL_TEXT_COLOR );
    ImplRegisterProperty( BASEPROPERTY_INACTIVE_SEL_TEXT_COLOR );
    ImplRegisterProperty( BASEPROPERTY_VERTICALALIGN );
}


UnoGridModel::UnoGridModel( const UnoGridModel& rModel )
    :UnoControlModel( rModel )
{
    osl_atomic_increment( &m_refCount );
    {
        Reference< XGridDataModel > xDataModel;
        // clone the data model
        const Reference< XFastPropertySet > xCloneSource( &const_cast< UnoGridModel& >( rModel ) );
        try
        {
            const Reference< XCloneable > xCloneable( xCloneSource->getFastPropertyValue( BASEPROPERTY_GRID_DATAMODEL ), UNO_QUERY_THROW );
            xDataModel.set( xCloneable->createClone(), UNO_QUERY_THROW );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
        if ( !xDataModel.is() )
            xDataModel = lcl_getDefaultDataModel_throw( m_xContext );
        std::unique_lock aGuard(m_aMutex);
        UnoControlModel::setFastPropertyValue_NoBroadcast( aGuard, BASEPROPERTY_GRID_DATAMODEL, Any( xDataModel ) );
            // do *not* use setFastPropertyValue here: The UnoControlModel ctor made a simple copy of all property values,
            // so before this call here, we share our data model with the own of the clone source. setFastPropertyValue,
            // then, disposes the old data model - which means the data model which in fact belongs to the clone source.
            // so, call the UnoControlModel's impl-method for setting the value.

        // clone the column model
        Reference< XGridColumnModel > xColumnModel;
        try
        {
            const Reference< XCloneable > xCloneable( xCloneSource->getFastPropertyValue( BASEPROPERTY_GRID_COLUMNMODEL ), UNO_QUERY_THROW );
            xColumnModel.set( xCloneable->createClone(), UNO_QUERY_THROW );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
        if ( !xColumnModel.is() )
            xColumnModel = lcl_getDefaultColumnModel_throw( m_xContext );
        UnoControlModel::setFastPropertyValue_NoBroadcast( aGuard, BASEPROPERTY_GRID_COLUMNMODEL, Any( xColumnModel ) );
            // same comment as above: do not use our own setPropertyValue here.
    }
    osl_atomic_decrement( &m_refCount );
}


rtl::Reference<UnoControlModel> UnoGridModel::Clone() const
{
    return new UnoGridModel( *this );
}


namespace
{
    void lcl_dispose_nothrow( const Any& i_component )
    {
        try
        {
            const Reference< XComponent > xComponent( i_component, UNO_QUERY );
            if (xComponent)
                xComponent->dispose();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
    }
}


void SAL_CALL UnoGridModel::dispose(  )
{
    lcl_dispose_nothrow( getFastPropertyValue( BASEPROPERTY_GRID_COLUMNMODEL ) );
    lcl_dispose_nothrow( getFastPropertyValue( BASEPROPERTY_GRID_DATAMODEL ) );

    UnoControlModel::dispose();
}


void UnoGridModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 nHandle, const Any& rValue )
{
    Any aOldSubModel;
    if ( ( nHandle == BASEPROPERTY_GRID_COLUMNMODEL ) || ( nHandle == BASEPROPERTY_GRID_DATAMODEL ) )
    {
        getFastPropertyValue( rGuard, aOldSubModel, nHandle );
        if ( aOldSubModel == rValue )
        {
            OSL_ENSURE( false, "UnoGridModel::setFastPropertyValue_NoBroadcast: setting the same value, again!" );
                // shouldn't this have been caught by convertFastPropertyValue?
            aOldSubModel.clear();
        }
    }

    UnoControlModel::setFastPropertyValue_NoBroadcast( rGuard, nHandle, rValue );

    if ( aOldSubModel.hasValue() )
        lcl_dispose_nothrow( aOldSubModel );
}


OUString UnoGridModel::getServiceName()
{
    return u"com.sun.star.awt.grid.UnoControlGridModel"_ustr;
}


Any UnoGridModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    switch( nPropId )
    {
        case BASEPROPERTY_DEFAULTCONTROL:
            return uno::Any( u"com.sun.star.awt.grid.UnoControlGrid"_ustr );
        case BASEPROPERTY_GRID_SELECTIONMODE:
            return uno::Any( SelectionType(1) );
        case BASEPROPERTY_GRID_SHOWROWHEADER:
        case BASEPROPERTY_USE_GRID_LINES:
            return uno::Any( false );
        case BASEPROPERTY_ROW_HEADER_WIDTH:
            return uno::Any( sal_Int32( 10 ) );
        case BASEPROPERTY_GRID_SHOWCOLUMNHEADER:
            return uno::Any( true );
        case BASEPROPERTY_COLUMN_HEADER_HEIGHT:
        case BASEPROPERTY_ROW_HEIGHT:
        case BASEPROPERTY_GRID_HEADER_BACKGROUND:
        case BASEPROPERTY_GRID_HEADER_TEXT_COLOR:
        case BASEPROPERTY_GRID_LINE_COLOR:
        case BASEPROPERTY_GRID_ROW_BACKGROUND_COLORS:
        case BASEPROPERTY_ACTIVE_SEL_BACKGROUND_COLOR:
        case BASEPROPERTY_INACTIVE_SEL_BACKGROUND_COLOR:
        case BASEPROPERTY_ACTIVE_SEL_TEXT_COLOR:
        case BASEPROPERTY_INACTIVE_SEL_TEXT_COLOR:
            return Any();
        default:
            return UnoControlModel::ImplGetDefaultValue( nPropId );
    }

}


::cppu::IPropertyArrayHelper& UnoGridModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}


// XMultiPropertySet
Reference< XPropertySetInfo > UnoGridModel::getPropertySetInfo(  )
{
    static Reference< XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}


//= UnoGridControl

UnoGridControl::UnoGridControl()
    :m_aSelectionListeners( *this )
    ,m_pEventForwarder( new toolkit::GridEventForwarder( *this ) )
{
}


UnoGridControl::~UnoGridControl()
{
}


OUString UnoGridControl::GetComponentServiceName() const
{
    return u"Grid"_ustr;
}


void SAL_CALL UnoGridControl::dispose(  )
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    m_aSelectionListeners.disposeAndClear( aEvt );
    UnoControl::dispose();
}


void SAL_CALL UnoGridControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControlBase::createPeer( rxToolkit, rParentPeer );

    const Reference< XGridRowSelection > xGrid( getPeer(), UNO_QUERY_THROW );
    xGrid->addSelectionListener( &m_aSelectionListeners );
}


namespace
{
    void lcl_setEventForwarding( const Reference< XControlModel >& i_gridControlModel, const std::unique_ptr< toolkit::GridEventForwarder >& i_listener,
        bool const i_add )
    {
        const Reference< XPropertySet > xModelProps( i_gridControlModel, UNO_QUERY );
        if ( !xModelProps.is() )
            return;

        try
        {
            Reference< XContainer > const xColModel(
                xModelProps->getPropertyValue(u"ColumnModel"_ustr),
                UNO_QUERY_THROW );
            if ( i_add )
                xColModel->addContainerListener( i_listener.get() );
            else
                xColModel->removeContainerListener( i_listener.get() );

            Reference< XGridDataModel > const xDataModel(
                xModelProps->getPropertyValue(u"GridDataModel"_ustr),
                UNO_QUERY_THROW
            );
            Reference< XMutableGridDataModel > const xMutableDataModel( xDataModel, UNO_QUERY );
            if ( xMutableDataModel.is() )
            {
                if ( i_add )
                    xMutableDataModel->addGridDataListener( i_listener.get() );
                else
                    xMutableDataModel->removeGridDataListener( i_listener.get() );
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
    }
}


sal_Bool SAL_CALL UnoGridControl::setModel( const Reference< XControlModel >& i_model )
{
    lcl_setEventForwarding( getModel(), m_pEventForwarder, false );
    if ( !UnoGridControl_Base::setModel( i_model ) )
        return false;
    lcl_setEventForwarding( getModel(), m_pEventForwarder, true );
    return true;
}


::sal_Int32 UnoGridControl::getRowAtPoint(::sal_Int32 x, ::sal_Int32 y)
{
    Reference< XGridControl > const xGrid ( getPeer(), UNO_QUERY_THROW );
    return xGrid->getRowAtPoint( x, y );
}


::sal_Int32 UnoGridControl::getColumnAtPoint(::sal_Int32 x, ::sal_Int32 y)
{
    Reference< XGridControl > const xGrid ( getPeer(), UNO_QUERY_THROW );
    return xGrid->getColumnAtPoint( x, y );
}


::sal_Int32 SAL_CALL UnoGridControl::getCurrentColumn(  )
{
    Reference< XGridControl > const xGrid ( getPeer(), UNO_QUERY_THROW );
    return xGrid->getCurrentColumn();
}


::sal_Int32 SAL_CALL UnoGridControl::getCurrentRow(  )
{
    Reference< XGridControl > const xGrid ( getPeer(), UNO_QUERY_THROW );
    return xGrid->getCurrentRow();
}


void SAL_CALL UnoGridControl::goToCell( ::sal_Int32 i_columnIndex, ::sal_Int32 i_rowIndex )
{
    Reference< XGridControl > const xGrid ( getPeer(), UNO_QUERY_THROW );
    xGrid->goToCell( i_columnIndex, i_rowIndex );
}


void SAL_CALL UnoGridControl::selectRow( ::sal_Int32 i_rowIndex )
{
    Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->selectRow( i_rowIndex );
}


void SAL_CALL UnoGridControl::selectAllRows()
{
    Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->selectAllRows();
}


void SAL_CALL UnoGridControl::deselectRow( ::sal_Int32 i_rowIndex )
{
    Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->deselectRow( i_rowIndex );
}


void SAL_CALL UnoGridControl::deselectAllRows()
{
    Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->deselectAllRows();
}


css::uno::Sequence< ::sal_Int32 > SAL_CALL UnoGridControl::getSelectedRows()
{
    return Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->getSelectedRows();
}


sal_Bool SAL_CALL UnoGridControl::hasSelectedRows()
{
    return Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->hasSelectedRows();
}


sal_Bool SAL_CALL UnoGridControl::isRowSelected(::sal_Int32 index)
{
    return Reference< XGridRowSelection >( getPeer(), UNO_QUERY_THROW )->isRowSelected( index );
}


void SAL_CALL UnoGridControl::addSelectionListener(const css::uno::Reference< css::awt::grid::XGridSelectionListener > & listener)
{
    m_aSelectionListeners.addInterface( listener );
}


void SAL_CALL UnoGridControl::removeSelectionListener(const css::uno::Reference< css::awt::grid::XGridSelectionListener > & listener)
{
    m_aSelectionListeners.removeInterface( listener );
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_GridControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new toolkit::UnoGridControl());
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_GridControlModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new toolkit::UnoGridModel(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

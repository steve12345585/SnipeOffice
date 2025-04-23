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


#include "gridcolumn.hxx"

#include <com/sun/star/awt/grid/XGridColumnModel.hpp>
#include <com/sun/star/awt/grid/XGridColumn.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/compbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <vector>

using namespace css::awt;
using namespace css::awt::grid;
using namespace css::container;
using namespace css::lang;
using namespace css::uno;
using namespace toolkit;

namespace {

typedef ::comphelper::WeakComponentImplHelper    <   css::awt::grid::XGridColumnModel
                                            ,   css::lang::XServiceInfo
                                            >   DefaultGridColumnModel_Base;

class DefaultGridColumnModel : public DefaultGridColumnModel_Base
{
public:
    DefaultGridColumnModel();
    DefaultGridColumnModel( DefaultGridColumnModel const & i_copySource );

    // XGridColumnModel
    virtual ::sal_Int32 SAL_CALL getColumnCount() override;
    virtual css::uno::Reference< css::awt::grid::XGridColumn > SAL_CALL createColumn(  ) override;
    virtual ::sal_Int32 SAL_CALL addColumn(const css::uno::Reference< css::awt::grid::XGridColumn > & column) override;
    virtual void SAL_CALL removeColumn( ::sal_Int32 i_columnIndex ) override;
    virtual css::uno::Sequence< css::uno::Reference< css::awt::grid::XGridColumn > > SAL_CALL getColumns() override;
    virtual css::uno::Reference< css::awt::grid::XGridColumn > SAL_CALL getColumn(::sal_Int32 index) override;
    virtual void SAL_CALL setDefaultColumns(sal_Int32 rowElements) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // XContainer
    virtual void SAL_CALL addContainerListener( const css::uno::Reference< css::container::XContainerListener >& xListener ) override;
    virtual void SAL_CALL removeContainerListener( const css::uno::Reference< css::container::XContainerListener >& xListener ) override;

    // XCloneable
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone(  ) override;

    // OComponentHelper
    virtual void disposing( std::unique_lock<std::mutex>& ) override;

private:
    typedef ::std::vector< rtl::Reference< GridColumn > >   Columns;

    ::comphelper::OInterfaceContainerHelper4<XContainerListener> m_aContainerListeners;
    Columns                             m_aColumns;
};

    DefaultGridColumnModel::DefaultGridColumnModel()
    {
    }

    DefaultGridColumnModel::DefaultGridColumnModel( DefaultGridColumnModel const & i_copySource )
    {
        Columns aColumns;
        aColumns.reserve( i_copySource.m_aColumns.size() );
        try
        {
            for (   Columns::const_iterator col = i_copySource.m_aColumns.begin();
                    col != i_copySource.m_aColumns.end();
                    ++col
                )
            {
                rtl::Reference< GridColumn > const xClone( new GridColumn(**col) );

                xClone->setIndex( col - i_copySource.m_aColumns.begin() );

                aColumns.push_back( xClone );
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
        if ( aColumns.size() == i_copySource.m_aColumns.size() )
            m_aColumns.swap( aColumns );
    }

    ::sal_Int32 SAL_CALL DefaultGridColumnModel::getColumnCount()
    {
        return m_aColumns.size();
    }


    Reference< XGridColumn > SAL_CALL DefaultGridColumnModel::createColumn(  )
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        return new GridColumn();
    }


    ::sal_Int32 SAL_CALL DefaultGridColumnModel::addColumn( const Reference< XGridColumn > & i_column )
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);

        GridColumn* const pGridColumn = dynamic_cast<GridColumn*>( i_column.get() );
        if ( pGridColumn == nullptr )
            throw css::lang::IllegalArgumentException( u"invalid column implementation"_ustr, *this, 1 );

        m_aColumns.push_back( pGridColumn );
        sal_Int32 index = m_aColumns.size() - 1;
        pGridColumn->setIndex( index );

        // fire insertion notifications
        ContainerEvent aEvent;
        aEvent.Source = *this;
        aEvent.Accessor <<= index;
        aEvent.Element <<= i_column;

        m_aContainerListeners.notifyEach( aGuard, &XContainerListener::elementInserted, aEvent );

        return index;
    }


    void SAL_CALL DefaultGridColumnModel::removeColumn( ::sal_Int32 i_columnIndex )
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);

        if ( ( i_columnIndex < 0 ) || ( o3tl::make_unsigned( i_columnIndex ) >= m_aColumns.size() ) )
            throw css::lang::IndexOutOfBoundsException( OUString(), *this );

        Columns::iterator const pos = m_aColumns.begin() + i_columnIndex;
        rtl::Reference< GridColumn > const xColumn( *pos );
        m_aColumns.erase( pos );

        // update indexes of all subsequent columns
        sal_Int32 columnIndex( i_columnIndex );
        for (   Columns::iterator updatePos = m_aColumns.begin() + columnIndex;
                updatePos != m_aColumns.end();
                ++updatePos, ++columnIndex
            )
        {
            GridColumn* pColumnImpl = updatePos->get();
            pColumnImpl->setIndex( columnIndex );
        }

        // fire removal notifications
        ContainerEvent aEvent;
        aEvent.Source = *this;
        aEvent.Accessor <<= i_columnIndex;
        aEvent.Element <<= Reference< XGridColumn >(xColumn);

        m_aContainerListeners.notifyEach( aGuard, &XContainerListener::elementRemoved, aEvent );

        aGuard.unlock();

        // dispose the removed column
        try
        {
            xColumn->dispose();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
    }


    Sequence< Reference< XGridColumn > > SAL_CALL DefaultGridColumnModel::getColumns()
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        return ::comphelper::containerToSequence<Reference<XGridColumn>>( m_aColumns );
    }


    Reference< XGridColumn > SAL_CALL DefaultGridColumnModel::getColumn(::sal_Int32 index)
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);

        if ( index >=0 && o3tl::make_unsigned(index) < m_aColumns.size())
            return m_aColumns[index];

        throw css::lang::IndexOutOfBoundsException();
    }


    void SAL_CALL DefaultGridColumnModel::setDefaultColumns(sal_Int32 rowElements)
    {
        ::std::vector< ContainerEvent > aRemovedColumns;
        ::std::vector< ContainerEvent > aInsertedColumns;

        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);

        // remove existing columns
        while ( !m_aColumns.empty() )
        {
            const size_t lastColIndex = m_aColumns.size() - 1;

            ContainerEvent aEvent;
            aEvent.Source = *this;
            aEvent.Accessor <<= sal_Int32( lastColIndex );
            aEvent.Element <<= Reference<XGridColumn>(m_aColumns[ lastColIndex ]);
            aRemovedColumns.push_back( aEvent );

            m_aColumns.erase( m_aColumns.begin() + lastColIndex );
        }

        // add new columns
        for ( sal_Int32 i=0; i<rowElements; ++i )
        {
            ::rtl::Reference< GridColumn > const pGridColumn = new GridColumn();
            OUString colTitle = "Column " + OUString::number( i + 1 );
            pGridColumn->setTitle( colTitle );
            pGridColumn->setColumnWidth( 80 /* APPFONT */ );
            pGridColumn->setFlexibility( 1 );
            pGridColumn->setResizeable( true );
            pGridColumn->setDataColumnIndex( i );

            ContainerEvent aEvent;
            aEvent.Source = *this;
            aEvent.Accessor <<= i;
            aEvent.Element <<= Reference<XGridColumn>(pGridColumn);
            aInsertedColumns.push_back( aEvent );

            m_aColumns.push_back( pGridColumn );
            pGridColumn->setIndex( i );
        }

        // fire removal notifications
        for (const auto& rEvent : aRemovedColumns)
        {
            m_aContainerListeners.notifyEach( aGuard, &XContainerListener::elementRemoved, rEvent );
        }

        // fire insertion notifications
        for (const auto& rEvent : aInsertedColumns)
        {
            m_aContainerListeners.notifyEach( aGuard, &XContainerListener::elementInserted, rEvent );
        }

        aGuard.unlock();

        // dispose removed columns
        for (const auto& rEvent : aRemovedColumns)
        {
            try
            {
                const Reference< XComponent > xColComp( rEvent.Element, UNO_QUERY );
                if (xColComp)
                    xColComp->dispose();
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("toolkit.controls");
            }
        }
    }


    OUString SAL_CALL DefaultGridColumnModel::getImplementationName(  )
    {
        return u"stardiv.Toolkit.DefaultGridColumnModel"_ustr;
    }

    sal_Bool SAL_CALL DefaultGridColumnModel::supportsService( const OUString& i_serviceName )
    {
        return cppu::supportsService(this, i_serviceName);
    }

    Sequence< OUString > SAL_CALL DefaultGridColumnModel::getSupportedServiceNames(  )
    {
        return { u"com.sun.star.awt.grid.DefaultGridColumnModel"_ustr };
    }


    void SAL_CALL DefaultGridColumnModel::addContainerListener( const Reference< XContainerListener >& i_listener )
    {
        std::unique_lock aGuard(m_aMutex);
        if ( i_listener.is() )
            m_aContainerListeners.addInterface( aGuard, i_listener );
    }


    void SAL_CALL DefaultGridColumnModel::removeContainerListener( const Reference< XContainerListener >& i_listener )
    {
        std::unique_lock aGuard(m_aMutex);
        if ( i_listener.is() )
            m_aContainerListeners.removeInterface( aGuard, i_listener );
    }


    void DefaultGridColumnModel::disposing( std::unique_lock<std::mutex>& rGuard )
    {
        DefaultGridColumnModel_Base::disposing(rGuard);

        EventObject aEvent( *this );
        m_aContainerListeners.disposeAndClear( rGuard, aEvent );

        // remove, dispose and clear columns
        while ( !m_aColumns.empty() )
        {
            try
            {
                m_aColumns[ 0 ]->dispose();
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("toolkit.controls");
            }

            m_aColumns.erase( m_aColumns.begin() );
        }

        Columns().swap(m_aColumns);
    }


    Reference< css::util::XCloneable > SAL_CALL DefaultGridColumnModel::createClone(  )
    {
        std::unique_lock aGuard(m_aMutex);
        throwIfDisposed(aGuard);
        return new DefaultGridColumnModel( *this );
    }

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_DefaultGridColumnModel_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new DefaultGridColumnModel());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

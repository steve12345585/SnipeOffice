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

#pragma once

#include <rtl/ustring.hxx>
#include <rtl/ref.hxx>
#include <cppuhelper/weak.hxx>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/multiinterfacecontainer4.hxx>
#include <memory>


class ContentResultSetWrapperListener;
class ContentResultSetWrapper
                : public cppu::OWeakObject
                , public css::lang::XComponent
                , public css::sdbc::XCloseable
                , public css::sdbc::XResultSetMetaDataSupplier
                , public css::beans::XPropertySet
                , public css::ucb::XContentAccess
                , public css::sdbc::XResultSet
                , public css::sdbc::XRow
{
protected:
    typedef comphelper::OMultiTypeInterfaceContainerHelperVar4<OUString, css::beans::XPropertyChangeListener>
        PropertyChangeListenerContainer_Impl;
    typedef comphelper::OMultiTypeInterfaceContainerHelperVar4<OUString, css::beans::XVetoableChangeListener>
        VetoableChangeListenerContainer_Impl;

    //members

    //my Mutex
    std::mutex              m_aMutex;

    //different Interfaces from Origin:
    css::uno::Reference< css::sdbc::XResultSet >
                            m_xResultSetOrigin;
    css::uno::Reference< css::sdbc::XRow >
                            m_xRowOrigin; //XRow-interface from m_xOrigin
                            //!! call impl_init_xRowOrigin() bevor you access this member
    css::uno::Reference< css::ucb::XContentAccess >
                            m_xContentAccessOrigin; //XContentAccess-interface from m_xOrigin
                            //!! call impl_init_xContentAccessOrigin() bevor you access this member
    css::uno::Reference< css::beans::XPropertySet >
                            m_xPropertySetOrigin; //XPropertySet-interface from m_xOrigin
                            //!! call impl_init_xPropertySetOrigin() bevor you access this member

    css::uno::Reference< css::beans::XPropertySetInfo >
                            m_xPropertySetInfo;
                            //call impl_initPropertySetInfo() bevor you access this member

    sal_Int32               m_nForwardOnly;

private:
    rtl::Reference<ContentResultSetWrapperListener>
                            m_xMyListenerImpl;

    css::uno::Reference< css::sdbc::XResultSetMetaData >
                            m_xMetaDataFromOrigin; //XResultSetMetaData from m_xOrigin

    //management of listeners
    bool                m_bDisposed; ///Dispose call ready.
    bool                m_bInDispose;///In dispose call
    comphelper::OInterfaceContainerHelper4<css::lang::XEventListener>
                            m_aDisposeEventListeners;
    PropertyChangeListenerContainer_Impl
                            m_aPropertyChangeListeners;
    VetoableChangeListenerContainer_Impl
                            m_aVetoableChangeListeners;


    //methods:
private:
    void verifyGet();

protected:


    ContentResultSetWrapper( css::uno::Reference< css::sdbc::XResultSet > const & xOrigin );

    virtual ~ContentResultSetWrapper() override;

    void impl_init();
    void impl_deinit();

    //--

    void impl_init_xRowOrigin(std::unique_lock<std::mutex>&);
    void impl_init_xContentAccessOrigin(std::unique_lock<std::mutex>&);
    void impl_init_xPropertySetOrigin(std::unique_lock<std::mutex>&);

    //--

    virtual void impl_initPropertySetInfo(std::unique_lock<std::mutex>& rGuard); //helping XPropertySet

    /// @throws css::lang::DisposedException
    /// @throws css::uno::RuntimeException
    void
    impl_EnsureNotDisposed(std::unique_lock<std::mutex>& rGuard);

    void
    impl_notifyPropertyChangeListeners(
            std::unique_lock<std::mutex>& rGuard,
            const css::beans::PropertyChangeEvent& rEvt );

    /// @throws css::beans::PropertyVetoException
    /// @throws css::uno::RuntimeException
    void
    impl_notifyVetoableChangeListeners(
            std::unique_lock<std::mutex>& rGuard,
            const css::beans::PropertyChangeEvent& rEvt );

    bool impl_isForwardOnly(std::unique_lock<std::mutex>& rGuard);

public:


    // XInterface

    virtual css::uno::Any SAL_CALL
    queryInterface( const css::uno::Type & rType ) override;


    // XComponent

    virtual void SAL_CALL
    dispose() override final;

    virtual void SAL_CALL
    addEventListener( const css::uno::Reference< css::lang::XEventListener >& Listener ) override;

    virtual void SAL_CALL
    removeEventListener( const css::uno::Reference< css::lang::XEventListener >& Listener ) override;


    //XCloseable

    virtual void SAL_CALL
    close() override;


    //XResultSetMetaDataSupplier

    virtual css::uno::Reference< css::sdbc::XResultSetMetaData > SAL_CALL
    getMetaData() override;


    // XPropertySet

    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
    getPropertySetInfo() override final;
    const css::uno::Reference< css::beans::XPropertySetInfo > &
    getPropertySetInfoImpl(std::unique_lock<std::mutex>& rGuard);

    virtual void SAL_CALL
    setPropertyValue( const OUString& aPropertyName,
                      const css::uno::Any& aValue ) final override;
    virtual void
    setPropertyValueImpl( std::unique_lock<std::mutex>& rGuard, const OUString& aPropertyName,
                      const css::uno::Any& aValue );

    virtual css::uno::Any SAL_CALL
    getPropertyValue( const OUString& PropertyName ) override;

    virtual void SAL_CALL
    addPropertyChangeListener( const OUString& aPropertyName,
                               const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener ) override;

    virtual void SAL_CALL
    removePropertyChangeListener( const OUString& aPropertyName,
                                  const css::uno::Reference< css::beans::XPropertyChangeListener >& aListener ) override;

    virtual void SAL_CALL
    addVetoableChangeListener( const OUString& PropertyName,
                               const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;

    virtual void SAL_CALL
    removeVetoableChangeListener( const OUString& PropertyName,
                                  const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;


    // own methods

    /// @throws css::uno::RuntimeException
    virtual void
        impl_disposing( const css::lang::EventObject& Source );

    /// @throws css::uno::RuntimeException
    virtual void
    impl_propertyChange( const css::beans::PropertyChangeEvent& evt );

    /// @throws css::beans::PropertyVetoException
    /// @throws css::uno::RuntimeException
    virtual void
    impl_vetoableChange( const css::beans::PropertyChangeEvent& aEvent );


    // XContentAccess

    virtual OUString SAL_CALL
    queryContentIdentifierString() override final;
    virtual OUString
    queryContentIdentifierStringImpl(std::unique_lock<std::mutex>& rGuard);

    virtual css::uno::Reference< css::ucb::XContentIdentifier > SAL_CALL
    queryContentIdentifier() override final;
    virtual css::uno::Reference< css::ucb::XContentIdentifier >
    queryContentIdentifierImpl(std::unique_lock<std::mutex>& rGuard);

    virtual css::uno::Reference< css::ucb::XContent > SAL_CALL
    queryContent() override final;
    virtual css::uno::Reference<css::ucb::XContent>
    queryContentImpl(std::unique_lock<std::mutex>& rGuard);


    // XResultSet

    virtual sal_Bool SAL_CALL
    next() override;
    virtual sal_Bool SAL_CALL
    isBeforeFirst() override;
    virtual sal_Bool SAL_CALL
    isAfterLast() override;
    virtual sal_Bool SAL_CALL
    isFirst() override;
    virtual sal_Bool SAL_CALL
    isLast() override;
    virtual void SAL_CALL
    beforeFirst() override;
    virtual void SAL_CALL
    afterLast() override;
    virtual sal_Bool SAL_CALL
    first() override;
    virtual sal_Bool SAL_CALL
    last() override;
    virtual sal_Int32 SAL_CALL
    getRow() override;
    virtual sal_Bool SAL_CALL
    absolute( sal_Int32 row ) override;
    virtual sal_Bool SAL_CALL
    relative( sal_Int32 rows ) override;
    virtual sal_Bool SAL_CALL
    previous() override;
    virtual void SAL_CALL
    refreshRow() override;
    virtual sal_Bool SAL_CALL
    rowUpdated() override;
    virtual sal_Bool SAL_CALL
    rowInserted() override;
    virtual sal_Bool SAL_CALL
    rowDeleted() override;
    virtual css::uno::Reference<
                css::uno::XInterface > SAL_CALL
    getStatement() override;


    // XRow

    virtual sal_Bool SAL_CALL
    wasNull() override;

    virtual OUString SAL_CALL
    getString( sal_Int32 columnIndex ) override;

    virtual sal_Bool SAL_CALL
    getBoolean( sal_Int32 columnIndex ) override;

    virtual sal_Int8 SAL_CALL
    getByte( sal_Int32 columnIndex ) override;

    virtual sal_Int16 SAL_CALL
    getShort( sal_Int32 columnIndex ) override;

    virtual sal_Int32 SAL_CALL
    getInt( sal_Int32 columnIndex ) override;

    virtual sal_Int64 SAL_CALL
    getLong( sal_Int32 columnIndex ) override;

    virtual float SAL_CALL
    getFloat( sal_Int32 columnIndex ) override;

    virtual double SAL_CALL
    getDouble( sal_Int32 columnIndex ) override;

    virtual css::uno::Sequence< sal_Int8 > SAL_CALL
    getBytes( sal_Int32 columnIndex ) override;

    virtual css::util::Date SAL_CALL
    getDate( sal_Int32 columnIndex ) override;

    virtual css::util::Time SAL_CALL
    getTime( sal_Int32 columnIndex ) override;

    virtual css::util::DateTime SAL_CALL
    getTimestamp( sal_Int32 columnIndex ) override;

    virtual css::uno::Reference< css::io::XInputStream > SAL_CALL
    getBinaryStream( sal_Int32 columnIndex ) override;

    virtual css::uno::Reference< css::io::XInputStream > SAL_CALL
    getCharacterStream( sal_Int32 columnIndex ) override;

    virtual css::uno::Any SAL_CALL
    getObject( sal_Int32 columnIndex,
               const css::uno::Reference< css::container::XNameAccess >& typeMap ) override;

    virtual css::uno::Reference< css::sdbc::XRef > SAL_CALL
    getRef( sal_Int32 columnIndex ) override;

    virtual css::uno::Reference< css::sdbc::XBlob > SAL_CALL
    getBlob( sal_Int32 columnIndex ) override;

    virtual css::uno::Reference< css::sdbc::XClob > SAL_CALL
    getClob( sal_Int32 columnIndex ) override;

    virtual css::uno::Reference< css::sdbc::XArray > SAL_CALL
    getArray( sal_Int32 columnIndex ) override;
};


class ContentResultSetWrapperListener
        : public cppu::OWeakObject
        , public css::beans::XPropertyChangeListener
        , public css::beans::XVetoableChangeListener
{
    ContentResultSetWrapper*    m_pOwner;

public:
    ContentResultSetWrapperListener( ContentResultSetWrapper* pOwner );

    virtual ~ContentResultSetWrapperListener() override;


    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire()
        noexcept override;
    virtual void SAL_CALL release()
        noexcept override;

    //XEventListener

    virtual void SAL_CALL
        disposing( const css::lang::EventObject& Source ) override;


    //XPropertyChangeListener

    virtual void SAL_CALL
    propertyChange( const css::beans::PropertyChangeEvent& evt ) override;


    //XVetoableChangeListener

    virtual void SAL_CALL
    vetoableChange( const css::beans::PropertyChangeEvent& aEvent ) override;


    // own methods:
    void impl_OwnerDies();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

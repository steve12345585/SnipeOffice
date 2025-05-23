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

#ifndef INCLUDED_UCBHELPER_RESULTSET_HXX
#define INCLUDED_UCBHELPER_RESULTSET_HXX

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <rtl/ref.hxx>
#include <salhelper/simplereferenceobject.hxx>
#include <cppuhelper/implbase.hxx>
#include <ucbhelper/ucbhelperdllapi.h>
#include <memory>
#include <mutex>

namespace com::sun::star::uno { class XComponentContext; }
namespace com::sun::star::ucb { class XCommandEnvironment; }

namespace ucbhelper {


inline constexpr OUString RESULTSET_SERVICE_NAME = u"com.sun.star.ucb.ContentResultSet"_ustr;


class ResultSetDataSupplier;
struct ResultSet_Impl;

/**
 * This is an implementation of the service com.sun.star.ucb.ContentResultSet.
 * It can be used to implement the method XDynamicResultSet::getStaticResultSet,
 * which needs to be implemented for instance to implement the command "open"
 * at folder objects. This class uses a user supplied ResultSetDataSupplier
 * object to request data on demand.
 *
 * @see ResultSetDataSupplier
 */
class SAL_DLLPUBLIC_RTTI ResultSet final :
                public cppu::WeakImplHelper<
                    css::lang::XServiceInfo,
                    css::lang::XComponent,
                    css::ucb::XContentAccess,
                    css::sdbc::XResultSet,
                    css::sdbc::XResultSetMetaDataSupplier,
                    css::sdbc::XRow,
                    css::sdbc::XCloseable,
                    css::beans::XPropertySet>
{
    std::unique_ptr<ResultSet_Impl> m_pImpl;

public:
    /**
      * Construction.
      *
      * @param rxSMgr is a Service Manager.
      * @param rProperties is a sequence of properties for that the resultset
      *        shall be able to obtain the values.
      * @param rDataSupplier is a supplier for the resultset data.
      */
    UCBHELPER_DLLPUBLIC ResultSet(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
            const css::uno::Sequence< css::beans::Property >& rProperties,
            const rtl::Reference< ResultSetDataSupplier >& rDataSupplier );
    /**
      * Construction.
      *
      * @param rxSMgr is a Service Manager.
      * @param rProperties is a sequence of properties for that the resultset
      *        shall be able to obtain the values.
      * @param rDataSupplier is a supplier for the resultset data.
      * @param rxEnv is the environment for interactions, progress propagation,
      *        ...
      */
    UCBHELPER_DLLPUBLIC ResultSet(
            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
            const css::uno::Sequence< css::beans::Property >& rProperties,
            const rtl::Reference< ResultSetDataSupplier >& rDataSupplier,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& rxEnv );
    virtual ~ResultSet() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XComponent
    virtual void SAL_CALL
    dispose() override;
    virtual void SAL_CALL
    addEventListener( const css::uno::Reference< css::lang::XEventListener >& Listener ) override;
    virtual void SAL_CALL
    removeEventListener( const css::uno::Reference< css::lang::XEventListener >& Listener ) override;

    // XContentAccess
    virtual OUString SAL_CALL
    queryContentIdentifierString() override;
    virtual css::uno::Reference< css::ucb::XContentIdentifier > SAL_CALL
    queryContentIdentifier() override;
    virtual css::uno::Reference< css::ucb::XContent > SAL_CALL
    queryContent() override;

    // XResultSetMetaDataSupplier
    virtual css::uno::Reference< css::sdbc::XResultSetMetaData > SAL_CALL
    getMetaData() override;

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
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL
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
    virtual css::uno::Reference<
                css::io::XInputStream > SAL_CALL
    getBinaryStream( sal_Int32 columnIndex ) override;
    virtual css::uno::Reference<
                css::io::XInputStream > SAL_CALL
    getCharacterStream( sal_Int32 columnIndex ) override;
    virtual css::uno::Any SAL_CALL
    getObject( sal_Int32 columnIndex,
               const css::uno::Reference< css::container::XNameAccess >& typeMap ) override;
    virtual css::uno::Reference< css::sdbc::XRef > SAL_CALL
    getRef( sal_Int32 columnIndex ) override;
    virtual css::uno::Reference<
                css::sdbc::XBlob > SAL_CALL
    getBlob( sal_Int32 columnIndex ) override;
    virtual css::uno::Reference< css::sdbc::XClob > SAL_CALL
    getClob( sal_Int32 columnIndex ) override;
    virtual css::uno::Reference< css::sdbc::XArray > SAL_CALL
    getArray( sal_Int32 columnIndex ) override;

    // XCloseable
    virtual void SAL_CALL
    close() override;

    // XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL
    getPropertySetInfo() override;
    virtual void SAL_CALL
    setPropertyValue( const OUString& aPropertyName,
                      const css::uno::Any& aValue ) override;
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


    // Non-interface methods.


    /**
      * This method propagates property value changes to all registered
      * listeners.
      *
      * @param rEvt is a property change event.
      */
    void propertyChanged(std::unique_lock<std::mutex>& rGuard,
                const css::beans::PropertyChangeEvent& rEvt ) const;

    /**
      * This method should be called by the data supplier for the result set
      * to indicate that there were new data obtained from the data source.
      *
      * @param nOld is the old count of rows; must be non-negative.
      * @param nnew is the new count of rows; must be non-negative.
      */
    UCBHELPER_DLLPUBLIC void rowCountChanged( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nOld, sal_uInt32 nNew );

    /**
      * This method should be called by the data supplier for the result set
      * to indicate that there were all rows obtained from the data source.
      */
    UCBHELPER_DLLPUBLIC void rowCountFinal(std::unique_lock<std::mutex>& rResultSetGuard);

    /**
      * This method returns a sequence containing all properties ( not the
      * values! ) of the result set.
      *
      * @return a sequence of properties.
      */
    UCBHELPER_DLLPUBLIC const css::uno::Sequence< css::beans::Property >&
    getProperties() const;

    /**
      * This method returns the environment to use for interactions, progress
      * propagation, ... It can by empty.
      *
      * @return an environment or an empty reference.
      */
    UCBHELPER_DLLPUBLIC const css::uno::Reference< css::ucb::XCommandEnvironment >&
    getEnvironment() const;
};


/**
 * This is the base class for an object that supplies data to a result set
 *
 * @see ResultSet
 */
class ResultSetDataSupplier : public salhelper::SimpleReferenceObject
{
    friend class ResultSet;

    // No ref, otherwise we get a cyclic reference between supplier and set!
    // Will be set from ResultSet ctor.
    ResultSet* m_pResultSet;

public:
    ResultSetDataSupplier() : m_pResultSet( nullptr ) {}

    /**
     * This method returns the resultset this supplier belongs to.
     *
     * @return the resultset for that the supplier supplies data.
     */
    ResultSet* getResultSet() const { return m_pResultSet; }

    /**
     * This method returns the identifier string of the content at the
     * specified index.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier; must be non-negative.
     * @return the content's identifier string.
     */
    virtual OUString queryContentIdentifierString( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) = 0;

    /**
     * This method returns the identifier of the content at the specified index.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier; must be non-negative.
     * @return the content's identifier.
     */
    virtual css::uno::Reference< css::ucb::XContentIdentifier >
    queryContentIdentifier( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) = 0;

    /**
     * This method returns the content at the specified index.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier; must be non-negative.
     * @return the content.
     */
    virtual css::uno::Reference< css::ucb::XContent >
    queryContent( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) = 0;

    /**
     * This method returns whether there is a content at the specified index.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier; must be non-negative.
     * @return true, if there is a content at the given index.
     */
    virtual bool getResult( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) = 0;

    /**
     * This method returns the total count of objects in the logical data array
     * of the supplier. The implementation of this method may be very
     * "expensive", because it can be necessary to obtain all data in order
     * to determine the count. Therefore the ResultSet implementation calls
     * it very seldom.
     *
     * @return the total count of objects; will always be non-negative.
     */
    virtual sal_uInt32 totalCount(std::unique_lock<std::mutex>& rResultSetGuard) = 0;

    /**
     * This method returns the count of objects obtained so far. There is no
     * for the implementation to obtain all objects at once. It can obtain
     * all data on demand.
     *
     * The implementation should call m_pResultSet->rowCountChanged(...)
     * every time it has inserted a new entry in its logical result array.
     *
     * @return the count of objects obtained so far; will always be
     * non-negative.
     */
    virtual sal_uInt32 currentCount() = 0;

    /**
     * This method returns whether the value returned by currentCount() is
     * "final". This is the case, if that there was all data obtained by the
     * supplier and the current count won't increase any more.
     *
     * The implementation should call m_pResultSet->rowCountFinal(...) if
     * it has inserted all entries in its logical result array.
     *
     * @return true, if the value returned by currentCount() won't change
               anymore.
     */
    virtual bool isCountFinal() = 0;

    /**
     * This method returns an object for accessing the property values at
     * the specified index. The implementation may use the helper class
     * ucb::PropertyValueSet to provide the return value.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier.
     * @return the object for accessing the property values.
     */
    virtual css::uno::Reference< css::sdbc::XRow >
    queryPropertyValues( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex  ) = 0;

    /**
     * This method is called to instruct the supplier to release the (possibly
     * present) property values at the given index.
     *
     * @param nIndex is the zero-based index within the logical data array
     *               of the supplier.
     */
    virtual void releasePropertyValues( sal_uInt32 nIndex ) = 0;

    /**
     * This method will be called by the resultset implementation in order
     * to instruct the data supplier to release all resources it has
     * allocated so far. In case the supplier is collecting data
     * asynchronously, that process must be stopped.
     */
    virtual void close() = 0;

    /**
     * This method will be called by the resultset implementation in order
     * check, whether an error has occurred while collecting data. The
     * implementation of this method must throw an exception in that case.
     *
     * Note: An exception thrown to indicate an error must always be thrown
     * by the thread that created the data supplier. If the supplier collects
     * data asynchronously ( i.e. in a separate thread ) and an error
     * occurs, throwing of the appropriate exception must be deferred
     * until validate() is called by the ResultSet implementation from
     * inside the main thread.
     * In case data are obtained synchronously, the ResultSetException can
     * be thrown directly.
     *
     * @exception ResultSetException thrown, if an error has occurred
     */
    virtual void validate() = 0;
};

}

#endif /* ! INCLUDED_UCBHELPER_RESULTSET_HXX */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

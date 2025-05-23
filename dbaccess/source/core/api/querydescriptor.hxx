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

#include <cppuhelper/implbase2.hxx>
#include <comphelper/proparrhlp.hxx>
#include <osl/mutex.hxx>

#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <apitools.hxx>
#include <column.hxx>
#include <datasettings.hxx>
#include <commandbase.hxx>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/uno3.hxx>

namespace dbaccess
{

// OQueryDescriptor_Base - a query descriptor (as the name suggests :)
typedef ::cppu::ImplHelper2<
        css::sdbcx::XColumnsSupplier,
        css::lang::XServiceInfo >  OQueryDescriptor_BASE;

class OQueryDescriptor_Base
        :public OQueryDescriptor_BASE
        ,public OCommandBase
        ,public IColumnFactory
        ,public ::connectivity::sdbcx::IRefreshableColumns
{
private:
    bool        m_bColumnsOutOfDate : 1;    // the columns have to be rebuild on the next getColumns ?
    ::osl::Mutex&   m_rMutex;

protected:
    std::unique_ptr<OColumns> m_pColumns;                 // our column descriptions
    OUString m_sElementName;
    virtual ~OQueryDescriptor_Base();

    void        setColumnsOutOfDate( bool _bOutOfDate = true );

    sal_Int32   getColumnCount() const { return m_pColumns ? m_pColumns->getCount() : 0; }
    void        clearColumns( );

    void        implAppendColumn( const OUString& _rName, OColumn* _pColumn );

public:
    OQueryDescriptor_Base(::osl::Mutex& _rMutex,::cppu::OWeakObject& _rMySelf);
    /** constructs the object with a UNO QueryDescriptor. If you use this ctor, the resulting object
        won't have any column information (the column container will be empty)
    */
    OQueryDescriptor_Base(const OQueryDescriptor_Base& _rSource,::cppu::OWeakObject& _rMySelf);

// css::sdbcx::XColumnsSupplier
    virtual css::uno::Reference< css::container::XNameAccess > SAL_CALL getColumns(  ) override;

// css::lang::XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

protected:

// IColumnFactory
    virtual rtl::Reference<OColumn> createColumn(const OUString& _rName) const override;
    virtual css::uno::Reference< css::beans::XPropertySet > createColumnDescriptor() override;
    virtual void columnAppended( const css::uno::Reference< css::beans::XPropertySet >& _rxSourceDescriptor ) override;
    virtual void columnDropped(const OUString& _sName) override;

    /** rebuild our columns set

        clearColumns has already been called before, do <em>NOT</em> call it, again
    */
    virtual void rebuildColumns( );

    void disposeColumns();

    // IRefreshableColumns overridables
    virtual void refreshColumns() override;
};

class OQueryDescriptor : public comphelper::OMutexAndBroadcastHelper
                        ,public ::cppu::OWeakObject
                        ,public OQueryDescriptor_Base
                        ,public ::comphelper::OPropertyArrayUsageHelper< OQueryDescriptor_Base >
                        ,public ODataSettings
{
    OQueryDescriptor(const OQueryDescriptor&) = delete;
    void operator =(const OQueryDescriptor&) = delete;
    // helper
    void registerProperties();
protected:
    // OPropertyArrayUsageHelper
    virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;

    // OPropertySetHelper
    virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

    virtual ~OQueryDescriptor() override;
public:
    OQueryDescriptor();
    explicit OQueryDescriptor(const OQueryDescriptor_Base& _rSource);

    virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
    virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;

// css::uno::XInterface
    DECLARE_XINTERFACE( )

    // css::beans::XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;

};
}   // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

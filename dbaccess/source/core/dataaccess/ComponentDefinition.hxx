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

#include <commandbase.hxx>
#include <com/sun/star/sdbcx/XRename.hpp>
#include <cppuhelper/implbase1.hxx>
#include <comphelper/proparrhlp.hxx>
#include <rtl/ref.hxx>
#include <datasettings.hxx>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <ContentHelper.hxx>
#include <apitools.hxx>
#include <column.hxx>

#include <memory>
namespace dbaccess
{

    typedef ::cppu::ImplHelper1< css::sdbcx::XColumnsSupplier > OComponentDefinition_BASE;

    class OComponentDefinition_Impl : public OContentHelper_Impl
                                     ,public ODataSettings_Base
    {
    public:
        typedef std::map  <   OUString
                            ,   css::uno::Reference< css::beans::XPropertySet >
                            >   Columns;
    typedef Columns::iterator           iterator;
    typedef Columns::const_iterator     const_iterator;

    private:
        Columns             m_aColumns;

    public:
        OUString     m_sSchemaName;
        OUString     m_sCatalogName;

    public:
        OComponentDefinition_Impl();
        virtual ~OComponentDefinition_Impl() override;

        size_t size() const { return m_aColumns.size(); }

        const_iterator begin() const   { return m_aColumns.begin(); }
        const_iterator end() const     { return m_aColumns.end(); }

        const_iterator find( const OUString& _rName ) const { return m_aColumns.find( _rName ); }

        void erase( const OUString& _rName ) { m_aColumns.erase( _rName ); }

        void insert( const OUString& _rName, const css::uno::Reference< css::beans::XPropertySet >& _rxColumn )
        {
            OSL_PRECOND( m_aColumns.find( _rName ) == m_aColumns.end(), "OComponentDefinition_Impl::insert: there's already an element with this name!" );
            m_aColumns.emplace(  _rName, _rxColumn );
        }
    };

class OColumnPropertyListener;
// OComponentDefinition - a database "document" which describes a query
class OComponentDefinition  :public OContentHelper
                            ,public ODataSettings
                            ,public IColumnFactory
                            ,public OComponentDefinition_BASE
                            ,public ::comphelper::OPropertyArrayUsageHelper< OComponentDefinition >
{
    // no Reference! see OCollection::acquire
    std::unique_ptr<OColumns> m_pColumns;
    rtl::Reference<OColumnPropertyListener> m_xColumnPropertyListener;
    bool                        m_bTable;

protected:
    virtual ~OComponentDefinition() override;
    virtual void SAL_CALL disposing() override;

    const   OComponentDefinition_Impl& getDefinition() const { return dynamic_cast< const OComponentDefinition_Impl& >( *m_pImpl ); }
            OComponentDefinition_Impl& getDefinition()       { return dynamic_cast<       OComponentDefinition_Impl& >( *m_pImpl ); }
public:
    OComponentDefinition(
        const css::uno::Reference< css::uno::XComponentContext >&,
        const css::uno::Reference< css::uno::XInterface >& _xParentContainer,
        const TContentPtr& _pImpl,
        bool _bTable = true);

    OComponentDefinition(
             const css::uno::Reference< css::uno::XInterface >& _rxContainer
            ,const OUString& _rElementName
            ,const css::uno::Reference< css::uno::XComponentContext >&
            ,const TContentPtr& _pImpl
            ,bool _bTable = true
        );

    virtual css::uno::Sequence<css::uno::Type> SAL_CALL getTypes() override;
    virtual css::uno::Sequence<sal_Int8> SAL_CALL getImplementationId() override;

// css::uno::XInterface
    DECLARE_XINTERFACE( )

    // css::lang::XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // XInitialization
    virtual void SAL_CALL initialize( css::uno::Sequence< css::uno::Any > const & rArguments) override;

    // css::beans::XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;

    // XColumnsSupplier
    virtual css::uno::Reference< css::container::XNameAccess > SAL_CALL getColumns(  ) override;

    // OPropertySetHelper
    virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

    // IColumnFactory
    virtual rtl::Reference<OColumn> createColumn(const OUString& _rName) const override;
    virtual css::uno::Reference< css::beans::XPropertySet > createColumnDescriptor() override;
    virtual void columnAppended( const css::uno::Reference< css::beans::XPropertySet >& _rxSourceDescriptor ) override;
    virtual void columnDropped(const OUString& _sName) override;
    using OContentHelper::notifyDataSourceModified;

protected:
// OPropertyArrayUsageHelper
    virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;

    virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
                                    sal_Int32 nHandle,
                                    const css::uno::Any& rValue) override;

    // OContentHelper overridables
    virtual OUString determineContentType() const override;

private:
    void registerProperties();
};

}   // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

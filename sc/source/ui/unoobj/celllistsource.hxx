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

#include <com/sun/star/form/binding/XListEntryTypedSource.hpp>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/propertycontainer2.hxx>
#include <comphelper/uno3.hxx>
#include <comphelper/proparrhlp.hxx>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/util/XModifyListener.hpp>

namespace com::sun::star::sheet { class XSpreadsheetDocument; }
namespace com::sun::star::table { class XCellRange; }

namespace calc
{

    //= OCellListSource

    class OCellListSource;
    // the base for our interfaces
    typedef ::comphelper::WeakComponentImplHelper <   css::form::binding::XListEntryTypedSource
                                            ,   css::util::XModifyListener
                                            ,   css::lang::XServiceInfo
                                            ,   css::lang::XInitialization
                                            >   OCellListSource_Base;
    // the base for the property handling
    typedef ::comphelper::OPropertyContainer2        OCellListSource_PBase;
    // the second base for property handling
    typedef ::comphelper::OPropertyArrayUsageHelper< OCellListSource >
                                                    OCellListSource_PABase;

    class OCellListSource :public OCellListSource_Base      // order matters! before OCellListSource_PBase, so rBHelper gets initialized
                            ,public OCellListSource_PBase
                            ,public OCellListSource_PABase
    {
    private:
        css::uno::Reference< css::sheet::XSpreadsheetDocument >
                    m_xDocument;            /// the document where our cell lives
        css::uno::Reference< css::table::XCellRange >
                    m_xRange;               /// the range of cells we're bound to
        ::comphelper::OInterfaceContainerHelper4<css::form::binding::XListEntryListener>
                    m_aListEntryListeners;  /// our listeners
        bool        m_bInitialized;         /// has XInitialization::initialize been called?

    public:
        explicit OCellListSource(
            const css::uno::Reference< css::sheet::XSpreadsheetDocument >& _rxDocument
        );

        using OCellListSource_PBase::getFastPropertyValue;

    protected:
        virtual ~OCellListSource( ) override;

    protected:
        // XInterface
        DECLARE_XINTERFACE()

        // XTypeProvider
        DECLARE_XTYPEPROVIDER()

        // XListEntrySource
        virtual sal_Int32 SAL_CALL getListEntryCount(  ) override;
        virtual OUString SAL_CALL getListEntry( sal_Int32 Position ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getAllListEntries(  ) override;
        virtual void SAL_CALL addListEntryListener( const css::uno::Reference< css::form::binding::XListEntryListener >& Listener ) override;
        virtual void SAL_CALL removeListEntryListener( const css::uno::Reference< css::form::binding::XListEntryListener >& Listener ) override;

        // XListEntryTypedSource
        virtual css::uno::Sequence< OUString > SAL_CALL getAllListEntriesTyped( css::uno::Sequence< css::uno::Any >& rDataValues ) override;

        // OComponentHelper/XComponent
        virtual void disposing( std::unique_lock<std::mutex>& ) override;

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName(  ) override;
        virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

        // XPropertySet
        virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;

        // OPropertySetHelper
        virtual ::cppu::IPropertyArrayHelper& getInfoHelper() override;
        virtual void getFastPropertyValue( std::unique_lock<std::mutex>& rGuard, css::uno::Any& _rValue, sal_Int32 _nHandle ) const override;

        // ::comphelper::OPropertyArrayUsageHelper
        virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;

        // XModifyListener
        virtual void SAL_CALL modified( const css::lang::EventObject& aEvent ) override;
        virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;

        // XInitialization
        virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

    private:
        void    checkInitialized();

        /** retrieves the actual address of our cell range
            @precond
                our m_xRange is not <NULL/>
        */
        css::table::CellRangeAddress
                getRangeAddress( ) const;

        /** retrieves the text of a cell within our range
            @param _nRangeRelativeRow
                the relative row index of the cell within our range
            @param pAny
                if not <NULL/> then the underlying data value is returned in the Any
            @precond
                our m_xRange is not <NULL/>
        */
        OUString
                getCellTextContent_noCheck(
                    std::unique_lock<std::mutex>& rGuard,
                    sal_Int32 _nRangeRelativeRow,
                    css::uno::Any* pAny
                );

        void    notifyModified();

        sal_Int32 getListEntryCount(std::unique_lock<std::mutex>& rGuard);

    private:
        OCellListSource( const OCellListSource& ) = delete;
        OCellListSource& operator=( const OCellListSource& ) = delete;
    };

}   // namespace calc

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include "contentresultsetwrapper.hxx"
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ucb/XFetchProvider.hpp>
#include <com/sun/star/ucb/XFetchProviderForContentAccess.hpp>
#include <com/sun/star/ucb/XCachedContentResultSetStubFactory.hpp>
#include <cppuhelper/implbase.hxx>

class CachedContentResultSetStub
                : public ContentResultSetWrapper
                , public css::lang::XTypeProvider
                , public css::lang::XServiceInfo
                , public css::ucb::XFetchProvider
                , public css::ucb::XFetchProviderForContentAccess
{
private:
    sal_Int32       m_nColumnCount;
    bool        m_bColumnCountCached;

    //members to propagate fetchsize and direction:
    bool        m_bNeedToPropagateFetchSize;
    bool        m_bFirstFetchSizePropagationDone;
    sal_Int32       m_nLastFetchSize;
    bool        m_bLastFetchDirection;
    const OUString     m_aPropertyNameForFetchSize;
    const OUString     m_aPropertyNameForFetchDirection;

    /// @throws css::sdbc::SQLException
    /// @throws css::uno::RuntimeException
    void
    impl_getCurrentRowContent(
        std::unique_lock<std::mutex>& rGuard,
        css::uno::Any& rRowContent,
        const css::uno::Reference< css::sdbc::XRow >& xRow );

    sal_Int32
    impl_getColumnCount(std::unique_lock<std::mutex>&);

    /// @throws css::uno::RuntimeException
    static void
    impl_getCurrentContentIdentifierString(
            std::unique_lock<std::mutex>& rGuard,
            css::uno::Any& rAny
            , const css::uno::Reference< css::ucb::XContentAccess >& xContentAccess );

    /// @throws css::uno::RuntimeException
    static void
    impl_getCurrentContentIdentifier(
            std::unique_lock<std::mutex>& rGuard,
            css::uno::Any& rAny
            , const css::uno::Reference< css::ucb::XContentAccess >& xContentAccess );

    /// @throws css::uno::RuntimeException
    static void
    impl_getCurrentContent(
            std::unique_lock<std::mutex>& rGuard,
            css::uno::Any& rAny
            , const css::uno::Reference< css::ucb::XContentAccess >& xContentAccess );

    /// @throws css::uno::RuntimeException
    void
    impl_propagateFetchSizeAndDirection( std::unique_lock<std::mutex>& rGuard, sal_Int32 nFetchSize, bool bFetchDirection );

    css::ucb::FetchResult impl_fetchHelper(
        std::unique_lock<std::mutex>& rGuard,
        sal_Int32 nRowStartPosition, sal_Int32 nRowCount, bool bDirection,
        std::function<void(std::unique_lock<std::mutex>&, css::uno::Any& rRowContent)> impl_loadRow);

public:
    CachedContentResultSetStub( css::uno::Reference< css::sdbc::XResultSet > const & xOrigin );

    virtual ~CachedContentResultSetStub() override;


    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire()
        noexcept override;
    virtual void SAL_CALL release()
        noexcept override;

    // own inherited

    virtual void
    impl_propertyChange( const css::beans::PropertyChangeEvent& evt ) override;

    virtual void
    impl_vetoableChange( const css::beans::PropertyChangeEvent& aEvent ) override;

    // XTypeProvider

    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() override;
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XFetchProvider


    virtual css::ucb::FetchResult SAL_CALL
    fetch( sal_Int32 nRowStartPosition
        , sal_Int32 nRowCount, sal_Bool bDirection ) override;


    // XFetchProviderForContentAccess

    virtual css::ucb::FetchResult SAL_CALL
         fetchContentIdentifierStrings( sal_Int32 nRowStartPosition
        , sal_Int32 nRowCount, sal_Bool bDirection ) override;

    virtual css::ucb::FetchResult SAL_CALL
         fetchContentIdentifiers( sal_Int32 nRowStartPosition
        , sal_Int32 nRowCount, sal_Bool bDirection ) override;

    virtual css::ucb::FetchResult SAL_CALL
         fetchContents( sal_Int32 nRowStartPosition
        , sal_Int32 nRowCount, sal_Bool bDirection ) override;
};


class CachedContentResultSetStubFactory final :
                public cppu::WeakImplHelper<
                    css::lang::XServiceInfo,
                    css::ucb::XCachedContentResultSetStubFactory>
{
public:

    CachedContentResultSetStubFactory();

    virtual ~CachedContentResultSetStubFactory() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XCachedContentResultSetStubFactory

    virtual css::uno::Reference< css::sdbc::XResultSet > SAL_CALL
    createCachedContentResultSetStub(
                const css::uno::Reference< css::sdbc::XResultSet > & xSource ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

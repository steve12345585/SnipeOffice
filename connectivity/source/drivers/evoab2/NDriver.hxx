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

#include <sal/config.h>

#include <string_view>

#include <com/sun/star/sdbc/XDriver.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/compbase.hxx>
#include <connectivity/CommonTools.hxx>
#include <unotools/weakref.hxx>

inline constexpr OUString EVOAB_DRIVER_IMPL_NAME = u"com.sun.star.comp.sdbc.evoab.OEvoabDriver"_ustr;

namespace connectivity::evoab
{
    class OEvoabConnection;

    typedef ::cppu::WeakComponentImplHelper< css::sdbc::XDriver,
                                             css::lang::XServiceInfo > ODriver_BASE;


    class OEvoabDriver final : public ODriver_BASE
    {
        ::osl::Mutex                                        m_aMutex;
        std::vector<unotools::WeakReference<OEvoabConnection>> m_xConnections;
        css::uno::Reference< css::uno::XComponentContext >  m_xContext;

    public:
        explicit OEvoabDriver(const css::uno::Reference< css::uno::XComponentContext >& );
        virtual ~OEvoabDriver() override;

        // OComponentHelper
        virtual void SAL_CALL disposing() override;

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName(  ) override;
        virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;


        // XDriver
        virtual css::uno::Reference< css::sdbc::XConnection > SAL_CALL connect( const OUString& url, const css::uno::Sequence< css::beans::PropertyValue >& info ) override;
        virtual sal_Bool SAL_CALL acceptsURL( const OUString& url ) override;
        virtual css::uno::Sequence< css::sdbc::DriverPropertyInfo > SAL_CALL getPropertyInfo( const OUString& url, const css::uno::Sequence< css::beans::PropertyValue >& info ) override;
        virtual sal_Int32 SAL_CALL getMajorVersion(  ) override;
        virtual sal_Int32 SAL_CALL getMinorVersion(  ) override;

    public:
        const css::uno::Reference< css::uno::XComponentContext >& getComponentContext( ) const { return m_xContext; }

        // static methods
        static bool acceptsURL_Stat( std::u16string_view url );
    };
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

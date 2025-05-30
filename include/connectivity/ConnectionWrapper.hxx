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

#ifndef INCLUDED_CONNECTIVITY_CONNECTIONWRAPPER_HXX
#define INCLUDED_CONNECTIVITY_CONNECTIONWRAPPER_HXX

#include <sal/config.h>

#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <connectivity/CommonTools.hxx>
#include <connectivity/dbtoolsdllapi.hxx>

namespace com::sun::star::beans { struct PropertyValue; }
namespace com::sun::star::sdbc { class XConnection; }
namespace com::sun::star::uno { class XComponentContext; }

namespace connectivity
{


    //= OConnectionWrapper - wraps all methods to the real connection from the driver
    //= but when disposed it doesn't dispose the real connection

    typedef cppu::WeakComponentImplHelper<css::lang::XServiceInfo,
                                        css::lang::XUnoTunnel
                                > OConnection_BASE;

    class OOO_DLLPUBLIC_DBTOOLS OConnectionWrapper : public cppu::BaseMutex,
                                                     public OConnection_BASE
    {
    protected:
        css::uno::Reference< css::uno::XAggregation >   m_xProxyConnection;
        css::uno::Reference< css::sdbc::XConnection >   m_xConnection;
        css::uno::Reference< css::lang::XTypeProvider > m_xTypeProvider;
        css::uno::Reference< css::lang::XUnoTunnel >    m_xUnoTunnel;
        css::uno::Reference< css::lang::XServiceInfo >  m_xServiceInfo;

        virtual ~OConnectionWrapper();
        void setDelegation(css::uno::Reference< css::uno::XAggregation >& _rxProxyConnection);
        void setDelegation(const css::uno::Reference< css::sdbc::XConnection >& _xConnection
            ,const css::uno::Reference< css::uno::XComponentContext>& _rxContext);
        virtual void SAL_CALL disposing() override;
    public:
        OConnectionWrapper( );

        // XServiceInfo
        DECLARE_SERVICE_INFO();
        virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type& _rType ) override;
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;

        // css::lang::XUnoTunnel
        virtual sal_Int64 SAL_CALL getSomething( const css::uno::Sequence< sal_Int8 >& aIdentifier ) override;
        static const css::uno::Sequence< sal_Int8 > & getUnoTunnelId();
        /** method to create unique ids
            @param  _rURL
                The URL.
            @param  _rInfo
                The info property of the datasource. It will be resorted if needed.
            @param  _pBuffer
                Here we store the digest. Must not NULL.
            @param  _rUserName
                The user name.
            @param  _rPassword
                The password.
        */
        static void createUniqueId( const OUString& _rURL
                    ,css::uno::Sequence< css::beans::PropertyValue >& _rInfo
                    ,sal_uInt8* _pBuffer
                    ,const OUString& _rUserName = OUString()
                    ,const OUString& _rPassword = OUString());
    };
}
#endif // INCLUDED_CONNECTIVITY_CONNECTIONWRAPPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

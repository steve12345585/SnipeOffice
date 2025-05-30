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

#include <ado/ADriver.hxx>
#include <ado/AConnection.hxx>
#include <ado/Awrapadox.hxx>
#include <ado/ACatalog.hxx>
#include <ado/Awrapado.hxx>
#include <ado/adoimp.hxx>
#include <com/sun/star/lang/DisposedException.hpp>
#include <comphelper/servicehelper.hxx>
#include <connectivity/dbexception.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/safeCoInitUninit.hxx>
#include <rtl/ref.hxx>
#include <strings.hrc>
#include <objbase.h>

#include <resource/sharedresources.hxx>

using namespace connectivity;
using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;


ODriver::ODriver(const css::uno::Reference< css::uno::XComponentContext >& _xORB)
    : ODriver_BASE(m_aMutex)
    ,m_xContext(_xORB)
    ,mnNbCallCoInitializeExForReinit(0)
{
     o3tl::safeCoInitializeEx(COINIT_APARTMENTTHREADED, mnNbCallCoInitializeExForReinit);
}

ODriver::~ODriver()
{
    o3tl::safeCoUninitializeReinit(COINIT_MULTITHREADED, mnNbCallCoInitializeExForReinit);
}

void ODriver::disposing()
{
    ::osl::MutexGuard aGuard(m_aMutex);


    for (auto& rxConnection : m_xConnections)
    {
        rtl::Reference< OConnection > xComp(rxConnection.get());
        if (xComp.is())
            xComp->dispose();
    }
    m_xConnections.clear();

    ODriver_BASE::disposing();
}
// static ServiceInfo

OUString ODriver::getImplementationName( )
{
    return "com.sun.star.comp.sdbc.ado.ODriver";
}

Sequence< OUString > ODriver::getSupportedServiceNames( )
{
    return { "com.sun.star.sdbc.Driver", "com.sun.star.sdbcx.Driver" };
}

sal_Bool SAL_CALL ODriver::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}

Reference< XConnection > SAL_CALL ODriver::connect( const OUString& url, const Sequence< PropertyValue >& info )
{
    if ( ! acceptsURL(url) )
        return nullptr;

    // we need to wrap the connection as the construct call might throw
    rtl::Reference<OConnection> pCon(new OConnection(this));
    pCon->construct(url,info);
    m_xConnections.push_back(pCon);

    return pCon;
}

sal_Bool SAL_CALL ODriver::acceptsURL( const OUString& url )
{
    return url.startsWith("sdbc:ado:");
}

void ODriver::impl_checkURL_throw(const OUString& _sUrl)
{
    if ( !acceptsURL(_sUrl) )
    {
        SharedResources aResources;
        const OUString sMessage = aResources.getResourceString(STR_URI_SYNTAX_ERROR);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    } // if ( !acceptsURL(_sUrl) )
}

Sequence< DriverPropertyInfo > SAL_CALL ODriver::getPropertyInfo( const OUString& url, const Sequence< PropertyValue >& /*info*/ )
{
    impl_checkURL_throw(url);
    if ( acceptsURL(url) )
    {
        Sequence< OUString > aBooleanValues{ "false", "true" };

        return
        {
            {
                 "IgnoreDriverPrivileges",
                 "Ignore the privileges from the database driver.",
                 false,
                 "false",
                 aBooleanValues
            },
            {
                "EscapeDateTime",
                "Escape date time format.",
                false,
                "true",
                aBooleanValues
            },
            {
                "TypeInfoSettings",
                "Defines how the type info of the database metadata should be manipulated.",
                false,
                {},
                {}
            }
        };
    }
    return Sequence< DriverPropertyInfo >();
}

sal_Int32 SAL_CALL ODriver::getMajorVersion(  )
{
    return 1;
}

sal_Int32 SAL_CALL ODriver::getMinorVersion(  )
{
    return 0;
}

// XDataDefinitionSupplier
Reference< XTablesSupplier > SAL_CALL ODriver::getDataDefinitionByConnection( const Reference< css::sdbc::XConnection >& connection )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    if (ODriver_BASE::rBHelper.bDisposed)
        throw DisposedException();

    OConnection* pConnection = nullptr;
    Reference< css::lang::XUnoTunnel> xTunnel(connection,UNO_QUERY);
    if (auto pSearchConnection = comphelper::getFromUnoTunnel<OConnection>(xTunnel))
    {
        auto foundConnection = std::any_of(m_xConnections.begin(), m_xConnections.end(),
            [&pSearchConnection](const css::uno::WeakReferenceHelper& rxConnection) {
                return static_cast<OConnection*>(Reference< XConnection >::query(rxConnection.get()).get()) == pSearchConnection; });
        if (foundConnection)
            pConnection = pSearchConnection;
    }

    Reference< XTablesSupplier > xTab;
    if(pConnection)
    {
        WpADOCatalog aCatalog;
        aCatalog.Create();
        if(aCatalog.IsValid())
        {
            aCatalog.putref_ActiveConnection(pConnection->getConnection());
            rtl::Reference<OCatalog> pCatalog = new OCatalog(aCatalog,pConnection);
            xTab = pCatalog;
            pConnection->setCatalog(pCatalog.get());
        }
    }
    return xTab;
}

Reference< XTablesSupplier > SAL_CALL ODriver::getDataDefinitionByURL( const OUString& url, const Sequence< PropertyValue >& info )
{
    impl_checkURL_throw(url);
    return getDataDefinitionByConnection(connect(url,info));
}


void ADOS::ThrowException(ADOConnection* _pAdoCon,const Reference< XInterface >& _xInterface)
{
    sal::systools::COMReference<ADOErrors> pErrors;
    _pAdoCon->get_Errors(&pErrors);
    if(!pErrors)
        return; // no error found

    // read all noted errors and issue them
    sal_Int32 nLen;
    pErrors->get_Count(&nLen);
    if (nLen)
    {
        SQLException aException;
        aException.ErrorCode = 1000;
        for (sal_Int32 i = nLen-1; i>=0; --i)
        {
            WpADOError aErr;
            pErrors->get_Item(OLEVariant(i),&aErr);
            OSL_ENSURE(aErr,"No error in collection found! BAD!");
            if(aErr)
            {
                if(i==nLen-1)
                    aException = SQLException(aErr.GetDescription(),_xInterface,aErr.GetSQLState(),aErr.GetNumber(),Any());
                else
                {
                    SQLException aTemp(aErr.GetDescription(),
                        _xInterface,aErr.GetSQLState(),aErr.GetNumber(),Any(aException));
                    aTemp.NextException <<= aException;
                    aException = aTemp;
                }
            }
        }
        pErrors->Clear();
        throw aException;
    }
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
connectivity_ado_ODriver_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new ODriver(context));
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

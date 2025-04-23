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

#include "NDriver.hxx"
#include "NConnection.hxx"
#include <com/sun/star/lang/DisposedException.hpp>
#include <connectivity/dbexception.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <rtl/ref.hxx>
#include <strings.hrc>
#include <resource/sharedresources.hxx>

using namespace osl;
using namespace connectivity::evoab;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::lang;


OEvoabDriver::OEvoabDriver(const Reference< XComponentContext >& _rxContext) :
        ODriver_BASE( m_aMutex ), m_xContext( _rxContext )
{
}

OEvoabDriver::~OEvoabDriver()
{
}

void OEvoabDriver::disposing()
{
    ::osl::MutexGuard aGuard(m_aMutex);

    // when driver will be destroyed so all our connections have to be destroyed as well
    for (const auto& rxConnection : m_xConnections)
    {
        rtl::Reference< OEvoabConnection > xComp(rxConnection);
        if (xComp.is())
        {
            try
            {
                xComp->dispose();
            }
            catch (const css::lang::DisposedException&)
            {
                xComp.clear();
            }
        }
    }
    m_xConnections.clear();

    ODriver_BASE::disposing();
}

// static ServiceInfo


OUString SAL_CALL OEvoabDriver::getImplementationName(  )
{
    return EVOAB_DRIVER_IMPL_NAME;
    // this name is referenced in the configuration and in the evoab.xml
    // Please take care when changing it.
}

sal_Bool SAL_CALL OEvoabDriver::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}

Sequence< OUString > SAL_CALL OEvoabDriver::getSupportedServiceNames(  )
{
    // which service is supported
    // for more information @see com.sun.star.sdbc.Driver
    return { u"com.sun.star.sdbc.Driver"_ustr };
}


Reference< XConnection > SAL_CALL OEvoabDriver::connect( const OUString& url, const Sequence< PropertyValue >& info )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    if (ODriver_BASE::rBHelper.bDisposed)
        throw DisposedException();

    if ( ! acceptsURL(url) )
        return nullptr;

    rtl::Reference<OEvoabConnection> pCon = new OEvoabConnection( *this );
    pCon->construct(url,info);
    m_xConnections.push_back(pCon);

    return pCon;
}

sal_Bool SAL_CALL OEvoabDriver::acceptsURL( const OUString& url )
{
    return acceptsURL_Stat(url);
}


Sequence< DriverPropertyInfo > SAL_CALL OEvoabDriver::getPropertyInfo( const OUString& url, const Sequence< PropertyValue >& /*info*/ )
{
    if ( ! acceptsURL(url) )
    {
        ::connectivity::SharedResources aResources;
        const OUString sMessage = aResources.getResourceString(STR_URI_SYNTAX_ERROR);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    } // if ( ! acceptsURL(url) )

    // if you have something special to say return it here :-)
    return Sequence< DriverPropertyInfo >();
}


sal_Int32 SAL_CALL OEvoabDriver::getMajorVersion(  )
{
    return 1;
}

sal_Int32 SAL_CALL OEvoabDriver::getMinorVersion(  )
{
    return 0;
}

bool OEvoabDriver::acceptsURL_Stat( std::u16string_view url )
{
    return ( url == u"sdbc:address:evolution:local" || url == u"sdbc:address:evolution:groupwise" || url == u"sdbc:address:evolution:ldap" ) && EApiInit();
}


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
connectivity_OEvoabDriver_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new OEvoabDriver(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

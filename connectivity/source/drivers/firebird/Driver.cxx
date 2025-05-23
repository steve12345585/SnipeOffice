/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include "Connection.hxx"
#include "Driver.hxx"
#include "SubComponent.hxx"

#include <connectivity/dbexception.hxx>
#include <strings.hrc>
#include <resource/sharedresources.hxx>

#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/file.hxx>
#include <osl/process.h>
#include <rtl/bootstrap.hxx>
#include <sal/log.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;

using namespace ::osl;

using namespace connectivity::firebird;

// Static const variables
namespace {
constexpr OUString our_sFirebirdTmpVar = u"FIREBIRD_TMP"_ustr;
constexpr OUString our_sFirebirdLockVar = u"FIREBIRD_LOCK"_ustr;
constexpr OUString our_sFirebirdMsgVar = u"FIREBIRD_MSG"_ustr;
#ifdef MACOSX
constexpr OUString our_sFirebirdLibVar = u"LIBREOFFICE_FIREBIRD_LIB"_ustr;
#endif
};

FirebirdDriver::FirebirdDriver(const css::uno::Reference< css::uno::XComponentContext >& _rxContext)
    : ODriver_BASE(m_aMutex)
    , m_aContext(_rxContext)
    , m_firebirdTMPDirectory(nullptr, true)
    , m_firebirdLockDirectory(nullptr, true)
{
    // ::utl::TempFile uses a unique temporary directory (subdirectory of
    // /tmp or other user specific tmp directory) per instance in which
    // we can create directories for firebird at will.
    m_firebirdTMPDirectory.EnableKillingFile(true);
    m_firebirdLockDirectory.EnableKillingFile(true);

    // Overrides firebird's default of /tmp or c:\temp
    osl_setEnvironment(our_sFirebirdTmpVar.pData, m_firebirdTMPDirectory.GetFileName().pData);

    // Overrides firebird's default of /tmp/firebird or c:\temp\firebird
    osl_setEnvironment(our_sFirebirdLockVar.pData, m_firebirdLockDirectory.GetFileName().pData);

#ifndef SYSTEM_FIREBIRD
    // Overrides firebird's hardcoded default of /usr/local/firebird on *nix,
    // however on Windows it seems to use the current directory as a default.
    OUString sMsgURL(u"$BRAND_BASE_DIR/$BRAND_SHARE_SUBDIR/firebird"_ustr);
    ::rtl::Bootstrap::expandMacros(sMsgURL);
    OUString sMsgPath;
    ::osl::FileBase::getSystemPathFromFileURL(sMsgURL, sMsgPath);
    osl_setEnvironment(our_sFirebirdMsgVar.pData, sMsgPath.pData);
#ifdef MACOSX
    // Set an env. variable to specify library location
    // for dlopen used in fbclient.
    OUString sLibURL("$LO_LIB_DIR");
    ::rtl::Bootstrap::expandMacros(sLibURL);
    OUString sLibPath;
    ::osl::FileBase::getSystemPathFromFileURL(sLibURL, sLibPath);
    osl_setEnvironment(our_sFirebirdLibVar.pData, sLibPath.pData);
#endif /*MACOSX*/
#endif /*!SYSTEM_FIREBIRD*/
}

FirebirdDriver::~FirebirdDriver() = default;

void FirebirdDriver::disposing()
{
    MutexGuard aGuard(m_aMutex);

    for (auto const& elem : m_xConnections)
    {
        rtl::Reference< Connection > xComp(elem.get());
        if (xComp.is())
            xComp->dispose();
    }
    m_xConnections.clear();

    osl_clearEnvironment(our_sFirebirdTmpVar.pData);
    osl_clearEnvironment(our_sFirebirdLockVar.pData);

#ifndef SYSTEM_FIREBIRD
    osl_clearEnvironment(our_sFirebirdMsgVar.pData);
#ifdef MACOSX
    osl_clearEnvironment(our_sFirebirdLibVar.pData);
#endif /*MACOSX*/
#endif /*!SYSTEM_FIREBIRD*/

    OSL_VERIFY(fb_shutdown(0, 1));

    ODriver_BASE::disposing();
}

OUString SAL_CALL FirebirdDriver::getImplementationName()
{
    return u"com.sun.star.comp.sdbc.firebird.Driver"_ustr;
}

sal_Bool SAL_CALL FirebirdDriver::supportsService(const OUString& _rServiceName)
{
    return cppu::supportsService(this, _rServiceName);
}

Sequence< OUString > SAL_CALL FirebirdDriver::getSupportedServiceNames()
{
    return { u"com.sun.star.sdbc.Driver"_ustr, u"com.sun.star.sdbcx.Driver"_ustr };
}

// ----  XDriver -------------------------------------------------------------
Reference< XConnection > SAL_CALL FirebirdDriver::connect(
    const OUString& url, const Sequence< PropertyValue >& info )
{
    SAL_INFO("connectivity.firebird", "connect(), URL: " << url );

    MutexGuard aGuard( m_aMutex );
    if (ODriver_BASE::rBHelper.bDisposed)
       throw DisposedException();

    if ( ! acceptsURL(url) )
        return nullptr;

    rtl::Reference<Connection> pCon = new Connection();
    pCon->construct(url, info);

    m_xConnections.push_back(pCon);

    return pCon;
}

sal_Bool SAL_CALL FirebirdDriver::acceptsURL( const OUString& url )
{
    return (url == "sdbc:embedded:firebird" || url.startsWith("sdbc:firebird:"));
}

Sequence< DriverPropertyInfo > SAL_CALL FirebirdDriver::getPropertyInfo(
    const OUString& url, const Sequence< PropertyValue >& )
{
    if ( ! acceptsURL(url) )
    {
        ::connectivity::SharedResources aResources;
        const OUString sMessage = aResources.getResourceString(STR_URI_SYNTAX_ERROR);
        ::dbtools::throwGenericSQLException(sMessage ,*this);
    }

    return Sequence< DriverPropertyInfo >();
}

sal_Int32 SAL_CALL FirebirdDriver::getMajorVersion(  )
{
    // The major and minor version are sdbc driver specific. Must begin with 1.0
    // as per https://api.libreoffice.org/docs/common/ref/com/sun/star/sdbc/XDriver.html
    return 1;
}

sal_Int32 SAL_CALL FirebirdDriver::getMinorVersion(  )
{
    return 0;
}

//----- XDataDefinitionSupplier
uno::Reference< XTablesSupplier > SAL_CALL FirebirdDriver::getDataDefinitionByConnection(
                                    const uno::Reference< XConnection >& rConnection)
{
    if (Connection* pConnection = comphelper::getFromUnoTunnel<Connection>(rConnection))
        return pConnection->createCatalog();
    return {};
}

uno::Reference< XTablesSupplier > SAL_CALL FirebirdDriver::getDataDefinitionByURL(
                    const OUString& rURL,
                    const uno::Sequence< PropertyValue >& rInfo)
{
    uno::Reference< XConnection > xConnection = connect(rURL, rInfo);
    return getDataDefinitionByConnection(xConnection);
}

namespace connectivity::firebird
{
        void checkDisposed(bool _bThrow)
        {
            if (_bThrow)
                throw DisposedException();

        }

} // namespace

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
connectivity_FirebirdDriver_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    try {
        return cppu::acquire(new FirebirdDriver(context));
    } catch (...) {
        return nullptr;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

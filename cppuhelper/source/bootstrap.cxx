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

#include <sal/config.h>

#include <chrono>
#include <cstring>
#include <thread>

#include <rtl/bootstrap.hxx>
#include <rtl/random.h>
#include <rtl/ustrbuf.hxx>
#include <rtl/uri.hxx>
#include <osl/file.hxx>
#include <osl/security.hxx>
#include <osl/thread.hxx>
#include <o3tl/char16_t2wchar_t.hxx>
#include <osl/process.h>

#include <cppuhelper/bootstrap.hxx>
#include <cppuhelper/findsofficepath.h>

#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <com/sun/star/bridge/XUnoUrlResolver.hpp>

#include "macro_expander.hxx"

namespace com :: sun :: star :: uno { class XComponentContext; }

using namespace ::osl;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

using rtl::Bootstrap;

namespace cppu
{

BootstrapException::BootstrapException()
{
}

BootstrapException::BootstrapException( const OUString & rMessage )
    :m_aMessage( rMessage )
{
}

BootstrapException::BootstrapException( const BootstrapException & e )
{
    m_aMessage = e.m_aMessage;
}

BootstrapException::~BootstrapException()
{
}

BootstrapException & BootstrapException::operator=( const BootstrapException & e )
{
    m_aMessage = e.m_aMessage;
    return *this;
}

const OUString & BootstrapException::getMessage() const
{
    return m_aMessage;
}

Reference< XComponentContext > SAL_CALL bootstrap()
{
    Reference< XComponentContext > xRemoteContext;

    try
    {
        auto* p1 = cppuhelper_detail_findSofficePath();
        if (p1 == nullptr) {
            throw BootstrapException(
                u"no soffice installation found!"_ustr);
        }
        OUString p2;
#if defined(_WIN32)
        p2 = o3tl::toU(p1);
        free(p1);
#else
        bool bOk = rtl_convertStringToUString(
                &p2.pData, p1, std::strlen(p1), osl_getThreadTextEncoding(),
                (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR |
                 RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR |
                 RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR));
        free(p1);
        if (!bOk)
        {
            throw BootstrapException(
                u"bad characters in soffice installation path!"_ustr);
        }
#endif
        OUString path;
        if (osl::FileBase::getFileURLFromSystemPath(p2, path) !=
            osl::FileBase::E_None)
        {
            throw BootstrapException(
                u"cannot convert soffice installation path to URL!"_ustr);
        }
        if (!path.isEmpty() && !path.endsWith("/")) {
            path += "/";
        }

        OUString uri;
        if (!Bootstrap::get(u"URE_BOOTSTRAP"_ustr, uri)) {
            Bootstrap::set(
                u"URE_BOOTSTRAP"_ustr,
                Bootstrap::encode(
                    path +
#if defined MACOSX
                    "../Resources/"
#endif
                    SAL_CONFIGFILE("fundamental")));
        }

        // create default local component context
        Reference< XComponentContext > xLocalContext(
            defaultBootstrap_InitialComponentContext() );
        if ( !xLocalContext.is() )
            throw BootstrapException( u"no local component context!"_ustr );

        // create a random pipe name
        sal_uInt8 bytes[ 16 ];
        if ( rtl_random_getBytes( nullptr, bytes, std::size( bytes ) )
            != rtl_Random_E_None )
            throw BootstrapException( u"random pool error!"_ustr );
        OUStringBuffer buf("uno");
        for (unsigned char byte : bytes)
            buf.append( static_cast< sal_Int32 >( byte ) );
        OUString sPipeName( buf.makeStringAndClear() );

        // arguments
        OUString args [] = {
            u"--nologo"_ustr,
            u"--nodefault"_ustr,
            u"--norestore"_ustr,
            u"--nolockcheck"_ustr,
            OUString("--accept=pipe,name=" + sPipeName + ";urp;")
        };
        rtl_uString * ar_args [] = {
            args[ 0 ].pData,
            args[ 1 ].pData,
            args[ 2 ].pData,
            args[ 3 ].pData,
            args[ 4 ].pData
        };
        ::osl::Security sec;

        // start office process
        oslProcess hProcess = nullptr;
        oslProcessError rc = osl_executeProcess(
            OUString(path + "soffice").pData, ar_args, std::size( ar_args ),
            osl_Process_DETACHED,
            sec.getHandle(),
            nullptr, // => current working dir
            nullptr, 0, // => no env vars
            &hProcess );
        switch ( rc )
        {
            case osl_Process_E_None:
                osl_freeProcessHandle( hProcess );
                break;
            case osl_Process_E_NotFound:
                throw BootstrapException( u"image not found!"_ustr );
            case osl_Process_E_TimedOut:
                throw BootstrapException( u"timeout occurred!"_ustr );
            case osl_Process_E_NoPermission:
                throw BootstrapException( u"permission denied!"_ustr );
            case osl_Process_E_Unknown:
                throw BootstrapException( u"unknown error!"_ustr );
            case osl_Process_E_InvalidError:
            default:
                throw BootstrapException( u"unmapped error!"_ustr );
        }

        // create a URL resolver
        Reference< bridge::XUnoUrlResolver > xUrlResolver(
            bridge::UnoUrlResolver::create( xLocalContext ) );

        // connection string
        OUString sConnectString( "uno:pipe,name=" + sPipeName + ";urp;StarOffice.ComponentContext" );

        // wait until office is started
        for ( ; ; )
        {
            try
            {
                // try to connect to office
                xRemoteContext.set(
                    xUrlResolver->resolve( sConnectString ), UNO_QUERY_THROW );
                break;
            }
            catch ( connection::NoConnectException & )
            {
                // wait 500 ms, then try to connect again
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    }
    catch ( Exception & e )
    {
        throw BootstrapException(
            "unexpected UNO exception caught: " + e.Message );
    }

    return xRemoteContext;
}

OUString bootstrap_expandUri(OUString const & uri) {
    OUString rest;
    return uri.startsWith("vnd.sun.star.expand:", &rest)
        ? cppuhelper::detail::expandMacros(
            rtl::Uri::decode(
                rest, rtl_UriDecodeWithCharset, RTL_TEXTENCODING_UTF8))
        : uri;
}

} // namespace cppu

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

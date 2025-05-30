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

#include <config_folders.h>

#include <sys/stat.h>
#include <limits.h>
#include <osl/file.hxx>
#include <osl/process.h>
#include <osl/thread.h>
#include <rtl/bootstrap.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <tools/urlobj.hxx>
#include <unx/helper.hxx>

#include <tuple>

using ::rtl::Bootstrap;

namespace psp {

OUString getOfficePath( whichOfficePath ePath )
{
    static const auto aPaths = [] {
        OUString sRoot, sUser, sConfig;
        Bootstrap::get(u"BRAND_BASE_DIR"_ustr, sRoot);
        Bootstrap aBootstrap(sRoot + "/" LIBO_ETC_FOLDER "/" SAL_CONFIGFILE("bootstrap"));
        aBootstrap.getFrom(u"UserInstallation"_ustr, sUser);
        aBootstrap.getFrom(u"CustomDataUrl"_ustr, sConfig);
        OUString aUPath = sUser + "/user/psprint";
        if (sRoot.startsWith("file://"))
        {
            OUString aSysPath;
            if (osl::FileBase::getSystemPathFromFileURL(sRoot, aSysPath) == osl::FileBase::E_None)
                sRoot = aSysPath;
        }
        if (sUser.startsWith("file://"))
        {
            OUString aSysPath;
            if (osl::FileBase::getSystemPathFromFileURL(sUser, aSysPath) == osl::FileBase::E_None)
                sUser = aSysPath;
        }
        if (sConfig.startsWith("file://"))
        {
            OUString aSysPath;
            if (osl::FileBase::getSystemPathFromFileURL(sConfig, aSysPath) == osl::FileBase::E_None)
                sConfig = aSysPath;
        }

        // ensure user path exists
        SAL_INFO("vcl.fonts", "Trying to create: " << aUPath);
        osl::Directory::createPath(aUPath);

        return std::make_tuple(sRoot, sUser, sConfig);
    }();
    const auto& [aInstallationRootPath, aUserPath, aConfigPath] = aPaths;

    switch( ePath )
    {
        case whichOfficePath::ConfigPath: return aConfigPath;
        case whichOfficePath::InstallationRootPath: return aInstallationRootPath;
        case whichOfficePath::UserPath: return aUserPath;
    }
    return OUString();
}

static OString getEnvironmentPath( const char* pKey )
{
    OString aPath;

    const char* pValue = getenv( pKey );
    if( pValue && *pValue )
    {
        aPath = OString( pValue );
    }
    return aPath;
}

} // namespace psp

void psp::getPrinterPathList( std::vector< OUString >& rPathList, const char* pSubDir )
{
    rPathList.clear();
    rtl_TextEncoding aEncoding = osl_getThreadTextEncoding();

    OUStringBuffer aPathBuffer( 256 );

    // append net path
    aPathBuffer.append( getOfficePath( whichOfficePath::InstallationRootPath ) );
    if( !aPathBuffer.isEmpty() )
    {
        aPathBuffer.append( "/" LIBO_SHARE_FOLDER "/psprint" );
        if( pSubDir )
        {
            aPathBuffer.append( '/' );
            aPathBuffer.appendAscii( pSubDir );
        }
        rPathList.push_back( aPathBuffer.makeStringAndClear() );
    }
    // append user path
    aPathBuffer.append( getOfficePath( whichOfficePath::UserPath ) );
    if( !aPathBuffer.isEmpty() )
    {
        aPathBuffer.append( "/user/psprint" );
        if( pSubDir )
        {
            aPathBuffer.append( '/' );
            aPathBuffer.appendAscii( pSubDir );
        }
        rPathList.push_back( aPathBuffer.makeStringAndClear() );
    }

    OString aPath( getEnvironmentPath("SAL_PSPRINT") );
    sal_Int32 nIndex = 0;
    do
    {
        OString aDir( aPath.getToken( 0, ':', nIndex ) );
        if( aDir.isEmpty() )
            continue;

        if( pSubDir )
        {
            aDir += OString::Concat("/") + pSubDir;
        }
        struct stat aStat;
        if( stat( aDir.getStr(), &aStat ) || ! S_ISDIR( aStat.st_mode ) )
            continue;

        rPathList.push_back( OStringToOUString( aDir, aEncoding ) );
    } while( nIndex != -1 );

    #ifdef SYSTEM_PPD_DIR
    if( pSubDir && rtl_str_compare( pSubDir, PRINTER_PPDDIR ) == 0 )
    {
        rPathList.push_back( OStringToOUString( OString( SYSTEM_PPD_DIR ), RTL_TEXTENCODING_UTF8 ) );
    }
    #endif

    if( !rPathList.empty() )
        return;

    // last resort: next to program file (mainly for setup)
    OUString aExe;
    if( osl_getExecutableFile( &aExe.pData ) == osl_Process_E_None )
    {
        INetURLObject aDir( aExe );
        aDir.removeSegment();
        aExe = aDir.GetMainURL( INetURLObject::DecodeMechanism::NONE );
        OUString aSysPath;
        if( osl_getSystemPathFromFileURL( aExe.pData, &aSysPath.pData ) == osl_File_E_None )
        {
            rPathList.push_back( aSysPath );
        }
    }
}

OUString const & psp::getFontPath()
{
    static OUString aPath;

    if (aPath.isEmpty())
    {
        OUStringBuffer aPathBuffer( 512 );

        OUString aConfigPath( getOfficePath( whichOfficePath::ConfigPath ) );
        OUString aInstallationRootPath( getOfficePath( whichOfficePath::InstallationRootPath ) );
        OUString aUserPath( getOfficePath( whichOfficePath::UserPath ) );
        if (!aInstallationRootPath.isEmpty())
        {
            // internal font resources, required for normal operation, like OpenSymbol
            aPathBuffer.append(aInstallationRootPath
                               + "/" LIBO_SHARE_RESOURCE_FOLDER "/common/fonts;");
        }
        if( !aConfigPath.isEmpty() )
        {
            // #i53530# Path from CustomDataUrl will completely
            // replace net share and user paths if the path exists
            OUString sPath = aConfigPath + "/" LIBO_SHARE_FOLDER "/fonts";
            // check existence of config path
            struct stat aStat;
            if( 0 != stat( OUStringToOString( sPath, osl_getThreadTextEncoding() ).getStr(), &aStat )
                || ! S_ISDIR( aStat.st_mode ) )
                aConfigPath.clear();
            else
            {
                aPathBuffer.append(sPath);
            }
        }
        if( aConfigPath.isEmpty() )
        {
            if( !aInstallationRootPath.isEmpty() )
            {
                aPathBuffer.append( aInstallationRootPath
                    + "/" LIBO_SHARE_FOLDER "/fonts/truetype;");
            }
            if( !aUserPath.isEmpty() )
            {
                aPathBuffer.append( aUserPath + "/user/fonts" );
            }
        }

        aPath = aPathBuffer.makeStringAndClear();
        SAL_INFO("vcl.fonts", "Initializing font path to: " << aPath);
    }
    return aPath;
}

void psp::normPath( OString& rPath )
{
    char buf[PATH_MAX];

    // double slashes and slash at end are probably
    // removed by realpath anyway, but since this runs
    // on many different platforms let's play it safe
    OString aPath = rPath.replaceAll("//"_ostr, "/"_ostr);

    if( aPath.endsWith("/") )
        aPath = aPath.copy(0, aPath.getLength()-1);

    if( ( aPath.indexOf("./") != -1 ||
          aPath.indexOf( '~' ) != -1 )
        && realpath( aPath.getStr(), buf ) )
    {
        rPath = buf;
    }
    else
    {
        rPath = aPath;
    }
}

void psp::splitPath( OString& rPath, OString& rDir, OString& rBase )
{
    normPath( rPath );
    sal_Int32 nIndex = rPath.lastIndexOf( '/' );
    if( nIndex > 0 )
        rDir = rPath.copy( 0, nIndex );
    else if( nIndex == 0 ) // root dir
        rDir = rPath.copy( 0, 1 );
    if( rPath.getLength() > nIndex+1 )
        rBase = rPath.copy( nIndex+1 );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

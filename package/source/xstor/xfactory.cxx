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
#include <sal/log.hxx>

#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/StorageFormats.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/io/XSeekable.hpp>

#include <comphelper/propertyvalue.hxx>
#include <comphelper/storagehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <osl/diagnose.h>
#include <unotools/tempfile.hxx>

#include "xfactory.hxx"
#include "xstorage.hxx"

using namespace ::com::sun::star;

#if OSL_DEBUG_LEVEL > 0
#define THROW_WHERE SAL_WHERE
#else
#define THROW_WHERE ""
#endif

static bool CheckPackageSignature_Impl( const uno::Reference< io::XInputStream >& xInputStream,
                                     const uno::Reference< io::XSeekable >& xSeekable )
{
    if ( !xInputStream.is() || !xSeekable.is() )
        throw uno::RuntimeException();

    if ( xSeekable->getLength() )
    {
        uno::Sequence< sal_Int8 > aData( 4 );
        xSeekable->seek( 0 );
        sal_Int32 nRead = xInputStream->readBytes( aData, 4 );
        xSeekable->seek( 0 );

        // TODO/LATER: should the disk spanned files be supported?
        // 0x50, 0x4b, 0x07, 0x08
        return ( nRead == 4 && aData[0] == 0x50 && aData[1] == 0x4b && aData[2] == 0x03 && aData[3] == 0x04 );
    }
    else
        return true; // allow to create a storage based on empty stream
}



uno::Reference< uno::XInterface > SAL_CALL OStorageFactory::createInstance()
{
    // TODO: reimplement TempStream service to support XStream interface
    uno::Reference < io::XStream > xTempStream(new utl::TempFileFastService);

    return cppu::getXWeak(new OStorage(xTempStream, embed::ElementModes::READWRITE,
                                                  uno::Sequence<beans::PropertyValue>(), m_xContext,
                                                  embed::StorageFormats::PACKAGE));
}

uno::Reference< uno::XInterface > SAL_CALL OStorageFactory::createInstanceWithArguments(
            const uno::Sequence< uno::Any >& aArguments )
{
    // The request for storage can be done with up to three arguments

    // The first argument specifies a source for the storage
    // it can be URL, XStream, XInputStream.
    // The second value is a mode the storage should be open in.
    // And the third value is a media descriptor.

    sal_Int32 nArgNum = aArguments.getLength();
    OSL_ENSURE( nArgNum < 4, "Wrong parameter number" );

    if ( !nArgNum )
        return createInstance();

    // first try to retrieve storage open mode if any
    // by default the storage will be open in readonly mode
    sal_Int32 nStorageMode = embed::ElementModes::READ;
    if ( nArgNum >= 2 )
    {
        if( !( aArguments[1] >>= nStorageMode ) )
        {
            OSL_FAIL( "Wrong second argument!" );
            throw lang::IllegalArgumentException(); // TODO:
        }
        // it's always possible to read written storage in this implementation
        nStorageMode |= embed::ElementModes::READ;
    }

    if ( ( nStorageMode & embed::ElementModes::TRUNCATE ) == embed::ElementModes::TRUNCATE
      && ( nStorageMode & embed::ElementModes::WRITE ) != embed::ElementModes::WRITE )
        throw lang::IllegalArgumentException(); // TODO:

    // retrieve storage source stream
    OUString aURL;
    uno::Reference< io::XStream > xStream;
    uno::Reference< io::XInputStream > xInputStream;

    if ( aArguments[0] >>= aURL )
    {
        if ( aURL.isEmpty() )
        {
            OSL_FAIL( "Empty URL is provided!" );
            throw lang::IllegalArgumentException(); // TODO:
        }

        if ( aURL.startsWithIgnoreAsciiCase("vnd.sun.star.pkg:") )
        {
            OSL_FAIL( "Packages URL's are not valid for storages!" ); // ???
            throw lang::IllegalArgumentException(); // TODO:
        }

        uno::Reference < ucb::XSimpleFileAccess3 > xTempAccess(
            ucb::SimpleFileAccess::create(
                m_xContext ) );

        if ( nStorageMode & embed::ElementModes::WRITE )
            xStream = xTempAccess->openFileReadWrite( aURL );
        else
            xInputStream = xTempAccess->openFileRead( aURL );
    }
    else if ( !( aArguments[0] >>= xStream ) && !( aArguments[0] >>= xInputStream ) )
    {
        OSL_FAIL( "Wrong first argument!" );
        throw uno::Exception(u"wrong first arg"_ustr, nullptr); // TODO: Illegal argument
    }

    // retrieve mediadescriptor and set storage properties
    uno::Sequence< beans::PropertyValue > aDescr;
    uno::Sequence< beans::PropertyValue > aPropsToSet;

    sal_Int32 nStorageType = embed::StorageFormats::PACKAGE;

    if ( nArgNum >= 3 )
    {
        if( aArguments[2] >>= aDescr )
        {
            if ( !aURL.isEmpty() )
            {
                aPropsToSet = { comphelper::makePropertyValue(u"URL"_ustr, aURL) };
            }

            sal_Int32 nNumArgs = 1;
            for (const auto& rProp : aDescr)
            {
                if ( rProp.Name == "InteractionHandler"
                  || rProp.Name == "Password"
                  || rProp.Name == "RepairPackage"
                  || rProp.Name == "StatusIndicator" )
                {
                    aPropsToSet.realloc( ++nNumArgs );
                    auto pPropsToSet = aPropsToSet.getArray();
                    pPropsToSet[nNumArgs-1].Name = rProp.Name;
                    pPropsToSet[nNumArgs-1].Value = rProp.Value;
                }
                else if ( rProp.Name == "StorageFormat" )
                {
                    OUString aFormatName;
                    sal_Int32 nFormatID = 0;
                    if ( rProp.Value >>= aFormatName )
                    {
                        if ( aFormatName == PACKAGE_STORAGE_FORMAT_STRING )
                            nStorageType = embed::StorageFormats::PACKAGE;
                        else if ( aFormatName == ZIP_STORAGE_FORMAT_STRING )
                            nStorageType = embed::StorageFormats::ZIP;
                        else if ( aFormatName == OFOPXML_STORAGE_FORMAT_STRING )
                            nStorageType = embed::StorageFormats::OFOPXML;
                        else
                            throw lang::IllegalArgumentException( THROW_WHERE, uno::Reference< uno::XInterface >(), 1 );
                    }
                    else if ( rProp.Value >>= nFormatID )
                    {
                        if ( nFormatID != embed::StorageFormats::PACKAGE
                          && nFormatID != embed::StorageFormats::ZIP
                          && nFormatID != embed::StorageFormats::OFOPXML )
                            throw lang::IllegalArgumentException( THROW_WHERE, uno::Reference< uno::XInterface >(), 1 );

                        nStorageType = nFormatID;
                    }
                    else
                        throw lang::IllegalArgumentException( THROW_WHERE, uno::Reference< uno::XInterface >(), 1 );
                }
                else if (rProp.Name == "NoFileSync")
                {
                    // Forward NoFileSync to the storage.
                    aPropsToSet.realloc(++nNumArgs);
                    auto pPropsToSet = aPropsToSet.getArray();
                    pPropsToSet[nNumArgs - 1].Name = rProp.Name;
                    pPropsToSet[nNumArgs - 1].Value = rProp.Value;
                }
                else
                    OSL_FAIL( "Unacceptable property, will be ignored!" );
            }
        }
        else
        {
            OSL_FAIL( "Wrong third argument!" );
            throw uno::Exception(u"wrong 3rd arg"_ustr, nullptr); // TODO: Illegal argument
        }

    }

    // create storage based on source
    if ( xInputStream.is() )
    {
        // if xInputStream is set the storage should be open from it
        if ( nStorageMode & embed::ElementModes::WRITE )
              throw uno::Exception(u"storagemode==write"_ustr, nullptr); // TODO: access denied

        uno::Reference< io::XSeekable > xSeekable( xInputStream, uno::UNO_QUERY );
        if ( !xSeekable.is() )
        {
            // TODO: wrap stream to let it be seekable
            OSL_FAIL( "Nonseekable streams are not supported for now!" );
        }

        if ( !CheckPackageSignature_Impl( xInputStream, xSeekable ) )
            throw io::IOException(u"package signature check failed, probably not a package file"_ustr, nullptr); // TODO: this is not a package file

        return cppu::getXWeak(
            new OStorage(xInputStream, nStorageMode, aPropsToSet, m_xContext, nStorageType));
    }
    else if ( xStream.is() )
    {
        if ( ( ( nStorageMode & embed::ElementModes::WRITE ) && !xStream->getOutputStream().is() )
             || !xStream->getInputStream().is() )
              throw uno::Exception(u"access denied"_ustr, nullptr); // TODO: access denied

        uno::Reference< io::XSeekable > xSeekable( xStream, uno::UNO_QUERY );
        if ( !xSeekable.is() )
        {
            // TODO: wrap stream to let it be seekable
            OSL_FAIL( "Nonseekable streams are not supported for now!" );
        }

        if ( !CheckPackageSignature_Impl( xStream->getInputStream(), xSeekable ) )
            throw io::IOException(); // TODO: this is not a package file

        return cppu::getXWeak(
            new OStorage(xStream, nStorageMode, aPropsToSet, m_xContext, nStorageType));
    }

    throw uno::Exception(u"no input stream or regular stream"_ustr, nullptr); // general error during creation
}

OUString SAL_CALL OStorageFactory::getImplementationName()
{
    return u"com.sun.star.comp.embed.StorageFactory"_ustr;
}

sal_Bool SAL_CALL OStorageFactory::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence< OUString > SAL_CALL OStorageFactory::getSupportedServiceNames()
{
    return  { u"com.sun.star.embed.StorageFactory"_ustr,
                u"com.sun.star.comp.embed.StorageFactory"_ustr };
}


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
package_OStorageFactory_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new OStorageFactory(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

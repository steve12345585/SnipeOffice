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

#include <osl/security.hxx>
#include <osl/file.hxx>
#include <osl/socket.h>
#include <cppuhelper/queryinterface.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/ucb/FileSystemNotation.hpp>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <cppuhelper/supportsservice.hxx>
#include "filglob.hxx"
#include "filid.hxx"
#include "filtask.hxx"
#include "bc.hxx"
#include "prov.hxx"

using namespace fileaccess;
using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::ucb;

#if OSL_DEBUG_LEVEL > 0
#define THROW_WHERE SAL_WHERE
#else
#define THROW_WHERE ""
#endif


/****************************************************************************/
/*                                                                          */
/*                                                                          */
/*                        FileProvider                                      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
FileProvider::FileProvider( const Reference< XComponentContext >& rxContext )
    : m_xContext(rxContext)
    , m_FileSystemNotation(FileSystemNotation::UNKNOWN_NOTATION)
{
}

FileProvider::~FileProvider()
{
}

// XInitialization
void FileProvider::init()
{
    if( ! m_pMyShell )
        m_pMyShell.reset( new TaskManager( m_xContext, this, true ) );
}


void SAL_CALL
FileProvider::initialize(
    const Sequence< Any >& aArguments )
{
    if( ! m_pMyShell ) {
        OUString config;
        if( aArguments.hasElements() &&
            (aArguments[0] >>= config) &&
            config == "NoConfig" )
            m_pMyShell.reset( new TaskManager( m_xContext, this, false ) );
        else
            m_pMyShell.reset( new TaskManager( m_xContext, this, true ) );
    }
}

// XServiceInfo methods.
OUString SAL_CALL
FileProvider::getImplementationName()
{
    return u"com.sun.star.comp.ucb.FileProvider"_ustr;
}

sal_Bool SAL_CALL FileProvider::supportsService(const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL
FileProvider::getSupportedServiceNames()
{
    return { u"com.sun.star.ucb.FileContentProvider"_ustr };
}

// XContent

Reference< XContent > SAL_CALL
FileProvider::queryContent(
    const Reference< XContentIdentifier >& xIdentifier )
{
    init();
    OUString aUnc;
    bool err = fileaccess::TaskManager::getUnqFromUrl( xIdentifier->getContentIdentifier(),
                                              aUnc );

    if(  err )
    {
        throw IllegalIdentifierException( THROW_WHERE );
    }

    return Reference<XContent>(new BaseContent(m_pMyShell.get(), xIdentifier, aUnc));
}

sal_Int32 SAL_CALL
FileProvider::compareContentIds(
                const Reference< XContentIdentifier >& Id1,
                const Reference< XContentIdentifier >& Id2 )
{
    init();
    OUString aUrl1 = Id1->getContentIdentifier();
    OUString aUrl2 = Id2->getContentIdentifier();

    sal_Int32   iComp = aUrl1.compareTo( aUrl2 );

    if ( 0 != iComp )
    {
        OUString aPath1, aPath2;

        fileaccess::TaskManager::getUnqFromUrl( aUrl1, aPath1 );
        fileaccess::TaskManager::getUnqFromUrl( aUrl2, aPath2 );

        osl::FileBase::RC   error;
        osl::DirectoryItem  aItem1, aItem2;

        error = osl::DirectoryItem::get( aPath1, aItem1 );
        if ( error == osl::FileBase::E_None )
            error = osl::DirectoryItem::get( aPath2, aItem2 );

        if ( error != osl::FileBase::E_None )
            return iComp;

        osl::FileStatus aStatus1( osl_FileStatus_Mask_FileURL );
        osl::FileStatus aStatus2( osl_FileStatus_Mask_FileURL );
        error = aItem1.getFileStatus( aStatus1 );
        if ( error == osl::FileBase::E_None )
            error = aItem2.getFileStatus( aStatus2 );

        if ( error == osl::FileBase::E_None )
        {
            iComp = aStatus1.getFileURL().compareTo( aStatus2.getFileURL() );

// Quick hack for Windows to threat all file systems as case insensitive
#ifdef _WIN32
            if ( 0 != iComp )
            {
                error = osl::FileBase::getSystemPathFromFileURL( aStatus1.getFileURL(), aPath1 );
                if ( error == osl::FileBase::E_None )
                    error = osl::FileBase::getSystemPathFromFileURL( aStatus2.getFileURL(), aPath2 );

                if ( error == osl::FileBase::E_None )
                    iComp = aPath1.compareToIgnoreAsciiCase( aPath2 );
            }
#endif
        }
    }

    return iComp;
}


Reference< XContentIdentifier > SAL_CALL
FileProvider::createContentIdentifier(
                      const OUString& ContentId )
{
    init();
    return new FileContentIdentifier( ContentId,false );
}


//XPropertySetInfoImpl

namespace fileaccess {

class XPropertySetInfoImpl2
    : public cppu::OWeakObject,
      public XPropertySetInfo
{
public:
    XPropertySetInfoImpl2();

    // XInterface
    virtual Any SAL_CALL
    queryInterface( const Type& aType ) override;

    virtual void SAL_CALL
    acquire()
        noexcept override;

    virtual void SAL_CALL
    release()
        noexcept override;


    virtual Sequence< Property > SAL_CALL
    getProperties() override;

    virtual Property SAL_CALL
    getPropertyByName( const OUString& aName ) override;

    virtual sal_Bool SAL_CALL
    hasPropertyByName( const OUString& Name ) override;


private:
    Sequence< Property > m_seq;
};

}

XPropertySetInfoImpl2::XPropertySetInfoImpl2()
    : m_seq{ Property( u"HostName"_ustr,
                         -1,
                         cppu::UnoType<OUString>::get(),
                         PropertyAttribute::READONLY ),
             Property( u"HomeDirectory"_ustr,
                         -1,
                         cppu::UnoType<OUString>::get(),
                         PropertyAttribute::READONLY ),
             Property( u"FileSystemNotation"_ustr,
                         -1,
                         cppu::UnoType<sal_Int32>::get(),
                         PropertyAttribute::READONLY )}
{
}

void SAL_CALL
XPropertySetInfoImpl2::acquire()
    noexcept
{
    OWeakObject::acquire();
}


void SAL_CALL
XPropertySetInfoImpl2::release()
    noexcept
{
    OWeakObject::release();
}


Any SAL_CALL
XPropertySetInfoImpl2::queryInterface( const Type& rType )
{
    Any aRet = cppu::queryInterface( rType,
                                     static_cast< XPropertySetInfo* >(this) );
    return aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType );
}


Property SAL_CALL
XPropertySetInfoImpl2::getPropertyByName( const OUString& aName )
{
    auto pProp = std::find_if(std::cbegin(m_seq), std::cend(m_seq),
            [&aName](const Property& rProp) { return rProp.Name == aName; });
    if (pProp != std::cend(m_seq))
        return *pProp;

    throw UnknownPropertyException( aName );
}


Sequence< Property > SAL_CALL
XPropertySetInfoImpl2::getProperties()
{
    return m_seq;
}


sal_Bool SAL_CALL
XPropertySetInfoImpl2::hasPropertyByName(
    const OUString& aName )
{
    return std::any_of(std::cbegin(m_seq), std::cend(m_seq),
        [&aName](const Property& rProp) { return rProp.Name == aName; });
}


void FileProvider::initProperties(std::unique_lock<std::mutex>& /*rGuard*/)
{
    if(  m_xPropertySetInfo.is() )
        return;

    osl_getLocalHostname( &m_HostName.pData );

#if defined ( UNX )
    m_FileSystemNotation = FileSystemNotation::UNIX_NOTATION;
#elif defined( _WIN32 )
    m_FileSystemNotation = FileSystemNotation::DOS_NOTATION;
#else
    m_FileSystemNotation = FileSystemNotation::UNKNOWN_NOTATION;
#endif
    osl::Security aSecurity;
    aSecurity.getHomeDir( m_HomeDirectory );

    // static const sal_Int32 UNKNOWN_NOTATION = (sal_Int32)0;
    // static const sal_Int32 UNIX_NOTATION = (sal_Int32)1;
    // static const sal_Int32 DOS_NOTATION = (sal_Int32)2;
    // static const sal_Int32 MAC_NOTATION = (sal_Int32)3;

    m_xPropertySetInfo = new XPropertySetInfoImpl2();
}


// XPropertySet

Reference< XPropertySetInfo > SAL_CALL
FileProvider::getPropertySetInfo(  )
{
    std::unique_lock aGuard( m_aMutex );
    initProperties(aGuard);
    return m_xPropertySetInfo;
}


void SAL_CALL
FileProvider::setPropertyValue( const OUString& aPropertyName,
                                const Any& )
{
    if( !(aPropertyName == "FileSystemNotation" ||
        aPropertyName == "HomeDirectory"      ||
        aPropertyName == "HostName") )
        throw UnknownPropertyException( aPropertyName );
}


Any SAL_CALL
FileProvider::getPropertyValue(
    const OUString& aPropertyName )
{
    std::unique_lock aGuard( m_aMutex );
    initProperties(aGuard);
    if( aPropertyName == "FileSystemNotation" )
    {
        return Any(m_FileSystemNotation);
    }
    else if( aPropertyName == "HomeDirectory" )
    {
        return Any(m_HomeDirectory);
    }
    else if( aPropertyName == "HostName" )
    {
        return Any(m_HostName);
    }
    else
        throw UnknownPropertyException( aPropertyName );
}


void SAL_CALL
FileProvider::addPropertyChangeListener(
    const OUString&,
    const Reference< XPropertyChangeListener >& )
{
}


void SAL_CALL
FileProvider::removePropertyChangeListener(
    const OUString&,
    const Reference< XPropertyChangeListener >& )
{
}

void SAL_CALL
FileProvider::addVetoableChangeListener(
    const OUString&,
    const Reference< XVetoableChangeListener >& )
{
}


void SAL_CALL
FileProvider::removeVetoableChangeListener(
    const OUString&,
    const Reference< XVetoableChangeListener >& )
{
}


// XFileIdentifierConverter

sal_Int32 SAL_CALL
FileProvider::getFileProviderLocality( const OUString& BaseURL )
{
    // If the base URL is a 'file' URL, return 10 (very 'local'), otherwise
    // return -1 (mismatch).  What is missing is a fast comparison to ASCII,
    // ignoring case:
    return BaseURL.getLength() >= 5
           && (BaseURL[0] == 'F' || BaseURL[0] == 'f')
           && (BaseURL[1] == 'I' || BaseURL[1] == 'i')
           && (BaseURL[2] == 'L' || BaseURL[2] == 'l')
           && (BaseURL[3] == 'E' || BaseURL[3] == 'e')
           && BaseURL[4] == ':' ?
               10 : -1;
}

OUString SAL_CALL FileProvider::getFileURLFromSystemPath( const OUString&,
                                                               const OUString& SystemPath )
{
    OUString aNormalizedPath;
    if ( osl::FileBase::getFileURLFromSystemPath( SystemPath,aNormalizedPath ) != osl::FileBase::E_None )
        return OUString();

    return aNormalizedPath;
}

OUString SAL_CALL FileProvider::getSystemPathFromFileURL( const OUString& URL )
{
    OUString aSystemPath;
    if (osl::FileBase::getSystemPathFromFileURL( URL,aSystemPath ) != osl::FileBase::E_None )
        return OUString();

    return aSystemPath;
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
ucb_file_FileProvider_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new FileProvider(context));
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

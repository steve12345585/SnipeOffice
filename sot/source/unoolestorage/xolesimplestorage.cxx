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

#include "xolesimplestorage.hxx"

#include <com/sun/star/embed/OLESimpleStorage.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/io/TempFile.hpp>

#include <comphelper/storagehelper.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sot/stg.hxx>
#include <sot/storinfo.hxx>
#include <utility>

using namespace ::com::sun::star;

const sal_Int32 nBytesCount = 32000;


OLESimpleStorage::OLESimpleStorage(
        css::uno::Reference<css::uno::XComponentContext> xContext,
        css::uno::Sequence<css::uno::Any> const &aArguments)
: m_bDisposed( false )
, m_xContext(std::move( xContext ))
, m_bNoTemporaryCopy( false )
{
    sal_Int32 nArgNum = aArguments.getLength();
    if ( nArgNum < 1 || nArgNum > 2 )
        throw lang::IllegalArgumentException(); // TODO:

    uno::Reference< io::XStream > xStream;
    uno::Reference< io::XInputStream > xInputStream;
    if ( !( aArguments[0] >>= xStream ) && !( aArguments[0] >>= xInputStream ) )
        throw lang::IllegalArgumentException(); // TODO:

    if ( nArgNum == 2 )
    {
        if ( !( aArguments[1] >>= m_bNoTemporaryCopy ) )
            throw lang::IllegalArgumentException(); // TODO:
    }

    if ( m_bNoTemporaryCopy )
    {
        // TODO: ???
        // If the temporary stream is not created, the original stream must be wrapped
        // since SvStream wrapper closes the stream is owns
        if ( xInputStream.is() )
        {
            // the stream must be seekable for direct access
            uno::Reference< io::XSeekable > xSeek( xInputStream, uno::UNO_QUERY_THROW );
            m_pStream = ::utl::UcbStreamHelper::CreateStream( xInputStream, false );
        }
        else if ( xStream.is() )
        {
            // the stream must be seekable for direct access
            uno::Reference< io::XSeekable > xSeek( xStream, uno::UNO_QUERY_THROW );
            m_pStream = ::utl::UcbStreamHelper::CreateStream( xStream, false );
        }
        else
            throw lang::IllegalArgumentException(); // TODO:
    }
    else
    {
        uno::Reference < io::XStream > xTempFile( io::TempFile::create(m_xContext),
                uno::UNO_QUERY_THROW );
        uno::Reference < io::XSeekable > xTempSeek( xTempFile, uno::UNO_QUERY_THROW );
        uno::Reference< io::XOutputStream > xTempOut = xTempFile->getOutputStream();
        if ( !xTempOut.is() )
            throw uno::RuntimeException();

        if ( xInputStream.is() )
        {
            try
            {
                uno::Reference< io::XSeekable > xSeek( xInputStream, uno::UNO_QUERY );
                if (xSeek)
                    xSeek->seek( 0 );
            }
            catch( uno::Exception& )
            {}

            ::comphelper::OStorageHelper::CopyInputToOutput( xInputStream, xTempOut );
            xTempOut->closeOutput();
            xTempSeek->seek( 0 );
            uno::Reference< io::XInputStream > xTempInput = xTempFile->getInputStream();
            m_pStream = ::utl::UcbStreamHelper::CreateStream( xTempInput, false );
        }
        else if ( xStream.is() )
        {
            // not sure that the storage flashes the stream on commit
            m_xStream = xStream;
            m_xTempStream = xTempFile;

            uno::Reference< io::XSeekable > xSeek( xStream, uno::UNO_QUERY_THROW );
            xSeek->seek( 0 );
            uno::Reference< io::XInputStream > xInpStream = xStream->getInputStream();
            if ( !xInpStream.is() || !xStream->getOutputStream().is() )
                throw uno::RuntimeException();

            ::comphelper::OStorageHelper::CopyInputToOutput( xInpStream, xTempOut );
            xTempOut->flush();
            xTempSeek->seek( 0 );

            m_pStream = ::utl::UcbStreamHelper::CreateStream( xTempFile, false );
        }
        else
            throw lang::IllegalArgumentException(); // TODO:
    }

    if ( !m_pStream || m_pStream->GetError() )
        throw io::IOException(); // TODO

    m_pStorage.reset(new Storage( *m_pStream, false ));
}

OLESimpleStorage::~OLESimpleStorage()
{
    try {
        osl_atomic_increment(&m_refCount);
        dispose();
    } catch( uno::Exception& )
    {}
}

void OLESimpleStorage::UpdateOriginal_Impl()
{
    if ( m_bNoTemporaryCopy )
        return;

    uno::Reference< io::XSeekable > xSeek( m_xStream, uno::UNO_QUERY_THROW );
    xSeek->seek( 0 );

    uno::Reference< io::XSeekable > xTempSeek( m_xTempStream, uno::UNO_QUERY_THROW );
    sal_Int64 nPos = xTempSeek->getPosition();
    xTempSeek->seek( 0 );

    uno::Reference< io::XInputStream > xTempInp = m_xTempStream->getInputStream();
    uno::Reference< io::XOutputStream > xOutputStream = m_xStream->getOutputStream();
    if ( !xTempInp.is() || !xOutputStream.is() )
        throw uno::RuntimeException();

    uno::Reference< io::XTruncate > xTrunc( xOutputStream, uno::UNO_QUERY_THROW );
    xTrunc->truncate();

    ::comphelper::OStorageHelper::CopyInputToOutput( xTempInp, xOutputStream );
    xOutputStream->flush();
    xTempSeek->seek( nPos );
}


void OLESimpleStorage::InsertInputStreamToStorage_Impl( BaseStorage* pStorage, const OUString & aName, const uno::Reference< io::XInputStream >& xInputStream )
{
    if ( !pStorage || aName.isEmpty() || !xInputStream.is() )
        throw uno::RuntimeException();

    if ( pStorage->IsContained( aName ) )
        throw container::ElementExistException(); // TODO:

    std::unique_ptr<BaseStorageStream> pNewStream(pStorage->OpenStream( aName ));
    if ( !pNewStream || pNewStream->GetError() || pStorage->GetError() )
    {
        pNewStream.reset();
        pStorage->ResetError();
        throw io::IOException(); // TODO
    }

    try
    {
        uno::Sequence< sal_Int8 > aData( nBytesCount );
        sal_Int32 nRead = 0;
        do
        {
            nRead = xInputStream->readBytes( aData, nBytesCount );

            sal_Int32 nWritten = pNewStream->Write( aData.getConstArray(), nRead );
            if ( nWritten < nRead )
                throw io::IOException();
        } while( nRead == nBytesCount );
    }
    catch( uno::Exception& )
    {
        pNewStream.reset();
        pStorage->Remove( aName );

        throw;
    }
}


void OLESimpleStorage::InsertNameAccessToStorage_Impl( BaseStorage* pStorage, const OUString & aName, const uno::Reference< container::XNameAccess >& xNameAccess )
{
    if ( !pStorage || aName.isEmpty() || !xNameAccess.is() )
        throw uno::RuntimeException();

    if ( pStorage->IsContained( aName ) )
        throw container::ElementExistException(); // TODO:

    std::unique_ptr<BaseStorage> pNewStorage(pStorage->OpenStorage( aName ));
    if ( !pNewStorage || pNewStorage->GetError() || pStorage->GetError() )
    {
        pNewStorage.reset();
        pStorage->ResetError();
        throw io::IOException(); // TODO
    }

    try
    {
        const uno::Sequence< OUString > aElements = xNameAccess->getElementNames();
        for ( const auto& rElement : aElements )
        {
            uno::Reference< io::XInputStream > xInputStream;
            uno::Reference< container::XNameAccess > xSubNameAccess;
            uno::Any aAny = xNameAccess->getByName( rElement );
            if ( aAny >>= xInputStream )
                InsertInputStreamToStorage_Impl( pNewStorage.get(), rElement, xInputStream );
            else if ( aAny >>= xSubNameAccess )
                InsertNameAccessToStorage_Impl( pNewStorage.get(), rElement, xSubNameAccess );
        }
    }
    catch( uno::Exception& )
    {
        pNewStorage.reset();
        pStorage->Remove( aName );

        throw;
    }
}


//  XNameContainer


void SAL_CALL OLESimpleStorage::insertByName( const OUString& aName, const uno::Any& aElement )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    uno::Reference< io::XStream > xStream;
    uno::Reference< io::XInputStream > xInputStream;
    uno::Reference< container::XNameAccess > xNameAccess;

    try
    {
        if ( !m_bNoTemporaryCopy && !m_xStream.is() )
            throw io::IOException(); // TODO

        if ( aElement >>= xStream )
            xInputStream = xStream->getInputStream();
        else if ( !( aElement >>= xInputStream ) && !( aElement >>= xNameAccess ) )
            throw lang::IllegalArgumentException(); // TODO:

        if ( xInputStream.is() )
            InsertInputStreamToStorage_Impl( m_pStorage.get(), aName, xInputStream );
        else if ( xNameAccess.is() )
            InsertNameAccessToStorage_Impl( m_pStorage.get(), aName, xNameAccess );
        else
            throw uno::RuntimeException();
    }
    catch( uno::RuntimeException& )
    {
        throw;
    }
    catch( container::ElementExistException& )
    {
        throw;
    }
    catch( const uno::Exception& )
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetException(u"Insert has failed!"_ustr,
                                            uno::Reference< uno::XInterface >(),
                                            anyEx );
    }
}


void SAL_CALL OLESimpleStorage::removeByName( const OUString& aName )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    if ( !m_bNoTemporaryCopy && !m_xStream.is() )
        throw lang::WrappedTargetException(); // io::IOException(); // TODO

    if ( !m_pStorage->IsContained( aName ) )
        throw container::NoSuchElementException(); // TODO:

    m_pStorage->Remove( aName );

    if ( m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw lang::WrappedTargetException(); // io::IOException(); // TODO
    }
}


void SAL_CALL OLESimpleStorage::replaceByName( const OUString& aName, const uno::Any& aElement )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    removeByName( aName );

    try
    {
        insertByName( aName, aElement );
    }
    catch( container::ElementExistException& )
    {
        uno::Any aCaught( ::cppu::getCaughtException() );

        throw lang::WrappedTargetException(u"Can't copy raw stream"_ustr,
                                            uno::Reference< uno::XInterface >(),
                                            aCaught );
    }
}


uno::Any SAL_CALL OLESimpleStorage::getByName( const OUString& aName )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    if ( !m_pStorage->IsContained( aName ) )
        throw container::NoSuchElementException(); // TODO:

    uno::Any aResult;

    uno::Reference< io::XStream > xTempFile = io::TempFile::create(m_xContext);
    uno::Reference< io::XSeekable > xSeekable( xTempFile, uno::UNO_QUERY_THROW );
    uno::Reference< io::XOutputStream > xOutputStream = xTempFile->getOutputStream();
    uno::Reference< io::XInputStream > xInputStream = xTempFile->getInputStream();
    if ( !xOutputStream.is() || !xInputStream.is() )
        throw uno::RuntimeException();

    if ( m_pStorage->IsStorage( aName ) )
    {
        std::unique_ptr<BaseStorage> pStrg(m_pStorage->OpenStorage( aName ));
        m_pStorage->ResetError();
        if ( !pStrg )
            throw lang::WrappedTargetException(); // io::IOException(); // TODO

        std::unique_ptr<SvStream> pStream = ::utl::UcbStreamHelper::CreateStream( xTempFile, false ); // do not close the original stream
        if ( !pStream )
            throw uno::RuntimeException();

        std::unique_ptr<BaseStorage> pNewStor(new Storage( *pStream, false ));
        bool bSuccess = ( pStrg->CopyTo( *pNewStor ) && pNewStor->Commit() &&
                          !pNewStor->GetError() && !pStrg->GetError() );

        pNewStor.reset();
        pStrg.reset();
        pStream.reset();

        if ( !bSuccess )
            throw uno::RuntimeException();

        uno::Reference< container::XNameContainer > xResultNameContainer(
            css::embed::OLESimpleStorage::createFromInputStream(m_xContext, xInputStream, true),
            uno::UNO_QUERY_THROW );

        aResult <<= xResultNameContainer;
    }
    else
    {
        std::unique_ptr<BaseStorageStream> pStream(m_pStorage->OpenStream( aName, StreamMode::READ | StreamMode::SHARE_DENYALL | StreamMode::NOCREATE ));
        try
        {
            if ( !pStream || pStream->GetError() || m_pStorage->GetError() )
            {
                m_pStorage->ResetError();
                throw io::IOException(); // TODO
            }

            uno::Sequence< sal_Int8 > aData( nBytesCount );
            sal_Int32 nSize = nBytesCount;
            sal_Int32 nRead = 0;
            while( 0 != ( nRead = pStream->Read( aData.getArray(), nSize ) ) )
            {
                if ( nRead < nSize )
                {
                    nSize = nRead;
                    aData.realloc( nSize );
                }

                xOutputStream->writeBytes( aData );
            }

            if ( pStream->GetError() )
                throw io::IOException(); // TODO

            xOutputStream->closeOutput();
            xSeekable->seek( 0 );
        }
        catch (const uno::RuntimeException&)
        {
            throw;
        }
        catch (const uno::Exception& ex)
        {
            css::uno::Any anyEx = cppu::getCaughtException();
            throw css::lang::WrappedTargetException( ex.Message,
                    nullptr, anyEx );
        }

        pStream.reset();

        aResult <<= xInputStream;
    }

    return aResult;
}


uno::Sequence< OUString > SAL_CALL OLESimpleStorage::getElementNames()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    SvStorageInfoList aList;
    m_pStorage->FillInfoList( &aList );

    if ( m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw uno::RuntimeException(); // TODO:
    }

    uno::Sequence< OUString > aSeq( aList.size() );
    auto aSeqRange = asNonConstRange(aSeq);
    for ( size_t nInd = 0; nInd < aList.size(); nInd++ )
        aSeqRange[nInd] = aList[nInd].GetName();

    return aSeq;
}


sal_Bool SAL_CALL OLESimpleStorage::hasByName( const OUString& aName )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    bool bResult = m_pStorage->IsContained( aName );

    if ( m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw uno::RuntimeException(); // TODO:
    }

    return bResult;
}


uno::Type SAL_CALL OLESimpleStorage::getElementType()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    return cppu::UnoType<io::XInputStream>::get();
}


sal_Bool SAL_CALL OLESimpleStorage::hasElements()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    SvStorageInfoList aList;
    m_pStorage->FillInfoList( &aList );

    if ( m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw uno::RuntimeException(); // TODO:
    }

    return !aList.empty();
}


//  XComponent


void SAL_CALL OLESimpleStorage::dispose()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        return;

    if ( m_aListenersContainer.getLength(aGuard) )
    {
        lang::EventObject aSource( getXWeak() );
        m_aListenersContainer.disposeAndClear( aGuard, aSource );
    }

    m_pStorage.reset();
    m_pStream.reset();

    m_xStream.clear();
    m_xTempStream.clear();

    m_bDisposed = true;
}


void SAL_CALL OLESimpleStorage::addEventListener(
            const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.addInterface( aGuard, xListener );
}


void SAL_CALL OLESimpleStorage::removeEventListener(
            const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.removeInterface( aGuard, xListener );
}


//  XTransactedObject


void SAL_CALL OLESimpleStorage::commit()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    if ( !m_bNoTemporaryCopy && !m_xStream.is() )
        throw io::IOException(); // TODO

    if ( !m_pStorage->Commit() || m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw io::IOException(); // TODO
    }

    UpdateOriginal_Impl();
}


void SAL_CALL OLESimpleStorage::revert()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    if ( !m_bNoTemporaryCopy && !m_xStream.is() )
        throw io::IOException(); // TODO

    if ( !m_pStorage->Revert() || m_pStorage->GetError() )
    {
        m_pStorage->ResetError();
        throw io::IOException(); // TODO
    }

    UpdateOriginal_Impl();
}


//  XClassifiedObject


uno::Sequence< sal_Int8 > SAL_CALL OLESimpleStorage::getClassID()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_pStorage )
        throw uno::RuntimeException();

    return m_pStorage->GetClassName().GetByteSequence();
}

OUString SAL_CALL OLESimpleStorage::getClassName()
{
    return OUString();
}

void SAL_CALL OLESimpleStorage::setClassInfo( const uno::Sequence< sal_Int8 >& /*aClassID*/,
                            const OUString& /*sClassName*/ )
{
    throw lang::NoSupportException();
}

//  XServiceInfo
OUString SAL_CALL OLESimpleStorage::getImplementationName()
{
    return u"com.sun.star.comp.embed.OLESimpleStorage"_ustr;
}

sal_Bool SAL_CALL OLESimpleStorage::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence< OUString > SAL_CALL OLESimpleStorage::getSupportedServiceNames()
{
    return { u"com.sun.star.embed.OLESimpleStorage"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_embed_OLESimpleStorage(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &arguments)
{
    return cppu::acquire(new OLESimpleStorage(context, arguments));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

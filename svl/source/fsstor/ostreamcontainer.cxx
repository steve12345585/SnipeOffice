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

#include "ostreamcontainer.hxx"

#include <cppuhelper/queryinterface.hxx>
#include <comphelper/sequence.hxx>


using namespace ::com::sun::star;

OFSStreamContainer::OFSStreamContainer( const uno::Reference < io::XStream >& xStream )
: m_bDisposed( false )
, m_bInputClosed( false )
, m_bOutputClosed( false )
{
    try
    {
        m_xStream = xStream;
        if ( !m_xStream.is() )
            throw uno::RuntimeException();

        m_xSeekable.set( xStream, uno::UNO_QUERY );
        m_xInputStream = xStream->getInputStream();
        m_xOutputStream = xStream->getOutputStream();
        m_xTruncate.set( m_xOutputStream, uno::UNO_QUERY );
        m_xAsyncOutputMonitor.set( m_xOutputStream, uno::UNO_QUERY );
    }
    catch( uno::Exception& )
    {
        m_xStream.clear();
        m_xSeekable.clear();
        m_xInputStream.clear();
        m_xOutputStream.clear();
        m_xTruncate.clear();
        m_xAsyncOutputMonitor.clear();
    }
}

OFSStreamContainer::~OFSStreamContainer()
{
}

// XInterface
uno::Any SAL_CALL OFSStreamContainer::queryInterface( const uno::Type& rType )
{
    uno::Any aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<lang::XTypeProvider*> ( this )
                    ,   static_cast<io::XStream*> ( this )
                    ,   static_cast<embed::XExtendedStorageStream*> ( this )
                    ,   static_cast<lang::XComponent*> ( this ) );

    if ( aReturn.hasValue() )
        return aReturn ;

    if ( m_xSeekable.is() )
    {
        aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<io::XSeekable*> ( this ) );

        if ( aReturn.hasValue() )
            return aReturn ;
    }

    if ( m_xInputStream.is() )
    {
        aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<io::XInputStream*> ( this ) );

        if ( aReturn.hasValue() )
            return aReturn ;
    }
    if ( m_xOutputStream.is() )
    {
        aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<io::XOutputStream*> ( this ) );

        if ( aReturn.hasValue() )
            return aReturn ;
    }
    if ( m_xTruncate.is() )
    {
        aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<io::XTruncate*> ( this ) );

        if ( aReturn.hasValue() )
            return aReturn ;
    }
    if ( m_xAsyncOutputMonitor.is() )
    {
        aReturn = ::cppu::queryInterface
                (   rType
                    ,   static_cast<io::XAsyncOutputMonitor*> ( this ) );

        if ( aReturn.hasValue() )
            return aReturn ;
    }

    return OWeakObject::queryInterface( rType );
}

void SAL_CALL OFSStreamContainer::acquire()
        noexcept
{
    OWeakObject::acquire();
}

void SAL_CALL OFSStreamContainer::release()
        noexcept
{
    OWeakObject::release();
}

//  XTypeProvider
uno::Sequence< uno::Type > SAL_CALL OFSStreamContainer::getTypes()
{
    std::scoped_lock aGuard( m_aMutex );
    if ( !m_aTypes.hasElements() )
    {
        std::vector<uno::Type> tmp
        {
            cppu::UnoType<lang::XTypeProvider>::get(),
            cppu::UnoType<embed::XExtendedStorageStream>::get()
        };

        if ( m_xSeekable.is() )
            tmp.push_back(cppu::UnoType<io::XSeekable>::get());
        if ( m_xInputStream.is() )
            tmp.push_back(cppu::UnoType<io::XInputStream>::get());
        if ( m_xOutputStream.is() )
            tmp.push_back(cppu::UnoType<io::XOutputStream>::get());
        if ( m_xTruncate.is() )
            tmp.push_back(cppu::UnoType<io::XTruncate>::get());
        if ( m_xAsyncOutputMonitor.is() )
            tmp.push_back(cppu::UnoType<io::XAsyncOutputMonitor>::get());

        m_aTypes = comphelper::containerToSequence(tmp);
    }
    return m_aTypes;
}

uno::Sequence< sal_Int8 > SAL_CALL OFSStreamContainer::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// XStream
uno::Reference< io::XInputStream > SAL_CALL OFSStreamContainer::getInputStream()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() )
        throw uno::RuntimeException();

    if ( m_xInputStream.is() )
        return uno::Reference< io::XInputStream >( static_cast< io::XInputStream* >( this ) );

    return uno::Reference< io::XInputStream >();
}

uno::Reference< io::XOutputStream > SAL_CALL OFSStreamContainer::getOutputStream()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() )
        throw uno::RuntimeException();

    if ( m_xOutputStream.is() )
        return uno::Reference< io::XOutputStream >( static_cast< io::XOutputStream* >( this ) );

    return uno::Reference< io::XOutputStream >();
}

// XComponent
void SAL_CALL OFSStreamContainer::dispose()
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        return;

    if ( !m_xStream.is() )
        throw uno::RuntimeException();

    if ( m_xInputStream.is() && !m_bInputClosed )
    {
        m_xInputStream->closeInput();
        m_bInputClosed = true;
    }

    if ( m_xOutputStream.is() && !m_bOutputClosed )
    {
        m_xOutputStream->closeOutput();
        m_bOutputClosed = true;
    }

    lang::EventObject aSource( getXWeak() );
    m_aListenersContainer.disposeAndClear( aGuard, aSource );
    m_bDisposed = true;
}

void SAL_CALL OFSStreamContainer::addEventListener( const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.addInterface( aGuard, xListener );
}

void SAL_CALL OFSStreamContainer::removeEventListener( const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.removeInterface( aGuard, xListener );
}


// XSeekable
void SAL_CALL OFSStreamContainer::seek( sal_Int64 location )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xSeekable.is() )
        throw uno::RuntimeException();

    m_xSeekable->seek( location );
}

sal_Int64 SAL_CALL OFSStreamContainer::getPosition()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xSeekable.is() )
        throw uno::RuntimeException();

    return m_xSeekable->getPosition();
}

sal_Int64 SAL_CALL OFSStreamContainer::getLength()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xSeekable.is() )
        throw uno::RuntimeException();

    return m_xSeekable->getLength();
}


// XInputStream
sal_Int32 SAL_CALL OFSStreamContainer::readBytes( uno::Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->readBytes( aData, nBytesToRead );
}

sal_Int32 SAL_CALL OFSStreamContainer::readSomeBytes( uno::Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->readSomeBytes( aData, nMaxBytesToRead );
}

void SAL_CALL OFSStreamContainer::skipBytes( sal_Int32 nBytesToSkip )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xInputStream.is() )
        throw uno::RuntimeException();

    m_xInputStream->skipBytes( nBytesToSkip );
}

sal_Int32 SAL_CALL OFSStreamContainer::available()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->available();
}

void SAL_CALL OFSStreamContainer::closeInput()
{
    {
        std::scoped_lock aGuard( m_aMutex );

        if ( m_bDisposed )
            throw lang::DisposedException();

        if ( !m_xStream.is() || !m_xInputStream.is() )
            throw uno::RuntimeException();

        if ( m_xInputStream.is() )
        {
            m_xInputStream->closeInput();
            m_bInputClosed = true;
        }
        if ( !m_bOutputClosed )
            return;
    }

    dispose();
}

// XOutputStream
void SAL_CALL OFSStreamContainer::writeBytes( const uno::Sequence< sal_Int8 >& aData )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xOutputStream.is() )
        throw uno::RuntimeException();

    return m_xOutputStream->writeBytes( aData );
}

void SAL_CALL OFSStreamContainer::flush()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xOutputStream.is() )
        throw uno::RuntimeException();

    return m_xOutputStream->flush();
}

void SAL_CALL OFSStreamContainer::closeOutput()
{
    {
        std::scoped_lock aGuard( m_aMutex );

        if ( m_bDisposed )
            throw lang::DisposedException();

        if ( !m_xStream.is() || !m_xOutputStream.is() )
            throw uno::RuntimeException();

        if ( m_xOutputStream.is() )
        {
            m_xOutputStream->closeOutput();
            m_bOutputClosed = true;
        }
        if ( !m_bInputClosed )
            return;
    }
    dispose();
}


// XTruncate
void SAL_CALL OFSStreamContainer::truncate()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xTruncate.is() )
        throw uno::RuntimeException();

    m_xTruncate->truncate();
}


// XAsyncOutputMonitor
void SAL_CALL OFSStreamContainer::waitForCompletion()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xStream.is() || !m_xAsyncOutputMonitor.is() )
        throw uno::RuntimeException();

    m_xAsyncOutputMonitor->waitForCompletion();
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include "oinputstreamcontainer.hxx"
#include <cppuhelper/typeprovider.hxx>
#include <cppuhelper/queryinterface.hxx>

using namespace ::com::sun::star;

OFSInputStreamContainer::OFSInputStreamContainer( const uno::Reference< io::XInputStream >& xStream )
: m_xInputStream( xStream )
, m_xSeekable( xStream, uno::UNO_QUERY )
, m_bSeekable( false )
, m_bDisposed( false )
{
    m_bSeekable = m_xSeekable.is();
}

OFSInputStreamContainer::~OFSInputStreamContainer()
{
}

uno::Sequence< uno::Type > SAL_CALL OFSInputStreamContainer::getTypes()
{
    if (m_bSeekable)
    {
        static cppu::OTypeCollection aTypeCollection(cppu::UnoType<io::XStream>::get(),
                                                     cppu::UnoType<io::XInputStream>::get(),
                                                     cppu::UnoType<io::XSeekable>::get());

        return aTypeCollection.getTypes();
    }
    else
    {
        static cppu::OTypeCollection aTypeCollection(cppu::UnoType<io::XStream>::get(),
                                                     cppu::UnoType<io::XInputStream>::get());

        return aTypeCollection.getTypes();
    }
}

uno::Any SAL_CALL OFSInputStreamContainer::queryInterface( const uno::Type& rType )
{
    // Attention:
    //  Don't use mutex or guard in this method!!! Is a method of XInterface.

    uno::Any aReturn;
    if ( m_bSeekable )
        aReturn = ::cppu::queryInterface( rType,
                                           static_cast< io::XStream* >( this ),
                                           static_cast< io::XInputStream* >( this ),
                                           static_cast< io::XSeekable* >( this ) );
    else
        aReturn = ::cppu::queryInterface( rType,
                                           static_cast< io::XStream* >( this ),
                                           static_cast< io::XInputStream* >( this ) );

    if ( aReturn.hasValue() )
        return aReturn ;

    return ::cppu::OWeakObject::queryInterface( rType ) ;
}

void SAL_CALL OFSInputStreamContainer::acquire()
        noexcept
{
    ::cppu::OWeakObject::acquire();
}

void SAL_CALL OFSInputStreamContainer::release()
        noexcept
{
    ::cppu::OWeakObject::release();
}

sal_Int32 SAL_CALL OFSInputStreamContainer::readBytes( uno::Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->readBytes( aData, nBytesToRead );
}

sal_Int32 SAL_CALL OFSInputStreamContainer::readSomeBytes( uno::Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->readSomeBytes( aData, nMaxBytesToRead );
}

void SAL_CALL OFSInputStreamContainer::skipBytes( sal_Int32 nBytesToSkip )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xInputStream.is() )
        throw uno::RuntimeException();

    m_xInputStream->skipBytes( nBytesToSkip );
}

sal_Int32 SAL_CALL OFSInputStreamContainer::available(  )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xInputStream.is() )
        throw uno::RuntimeException();

    return m_xInputStream->available();
}

void SAL_CALL OFSInputStreamContainer::closeInput(  )
{
    {
        std::scoped_lock aGuard( m_aMutex );

        if ( m_bDisposed )
            throw lang::DisposedException();

        if ( !m_xInputStream.is() )
            throw uno::RuntimeException();
    }
    dispose();
}

uno::Reference< io::XInputStream > SAL_CALL OFSInputStreamContainer::getInputStream()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xInputStream.is() )
        return uno::Reference< io::XInputStream >();

    return this;
}

uno::Reference< io::XOutputStream > SAL_CALL OFSInputStreamContainer::getOutputStream()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    return uno::Reference< io::XOutputStream >();
}

void SAL_CALL OFSInputStreamContainer::seek( sal_Int64 location )
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xSeekable.is() )
        throw uno::RuntimeException();

    m_xSeekable->seek( location );
}

sal_Int64 SAL_CALL OFSInputStreamContainer::getPosition()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xSeekable.is() )
        throw uno::RuntimeException();

    return m_xSeekable->getPosition();
}

sal_Int64 SAL_CALL OFSInputStreamContainer::getLength()
{
    std::scoped_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( !m_xSeekable.is() )
        throw uno::RuntimeException();

    return m_xSeekable->getLength();
}

void SAL_CALL OFSInputStreamContainer::dispose(  )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        return;

    if ( !m_xInputStream.is() )
        throw uno::RuntimeException();

    m_xInputStream->closeInput();

    lang::EventObject aSource( getXWeak() );
    m_aListenersContainer.disposeAndClear( aGuard, aSource );

    m_bDisposed = true;
}

void SAL_CALL OFSInputStreamContainer::addEventListener( const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.addInterface( aGuard, xListener );
}

void SAL_CALL OFSInputStreamContainer::removeEventListener( const uno::Reference< lang::XEventListener >& xListener )
{
    std::unique_lock aGuard( m_aMutex );

    if ( m_bDisposed )
        throw lang::DisposedException();

    m_aListenersContainer.removeInterface( aGuard, xListener );
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

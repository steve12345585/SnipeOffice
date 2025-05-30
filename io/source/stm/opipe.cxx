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

#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/XPipe.hpp>
#include <com/sun/star/io/XConnectable.hpp>

#include <com/sun/star/lang/XServiceInfo.hpp>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <osl/conditn.hxx>
#include <osl/mutex.hxx>

#include <limits>
#include <memory>
#include <optional>
#include <string.h>

using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;

#include "streamhelper.hxx"

namespace com::sun::star::uno { class XComponentContext; }

namespace io_stm{

namespace {

class OPipeImpl :
    public WeakImplHelper< XPipe , XConnectable , XServiceInfo >
{
public:
    OPipeImpl( );

public: // XInputStream
    virtual sal_Int32 SAL_CALL readBytes(Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead) override;
    virtual sal_Int32 SAL_CALL readSomeBytes(Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead) override;
    virtual void SAL_CALL skipBytes(sal_Int32 nBytesToSkip) override;
    virtual sal_Int32 SAL_CALL available() override;
    virtual void SAL_CALL closeInput() override;

public: // XOutputStream

    virtual void SAL_CALL writeBytes(const Sequence< sal_Int8 >& aData) override;
    virtual void SAL_CALL flush() override;
    virtual void SAL_CALL closeOutput() override;

public: // XConnectable
    virtual void SAL_CALL setPredecessor(const Reference< XConnectable >& aPredecessor) override;
    virtual Reference< XConnectable > SAL_CALL getPredecessor() override;
    virtual void SAL_CALL setSuccessor(const Reference < XConnectable > & aSuccessor) override;
    virtual Reference < XConnectable > SAL_CALL getSuccessor() override ;


public: // XServiceInfo
    OUString                    SAL_CALL getImplementationName() override;
    Sequence< OUString >         SAL_CALL getSupportedServiceNames() override;
    sal_Bool                        SAL_CALL supportsService(const OUString& ServiceName) override;

private:

    Reference < XConnectable >  m_succ;
    Reference < XConnectable >  m_pred;

    sal_Int32 m_nBytesToSkip;

    bool m_bOutputStreamClosed;
    bool m_bInputStreamClosed;

    osl::Condition m_conditionBytesAvail;
    Mutex          m_mutexAccess;
    std::optional<MemFIFO> m_oFIFO;
};

}

OPipeImpl::OPipeImpl()
    : m_nBytesToSkip(0 )
    , m_bOutputStreamClosed(false )
    , m_bInputStreamClosed( false )
    , m_oFIFO( std::in_place )
{
}



sal_Int32 OPipeImpl::readBytes(Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead)
{
    while( true )
    {
        { // start guarded section
            MutexGuard guard( m_mutexAccess );
            if( m_bInputStreamClosed )
            {
                throw NotConnectedException(
                    u"Pipe::readBytes NotConnectedException"_ustr,
                    *this );
            }
            sal_Int32 nOccupiedBufferLen = m_oFIFO->getSize();

            if( m_bOutputStreamClosed && nBytesToRead > nOccupiedBufferLen )
            {
                nBytesToRead = nOccupiedBufferLen;
            }

            if( nOccupiedBufferLen < nBytesToRead )
            {
                // wait outside guarded section
                m_conditionBytesAvail.reset();
            }
            else {
                // necessary bytes are available
                m_oFIFO->read( aData , nBytesToRead );
                return nBytesToRead;
            }
        } // end guarded section

        // wait for new data outside guarded section!
        m_conditionBytesAvail.wait();
    }
}


sal_Int32 OPipeImpl::readSomeBytes(Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead)
{
    while( true ) {
        {
            MutexGuard guard( m_mutexAccess );
            if( m_bInputStreamClosed )
            {
                throw NotConnectedException(
                    u"Pipe::readSomeBytes NotConnectedException"_ustr,
                    *this );
            }
            if( m_oFIFO->getSize() )
            {
                sal_Int32 nSize = std::min( nMaxBytesToRead , m_oFIFO->getSize() );
                aData.realloc( nSize );
                m_oFIFO->read( aData , nSize );
                return nSize;
            }

            if( m_bOutputStreamClosed )
            {
                // no bytes in buffer anymore
                return 0;
            }
        }

        m_conditionBytesAvail.wait();
    }
}


void OPipeImpl::skipBytes(sal_Int32 nBytesToSkip)
{
    MutexGuard guard( m_mutexAccess );
    if( m_bInputStreamClosed )
    {
        throw NotConnectedException(
            u"Pipe::skipBytes NotConnectedException"_ustr,
            *this );
    }

    if( nBytesToSkip < 0
        || (nBytesToSkip
            > std::numeric_limits< sal_Int32 >::max() - m_nBytesToSkip) )
    {
        throw BufferSizeExceededException(
            u"Pipe::skipBytes BufferSizeExceededException"_ustr,
            *this );
    }
    m_nBytesToSkip += nBytesToSkip;

    nBytesToSkip = std::min( m_oFIFO->getSize() , m_nBytesToSkip );
    m_oFIFO->skip( nBytesToSkip );
    m_nBytesToSkip -= nBytesToSkip;
}


sal_Int32 OPipeImpl::available()
 {
    MutexGuard guard( m_mutexAccess );
    if( m_bInputStreamClosed )
    {
        throw NotConnectedException(
            u"Pipe::available NotConnectedException"_ustr,
            *this );
    }
    return m_oFIFO->getSize();
}

void OPipeImpl::closeInput()
{
    MutexGuard guard( m_mutexAccess );

    m_bInputStreamClosed = true;

    m_oFIFO.reset();

    // readBytes may throw an exception
    m_conditionBytesAvail.set();

    setSuccessor( Reference< XConnectable > () );
}


void OPipeImpl::writeBytes(const Sequence< sal_Int8 >& aData)
{
    MutexGuard guard( m_mutexAccess );

    if( m_bOutputStreamClosed )
    {
        throw NotConnectedException(
            u"Pipe::writeBytes NotConnectedException (outputstream)"_ustr,
            *this );
    }

    if( m_bInputStreamClosed )
    {
        throw NotConnectedException(
            u"Pipe::writeBytes NotConnectedException (inputstream)"_ustr,
            *this );
    }

    // check skipping
    sal_Int32 nLen = aData.getLength();
    if( m_nBytesToSkip  && m_nBytesToSkip >= nLen  ) {
        // all must be skipped - forget whole call
        m_nBytesToSkip -= nLen;
        return;
    }

    // adjust buffersize if necessary
    if( m_nBytesToSkip )
    {
        Sequence< sal_Int8 > seqCopy( nLen - m_nBytesToSkip );
        memcpy( seqCopy.getArray() , &( aData.getConstArray()[m_nBytesToSkip] ) , nLen-m_nBytesToSkip );
        m_oFIFO->write( seqCopy );
    }
    else
    {
        m_oFIFO->write( aData );
    }
    m_nBytesToSkip = 0;

    // readBytes may check again if enough bytes are available
    m_conditionBytesAvail.set();
}


void OPipeImpl::flush()
{
    // nothing to do for a pipe
}

void OPipeImpl::closeOutput()
{
    MutexGuard guard( m_mutexAccess );

    m_bOutputStreamClosed = true;
    m_conditionBytesAvail.set();
    setPredecessor( Reference < XConnectable > () );
}


void OPipeImpl::setSuccessor( const Reference < XConnectable >  &r )
{
     /// if the references match, nothing needs to be done
     if( m_succ != r ) {
         /// store the reference for later use
         m_succ = r;

         if( m_succ.is() )
         {
              m_succ->setPredecessor(
                  Reference< XConnectable > ( static_cast< XConnectable * >(this) ) );
         }
     }
}

Reference < XConnectable > OPipeImpl::getSuccessor()
{
    return m_succ;
}


// XDataSource
void OPipeImpl::setPredecessor( const Reference < XConnectable > &r )
{
    if( r != m_pred ) {
        m_pred = r;
        if( m_pred.is() ) {
            m_pred->setSuccessor(
                Reference < XConnectable > ( static_cast< XConnectable * >(this) ) );
        }
    }
}

Reference < XConnectable > OPipeImpl::getPredecessor()
{
    return m_pred;
}


// XServiceInfo
OUString OPipeImpl::getImplementationName()
{
    return u"com.sun.star.comp.io.stm.Pipe"_ustr;
}

// XServiceInfo
sal_Bool OPipeImpl::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > OPipeImpl::getSupportedServiceNames()
{
    return { u"com.sun.star.io.Pipe"_ustr };
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
io_OPipeImpl_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new io_stm::OPipeImpl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

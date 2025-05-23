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


#include <map>
#include <memory>
#include <optional>

#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/XMarkableStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XActiveDataSource.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XConnectable.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cppuhelper/weak.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <osl/diagnose.h>
#include <mutex>

using namespace ::cppu;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;

#include "streamhelper.hxx"

namespace io_stm {

namespace {

/***********************
*
* OMarkableOutputStream.
*
* This object allows to set marks in an outputstream. It is allowed to jump back to the marks and
* rewrite the same bytes.
*
*         The object must buffer the data since the last mark set. Flush will not
*         have any effect. As soon as the last mark has been removed, the object may write the data
*         through to the chained object.
*
**********************/
class OMarkableOutputStream :
    public WeakImplHelper< XOutputStream ,
                            XActiveDataSource ,
                            XMarkableStream ,
                            XConnectable,
                            XServiceInfo
                          >
{
public:
    OMarkableOutputStream(  );

public: // XOutputStream
    virtual void SAL_CALL writeBytes(const Sequence< sal_Int8 >& aData) override;
    virtual void SAL_CALL flush() override;
    virtual void SAL_CALL closeOutput() override;

public: // XMarkable
    virtual sal_Int32 SAL_CALL createMark() override;
    virtual void SAL_CALL deleteMark(sal_Int32 Mark) override;
    virtual void SAL_CALL jumpToMark(sal_Int32 nMark) override;
    virtual void SAL_CALL jumpToFurthest() override;
    virtual sal_Int32 SAL_CALL offsetToMark(sal_Int32 nMark) override;

public: // XActiveDataSource
    virtual void SAL_CALL setOutputStream(const Reference < XOutputStream > & aStream) override;
    virtual Reference < XOutputStream > SAL_CALL getOutputStream() override;

public: // XConnectable
    virtual void SAL_CALL setPredecessor(const Reference < XConnectable > & aPredecessor) override;
    virtual Reference < XConnectable > SAL_CALL getPredecessor() override;
    virtual void SAL_CALL setSuccessor(const Reference < XConnectable >& aSuccessor) override;
    virtual Reference<  XConnectable >  SAL_CALL getSuccessor() override;

public: // XServiceInfo
    OUString                     SAL_CALL getImplementationName() override;
    Sequence< OUString >         SAL_CALL getSupportedServiceNames() override;
    sal_Bool                        SAL_CALL supportsService(const OUString& ServiceName) override;

private:
    // helper methods
    /// @throws NotConnectedException
    /// @throws BufferSizeExceededException
    void checkMarksAndFlush();

    Reference< XConnectable > m_succ;
    Reference< XConnectable > m_pred;

    Reference< XOutputStream >  m_output;
    bool m_bValidStream;

    MemRingBuffer m_aRingBuffer;
    std::map<sal_Int32,sal_Int32,std::less< sal_Int32 > > m_mapMarks;
    sal_Int32 m_nCurrentPos;
    sal_Int32 m_nCurrentMark;

    std::mutex m_mutex;
};

}

OMarkableOutputStream::OMarkableOutputStream( )
    : m_bValidStream(false)
    , m_nCurrentPos(0)
    , m_nCurrentMark(0)
{
}

// XOutputStream
void OMarkableOutputStream::writeBytes(const Sequence< sal_Int8 >& aData)
{
    std::unique_lock guard( m_mutex );

    if( !m_bValidStream ) {
        throw NotConnectedException();
    }
    if( m_mapMarks.empty() && ( m_aRingBuffer.getSize() == 0 ) ) {
        // no mark and  buffer active, simple write through
        m_output->writeBytes( aData );
    }
    else {
        // new data must be buffered
        m_aRingBuffer.writeAt( m_nCurrentPos , aData );
        m_nCurrentPos += aData.getLength();
        checkMarksAndFlush();
    }

}

void OMarkableOutputStream::flush()
{
    Reference< XOutputStream > output;
    {
        std::unique_lock guard( m_mutex );
        output = m_output;
    }

    // Markable cannot flush buffered data, because the data may get rewritten,
    // however one can forward the flush to the chained stream to give it
    // a chance to write data buffered in the chained stream.
    if( output.is() )
    {
        output->flush();
    }
}

void OMarkableOutputStream::closeOutput()
{
    if( !m_bValidStream ) {
        throw NotConnectedException();
    }
    std::unique_lock guard( m_mutex );
    // all marks must be cleared and all

    m_mapMarks.clear();
    m_nCurrentPos = m_aRingBuffer.getSize();
    checkMarksAndFlush();

    m_output->closeOutput();

    setOutputStream( Reference< XOutputStream > () );
    setPredecessor( Reference < XConnectable >() );
    setSuccessor( Reference< XConnectable > () );

}


sal_Int32 OMarkableOutputStream::createMark()
{
    std::unique_lock guard( m_mutex );
    sal_Int32 nMark = m_nCurrentMark;

    m_mapMarks[nMark] = m_nCurrentPos;

    m_nCurrentMark ++;
    return nMark;
}

void OMarkableOutputStream::deleteMark(sal_Int32 Mark)
{
    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::iterator ii = m_mapMarks.find( Mark );

    if( ii == m_mapMarks.end() ) {
        throw IllegalArgumentException(
            "MarkableOutputStream::deleteMark unknown mark (" + OUString::number(Mark) + ")",
            *this, 0);
    }
    m_mapMarks.erase( ii );
    checkMarksAndFlush();
}

void OMarkableOutputStream::jumpToMark(sal_Int32 nMark)
{
    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::iterator ii = m_mapMarks.find( nMark );

    if( ii == m_mapMarks.end() ) {
        throw IllegalArgumentException(
            "MarkableOutputStream::jumpToMark unknown mark (" + OUString::number(nMark) + ")",
            *this, 0);
    }
    m_nCurrentPos = (*ii).second;
}

void OMarkableOutputStream::jumpToFurthest()
{
    std::unique_lock guard( m_mutex );
    m_nCurrentPos = m_aRingBuffer.getSize();
    checkMarksAndFlush();
}

sal_Int32 OMarkableOutputStream::offsetToMark(sal_Int32 nMark)
{

    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::const_iterator ii = m_mapMarks.find( nMark );

    if( ii == m_mapMarks.end() )
    {
        throw IllegalArgumentException(
            "MarkableOutputStream::offsetToMark unknown mark (" + OUString::number(nMark) + ")",
            *this, 0);
    }
    return m_nCurrentPos - (*ii).second;
}


// XActiveDataSource2
void OMarkableOutputStream::setOutputStream(const Reference < XOutputStream >& aStream)
{
    if( m_output != aStream ) {
        m_output = aStream;

        Reference < XConnectable > succ( m_output , UNO_QUERY );
        setSuccessor( succ );
    }
    m_bValidStream = m_output.is();
}

Reference< XOutputStream > OMarkableOutputStream::getOutputStream()
{
    return m_output;
}


void OMarkableOutputStream::setSuccessor( const Reference< XConnectable > &r )
{
     /// if the references match, nothing needs to be done
     if( m_succ != r ) {
         /// store the reference for later use
         m_succ = r;

         if( m_succ.is() ) {
              m_succ->setPredecessor( Reference < XConnectable > (
                  static_cast< XConnectable *  >(this) ) );
         }
     }
}
Reference <XConnectable > OMarkableOutputStream::getSuccessor()
{
    return m_succ;
}


// XDataSource
void OMarkableOutputStream::setPredecessor( const Reference< XConnectable > &r )
{
    if( r != m_pred ) {
        m_pred = r;
        if( m_pred.is() ) {
            m_pred->setSuccessor( Reference < XConnectable > (
                static_cast< XConnectable *  >(this ) ) );
        }
    }
}
Reference < XConnectable > OMarkableOutputStream::getPredecessor()
{
    return m_pred;
}


// private methods

void OMarkableOutputStream::checkMarksAndFlush()
{
    // find the smallest mark
    sal_Int32 nNextFound = m_nCurrentPos;
    for (auto const& mark : m_mapMarks)
    {
        if( mark.second <= nNextFound )  {
            nNextFound = mark.second;
        }
    }

    if( nNextFound ) {
        // some data must be released !
        m_nCurrentPos -= nNextFound;
        for (auto & mark : m_mapMarks)
        {
            mark.second -= nNextFound;
        }

        Sequence<sal_Int8> seq(nNextFound);
        m_aRingBuffer.readAt( 0 , seq , nNextFound );
        m_aRingBuffer.forgetFromStart( nNextFound );

        // now write data through to streams
        m_output->writeBytes( seq );
    }
    else {
        // nothing to do. There is a mark or the current cursor position, that prevents
        // releasing data !
    }
}


// XServiceInfo
OUString OMarkableOutputStream::getImplementationName()
{
    return u"com.sun.star.comp.io.stm.MarkableOutputStream"_ustr;
}

// XServiceInfo
sal_Bool OMarkableOutputStream::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > OMarkableOutputStream::getSupportedServiceNames()
{
    return { u"com.sun.star.io.MarkableOutputStream"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
io_OMarkableOutputStream_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new OMarkableOutputStream());
}


// XMarkableInputStream

namespace {

class OMarkableInputStream :
    public WeakImplHelper
    <
             XInputStream,
             XActiveDataSink,
             XMarkableStream,
             XConnectable,
             XServiceInfo
    >
{
public:
    OMarkableInputStream(  );


public: // XInputStream
    virtual sal_Int32 SAL_CALL readBytes(Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead) override ;
    virtual sal_Int32 SAL_CALL readSomeBytes(Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead) override;
    virtual void SAL_CALL skipBytes(sal_Int32 nBytesToSkip) override;

    virtual sal_Int32 SAL_CALL available() override;
    virtual void SAL_CALL closeInput() override;

public: // XMarkable
    virtual sal_Int32 SAL_CALL createMark() override;
    virtual void SAL_CALL deleteMark(sal_Int32 Mark) override;
    virtual void SAL_CALL jumpToMark(sal_Int32 nMark) override;
    virtual void SAL_CALL jumpToFurthest() override;
    virtual sal_Int32 SAL_CALL offsetToMark(sal_Int32 nMark) override;

public: // XActiveDataSink
    virtual void SAL_CALL setInputStream(const Reference < XInputStream > & aStream) override;
    virtual Reference < XInputStream > SAL_CALL getInputStream() override;

public: // XConnectable
    virtual void SAL_CALL setPredecessor(const Reference < XConnectable > & aPredecessor) override;
    virtual Reference < XConnectable > SAL_CALL getPredecessor() override;
    virtual void SAL_CALL setSuccessor(const Reference < XConnectable > & aSuccessor) override;
    virtual Reference < XConnectable > SAL_CALL getSuccessor() override;

public: // XServiceInfo
    OUString                     SAL_CALL getImplementationName() override;
    Sequence< OUString >         SAL_CALL getSupportedServiceNames() override;
    sal_Bool                         SAL_CALL  supportsService(const OUString& ServiceName) override;

private:
    void checkMarksAndFlush();

    Reference < XConnectable >  m_succ;
    Reference < XConnectable >  m_pred;

    Reference< XInputStream > m_input;
    bool m_bValidStream;

    std::optional<MemRingBuffer> m_oBuffer;
    std::map<sal_Int32,sal_Int32,std::less< sal_Int32 > > m_mapMarks;
    sal_Int32 m_nCurrentPos;
    sal_Int32 m_nCurrentMark;

    std::mutex m_mutex;
};

}

OMarkableInputStream::OMarkableInputStream()
    : m_bValidStream(false)
    , m_nCurrentPos(0)
    , m_nCurrentMark(0)
{
    m_oBuffer.emplace();
}


// XInputStream

sal_Int32 OMarkableInputStream::readBytes(Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead)
{
    std::unique_lock guard( m_mutex );

    if( !m_bValidStream ) {
        throw NotConnectedException(
            u"MarkableInputStream::readBytes NotConnectedException"_ustr,
            *this );
    }

    sal_Int32 nBytesRead;
    if( m_mapMarks.empty() && ! m_oBuffer->getSize() ) {
        // normal read !
        nBytesRead = m_input->readBytes( aData, nBytesToRead );
    }
    else {
        // read from buffer
        sal_Int32 nRead;

        // read enough bytes into buffer
        if( m_oBuffer->getSize() - m_nCurrentPos < nBytesToRead  ) {
            sal_Int32 nToRead = nBytesToRead - ( m_oBuffer->getSize() - m_nCurrentPos );
            nRead = m_input->readBytes( aData , nToRead );

            OSL_ASSERT( aData.getLength() == nRead );

            m_oBuffer->writeAt( m_oBuffer->getSize() , aData );

            if( nRead < nToRead ) {
                nBytesToRead = nBytesToRead - (nToRead-nRead);
            }
        }

        OSL_ASSERT( m_oBuffer->getSize() - m_nCurrentPos >= nBytesToRead  );

        m_oBuffer->readAt( m_nCurrentPos , aData , nBytesToRead );

        m_nCurrentPos += nBytesToRead;
        nBytesRead = nBytesToRead;
    }

    return nBytesRead;
}


sal_Int32 OMarkableInputStream::readSomeBytes(Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead)
{
    std::unique_lock guard( m_mutex );

    if( !m_bValidStream )    {
        throw NotConnectedException(
            u"MarkableInputStream::readSomeBytes NotConnectedException"_ustr,
            *this );
    }

    sal_Int32 nBytesRead;
    if( m_mapMarks.empty() && ! m_oBuffer->getSize() ) {
        // normal read !
        nBytesRead = m_input->readSomeBytes( aData, nMaxBytesToRead );
    }
    else {
        // read from buffer
        sal_Int32 nRead = 0;
        sal_Int32 nInBuffer = m_oBuffer->getSize() - m_nCurrentPos;
        sal_Int32 nAdditionalBytesToRead = std::min<sal_Int32>(nMaxBytesToRead-nInBuffer,m_input->available());
        nAdditionalBytesToRead = std::max<sal_Int32>(0 , nAdditionalBytesToRead );

        // read enough bytes into buffer
        if( 0 == nInBuffer ) {
            nRead = m_input->readSomeBytes( aData , nMaxBytesToRead );
        }
        else if( nAdditionalBytesToRead ) {
            nRead = m_input->readBytes( aData , nAdditionalBytesToRead );
        }

        if( nRead ) {
            aData.realloc( nRead );
            m_oBuffer->writeAt( m_oBuffer->getSize() , aData );
        }

        nBytesRead = std::min( nMaxBytesToRead , nInBuffer + nRead );

        // now take everything from buffer !
        m_oBuffer->readAt( m_nCurrentPos , aData , nBytesRead );

        m_nCurrentPos += nBytesRead;
    }

    return nBytesRead;


}


void OMarkableInputStream::skipBytes(sal_Int32 nBytesToSkip)
{
    if ( nBytesToSkip < 0 )
        throw BufferSizeExceededException(
            u"precondition not met: XInputStream::skipBytes: non-negative integer required!"_ustr,
            *this
        );

    // this method is blocking
    Sequence<sal_Int8> seqDummy( nBytesToSkip );
    readBytes( seqDummy , nBytesToSkip );
}

sal_Int32 OMarkableInputStream::available()
{
    std::unique_lock guard( m_mutex );

    if( !m_bValidStream )    {
        throw NotConnectedException(
            u"MarkableInputStream::available NotConnectedException"_ustr,
            *this );
    }

    sal_Int32 nAvail = m_input->available() + ( m_oBuffer->getSize() - m_nCurrentPos );
    return nAvail;
}


void OMarkableInputStream::closeInput()
{
    std::unique_lock guard( m_mutex );

    if( !m_bValidStream ) {
        throw NotConnectedException(
            u"MarkableInputStream::closeInput NotConnectedException"_ustr,
            *this );
    }

    m_input->closeInput();

    m_input.clear();
    if( m_pred )
        m_pred.clear();
    if( m_succ )
        m_succ.clear();
    m_bValidStream = false;
    m_oBuffer.reset();
    m_nCurrentPos = 0;
    m_nCurrentMark = 0;
}

// XMarkable

sal_Int32 OMarkableInputStream::createMark()
{
    std::unique_lock guard( m_mutex );
    sal_Int32 nMark = m_nCurrentMark;

    m_mapMarks[nMark] = m_nCurrentPos;

    m_nCurrentMark ++;
    return nMark;
}

void OMarkableInputStream::deleteMark(sal_Int32 Mark)
{
    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::iterator ii = m_mapMarks.find( Mark );

    if( ii == m_mapMarks.end() ) {
        throw IllegalArgumentException(
            "MarkableInputStream::deleteMark unknown mark (" + OUString::number(Mark) + ")",
            *this , 0 );
    }
    m_mapMarks.erase( ii );
    checkMarksAndFlush();
}

void OMarkableInputStream::jumpToMark(sal_Int32 nMark)
{
    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::iterator ii = m_mapMarks.find( nMark );

    if( ii == m_mapMarks.end() )
    {
        throw IllegalArgumentException(
            "MarkableInputStream::jumpToMark unknown mark (" + OUString::number(nMark) + ")",
            *this , 0 );
    }
    m_nCurrentPos = (*ii).second;
}

void OMarkableInputStream::jumpToFurthest()
{
    std::unique_lock guard( m_mutex );
    m_nCurrentPos = m_oBuffer->getSize();
    checkMarksAndFlush();
}

sal_Int32 OMarkableInputStream::offsetToMark(sal_Int32 nMark)
{
    std::unique_lock guard( m_mutex );
    std::map<sal_Int32,sal_Int32,std::less<sal_Int32> >::const_iterator ii = m_mapMarks.find( nMark );

    if( ii == m_mapMarks.end() )
    {
        throw IllegalArgumentException(
            "MarkableInputStream::offsetToMark unknown mark (" + OUString::number(nMark) + ")",
            *this, 0 );
    }
    return m_nCurrentPos - (*ii).second;
}


// XActiveDataSource
void OMarkableInputStream::setInputStream(const Reference< XInputStream > & aStream)
{
    Reference < XConnectable > pred;
    {
        std::unique_lock guard( m_mutex );
        if( m_input == aStream )
            return;

        m_input = aStream;
        m_bValidStream = m_input.is();
        pred.set( m_input , UNO_QUERY );
    }
    setPredecessor( pred );
}

Reference< XInputStream > OMarkableInputStream::getInputStream()
{
    std::unique_lock guard( m_mutex );
    return m_input;
}


// XDataSink
void OMarkableInputStream::setSuccessor( const Reference< XConnectable > &r )
{
    {
        std::unique_lock guard( m_mutex );
        /// if the references match, nothing needs to be done
        if( m_succ == r )
            return;

        /// store the reference for later use
        m_succ = r;
    }
    if( r ) {
        /// set this instance as the sink !
        r->setPredecessor( Reference< XConnectable > ( static_cast< XConnectable * >(this) ) );
    }
}

Reference < XConnectable >  OMarkableInputStream::getSuccessor()
{
    std::unique_lock guard( m_mutex );
    return m_succ;
}


// XDataSource
void OMarkableInputStream::setPredecessor( const Reference < XConnectable >  &r )
{
    {
        std::unique_lock guard( m_mutex );
        if( r == m_pred )
            return;
        m_pred = r;
    }
    if( r ) {
        r->setSuccessor( Reference< XConnectable > (
            static_cast< XConnectable * >(this) ) );
    }
}

Reference< XConnectable >  OMarkableInputStream::getPredecessor()
{
    std::unique_lock guard( m_mutex );
    return m_pred;
}


void OMarkableInputStream::checkMarksAndFlush()
{
    // find the smallest mark
    sal_Int32 nNextFound = m_nCurrentPos;
    for (auto const& mark : m_mapMarks)
    {
        if( mark.second <= nNextFound )  {
            nNextFound = mark.second;
        }
    }

    if( nNextFound ) {
        // some data must be released !
        m_nCurrentPos -= nNextFound;
        for (auto & mark : m_mapMarks)
        {
            mark.second -= nNextFound;
        }

        m_oBuffer->forgetFromStart( nNextFound );

    }
    else {
        // nothing to do. There is a mark or the current cursor position, that prevents
        // releasing data !
    }
}

// XServiceInfo
OUString OMarkableInputStream::getImplementationName()
{
    return u"com.sun.star.comp.io.stm.MarkableInputStream"_ustr;
}

// XServiceInfo
sal_Bool OMarkableInputStream::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > OMarkableInputStream::getSupportedServiceNames()
{
    return { u"com.sun.star.io.MarkableInputStream"_ustr };
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
io_OMarkableInputStream_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new OMarkableInputStream());
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

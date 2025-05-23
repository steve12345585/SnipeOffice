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

#include "acceptor.hxx"

#include <unordered_set>

#include <mutex>
#include <rtl/ref.hxx>
#include <com/sun/star/connection/XConnection.hpp>
#include <com/sun/star/connection/XConnectionBroadcaster.hpp>
#include <com/sun/star/connection/ConnectionSetupException.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <cppuhelper/implbase.hxx>
#include <utility>

using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::connection;


namespace io_acceptor {

    typedef std::unordered_set< css::uno::Reference< css::io::XStreamListener> >
            XStreamListener_hash_set;

    namespace {

    class SocketConnection : public ::cppu::WeakImplHelper<
        css::connection::XConnection,
        css::connection::XConnectionBroadcaster>

    {
    public:
        explicit SocketConnection( OUString sConnectionDescription );

        virtual sal_Int32 SAL_CALL read( css::uno::Sequence< sal_Int8 >& aReadBytes,
                                         sal_Int32 nBytesToRead ) override;
        virtual void SAL_CALL write( const css::uno::Sequence< sal_Int8 >& aData ) override;
        virtual void SAL_CALL flush(  ) override;
        virtual void SAL_CALL close(  ) override;
        virtual OUString SAL_CALL getDescription(  ) override;

        // XConnectionBroadcaster
        virtual void SAL_CALL addStreamListener(const css::uno::Reference< css::io::XStreamListener>& aListener) override;
        virtual void SAL_CALL removeStreamListener(const css::uno::Reference< css::io::XStreamListener>& aListener) override;

    public:
        void completeConnectionString();

        ::osl::StreamSocket m_socket;
        oslInterlockedCount m_nStatus;
        OUString m_sDescription;

        std::mutex _mutex;
        bool     _started;
        bool     _closed;
        bool     _error;
        XStreamListener_hash_set _listeners;
    };

    }

    template<class T>
    static void notifyListeners(SocketConnection * pCon, bool * notified, T t)
    {
        XStreamListener_hash_set listeners;

        {
            std::unique_lock guard(pCon->_mutex);
            if(!*notified)
            {
                *notified = true;
                listeners = pCon->_listeners;
            }
        }

        for(auto& listener : listeners)
            t(listener);
    }

    static void callStarted(const Reference<XStreamListener>& xStreamListener)
    {
        xStreamListener->started();
    }

    namespace {

    struct callError {
        const Any & any;

        explicit callError(const Any & any);

        void operator () (const Reference<XStreamListener>& xStreamListener);
    };

    }

    callError::callError(const Any & aAny)
        : any(aAny)
    {
    }

    void callError::operator () (const Reference<XStreamListener>& xStreamListener)
    {
        xStreamListener->error(any);
    }

    static void callClosed(const Reference<XStreamListener>& xStreamListener)
    {
        xStreamListener->closed();
    }


    SocketConnection::SocketConnection( OUString sConnectionDescription) :
        m_nStatus( 0 ),
        m_sDescription(std::move( sConnectionDescription )),
        _started(false),
        _closed(false),
        _error(false)
    {
        // make it unique
        m_sDescription += ",uniqueValue=" ;
        m_sDescription += OUString::number(
            sal::static_int_cast< sal_Int64 >(
                reinterpret_cast< sal_IntPtr >(&m_socket)) );
    }

    void SocketConnection::completeConnectionString()
    {
        m_sDescription +=
            ",peerPort=" + OUString::number(m_socket.getPeerPort()) +
            ",peerHost=" + m_socket.getPeerHost( ) +
            ",localPort=" + OUString::number( m_socket.getLocalPort() ) +
            ",localHost=" + m_socket.getLocalHost();
    }

    sal_Int32 SocketConnection::read( Sequence < sal_Int8 > & aReadBytes , sal_Int32 nBytesToRead )
    {
        if( ! m_nStatus )
        {
            notifyListeners(this, &_started, callStarted);

            if( aReadBytes.getLength() != nBytesToRead )
            {
                aReadBytes.realloc( nBytesToRead );
            }

            sal_Int32 i = m_socket.read(
                aReadBytes.getArray(), aReadBytes.getLength());

            if(i != nBytesToRead)
            {
                OUString message = "acc_socket.cxx:SocketConnection::read: error - " +
                    m_socket.getErrorAsString();

                IOException ioException(message, static_cast<XConnection *>(this));

                Any any;
                any <<= ioException;

                notifyListeners(this, &_error, callError(any));

                throw ioException;
            }

            return i;
        }
        else
        {
            IOException ioException(u"acc_socket.cxx:SocketConnection::read: error - connection already closed"_ustr, static_cast<XConnection *>(this));

            Any any;
            any <<= ioException;

            notifyListeners(this, &_error, callError(any));

            throw ioException;
        }
    }

    void SocketConnection::write( const Sequence < sal_Int8 > &seq )
    {
        if( ! m_nStatus )
        {
            if( m_socket.write( seq.getConstArray() , seq.getLength() ) != seq.getLength() )
            {
                OUString message = "acc_socket.cxx:SocketConnection::write: error - " +
                    m_socket.getErrorAsString();

                IOException ioException(message, static_cast<XConnection *>(this));

                Any any;
                any <<= ioException;

                notifyListeners(this, &_error, callError(any));

                throw ioException;
            }
        }
        else
        {
            IOException ioException(u"acc_socket.cxx:SocketConnection::write: error - connection already closed"_ustr, static_cast<XConnection *>(this));

            Any any;
            any <<= ioException;

            notifyListeners(this, &_error, callError(any));

            throw ioException;
        }
    }

    void SocketConnection::flush( )
    {

    }

    void SocketConnection::close()
    {
        // ensure close is called only once
        if(  1 == osl_atomic_increment( (&m_nStatus) ) )
        {
            m_socket.shutdown();
            notifyListeners(this, &_closed, callClosed);
        }
    }

    OUString SocketConnection::getDescription()
    {
        return m_sDescription;
    }


    // XConnectionBroadcaster
    void SAL_CALL SocketConnection::addStreamListener(const Reference<XStreamListener> & aListener)
    {
        std::unique_lock guard(_mutex);

        _listeners.insert(aListener);
    }

    void SAL_CALL SocketConnection::removeStreamListener(const Reference<XStreamListener> & aListener)
    {
        std::unique_lock guard(_mutex);

        _listeners.erase(aListener);
    }

    SocketAcceptor::SocketAcceptor( OUString sSocketName,
                                    sal_uInt16 nPort,
                                    bool bTcpNoDelay,
                                    OUString sConnectionDescription) :
        m_sSocketName(std::move( sSocketName )),
        m_sConnectionDescription(std::move( sConnectionDescription )),
        m_nPort( nPort ),
        m_bTcpNoDelay( bTcpNoDelay ),
        m_bClosed( false )
    {
    }


    void SocketAcceptor::init()
    {
        if( ! m_addr.setPort( m_nPort ) )
        {
            throw ConnectionSetupException(
                "acc_socket.cxx:SocketAcceptor::init - error - invalid tcp/ip port " +
                OUString::number( m_nPort ));
        }
        if( ! m_addr.setHostname( m_sSocketName.pData ) )
        {
            throw ConnectionSetupException(
                "acc_socket.cxx:SocketAcceptor::init - error - invalid host " + m_sSocketName );
        }
        m_socket.setOption( osl_Socket_OptionReuseAddr, 1);

        if(! m_socket.bind(m_addr) )
        {
            throw ConnectionSetupException(
                "acc_socket.cxx:SocketAcceptor::init - error - couldn't bind on " +
                m_sSocketName + ":" + OUString::number(m_nPort));
        }

        if(! m_socket.listen() )
        {
            throw ConnectionSetupException(
                "acc_socket.cxx:SocketAcceptor::init - error - can't listen on " +
                m_sSocketName  + ":" + OUString::number(m_nPort) );
        }
    }

    Reference< XConnection > SocketAcceptor::accept( )
    {
        rtl::Reference<SocketConnection> pConn(new SocketConnection( m_sConnectionDescription ));

        if( m_socket.acceptConnection( pConn->m_socket )!= osl_Socket_Ok )
        {
            // stopAccepting was called
            return Reference < XConnection > ();
        }
        if( m_bClosed )
        {
            return Reference < XConnection > ();
        }

        pConn->completeConnectionString();
        ::osl::SocketAddr remoteAddr;
        pConn->m_socket.getPeerAddr(remoteAddr);
        OUString remoteHostname = remoteAddr.getHostname();
        // we enable tcpNoDelay for loopback connections because
        // it can make a significant speed difference on linux boxes.
        if( m_bTcpNoDelay || remoteHostname == "localhost" ||
            remoteHostname.startsWith("127.0.0.") )
        {
            sal_Int32 nTcpNoDelay = sal_Int32(true);
            pConn->m_socket.setOption( osl_Socket_OptionTcpNoDelay , &nTcpNoDelay,
                                       sizeof( nTcpNoDelay ) , osl_Socket_LevelTcp );
        }

        return pConn;
    }

    void SocketAcceptor::stopAccepting()
    {
        m_bClosed = true;
        m_socket.close();
    }
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

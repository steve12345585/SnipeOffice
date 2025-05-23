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
#include "acceptor.hxx"
#include <com/sun/star/connection/XConnection.hpp>
#include <com/sun/star/connection/ConnectionSetupException.hpp>
#include <com/sun/star/io/IOException.hpp>

#include <osl/diagnose.h>
#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>
#include <utility>

using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::connection;
using namespace ::com::sun::star::io;


namespace io_acceptor
{
    namespace {

    class PipeConnection :
        public WeakImplHelper< XConnection >
    {
    public:
        explicit PipeConnection( OUString sConnectionDescription);

        virtual sal_Int32 SAL_CALL read( Sequence< sal_Int8 >& aReadBytes, sal_Int32 nBytesToRead ) override;
        virtual void SAL_CALL write( const Sequence< sal_Int8 >& aData ) override;
        virtual void SAL_CALL flush(  ) override;
        virtual void SAL_CALL close(  ) override;
        virtual OUString SAL_CALL getDescription(  ) override;
    public:
        ::osl::StreamPipe m_pipe;
        oslInterlockedCount m_nStatus;
        OUString m_sDescription;
    };

    }

    PipeConnection::PipeConnection( OUString sConnectionDescription) :
        m_nStatus( 0 ),
        m_sDescription(std::move( sConnectionDescription ))
    {
        // make it unique
        m_sDescription += ",uniqueValue=";
        m_sDescription += OUString::number(
            sal::static_int_cast<sal_Int64 >(
                reinterpret_cast< sal_IntPtr >(&m_pipe)) );
    }

    sal_Int32 PipeConnection::read( Sequence < sal_Int8 > & aReadBytes , sal_Int32 nBytesToRead )
    {
        if( m_nStatus )
        {
            throw IOException(u"pipe already closed"_ustr);
        }
        if( aReadBytes.getLength() < nBytesToRead )
        {
            aReadBytes.realloc( nBytesToRead );
        }
        sal_Int32 n = m_pipe.read( aReadBytes.getArray(), nBytesToRead );
        OSL_ASSERT( n >= 0 && n <= aReadBytes.getLength() );
        if( n < aReadBytes.getLength() )
        {
            aReadBytes.realloc( n );
        }
        return n;

    }

    void PipeConnection::write( const Sequence < sal_Int8 > &seq )
    {
        if( m_nStatus )
        {
            throw IOException(u"pipe already closed"_ustr);
        }
        if( m_pipe.write( seq.getConstArray() , seq.getLength() ) != seq.getLength() )
        {
            throw IOException(u"short write"_ustr);
        }
    }

    void PipeConnection::flush( )
    {
    }

    void PipeConnection::close()
    {
        if(  1 == osl_atomic_increment( (&m_nStatus) ) )
        {
            m_pipe.close();
        }
    }

    OUString PipeConnection::getDescription()
    {
        return m_sDescription;
    }

    /***************
     * PipeAcceptor
     **************/
    PipeAcceptor::PipeAcceptor( OUString sPipeName , OUString sConnectionDescription) :
        m_sPipeName(std::move( sPipeName )),
        m_sConnectionDescription(std::move( sConnectionDescription )),
        m_bClosed( false )
    {
    }


    void PipeAcceptor::init()
    {
        m_pipe = Pipe( m_sPipeName.pData , osl_Pipe_CREATE , osl::Security() );
        if( ! m_pipe.is() )
        {
            OUString error = "io.acceptor: Couldn't setup pipe " + m_sPipeName;
            throw ConnectionSetupException( error );
        }
    }

    Reference< XConnection > PipeAcceptor::accept( )
    {
        Pipe pipe;
        {
            std::unique_lock guard( m_mutex );
            pipe = m_pipe;
        }
        if( ! pipe.is() )
        {
            OUString error = "io.acceptor: pipe already closed" + m_sPipeName;
            throw ConnectionSetupException( error );
        }
        rtl::Reference<PipeConnection> pConn(new PipeConnection( m_sConnectionDescription ));

        oslPipeError status = pipe.accept( pConn->m_pipe );

        if( m_bClosed )
        {
            // stopAccepting was called !
            return Reference < XConnection >();
        }
        else if( osl_Pipe_E_None == status )
        {
            return pConn;
        }
        else
        {
            OUString error = "io.acceptor: Couldn't setup pipe " + m_sPipeName;
            throw ConnectionSetupException( error );
        }
    }

    void PipeAcceptor::stopAccepting()
    {
        m_bClosed = true;
        Pipe pipe;
        {
            std::unique_lock guard( m_mutex );
            pipe = m_pipe;
            m_pipe.clear();
        }
        if( pipe.is() )
        {
            pipe.close();
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

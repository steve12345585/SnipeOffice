/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <com/sun/star/io/IOException.hpp>
#include <sal/log.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <utility>

#include "std_outputstream.hxx"

using namespace com::sun::star;

namespace cmis
{
    StdOutputStream::StdOutputStream( boost::shared_ptr< std::ostream > pStream ) :
        m_pStream(std::move( pStream ))
    {
    }

    StdOutputStream::~StdOutputStream()
    {
        if (m_pStream)
            m_pStream->setstate( std::ios::eofbit );
    }

    uno::Any SAL_CALL StdOutputStream::queryInterface( const uno::Type& rType )
    {
        uno::Any aRet = ::cppu::queryInterface( rType, static_cast< XOutputStream* >( this ) );

        return aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType );
    }

    void SAL_CALL StdOutputStream::acquire( ) noexcept
    {
        OWeakObject::acquire();
    }

    void SAL_CALL StdOutputStream::release( ) noexcept
    {
        OWeakObject::release();
    }

    void SAL_CALL StdOutputStream::writeBytes ( const uno::Sequence< sal_Int8 >& aData )
    {
        std::scoped_lock aGuard( m_aMutex );

        if (!m_pStream)
            throw io::IOException( );

        try
        {
            m_pStream->write( reinterpret_cast< const char* >( aData.getConstArray( ) ), aData.getLength( ) );
        }
        catch ( const std::ios_base::failure& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Exception caught when calling write: " << e.what() );
            throw io::IOException( );
        }
    }

    void SAL_CALL StdOutputStream::flush ( )
    {
        std::scoped_lock aGuard( m_aMutex );

        if (!m_pStream)
            throw io::IOException( );

        try
        {
            m_pStream->flush( );
        }
        catch ( const std::ios_base::failure& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Exception caught when calling flush: " << e.what() );
            throw io::IOException( );
        }
    }

    void SAL_CALL StdOutputStream::closeOutput ( )
    {
        std::scoped_lock aGuard( m_aMutex );

        if (!m_pStream)
            throw io::IOException( );

        m_pStream->setstate( std::ios_base::eofbit );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>

#include <osl/diagnose.h>

#include "filinpstr.hxx"
#include "filerror.hxx"

using namespace fileaccess;
using namespace com::sun::star;

#if OSL_DEBUG_LEVEL > 0
#define THROW_WHERE SAL_WHERE
#else
#define THROW_WHERE ""
#endif

XInputStream_impl::XInputStream_impl( const OUString& aUncPath, bool bLock )
    : m_aFile( aUncPath ),
      m_nErrorCode( TaskHandlerErr::NO_ERROR ),
      m_nMinorErrorCode( 0 )
{
    sal_uInt32 nFlags = osl_File_OpenFlag_Read;
    if ( !bLock )
        nFlags |= osl_File_OpenFlag_NoLock;

    osl::FileBase::RC err = m_aFile.open( nFlags );
    if( err != osl::FileBase::E_None )
    {
        m_nIsOpen = false;
        m_aFile.close();

        m_nErrorCode = TaskHandlerErr::OPEN_FOR_INPUTSTREAM;
        m_nMinorErrorCode = err;
    }
    else
        m_nIsOpen = true;
}


XInputStream_impl::~XInputStream_impl()
{
    try
    {
        closeInput();
    }
    catch (io::IOException const &)
    {
        OSL_FAIL("unexpected situation");
    }
    catch (uno::RuntimeException const &)
    {
        OSL_FAIL("unexpected situation");
    }
}

sal_Int32 SAL_CALL
XInputStream_impl::readBytes(
                 uno::Sequence< sal_Int8 >& aData,
                 sal_Int32 nBytesToRead )
{
    if( ! m_nIsOpen ) throw io::IOException( THROW_WHERE );

    aData.realloc(nBytesToRead);
        //TODO! translate memory exhaustion (if it were detectable...) into
        // io::BufferSizeExceededException

    sal_uInt64 nrc(0);
    if(m_aFile.read( aData.getArray(),sal_uInt64(nBytesToRead),nrc )
       != osl::FileBase::E_None)
        throw io::IOException( THROW_WHERE );

    // Shrink aData in case we read less than nBytesToRead (XInputStream
    // documentation does not tell whether this is required, and I do not know
    // if any code relies on this, so be conservative---SB):
    if (sal::static_int_cast<sal_Int32>(nrc) != nBytesToRead)
        aData.realloc(sal_Int32(nrc));
    return static_cast<sal_Int32>(nrc);
}

sal_Int32
XInputStream_impl::readSomeBytes(
                 sal_Int8* aData,
                 sal_Int32 nBytesToRead )
{
    if( ! m_nIsOpen ) throw io::IOException( THROW_WHERE );

        //TODO! translate memory exhaustion (if it were detectable...) into
        // io::BufferSizeExceededException

    sal_uInt64 nrc(0);
    if(m_aFile.read( aData, sal_uInt64(nBytesToRead),nrc )
       != osl::FileBase::E_None)
        throw io::IOException( THROW_WHERE );

    return static_cast<sal_Int32>(nrc);
}

sal_Int32 SAL_CALL
XInputStream_impl::readSomeBytes(
    uno::Sequence< sal_Int8 >& aData,
    sal_Int32 nMaxBytesToRead )
{
    return readBytes( aData,nMaxBytesToRead );
}


void SAL_CALL
XInputStream_impl::skipBytes( sal_Int32 nBytesToSkip )
{
    m_aFile.setPos( osl_Pos_Current, sal_uInt64( nBytesToSkip ) );
}


sal_Int32 SAL_CALL
XInputStream_impl::available()
{
    sal_Int64 avail = getLength() - getPosition();
    return std::min<sal_Int64>(avail, SAL_MAX_INT32);
}


void SAL_CALL
XInputStream_impl::closeInput()
{
    if( m_nIsOpen )
    {
        osl::FileBase::RC err = m_aFile.close();
        if( err != osl::FileBase::E_None )
            throw io::IOException( THROW_WHERE );
        m_nIsOpen = false;
    }
}


void SAL_CALL
XInputStream_impl::seek( sal_Int64 location )
{
    if( location < 0 )
        throw lang::IllegalArgumentException( THROW_WHERE, uno::Reference< uno::XInterface >(), 0 );
    if( osl::FileBase::E_None != m_aFile.setPos( osl_Pos_Absolut, sal_uInt64( location ) ) )
        throw io::IOException( THROW_WHERE );
}


sal_Int64 SAL_CALL
XInputStream_impl::getPosition()
{
    sal_uInt64 uPos;
    if( osl::FileBase::E_None != m_aFile.getPos( uPos ) )
        throw io::IOException( THROW_WHERE );
    return sal_Int64( uPos );
}

sal_Int64 SAL_CALL
XInputStream_impl::getLength()
{
    sal_uInt64 uEndPos;
    if ( m_aFile.getSize(uEndPos) != osl::FileBase::E_None )
        throw io::IOException( THROW_WHERE );
    return sal_Int64( uEndPos );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

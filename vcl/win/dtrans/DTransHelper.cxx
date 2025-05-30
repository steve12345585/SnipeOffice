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

#include <rtl/ustring.h>
#include <osl/diagnose.h>
#include "DTransHelper.hxx"
#include <assert.h>

// implementation

CStgTransferHelper::CStgTransferHelper( bool bAutoInit,
                                        HGLOBAL hGlob,
                                        bool bDelStgOnRelease ) :
    m_lpStream( nullptr ),
    m_bDelStgOnRelease( bDelStgOnRelease )
{
    if ( bAutoInit )
        init( hGlob, m_bDelStgOnRelease );
}

// dtor

CStgTransferHelper::~CStgTransferHelper( )
{
    if ( m_lpStream )
        m_lpStream->Release( );
}

// TransferData into the

void CStgTransferHelper::write( const void* lpData, ULONG cb, ULONG* cbWritten )
{
    HRESULT hr = E_FAIL;

    if ( m_lpStream )
        hr = m_lpStream->Write( lpData, cb, cbWritten );

    if ( FAILED( hr ) )
        throw CStgTransferException( hr );

#if OSL_DEBUG_LEVEL > 0
    HGLOBAL hGlob;
    hr = GetHGlobalFromStream( m_lpStream, &hGlob );
    OSL_ASSERT( SUCCEEDED( hr ) );

    /*DWORD dwSize =*/ GlobalSize( hGlob );
    /*LPVOID lpdbgData =*/ GlobalLock( hGlob );
    GlobalUnlock( hGlob );
#endif
}

// read

void CStgTransferHelper::read( LPVOID pv, ULONG cb, ULONG* pcbRead )
{
    HRESULT hr = E_FAIL;

    if ( m_lpStream )
        hr = m_lpStream->Read( pv, cb , pcbRead );

    if ( FAILED( hr ) )
        throw CStgTransferException( hr );
}

// GetHGlobal

HGLOBAL CStgTransferHelper::getHGlobal( ) const
{
    OSL_ASSERT( m_lpStream );

    HGLOBAL hGlob = nullptr;

    if ( m_lpStream )
    {
        HRESULT hr = GetHGlobalFromStream( m_lpStream, &hGlob );
        if ( FAILED( hr ) )
            hGlob = nullptr;
    }

    return hGlob;
}

// getIStream

void CStgTransferHelper::getIStream( LPSTREAM* ppStream )
{
    assert(ppStream);
    *ppStream = m_lpStream;
    if ( *ppStream )
        static_cast< LPUNKNOWN >( *ppStream )->AddRef( );
}

// Init

void CStgTransferHelper::init( SIZE_T newSize,
                                        sal_uInt32 uiFlags,
                                        bool bDelStgOnRelease )
{
    cleanup( );

    m_bDelStgOnRelease      = bDelStgOnRelease;

    HGLOBAL hGlob = GlobalAlloc( uiFlags, newSize );
    if ( nullptr == hGlob )
        throw CStgTransferException( STG_E_MEDIUMFULL );

    HRESULT hr = CreateStreamOnHGlobal( hGlob, m_bDelStgOnRelease, &m_lpStream );
    if ( FAILED( hr ) )
    {
        GlobalFree( hGlob );
        m_lpStream = nullptr;
        throw CStgTransferException( hr );
    }

#if OSL_DEBUG_LEVEL > 0
    STATSTG statstg;
    hr = m_lpStream->Stat( &statstg, STATFLAG_DEFAULT );
    OSL_ASSERT( SUCCEEDED( hr ) );
#endif
}

// Init

void CStgTransferHelper::init( HGLOBAL hGlob,
                                        bool bDelStgOnRelease )
{
    cleanup( );

    m_bDelStgOnRelease      = bDelStgOnRelease;

    HRESULT hr = CreateStreamOnHGlobal( hGlob, m_bDelStgOnRelease, &m_lpStream );
    if ( FAILED( hr ) )
        throw CStgTransferException( hr );
}

// free the global memory and invalidate the stream pointer

void CStgTransferHelper::cleanup( )
{
    if ( m_lpStream && !m_bDelStgOnRelease )
    {
        HGLOBAL hGlob;
        GetHGlobalFromStream( m_lpStream, &hGlob );
        GlobalFree( hGlob );
    }

    if ( m_lpStream )
    {
        m_lpStream->Release( );
        m_lpStream = nullptr;
    }
}

// return the size of memory we point to

sal_uInt32 CStgTransferHelper::memSize( CLIPFORMAT cf ) const
{
    DWORD dwSize = 0;

    if ( nullptr != m_lpStream )
    {
        HGLOBAL hGlob;
        GetHGlobalFromStream( m_lpStream, &hGlob );

        if ( CF_TEXT == cf || RegisterClipboardFormatW( L"HTML Format" ) == cf )
        {
            char* pText = static_cast< char* >( GlobalLock( hGlob ) );
            if ( pText )
            {
                dwSize = strlen(pText) + 1; // strlen + trailing '\0'
                GlobalUnlock( hGlob );
            }
        }
        else if ( CF_UNICODETEXT == cf )
        {
            sal_Unicode* pText = static_cast< sal_Unicode* >( GlobalLock( hGlob ) );
            if ( pText )
            {
                dwSize = rtl_ustr_getLength( pText ) * sizeof( sal_Unicode );
                GlobalUnlock( hGlob );
            }
        }
        else
            dwSize = GlobalSize( hGlob );
    }

    return dwSize;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

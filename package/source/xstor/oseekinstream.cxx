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

#include <com/sun/star/lang/DisposedException.hpp>
#include <cppuhelper/typeprovider.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <osl/diagnose.h>
#include <sal/log.hxx>

#include "oseekinstream.hxx"
#include "owriteablestream.hxx"

using namespace ::com::sun::star;

OInputSeekStream::OInputSeekStream( OWriteStream_Impl& pImpl,
                                    uno::Reference < io::XInputStream > const & xStream,
                                    const uno::Sequence< beans::PropertyValue >& aProps,
                                    sal_Int32 nStorageType )
    : OInputSeekStream_BASE(pImpl, xStream, aProps, nStorageType)
{
    m_xSeekable.set( m_xStream, uno::UNO_QUERY );
    OSL_ENSURE( m_xSeekable.is(), "No seeking support!" );
}

OInputSeekStream::OInputSeekStream( uno::Reference < io::XInputStream > const & xStream,
                                    const uno::Sequence< beans::PropertyValue >& aProps,
                                    sal_Int32 nStorageType )
    : OInputSeekStream_BASE(xStream, aProps, nStorageType)
{
    m_xSeekable.set( m_xStream, uno::UNO_QUERY );
    OSL_ENSURE( m_xSeekable.is(), "No seeking support!" );
}

OInputSeekStream::~OInputSeekStream()
{
}

void SAL_CALL OInputSeekStream::seek( sal_Int64 location )
{
    ::osl::MutexGuard aGuard( m_xMutex->GetMutex() );
    if ( m_bDisposed )
    {
        SAL_INFO("package.xstor", "Disposed!");
        throw lang::DisposedException();
    }

    if ( !m_xSeekable.is() )
    {
        SAL_INFO("package.xstor", "No seekable!");
        throw uno::RuntimeException();
    }

    m_xSeekable->seek( location );
}

sal_Int64 SAL_CALL OInputSeekStream::getPosition()
{
    ::osl::MutexGuard aGuard( m_xMutex->GetMutex() );
    if ( m_bDisposed )
    {
        SAL_INFO("package.xstor", "Disposed!");
        throw lang::DisposedException();
    }

    if ( !m_xSeekable.is() )
    {
        SAL_INFO("package.xstor", "No seekable!");
        throw uno::RuntimeException();
    }

    return m_xSeekable->getPosition();
}

sal_Int64 SAL_CALL OInputSeekStream::getLength()
{
    ::osl::MutexGuard aGuard( m_xMutex->GetMutex() );
    if ( m_bDisposed )
    {
        SAL_INFO("package.xstor", "Disposed!");
        throw lang::DisposedException();
    }

    if ( !m_xSeekable.is() )
    {
        SAL_INFO("package.xstor", "No seekable!");
        throw uno::RuntimeException();
    }

    return m_xSeekable->getLength();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#pragma once

#include <com/sun/star/uno/Reference.hxx>

#include <osl/conditn.hxx>
#include <tools/stream.hxx>
#include <comphelper/errcode.hxx>
#include <mutex>

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace task
            {
                class XInteractionHandler;
            }
            namespace io
            {
                class XStream;
                class XInputStream;
                class XOutputStream;
                class XSeekable;
            }
            namespace ucb
            {
                class XContent;
            }
            namespace beans
            {
                struct PropertyValue;
            }
        }
    }
}

namespace utl
{
class UcbLockBytes;
typedef tools::SvRef<UcbLockBytes> UcbLockBytesRef;

class UcbLockBytes : public SvLockBytes
{
    osl::Condition          m_aInitialized;
    osl::Condition          m_aTerminated;
    std::mutex              m_aMutex;

    css::uno::Reference < css::io::XInputStream >  m_xInputStream;
    css::uno::Reference < css::io::XOutputStream > m_xOutputStream;
    css::uno::Reference < css::io::XSeekable >     m_xSeekable;

    ErrCode                 m_nError;

    bool                    m_bTerminated;
    bool                    m_bDontClose;
    bool                    m_bStreamValid;

                            UcbLockBytes();
protected:
    virtual                 ~UcbLockBytes() override;

public:
                            // properties: Referer, PostMimeType
    static UcbLockBytesRef  CreateLockBytes( const css::uno::Reference < css::ucb::XContent >& xContent,
                                            const css::uno::Sequence < css::beans::PropertyValue >& rProps,
                                            StreamMode eMode,
                                            const css::uno::Reference < css::task::XInteractionHandler >& xInter );

    static UcbLockBytesRef  CreateInputLockBytes( const css::uno::Reference < css::io::XInputStream >& xContent );
    static UcbLockBytesRef  CreateLockBytes( const css::uno::Reference < css::io::XStream >& xContent );

    // SvLockBytes
    virtual ErrCode         ReadAt(sal_uInt64 nPos, void *pBuffer, std::size_t nCount, std::size_t *pRead) const override;
    virtual ErrCode         WriteAt(sal_uInt64, const void*, std::size_t, std::size_t *pWritten) override;
    virtual ErrCode         Flush() const override;
    virtual ErrCode         SetSize(sal_uInt64) override;
    virtual ErrCode         Stat ( SvLockBytesStat *pStat ) const override;

    void                    SetError( ErrCode nError )
                            { m_nError = nError; }

    ErrCode const &         GetError() const
                            { return m_nError; }

    // calling this method delegates the responsibility to call closeinput to the caller!
    css::uno::Reference < css::io::XInputStream > getInputStream();

    bool                    setInputStream( const css::uno::Reference < css::io::XInputStream > &rxInputStream );
    void                    setStream( const css::uno::Reference < css::io::XStream > &rxStream );
    void                    terminate();

    css::uno::Reference < css::io::XInputStream > getInputStream() const
                            {
                                std::unique_lock aGuard( const_cast< UcbLockBytes* >(this)->m_aMutex );
                                return m_xInputStream;
                            }

    css::uno::Reference < css::io::XOutputStream > getOutputStream() const
                            {
                                std::unique_lock aGuard( const_cast< UcbLockBytes* >(this)->m_aMutex );
                                return m_xOutputStream;
                            }

    css::uno::Reference < css::io::XSeekable > getSeekable() const
                            {
                                std::unique_lock aGuard( const_cast< UcbLockBytes* >(this)->m_aMutex );
                                return m_xSeekable;
                            }

    void                    setDontClose()
                            { m_bDontClose = true; }

    void                    SetStreamValid();

private:
    bool                    setInputStreamImpl( std::unique_lock<std::mutex>& rGuard,
                                                 const css::uno::Reference < css::io::XInputStream > &rxInputStream,
                                                 bool bSetXSeekable = true );
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

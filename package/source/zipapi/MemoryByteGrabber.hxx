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
#ifndef INCLUDED_PACKAGE_SOURCE_ZIPAPI_MEMORYBYTEGRABBER_HXX
#define INCLUDED_PACKAGE_SOURCE_ZIPAPI_MEMORYBYTEGRABBER_HXX

#include <com/sun/star/uno/Sequence.h>

class MemoryByteGrabber final
{
    const sal_Int8 *mpBuffer;
    sal_Int32 mnCurrent, mnEnd;
public:
    MemoryByteGrabber ( const css::uno::Sequence < sal_Int8 > & rBuffer )
    : mpBuffer ( rBuffer.getConstArray() )
    , mnCurrent ( 0 )
    , mnEnd ( rBuffer.getLength() )
    {
    }
    MemoryByteGrabber ( const sal_Int8* pBuffer, sal_Int32 nBufLen )
    : mpBuffer ( pBuffer )
    , mnCurrent ( 0 )
    , mnEnd ( nBufLen )
    {
    }
    MemoryByteGrabber(css::uno::Sequence<sal_Int8> &&) = delete;

    const sal_Int8 * getCurrentPos () const { return mpBuffer + mnCurrent; }

    sal_Int32 remainingSize() const { return mnEnd - mnCurrent; }

    // XInputStream chained

    /// @throws css::io::NotConnectedException
    /// @throws css::io::BufferSizeExceededException
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    void skipBytes( sal_Int32 nBytesToSkip )
    {
        mnCurrent += nBytesToSkip;
    }

    sal_Int8 ReadUInt8()
    {
        if (mnCurrent + 1 > mnEnd)
            return 0;
        sal_uInt8 nInt8 = mpBuffer[mnCurrent++];
        return nInt8;
    }

    // XSeekable chained...
    sal_Int16 ReadInt16()
    {
        if (mnCurrent + 2 > mnEnd )
            return 0;
        sal_Int16 nInt16  =   mpBuffer[mnCurrent++] & 0xFF;
        nInt16 |= ( mpBuffer[mnCurrent++] & 0xFF ) << 8;
        return nInt16;
    }

    sal_Int16 ReadUInt16()
    {
        if (mnCurrent + 2 > mnEnd )
            return 0;
        sal_uInt16 nInt16  =  mpBuffer[mnCurrent++] & 0xFF;
        nInt16 |= ( mpBuffer[mnCurrent++] & 0xFF ) << 8;
        return nInt16;
    }

    sal_Int32 ReadInt32()
    {
        if (mnCurrent + 4 > mnEnd )
            return 0;

        sal_Int32 nInt32  =   mpBuffer[mnCurrent++] & 0xFF;
        nInt32 |= ( mpBuffer[mnCurrent++] & 0xFF ) << 8;
        nInt32 |= ( mpBuffer[mnCurrent++] & 0xFF ) << 16;
        nInt32 |= ( mpBuffer[mnCurrent++] & 0xFF ) << 24;
        return nInt32;
    }

    sal_uInt32 ReadUInt32()
    {
        if (mnCurrent + 4 > mnEnd )
            return 0;

        sal_uInt32 nInt32  =   mpBuffer [mnCurrent++] & 0xFF;
        nInt32 |= ( mpBuffer [mnCurrent++] & 0xFF ) << 8;
        nInt32 |= ( mpBuffer [mnCurrent++] & 0xFF ) << 16;
        nInt32 |= ( mpBuffer [mnCurrent++] & 0xFF ) << 24;
        return nInt32;
    }

    sal_uInt64 ReadUInt64()
    {
        if (mnCurrent + 8 > mnEnd)
            return 0;

        sal_uInt64 nInt64 = mpBuffer[mnCurrent++] & 0xFF;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 8;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 16;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 24;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 32;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 40;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 48;
        nInt64 |= static_cast<sal_Int64>(mpBuffer[mnCurrent++] & 0xFF) << 56;
        return nInt64;
    }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

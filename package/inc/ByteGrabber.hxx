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
#ifndef INCLUDED_PACKAGE_INC_BYTEGRABBER_HXX
#define INCLUDED_PACKAGE_INC_BYTEGRABBER_HXX

#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/uno/Reference.h>
#include <comphelper/bytereader.hxx>
#include <array>

namespace com::sun::star {
    namespace io { class XSeekable; class XInputStream; }
}
class ByteGrabber final
{
    css::uno::Reference < css::io::XInputStream > xStream;
    css::uno::Reference < css::io::XSeekable > xSeek;
    comphelper::ByteReader* mpByteReader;
    std::array<sal_Int8, 8> maBuffer;

public:
    ByteGrabber (css::uno::Reference < css::io::XInputStream > const & xIstream);
    ~ByteGrabber();

    void setInputStream (const css::uno::Reference < css::io::XInputStream >& xNewStream);
    // XInputStream
    /// @throws css::io::NotConnectedException
    /// @throws css::io::BufferSizeExceededException
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    sal_Int32 readBytes( sal_Int8* aData, sal_Int32 nBytesToRead );
    // XSeekable
    /// @throws css::lang::IllegalArgumentException
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    void seek( sal_Int64 location );
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    sal_Int64 getPosition(  );
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    sal_Int64 getLength(  );

    sal_uInt16 ReadUInt16();
    sal_uInt32 ReadUInt32();
    sal_uInt64 ReadUInt64();
    sal_Int16 ReadInt16()
    {
        return static_cast<sal_Int16>(ReadUInt16());
    }
    sal_Int32 ReadInt32()
    {
        return static_cast<sal_Int32>(ReadUInt32());
    }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

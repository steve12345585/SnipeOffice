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

#ifndef INCLUDED_OOX_HELPER_BINARYINPUTSTREAM_HXX
#define INCLUDED_OOX_HELPER_BINARYINPUTSTREAM_HXX

#include <cstddef>
#include <memory>
#include <vector>

#include <com/sun/star/uno/Reference.hxx>
#include <oox/dllapi.h>
#include <oox/helper/binarystreambase.hxx>
#include <oox/helper/helper.hxx>
#include <rtl/string.hxx>
#include <rtl/textenc.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace io { class XInputStream; }
}

namespace oox {

class BinaryOutputStream;


/** Interface for binary input stream classes.

    The binary data in the stream is assumed to be in little-endian format.
 */
class OOX_DLLPUBLIC BinaryInputStream : public virtual BinaryStreamBase
{
public:
    /** Derived classes implement reading nBytes bytes to the passed sequence.
        The sequence will be reallocated internally.

        @param nAtomSize
            The size of the elements in the memory block, if available. Derived
            classes may be interested in this information.

        @return
            Number of bytes really read.
     */
    virtual sal_Int32   readData( StreamDataSequence& orData, sal_Int32 nBytes, size_t nAtomSize = 1 ) = 0;

    /** Derived classes implement reading nBytes bytes to the (preallocated!)
        memory buffer opMem.

        @param nAtomSize
            The size of the elements in the memory block, if available. Derived
            classes may be interested in this information.

        @return
            Number of bytes really read.
     */
    virtual sal_Int32   readMemory( void* opMem, sal_Int32 nBytes, size_t nAtomSize = 1 ) = 0;

    /** Derived classes implement seeking the stream forward by the passed
        number of bytes. This should work for non-seekable streams too.

        @param nAtomSize
            The size of the elements in the memory block, if available. Derived
            classes may be interested in this information.
     */
    virtual void        skip( sal_Int32 nBytes, size_t nAtomSize = 1 ) = 0;

    /** Reads a value from the stream and converts it to platform byte order.
        All data types supported by the ByteOrderConverter class can be used.
     */
    template< typename Type >
    [[nodiscard]]
    Type                 readValue();

    [[nodiscard]]
    sal_Int8             readInt8()   { return readValue<sal_Int8>(); }
    [[nodiscard]]
    sal_uInt8            readuInt8()  { return readValue<sal_uInt8>(); }
    [[nodiscard]]
    sal_Int16            readInt16()  { return readValue<sal_Int16>(); }
    [[nodiscard]]
    sal_uInt16           readuInt16() { return readValue<sal_uInt16>(); }
    [[nodiscard]]
    sal_Int32            readInt32()  { return readValue<sal_Int32>(); }
    [[nodiscard]]
    sal_uInt32           readuInt32() { return readValue<sal_uInt32>(); }
    [[nodiscard]]
    sal_Int64            readInt64()  { return readValue<sal_Int64>(); }
    [[nodiscard]]
    float                readFloat()  { return readValue<float>(); }
    [[nodiscard]]
    double               readDouble() { return readValue<double>(); }
    [[nodiscard]]
    unsigned char        readuChar()  { return readValue<unsigned char>(); }

    /** Reads a (preallocated!) C array of values from the stream.

        Converts all values in the array to platform byte order. All data types
        supported by the ByteOrderConverter class can be used.

        @param nElemCount
            Number of array elements to read (NOT byte count).

        @return
            Number of array elements really read (NOT byte count).
     */
    template< typename Type >
    sal_Int32           readArray( Type* opnArray, sal_Int32 nElemCount );

    /** Reads a vector of values from the stream.

        The vector will be resized internally. Converts all values in the
        vector to platform byte order. All data types supported by the
        ByteOrderConverter class can be used.

        @param nElemCount
            Number of elements to put into the vector (NOT byte count).

        @return
            Number of vector elements really read (NOT byte count).
     */
    template< typename Type >
    sal_Int32           readArray( ::std::vector< Type >& orVector, sal_Int32 nElemCount );

    /** Reads a NUL-terminated Unicode character array and returns the string.
     */
    OUString     readNulUnicodeArray();

    /** Reads a byte character array and returns the string.
        NUL characters are replaced by question marks.

        @param nChars
            Number of characters (bytes) to read from the stream.
     */
    OString      readCharArray( sal_Int32 nChars );

    /** Reads a byte character array and returns a Unicode string.
        NUL characters are replaced by question marks.

        @param nChars
            Number of characters (bytes) to read from the stream.

        @param eTextEnc
            The text encoding used to create the Unicode string.
     */
    OUString     readCharArrayUC( sal_Int32 nChars, rtl_TextEncoding eTextEnc );

    /** Reads a Unicode character array and returns the string.
        NUL characters are replaced by question marks (default).

        @param nChars
            Number of 16-bit characters to read from the stream.
     */
    OUString     readUnicodeArray( sal_Int32 nChars );

    /** Reads a Unicode character array (may be compressed) and returns the
        string.
        NUL characters are replaced by question marks (default).

        @param nChars
            Number of 8-bit or 16-bit characters to read from the stream.

        @param bCompressed
            True = Character array is compressed (stored as 8-bit characters).
            False = Character array is not compressed (stored as 16-bit characters).
     */
    OUString     readCompressedUnicodeArray( sal_Int32 nChars, bool bCompressed );

    /** Copies bytes from the current position to the passed output stream.
     */
    void         copyToStream( BinaryOutputStream& rOutStrm );

protected:
    BinaryInputStream() = default;

private:
    BinaryInputStream( BinaryInputStream const& ) = delete;
    BinaryInputStream& operator=( BinaryInputStream const& ) = delete;
};

typedef std::shared_ptr< BinaryInputStream > BinaryInputStreamRef;


template< typename Type >
Type BinaryInputStream::readValue()
{
    Type ornValue = Type();
    readMemory( &ornValue, static_cast< sal_Int32 >( sizeof( Type ) ), sizeof( Type ) );
    ByteOrderConverter::convertLittleEndian( ornValue );
    return ornValue;
}

template< typename Type >
sal_Int32 BinaryInputStream::readArray( Type* opnArray, sal_Int32 nElemCount )
{
    sal_Int32 nRet = 0;
    if( !mbEof )
    {
        sal_Int32 nReadSize = getLimitedValue< sal_Int32, sal_Int32 >( nElemCount, 0, SAL_MAX_INT32 / sizeof( Type ) ) * sizeof( Type );
        nRet = readMemory( opnArray, nReadSize, sizeof( Type ) ) / sizeof( Type );
        ByteOrderConverter::convertLittleEndianArray( opnArray, static_cast< size_t >( nRet ) );
    }
    return nRet;
}

template< typename Type >
sal_Int32 BinaryInputStream::readArray( ::std::vector< Type >& orVector, sal_Int32 nElemCount )
{
    orVector.resize( static_cast< size_t >( nElemCount ) );
    return orVector.empty() ? 0 : readArray(orVector.data(), nElemCount);
}


/** Wraps a UNO input stream and provides convenient access functions.

    The binary data in the stream is assumed to be in little-endian format.
 */
class OOX_DLLPUBLIC BinaryXInputStream final : public BinaryXSeekableStream, public BinaryInputStream
{
public:
    /** Constructs the wrapper object for the passed input stream.

        @param rxInStream
            The com.sun.star.io.XInputStream interface of the UNO input stream
            to be wrapped.

        @param bAutoClose
            True = automatically close the wrapped input stream on destruction
            of this wrapper or when close() is called.
     */
    explicit            BinaryXInputStream(
                            const css::uno::Reference< css::io::XInputStream >& rxInStrm,
                            bool bAutoClose );

    virtual             ~BinaryXInputStream() override;

    /** Closes the input stream. Does also close the wrapped UNO input stream
        if bAutoClose has been set to true in the constructor. */
    virtual void        close() override;

    /** Reads nBytes bytes to the passed sequence.
        @return  Number of bytes really read. */
    virtual sal_Int32   readData( StreamDataSequence& orData, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Reads nBytes bytes to the (existing) buffer opMem.
        @return  Number of bytes really read. */
    virtual sal_Int32   readMemory( void* opMem, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Seeks the stream forward by the passed number of bytes. This works for
        non-seekable streams too. */
    virtual void        skip( sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

private:
    StreamDataSequence  maBuffer;       ///< Data buffer used in readMemory() function.
    css::uno::Reference< css::io::XInputStream >
                        mxInStrm;       ///< Reference to the input stream.
    bool                mbAutoClose;    ///< True = automatically close stream on destruction.
};


/** Wraps a StreamDataSequence and provides convenient access functions.

    The binary data in the stream is assumed to be in little-endian format.
 */
class OOX_DLLPUBLIC SequenceInputStream final : public SequenceSeekableStream, public BinaryInputStream
{
public:
    /** Constructs the wrapper object for the passed data sequence.

        @attention
            The passed data sequence MUST live at least as long as this stream
            wrapper. The data sequence MUST NOT be changed from outside as long
            as this stream wrapper is used to read from it.
     */
    explicit            SequenceInputStream( const StreamDataSequence& rData );

    /** Reads nBytes bytes to the passed sequence.
        @return  Number of bytes really read. */
    virtual sal_Int32   readData( StreamDataSequence& orData, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Reads nBytes bytes to the (existing) buffer opMem.
        @return  Number of bytes really read. */
    virtual sal_Int32   readMemory( void* opMem, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Seeks the stream forward by the passed number of bytes. This works for
        non-seekable streams too. */
    virtual void        skip( sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

private:
    /** Returns the number of bytes available in the sequence for the passed byte count. */
    sal_Int32    getMaxBytes( sal_Int32 nBytes ) const
                            { return getLimitedValue< sal_Int32, sal_Int32 >( nBytes, 0, mpData->getLength() - mnPos ); }
};


/** Wraps a BinaryInputStream and provides access to a specific part of the
    stream data.

    Provides access to the stream data block starting at the current position
    of the stream, and with a specific length. If the wrapped stream is
    seekable, this wrapper will treat the position of the wrapped stream at
    construction time as position "0" (therefore the class name).

    The passed input stream MUST live at least as long as this stream wrapper.
    The stream MUST NOT be changed from outside as long as this stream wrapper
    is used to read from it.
 */
class RelativeInputStream final : public BinaryInputStream
{
public:
    /** Constructs the wrapper object for the passed stream.

        @param nSize
            If specified, restricts the amount of data that can be read from
            the passed input stream.
     */
    explicit            RelativeInputStream(
                            BinaryInputStream& rInStrm,
                            sal_Int64 nSize );

    /** Returns the size of the data block in the wrapped stream offered by
        this wrapper. */
    virtual sal_Int64   size() const override;

    /** Returns the current relative stream position. */
    virtual sal_Int64   tell() const override;

    /** Seeks the stream to the passed relative position, if the wrapped stream
        is seekable. */
    virtual void        seek( sal_Int64 nPos ) override;

    /** Closes the input stream but not the wrapped stream. */
    virtual void        close() override;

    /** Reads nBytes bytes to the passed sequence. Does not read out of the
        data block whose size has been specified on construction.
        @return  Number of bytes really read. */
    virtual sal_Int32   readData( StreamDataSequence& orData, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Reads nBytes bytes to the (existing) buffer opMem. Does not read out of
        the data block whose size has been specified on construction.
        @return  Number of bytes really read. */
    virtual sal_Int32   readMemory( void* opMem, sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

    /** Seeks the stream forward by the passed number of bytes. This works for
        non-seekable streams too. Does not seek out of the data block. */
    virtual void        skip( sal_Int32 nBytes, size_t nAtomSize = 1 ) override;

private:
    /** Returns the number of bytes available in the sequence for the passed byte count. */
    sal_Int32    getMaxBytes( sal_Int32 nBytes ) const
                            { return getLimitedValue< sal_Int32, sal_Int64 >( nBytes, 0, mnSize - mnRelPos ); }

private:
    BinaryInputStream*  mpInStrm;
    sal_Int64           mnStartPos;
    sal_Int64           mnRelPos;
    sal_Int64           mnSize;
};


} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

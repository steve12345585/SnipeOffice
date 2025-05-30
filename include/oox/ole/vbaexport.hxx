/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_OOX_OLE_VBAEXPORT_HXX
#define INCLUDED_OOX_OLE_VBAEXPORT_HXX

#include <cstddef>

#include <com/sun/star/uno/Reference.hxx>
#include <oox/dllapi.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>

class SotStorage;
class SvStream;
class SvMemoryStream;

namespace com::sun::star {
    namespace container { class XNameContainer; }
    namespace frame { class XModel; }
    namespace script { class XLibraryContainer; }
}

class OOX_DLLPUBLIC VbaExport
{
public:
    VbaExport(css::uno::Reference<css::frame::XModel> xModel);

    void exportVBA(SotStorage* pRootStorage);

    bool containsVBAProject();

private:

    css::uno::Reference<css::container::XNameContainer>
        getBasicLibrary() const;

    css::uno::Reference<css::script::XLibraryContainer>
        getLibraryContainer() const;

    OUString getProjectName() const;
    rtl_TextEncoding getVBATextEncoding() const;

    css::uno::Reference<css::frame::XModel> mxModel;
};

class VBACompressionChunk
{
public:

    VBACompressionChunk(SvStream& rCompressedStream, const sal_uInt8* pData, std::size_t nChunkSize);

    void write();

private:
    SvStream& mrCompressedStream;
    const sal_uInt8* mpUncompressedData;
    sal_uInt8* mpCompressedChunkStream;

    // same as DecompressedChunkEnd in the spec
    std::size_t mnChunkSize;

    // CompressedCurrent according to the spec
    sal_uInt64 mnCompressedCurrent;

    // CompressedEnd according to the spec
    sal_uInt64 mnCompressedEnd;

    // DecompressedCurrent according to the spec
    sal_uInt64 mnDecompressedCurrent;

    // DecompressedEnd according to the spec
    sal_uInt64 mnDecompressedEnd;

    static void PackCompressedChunkSize(size_t nSize, sal_uInt16& rHeader);

    static void PackCompressedChunkFlag(bool bCompressed, sal_uInt16& rHeader);

    static void PackCompressedChunkSignature(sal_uInt16& rHeader);

    void compressTokenSequence();

    void compressToken(size_t index, sal_uInt8& nFlagByte);

    static void SetFlagBit(size_t index, bool bVal, sal_uInt8& rFlag);

    sal_uInt16 CopyToken(size_t nLength, size_t nOffset);

    void match(size_t& rLength, size_t& rOffset);

    void CopyTokenHelp(sal_uInt16& rLengthMask, sal_uInt16& rOffsetMask,
            sal_uInt16& rBitCount, sal_uInt16& rMaximumLength);

    void writeRawChunk();

    sal_uInt16 handleHeader(bool bCompressed);
};

class OOX_DLLPUBLIC VBACompression
{
public:
    VBACompression(SvStream& rCompressedStream,
            SvMemoryStream& rUncompressedStream);

    void write();

private:
    SvStream& mrCompressedStream;
    SvMemoryStream& mrUncompressedStream;
};

class OOX_DLLPUBLIC VBAEncryption
{
public:
    VBAEncryption(const sal_uInt8* pData,
                  const sal_uInt16 nLength,
                  SvStream& rEncryptedData,
                  sal_uInt8 nProjKey,
                  rtl_TextEncoding eTextEncoding);

    void write();

    static sal_uInt8 calculateProjKey(const OUString& rString);

private:
    const sal_uInt8* mpData; // an array of bytes to be obfuscated
    const sal_uInt16 mnLength; // the length of Data
    SvStream& mrEncryptedData; // Encrypted Data Structure
    sal_uInt8 mnUnencryptedByte1; // the last unencrypted byte read or written
    sal_uInt8 mnEncryptedByte1; // the last encrypted byte read or written
    sal_uInt8 mnEncryptedByte2; // the next-to-last encrypted byte read or written
    sal_Unicode mnProjKey; // a project-specific encryption key
    sal_uInt8 mnIgnoredLength; // the length in bytes of IgnoredEnc

    sal_uInt8 mnSeed; // the seed value
    sal_uInt8 mnVersionEnc; // the version encoding
    rtl_TextEncoding meTextEncoding; // the VBA text encoding on export

    void writeSeed();
    void writeVersionEnc();
    void writeProjKeyEnc();
    void writeIgnoredEnc();
    void writeDataLengthEnc();
    void writeDataEnc();
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

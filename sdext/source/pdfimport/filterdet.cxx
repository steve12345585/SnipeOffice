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


#include "filterdet.hxx"
#include "inc/pdfihelper.hxx"
#include "inc/pdfparse.hxx"

#include <osl/file.h>
#include <osl/thread.h>
#include <rtl/digest.h>
#include <sal/log.hxx>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <comphelper/fileurl.hxx>
#include <comphelper/hash.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/stream.hxx>
#include <vcl/filter/PDFiumLibrary.hxx>
#include <memory>
#include <utility>
#include <string.h>

using namespace com::sun::star;

namespace pdfi
{

// TODO(T3): locking/thread safety

namespace {

class FileEmitContext : public pdfparse::EmitContext
{
private:
    oslFileHandle                        m_aReadHandle;
    unsigned int                         m_nReadLen;
    uno::Reference< io::XStream >        m_xContextStream;
    uno::Reference< io::XSeekable >      m_xSeek;
    uno::Reference< io::XOutputStream >  m_xOut;

public:
    FileEmitContext( const OUString&                            rOrigFile,
                     const uno::Reference< uno::XComponentContext >& xContext,
                     const pdfparse::PDFContainer*                   pTop );
    virtual ~FileEmitContext() override;

    virtual bool         write( const void* pBuf, unsigned int nLen ) override;
    virtual unsigned int getCurPos() override;
    virtual bool         copyOrigBytes( unsigned int nOrigOffset, unsigned int nLen ) override;
    virtual unsigned int readOrigBytes( unsigned int nOrigOffset, unsigned int nLen, void* pBuf ) override;

    const uno::Reference< io::XStream >& getContextStream() const { return m_xContextStream; }
};

}

FileEmitContext::FileEmitContext( const OUString&                            rOrigFile,
                                  const uno::Reference< uno::XComponentContext >& xContext,
                                  const pdfparse::PDFContainer*                   pTop ) :
    pdfparse::EmitContext( pTop ),
    m_aReadHandle(nullptr),
    m_nReadLen(0)
{
    m_xContextStream.set( io::TempFile::create(xContext), uno::UNO_QUERY_THROW );
    m_xOut = m_xContextStream->getOutputStream();
    m_xSeek.set(m_xOut, uno::UNO_QUERY_THROW );

    if( osl_openFile( rOrigFile.pData,
                      &m_aReadHandle,
                      osl_File_OpenFlag_Read ) == osl_File_E_None )
    {
        oslFileError aErr = osl_setFilePos( m_aReadHandle, osl_Pos_End, 0 );
        if( aErr == osl_File_E_None )
        {
            sal_uInt64 nFileSize = 0;
            if( (aErr=osl_getFilePos( m_aReadHandle,
                                      &nFileSize )) == osl_File_E_None )
            {
                m_nReadLen = static_cast<unsigned int>(nFileSize);
            }
        }
        if( aErr != osl_File_E_None )
        {
            osl_closeFile( m_aReadHandle );
            m_aReadHandle = nullptr;
        }
    }
    m_bDeflate = true;
}

FileEmitContext::~FileEmitContext()
{
    if( m_aReadHandle )
        osl_closeFile( m_aReadHandle );
}

bool FileEmitContext::write( const void* pBuf, unsigned int nLen )
{
    if( ! m_xOut.is() )
        return false;

    uno::Sequence< sal_Int8 > aSeq( nLen );
    memcpy( aSeq.getArray(), pBuf, nLen );
    m_xOut->writeBytes( aSeq );
    return true;
}

unsigned int FileEmitContext::getCurPos()
{
    unsigned int nPos = 0;
    if( m_xSeek.is() )
    {
        nPos = static_cast<unsigned int>( m_xSeek->getPosition() );
    }
    return nPos;
}

bool FileEmitContext::copyOrigBytes( unsigned int nOrigOffset, unsigned int nLen )
{
    if( nOrigOffset + nLen > m_nReadLen )
        return false;

    if( osl_setFilePos( m_aReadHandle, osl_Pos_Absolut, nOrigOffset ) != osl_File_E_None )
        return false;

    uno::Sequence< sal_Int8 > aSeq( nLen );

    sal_uInt64 nBytesRead = 0;
    if( osl_readFile( m_aReadHandle,
                      aSeq.getArray(),
                      nLen,
                      &nBytesRead ) != osl_File_E_None
        || nBytesRead != static_cast<sal_uInt64>(nLen) )
    {
        return false;
    }

    m_xOut->writeBytes( aSeq );
    return true;
}

unsigned int FileEmitContext::readOrigBytes( unsigned int nOrigOffset, unsigned int nLen, void* pBuf )
{
    if( nOrigOffset + nLen > m_nReadLen )
        return 0;

    if( osl_setFilePos( m_aReadHandle,
                        osl_Pos_Absolut,
                        nOrigOffset ) != osl_File_E_None )
    {
        return 0;
    }

    sal_uInt64 nBytesRead = 0;
    if( osl_readFile( m_aReadHandle,
                      pBuf,
                      nLen,
                      &nBytesRead ) != osl_File_E_None )
    {
        return 0;
    }
    return static_cast<unsigned int>(nBytesRead);
}


PDFDetector::PDFDetector( uno::Reference< uno::XComponentContext > xContext) :
    m_xContext(std::move( xContext ))
{}

namespace
{

sal_Int32 fillAttributes(uno::Sequence<beans::PropertyValue> const& rFilterData, uno::Reference<io::XInputStream>& xInput, OUString& aURL, sal_Int32& nFilterNamePos, sal_Int32& nPasswordPos, OUString& aPassword)
{
    const beans::PropertyValue* pAttribs = rFilterData.getConstArray();
    sal_Int32 nAttribs = rFilterData.getLength();
    for (sal_Int32 i = 0; i < nAttribs; i++)
    {
        OUString aVal( u"<no string>"_ustr );
        pAttribs[i].Value >>= aVal;
        SAL_INFO("sdext.pdfimport", "doDetection: Attrib: " + pAttribs[i].Name + " = " + aVal);

        if (pAttribs[i].Name == "InputStream")
            pAttribs[i].Value >>= xInput;
        else if (pAttribs[i].Name == "URL")
            pAttribs[i].Value >>= aURL;
        else if (pAttribs[i].Name == "FilterName")
            nFilterNamePos = i;
        else if (pAttribs[i].Name == "Password")
        {
            nPasswordPos = i;
            pAttribs[i].Value >>= aPassword;
        }
    }
    return nAttribs;
}

// read the first 1024 byte (see PDF reference implementation note 12)
constexpr const sal_Int32 constHeaderSize = 1024;

bool detectPDF(uno::Reference<io::XInputStream> const& xInput, uno::Sequence<sal_Int8>& aHeader, sal_uInt64& nHeaderReadSize)
{
    try
    {
        uno::Reference<io::XSeekable> xSeek(xInput, uno::UNO_QUERY);
        if (xSeek.is())
            xSeek->seek(0);

        nHeaderReadSize = xInput->readBytes(aHeader, constHeaderSize);
        if (nHeaderReadSize <= 5)
            return false;

        const sal_Int8* pBytes = aHeader.getConstArray();
        for (sal_uInt64 i = 0; i < nHeaderReadSize - 5; i++)
        {
            if (pBytes[i+0] == '%' &&
                pBytes[i+1] == 'P' &&
                pBytes[i+2] == 'D' &&
                pBytes[i+3] == 'F' &&
                pBytes[i+4] == '-')
            {
                return true;
            }
        }
    }
    catch (const css::io::IOException &)
    {
        TOOLS_WARN_EXCEPTION("sdext.pdfimport", "caught");
    }
    return false;
}

bool copyToTemp(uno::Reference<io::XInputStream> const& xInput, oslFileHandle& rFileHandle, uno::Sequence<sal_Int8> const& aHeader, sal_uInt64 nHeaderReadSize)
{
    try
    {
        sal_uInt64 nWritten = 0;
        osl_writeFile(rFileHandle, aHeader.getConstArray(), nHeaderReadSize, &nWritten);

        const sal_uInt64 nBufferSize = 4096;
        uno::Sequence<sal_Int8> aBuffer(nBufferSize);

        // copy the bytes
        sal_uInt64 nRead = 0;
        do
        {
            nRead = xInput->readBytes(aBuffer, nBufferSize);
            if (nRead > 0)
            {
                osl_writeFile(rFileHandle, aBuffer.getConstArray(), nRead, &nWritten);
                if (nWritten != nRead)
                    return false;
            }
        }
        while (nRead == nBufferSize);
    }
    catch (const css::io::IOException &)
    {
        TOOLS_WARN_EXCEPTION("sdext.pdfimport", "caught");
    }
    return false;
}

struct FilenameMime {
    OUString aFilename;
    OUString aMimetype;
};

constexpr FilenameMime aFilenameMimeMap[] = {
    { u"Original.odt"_ustr, u"application/vnd.oasis.opendocument.text"_ustr },
    { u"Original.odp"_ustr, u"application/vnd.oasis.opendocument.presentation"_ustr },
    { u"Original.ods"_ustr, u"application/vnd.oasis.opendocument.spreadsheet"_ustr },
    { u"Original.odg"_ustr, u"application/vnd.oasis.opendocument.graphics"_ustr },
};

} // end anonymous namespace

// Check for a hybrid that is stored using the newer method, the standard PDF embedded file
// with a name of Original.o** and the matching MIME type.  For this to match there must
// be exactly one embedded file.
// This uses PDFium to do the legwork.
uno::Reference<io::XStream> getEmbeddedFile(const OUString& rInPDFFileURL,
                                            OUString& rOutMimetype,
                                            OUString& io_rPwd,
                                            const uno::Reference<uno::XComponentContext>& xContext,
                                            const uno::Sequence<beans::PropertyValue>& rFilterData,
                                            bool bMayUseUI)
{
    uno::Reference<io::XStream> xEmbed;
    OUString aSysUPath;
    auto pPdfium = vcl::pdf::PDFiumLibrary::get();
    if (pPdfium)
    {
        // Needs rewriting more C++ with autocleanup
        // Start by mmapping the file because our pdfium wrapper only wraps the LoadMemDocument
        oslFileHandle fileHandle = nullptr;
        SAL_INFO("sdext.pdfimport", "getEmbeddedFile prior to openFile" << aSysUPath);
        if (osl_openFile(rInPDFFileURL.pData, &fileHandle, osl_File_OpenFlag_Read)
            != osl_File_E_None)
        {
            return xEmbed;
        }

        sal_uInt64 nFileSize;
        if (osl_getFileSize(fileHandle, &nFileSize) != osl_File_E_None)
        {
            osl_closeFile(fileHandle);
            return xEmbed;
        }

        void* pMemRawPdf;
        if (osl_mapFile(fileHandle, &pMemRawPdf, nFileSize, 0, osl_File_MapFlag_RandomAccess)
            != osl_File_E_None)
        {
            osl_closeFile(fileHandle);
            return xEmbed;
        }

        bool bAgain = false;
        do {
            OString aIsoPwd = OUStringToOString(io_rPwd, RTL_TEXTENCODING_ISO_8859_1);
            auto pPdfiumDoc = pPdfium->openDocument(pMemRawPdf, nFileSize, aIsoPwd);
            SAL_INFO("sdext.pdfimport", "getEmbeddedFile pdfium docptr: " << pPdfiumDoc);

            auto nPdfiumErr = pPdfium->getLastErrorCode();
            if (pPdfiumDoc == nullptr
                && (nPdfiumErr != vcl::pdf::PDFErrorType::Success
                    && nPdfiumErr != vcl::pdf::PDFErrorType::Password))
            {
                SAL_WARN("sdext.pdfimport",
                         "getEmbeddedFile pdfium err: " << pPdfium->getLastError());
                break;
            }
            if (pPdfiumDoc == nullptr && nPdfiumErr == vcl::pdf::PDFErrorType::Password)
            {
                uno::Reference<task::XInteractionHandler> xIntHdl;
                for (const beans::PropertyValue& rAttrib : rFilterData)
                {
                    if (rAttrib.Name == "InteractionHandler")
                        rAttrib.Value >>= xIntHdl;
                }
                SAL_INFO("sdext.pdfimport",
                         "getEmbeddedFile pdfium Pass needed: UI: " << bMayUseUI);
                if (bMayUseUI && xIntHdl.is())
                {
                    OUString aDocName(rInPDFFileURL.copy(rInPDFFileURL.lastIndexOf('/') + 1));
                    bAgain = getPassword(xIntHdl, io_rPwd, !bAgain, aDocName);
                    SAL_INFO("sdext.pdfimport", "getEmbeddedFile pdfium Pass result: " << bAgain);
                    continue;
                }
                break;
            }
            bAgain = false;
            // The new style hybrids have exactly one embedded file
            if (pPdfiumDoc->getAttachmentCount() != 1)
            {
                SAL_INFO("sdext.pdfimport", "getEmbeddedFile incorrect attachment count");
                break;
            }
            auto pAttachment = pPdfiumDoc->getAttachment(0);
            auto aName = pAttachment->getName();
            // pdfium currently has no way to read the MIME type (aka Subtype field)
            // see https://issues.chromium.org/issues/408241034
            // When it does we can check the filename matches the expected mimetype
            SAL_INFO("sdext.pdfimport", "getEmbeddedFile attachment name: " << aName);

            // Find the mimetype for the filename
            OUString aMimetype;

            for (auto& rFM : aFilenameMimeMap)
            {
                if (rFM.aFilename == aName)
                {
                    aMimetype = rFM.aMimetype;
                    break;
                }
            }
            SAL_INFO("sdext.pdfimport", "getEmbeddedFile mimetype: " << aMimetype);
            // If we don't match, then this is a non-hybrid file with a normal attachment
            if (aMimetype.isEmpty())
            {
                break;
            }

            SAL_INFO("sdext.pdfimport", "getEmbeddedFile pdfium open");
            std::vector<sal_uInt8> aExtractedFileBuf;
            if (!pAttachment->getFile(aExtractedFileBuf))
            {
                break;
            }
            SAL_INFO("sdext.pdfimport", "getEmbeddedFile file buffer length: " << aExtractedFileBuf.size());
            // Based on FileEmitContext above, we want to stash the data in a TempFile
            // but need an XStream
            uno::Reference<io::XStream> xContextStream;
            uno::Reference<io::XSeekable> xSeek;
            xContextStream.set(io::TempFile::create(xContext), uno::UNO_QUERY_THROW);
            auto xOut = xContextStream->getOutputStream();
            xSeek.set(xOut, uno::UNO_QUERY_THROW);
            // writeBytes wants a Uno::Sequence rather than the std::vector above, convert again
            uno::Sequence<sal_Int8> aExtractedFileSeq(reinterpret_cast<sal_Int8 *>(aExtractedFileBuf.data()), aExtractedFileBuf.size());
            xOut->writeBytes(aExtractedFileSeq);

            xEmbed = xContextStream;
            rOutMimetype = aMimetype;
            SAL_INFO("sdext.pdfimport", "getEmbeddedFile returning stream");
        } while (bAgain);

        osl_unmapMappedFile(fileHandle, pMemRawPdf, nFileSize);
        osl_closeFile(fileHandle);
    }

    return xEmbed;
}
// XExtendedFilterDetection
OUString SAL_CALL PDFDetector::detect( uno::Sequence< beans::PropertyValue >& rFilterData )
{
    std::unique_lock guard( m_aMutex );
    bool bSuccess = false;

    // get the InputStream carrying the PDF content
    uno::Reference<io::XInputStream> xInput;
    uno::Reference<io::XStream> xEmbedStream;
    OUString aOutFilterName;
    OUString aOutTypeName;
    OUString aURL;
    OUString aPassword;

    sal_Int32 nFilterNamePos = -1;
    sal_Int32 nPasswordPos = -1;
    sal_Int32 nAttribs = fillAttributes(rFilterData, xInput, aURL, nFilterNamePos, nPasswordPos, aPassword);

    if (!xInput.is())
        return OUString();


    uno::Sequence<sal_Int8> aHeader(constHeaderSize);
    sal_uInt64 nHeaderReadSize = 0;
    bSuccess = detectPDF(xInput, aHeader, nHeaderReadSize);

    if (!bSuccess)
        return OUString();

    oslFileHandle aFileHandle = nullptr;

    // check for hybrid PDF
    if (aURL.isEmpty() || !comphelper::isFileUrl(aURL))
    {
        if (osl_createTempFile(nullptr, &aFileHandle, &aURL.pData) != osl_File_E_None)
        {
            bSuccess = false;
        }
        else
        {
            SAL_INFO( "sdext.pdfimport", "created temp file " + aURL);
            bSuccess = copyToTemp(xInput, aFileHandle, aHeader, nHeaderReadSize);
        }
        osl_closeFile(aFileHandle);
    }

    if (!bSuccess)
    {
        if (aFileHandle)
            osl_removeFile(aURL.pData);
        return OUString();
    }

    OUString aEmbedMimetype;

    SAL_INFO( "sdext.pdfimport", "PDFDetector::detect before getEmbeddedFile" );
    // Try testing for the newer embedded file format
    xEmbedStream = getEmbeddedFile(aURL, aEmbedMimetype, aPassword, m_xContext, rFilterData, true);

    if (aEmbedMimetype.isEmpty())
    {
        SAL_INFO( "sdext.pdfimport", "PDFDetector::detect before getAdditionalStream" );
        // No success with embedded file, try the older trailer based AdditionalStream
        xEmbedStream =
            getAdditionalStream(aURL, aEmbedMimetype, aPassword, m_xContext, rFilterData, false);
    }

    SAL_INFO( "sdext.pdfimport", "PDFDetector::detect after emb/addit: "  << aEmbedMimetype);
    if (aFileHandle)
        osl_removeFile(aURL.pData);

    if (!aEmbedMimetype.isEmpty())
    {
        if( aEmbedMimetype == "application/vnd.oasis.opendocument.text"
            || aEmbedMimetype == "application/vnd.oasis.opendocument.text-master" )
            aOutFilterName = "writer_pdf_addstream_import";
        else if ( aEmbedMimetype == "application/vnd.oasis.opendocument.presentation" )
            aOutFilterName = "impress_pdf_addstream_import";
        else if( aEmbedMimetype == "application/vnd.oasis.opendocument.graphics"
                 || aEmbedMimetype == "application/vnd.oasis.opendocument.drawing" )
            aOutFilterName = "draw_pdf_addstream_import";
        else if ( aEmbedMimetype == "application/vnd.oasis.opendocument.spreadsheet" )
            aOutFilterName = "calc_pdf_addstream_import";
    }

    // Stash the password so that the importer can use it, even if we came to the
    // conclusion that it's not a hybrid, the PDF import side can use it.
    if (!aPassword.isEmpty())
    {
        if (nPasswordPos == -1)
        {
            nPasswordPos = nAttribs;
            rFilterData.realloc(++nAttribs);
            rFilterData.getArray()[nPasswordPos].Name = "Password";
        }
        rFilterData.getArray()[nPasswordPos].Value <<= aPassword;
    }

    if (!aOutFilterName.isEmpty())
    {
        if( nFilterNamePos == -1 )
        {
            nFilterNamePos = nAttribs;
            rFilterData.realloc( ++nAttribs );
            rFilterData.getArray()[ nFilterNamePos ].Name = "FilterName";
        }
        auto pFilterData = rFilterData.getArray();
        aOutTypeName = "pdf_Portable_Document_Format";

        pFilterData[nFilterNamePos].Value <<= aOutFilterName;
        if( xEmbedStream.is() )
        {
            rFilterData.realloc( ++nAttribs );
            pFilterData = rFilterData.getArray();
            pFilterData[nAttribs-1].Name = "EmbeddedSubstream";
            pFilterData[nAttribs-1].Value <<= xEmbedStream;
        }
    }
    else
    {
        css::beans::PropertyValue* pFilterData;
        if( nFilterNamePos == -1 )
        {
            nFilterNamePos = nAttribs;
            rFilterData.realloc( ++nAttribs );
            pFilterData = rFilterData.getArray();
            pFilterData[ nFilterNamePos ].Name = "FilterName";
        }
        else
            pFilterData = rFilterData.getArray();

        const sal_Int32 nDocumentType = 0; //const sal_Int32 nDocumentType = queryDocumentTypeDialog(m_xContext,aURL);
        if( nDocumentType < 0 )
        {
            return OUString();
        }
        else
        {
            switch (nDocumentType)
            {
                case 0:
                    pFilterData[nFilterNamePos].Value <<= u"draw_pdf_import"_ustr;
                    break;

                case 1:
                    pFilterData[nFilterNamePos].Value <<= u"impress_pdf_import"_ustr;
                    break;

                case 2:
                    pFilterData[nFilterNamePos].Value <<= u"writer_pdf_import"_ustr;
                    break;

                default:
                    assert(!"Unexpected case");
            }
        }

        aOutTypeName = "pdf_Portable_Document_Format";
    }

    return aOutTypeName;
}

OUString PDFDetector::getImplementationName()
{
    return u"org.libreoffice.comp.documents.PDFDetector"_ustr;
}

sal_Bool PDFDetector::supportsService(OUString const & ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> PDFDetector::getSupportedServiceNames()
{
    return {u"com.sun.star.document.ImportFilter"_ustr};
}

bool checkDocChecksum( const OUString& rInPDFFileURL,
                       sal_uInt32           nBytes,
                       const OUString& rChkSum )
{
    if( rChkSum.getLength() != 2* RTL_DIGEST_LENGTH_MD5 )
    {
        SAL_INFO(
            "sdext.pdfimport",
            "checksum of length " << rChkSum.getLength() << ", expected "
                << 2*RTL_DIGEST_LENGTH_MD5);
        return false;
    }

    // prepare checksum to test
    sal_uInt8 nTestChecksum[ RTL_DIGEST_LENGTH_MD5 ];
    const sal_Unicode* pChar = rChkSum.getStr();
    for(sal_uInt8 & rn : nTestChecksum)
    {
        sal_uInt8 nByte = sal_uInt8( ( (*pChar >= '0' && *pChar <= '9') ? *pChar - '0' :
                          ( (*pChar >= 'A' && *pChar <= 'F') ? *pChar - 'A' + 10 :
                          ( (*pChar >= 'a' && *pChar <= 'f') ? *pChar - 'a' + 10 :
                          0 ) ) ) );
        nByte <<= 4;
        pChar++;
        nByte |= ( (*pChar >= '0' && *pChar <= '9') ? *pChar - '0' :
                 ( (*pChar >= 'A' && *pChar <= 'F') ? *pChar - 'A' + 10 :
                 ( (*pChar >= 'a' && *pChar <= 'f') ? *pChar - 'a' + 10 :
                 0 ) ) );
        pChar++;
        rn = nByte;
    }

    // open file and calculate actual checksum up to index nBytes
    ::std::vector<unsigned char> nChecksum;
    ::comphelper::Hash aDigest(::comphelper::HashType::MD5);
    oslFileHandle aRead = nullptr;
    if( osl_openFile(rInPDFFileURL.pData,
                     &aRead,
                     osl_File_OpenFlag_Read ) == osl_File_E_None )
    {
        sal_uInt8 aBuf[4096];
        sal_uInt32 nCur = 0;
        sal_uInt64 nBytesRead = 0;
        while( nCur < nBytes )
        {
            sal_uInt32 nPass = std::min<sal_uInt32>(nBytes - nCur, sizeof( aBuf ));
            if( osl_readFile( aRead, aBuf, nPass, &nBytesRead) != osl_File_E_None
                || nBytesRead == 0 )
            {
                break;
            }
            nPass = static_cast<sal_uInt32>(nBytesRead);
            nCur += nPass;
            aDigest.update(aBuf, nPass);
        }

        nChecksum = aDigest.finalize();
        osl_closeFile( aRead );
    }

    // compare the contents
    return nChecksum.size() == RTL_DIGEST_LENGTH_MD5
        && (0 == memcmp(nChecksum.data(), nTestChecksum, nChecksum.size()));
}

/* https://github.com/CollaboraOnline/online/issues/7307

   Light-weight detection to determine if this is a hybrid
   pdf document worth parsing to get its AdditionalStream
   and mimetype.

   TODO: a) do we really ignore the contents of the AdditionalStream
   and re-parse to get it in the final importer?
         b) in which case we could presumably parse the mimetype in
   AdditionalStream here and drop the extraction of the stream.
*/
static bool detectHasAdditionalStreams(const OUString& rSysUPath)
{
    SvFileStream aHybridDetect(rSysUPath, StreamMode::READ);
    std::vector<OString> aTrailingLines;
    const sal_uInt64 nLen = aHybridDetect.remainingSize();
    aHybridDetect.Seek(nLen - std::min<sal_uInt64>(nLen, 4096));
    OString aLine;
    while (aHybridDetect.ReadLine(aLine))
        aTrailingLines.push_back(aLine);
    bool bAdditionalStreams(false);
    for (auto it = aTrailingLines.rbegin(); it != aTrailingLines.rend(); ++it)
    {
        if (*it == "trailer")
            break;
        if (it->startsWith("/AdditionalStreams "))
        {
            bAdditionalStreams = true;
            break;
        }
    }
    return bAdditionalStreams;
}

uno::Reference< io::XStream > getAdditionalStream( const OUString&                          rInPDFFileURL,
                                                   OUString&                                rOutMimetype,
                                                   OUString&                                io_rPwd,
                                                   const uno::Reference<uno::XComponentContext>& xContext,
                                                   const uno::Sequence<beans::PropertyValue>&    rFilterData,
                                                   bool                                          bMayUseUI )
{
    uno::Reference< io::XStream > xEmbed;
    OUString aSysUPath;
    if( osl_getSystemPathFromFileURL( rInPDFFileURL.pData, &aSysUPath.pData ) != osl_File_E_None )
        return xEmbed;

    if (!detectHasAdditionalStreams(aSysUPath))
        return xEmbed;

    std::unique_ptr<pdfparse::PDFEntry> pEntry(pdfparse::PDFReader::read(aSysUPath));
    if( pEntry )
    {
        pdfparse::PDFFile* pPDFFile = dynamic_cast<pdfparse::PDFFile*>(pEntry.get());
        if( pPDFFile )
        {
            unsigned int nElements = pPDFFile->m_aSubElements.size();
            while( nElements-- > 0 )
            {
                pdfparse::PDFTrailer* pTrailer = dynamic_cast<pdfparse::PDFTrailer*>(pPDFFile->m_aSubElements[nElements].get());
                if( pTrailer && pTrailer->m_pDict )
                {
                    // search document checksum entry
                    auto chk = pTrailer->m_pDict->m_aMap.find( "DocChecksum"_ostr );
                    if( chk == pTrailer->m_pDict->m_aMap.end() )
                    {
                        SAL_INFO( "sdext.pdfimport", "no DocChecksum entry" );
                        continue;
                    }
                    pdfparse::PDFName* pChkSumName = dynamic_cast<pdfparse::PDFName*>(chk->second);
                    if( pChkSumName == nullptr )
                    {
                        SAL_INFO( "sdext.pdfimport", "no name for DocChecksum entry" );
                        continue;
                    }

                    // search for AdditionalStreams entry
                    auto add_stream = pTrailer->m_pDict->m_aMap.find( "AdditionalStreams"_ostr );
                    if( add_stream == pTrailer->m_pDict->m_aMap.end() )
                    {
                        SAL_INFO( "sdext.pdfimport", "no AdditionalStreams entry" );
                        continue;
                    }
                    pdfparse::PDFArray* pStreams = dynamic_cast<pdfparse::PDFArray*>(add_stream->second);
                    if( ! pStreams || pStreams->m_aSubElements.size() < 2 )
                    {
                        SAL_INFO( "sdext.pdfimport", "AdditionalStreams array too small" );
                        continue;
                    }

                    // check checksum
                    OUString aChkSum = pChkSumName->getFilteredName();
                    if( ! checkDocChecksum( rInPDFFileURL, pTrailer->m_nOffset, aChkSum ) )
                        continue;

                    // extract addstream and mimetype
                    pdfparse::PDFName* pMimeType = dynamic_cast<pdfparse::PDFName*>(pStreams->m_aSubElements[0].get());
                    pdfparse::PDFObjectRef* pStreamRef = dynamic_cast<pdfparse::PDFObjectRef*>(pStreams->m_aSubElements[1].get());

                    SAL_WARN_IF( !pMimeType, "sdext.pdfimport", "error: no mimetype element" );
                    SAL_WARN_IF( !pStreamRef, "sdext.pdfimport", "error: no stream ref element" );

                    if( pMimeType && pStreamRef )
                    {
                        pdfparse::PDFObject* pObject = pPDFFile->findObject( pStreamRef->m_nNumber, pStreamRef->m_nGeneration );
                        SAL_WARN_IF( !pObject, "sdext.pdfimport", "object not found" );
                        if( pObject )
                        {
                            if( pPDFFile->isEncrypted() )
                            {
                                bool bAuthenticated = false;
                                if( !io_rPwd.isEmpty() )
                                {
                                    OString aIsoPwd = OUStringToOString( io_rPwd,
                                                                                   RTL_TEXTENCODING_ISO_8859_1 );
                                    bAuthenticated = pPDFFile->setupDecryptionData( aIsoPwd );
                                }
                                else
                                {
                                    uno::Reference< task::XInteractionHandler > xIntHdl;
                                    for( const beans::PropertyValue& rAttrib : rFilterData )
                                    {
                                        if ( rAttrib.Name == "InteractionHandler" )
                                            rAttrib.Value >>= xIntHdl;
                                    }
                                    if( ! bMayUseUI || ! xIntHdl.is() )
                                    {
                                        rOutMimetype = pMimeType->getFilteredName();
                                        xEmbed.clear();
                                        break;
                                    }

                                    OUString aDocName( rInPDFFileURL.copy( rInPDFFileURL.lastIndexOf( '/' )+1 ) );

                                    bool bEntered = false;
                                    do
                                    {
                                        bEntered = getPassword( xIntHdl, io_rPwd, ! bEntered, aDocName );
                                        OString aIsoPwd = OUStringToOString( io_rPwd,
                                                                                       RTL_TEXTENCODING_ISO_8859_1 );
                                        bAuthenticated = pPDFFile->setupDecryptionData( aIsoPwd );
                                    } while( bEntered && ! bAuthenticated );
                                }

                                if( ! bAuthenticated )
                                    continue;
                            }
                            rOutMimetype = pMimeType->getFilteredName();
                            FileEmitContext aContext( rInPDFFileURL,
                                                      xContext,
                                                      pPDFFile );
                            aContext.m_bDecrypt = pPDFFile->isEncrypted();
                            pObject->writeStream( aContext, pPDFFile );
                            xEmbed = aContext.getContextStream();
                            break; // success
                        }
                    }
                }
            }
        }
    }

    return xEmbed;
}


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
sdext_PDFDetector_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new PDFDetector(context));
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

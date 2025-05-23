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


#include <filter/EpsReader.hxx>
#include <vcl/svapp.hxx>
#include <vcl/gdimtf.hxx>
#include <vcl/graph.hxx>
#include <vcl/metaact.hxx>
#include <vcl/virdev.hxx>
#include <vcl/cvtgrf.hxx>
#include <vcl/BitmapTools.hxx>
#include <comphelper/configuration.hxx>
#include <unotools/tempfile.hxx>
#include <osl/process.h>
#include <osl/file.hxx>
#include <osl/thread.h>
#include <rtl/byteseq.hxx>
#include <sal/log.hxx>
#include <o3tl/char16_t2wchar_t.hxx>
#include <o3tl/safeint.hxx>
#include <memory>
#include <string_view>

class FilterConfigItem;

/*************************************************************************
|*
|*    ImpSearchEntry()
|*
|*    Description       Checks if there is a string(pDest) of length nSize
|*                      inside the memory area pSource which is nComp bytes long.
|*                      Check is NON-CASE-SENSITIVE. The return value is the
|*                      address where the string is found or NULL
|*
*************************************************************************/

static const sal_uInt8* ImplSearchEntry( const sal_uInt8* pSource, sal_uInt8 const * pDest, size_t nComp, size_t nSize )
{
    while ( nComp-- >= nSize )
    {
        size_t i;
        for ( i = 0; i < nSize; i++ )
        {
            if ( ( pSource[i]&~0x20 ) != ( pDest[i]&~0x20 ) )
                break;
        }
        if ( i == nSize )
            return pSource;
        pSource++;
    }
    return nullptr;
}


// SecurityCount is the buffersize of the buffer in which we will parse for a number
static tools::Long ImplGetNumber(const sal_uInt8* &rBuf, sal_uInt32& nSecurityCount)
{
    bool    bValid = true;
    bool    bNegative = false;
    tools::Long    nRetValue = 0;
    while (nSecurityCount && (*rBuf == ' ' || *rBuf == 0x9))
    {
        ++rBuf;
        --nSecurityCount;
    }
    while ( nSecurityCount && ( *rBuf != ' ' ) && ( *rBuf != 0x9 ) && ( *rBuf != 0xd ) && ( *rBuf != 0xa ) )
    {
        switch ( *rBuf )
        {
            case '.' :
                // we'll only use the integer format
                bValid = false;
                break;
            case '-' :
                bNegative = true;
                break;
            default :
                if ( ( *rBuf < '0' ) || ( *rBuf > '9' ) )
                    nSecurityCount = 1;         // error parsing the bounding box values
                else if ( bValid )
                {
                    const bool bFail = o3tl::checked_multiply<tools::Long>(nRetValue, 10, nRetValue) ||
                                       o3tl::checked_add<tools::Long>(nRetValue, *rBuf - '0', nRetValue);
                    if (bFail)
                        return 0;
                }
                break;
        }
        nSecurityCount--;
        ++rBuf;
    }
    if ( bNegative )
        nRetValue = -nRetValue;
    return nRetValue;
}


static int ImplGetLen(const sal_uInt8* pBuf, int nMax)
{
    int nLen = 0;
    while( nLen != nMax )
    {
        sal_uInt8 nDat = *pBuf++;
        if ( nDat == 0x0a || nDat == 0x25 )
            break;
        nLen++;
    }
    return nLen;
}

static void MakeAsMeta(Graphic &rGraphic)
{
    ScopedVclPtrInstance< VirtualDevice > pVDev;
    GDIMetaFile     aMtf;
    Size            aSize = rGraphic.GetPrefSize();

    if( !aSize.Width() || !aSize.Height() )
        aSize = Application::GetDefaultDevice()->PixelToLogic(
            rGraphic.GetSizePixel(), MapMode(MapUnit::Map100thMM));
    else
        aSize = OutputDevice::LogicToLogic( aSize,
            rGraphic.GetPrefMapMode(), MapMode(MapUnit::Map100thMM));

    pVDev->EnableOutput( false );
    aMtf.Record( pVDev );
    pVDev->DrawBitmapEx( Point(), aSize, rGraphic.GetBitmapEx() );
    aMtf.Stop();
    aMtf.WindStart();
    aMtf.SetPrefMapMode(MapMode(MapUnit::Map100thMM));
    aMtf.SetPrefSize( aSize );
    rGraphic = aMtf;
}

static oslProcessError runProcessWithPathSearch(const OUString &rProgName,
    rtl_uString* pArgs[], sal_uInt32 nArgs, oslProcess *pProcess,
    oslFileHandle *pIn, oslFileHandle *pOut, oslFileHandle *pErr)
{
    // run things that directly or indirectly might call gs in a tmpdir of their own
    utl::TempFileNamed aTMPDirectory(nullptr, true);
    aTMPDirectory.EnableKillingFile(true);
    OUString sTmpDirEnv = u"TMPDIR="_ustr + aTMPDirectory.GetFileName();

    rtl_uString* ustrEnvironment[1];
    ustrEnvironment[0] = sTmpDirEnv.pData;

    oslProcessError result = osl_Process_E_None;
    oslSecurity pSecurity = osl_getCurrentSecurity();
#ifdef _WIN32
    /*
     * ooo#72096
     * On Window the underlying SearchPath searches in order of...
     * The directory from which the application loaded.
     * The current directory.
     * The Windows system directory.
     * The Windows directory.
     * The directories that are listed in the PATH environment variable.
     *
     * Because one of our programs is called "convert" and there is a convert
     * in the windows system directory, we want to explicitly search the PATH
     * to avoid picking up on that one if ImageMagick's convert precedes it in
     * PATH.
     *
     */
    OUString url;
    OUString path(o3tl::toU(_wgetenv(L"PATH")));

    oslFileError err = osl_searchFileURL(rProgName.pData, path.pData, &url.pData);
    if (err != osl_File_E_None)
        result = osl_Process_E_NotFound;
    else
        result = osl_executeProcess_WithRedirectedIO(url.pData,
            pArgs, nArgs, osl_Process_HIDDEN,
            pSecurity, nullptr, ustrEnvironment, 1, pProcess, pIn, pOut, pErr);
#else
    result = osl_executeProcess_WithRedirectedIO(rProgName.pData,
        pArgs, nArgs, osl_Process_SEARCHPATH | osl_Process_HIDDEN,
        pSecurity, nullptr, ustrEnvironment, 1, pProcess, pIn, pOut, pErr);
#endif
    osl_freeSecurityHandle( pSecurity );
    return result;
}

#if defined(_WIN32)
#    define EXESUFFIX ".exe"
#else
#    define EXESUFFIX ""
#endif

static bool RenderAsEMF(const sal_uInt8* pBuf, sal_uInt32 nBytesRead, Graphic &rGraphic)
{
    utl::TempFileNamed aTempOutput;
    utl::TempFileNamed aTempInput;
    aTempOutput.EnableKillingFile();
    aTempInput.EnableKillingFile();
    OUString output;
    osl::FileBase::getSystemPathFromFileURL(aTempOutput.GetURL(), output);
    OUString input;
    osl::FileBase::getSystemPathFromFileURL(aTempInput.GetURL(), input);

    SvStream* pInputStream = aTempInput.GetStream(StreamMode::WRITE);
    sal_uInt64 nCount = pInputStream->WriteBytes(pBuf, nBytesRead);
    aTempInput.CloseStream();

    //fdo#64161 pstoedit under non-windows uses libEMF to output the EMF, but
    //libEMF cannot calculate the bounding box of text, so the overall bounding
    //box is not increased to include that of any text in the eps
    //
    //-drawbb will force pstoedit to draw a pair of pixels with the bg color to
    //the topleft and bottom right of the bounding box as pstoedit sees it,
    //which libEMF will then extend its bounding box to fit
    //
    //-usebbfrominput forces pstoedit to take the original ps bounding box
    //as the bounding box as it sees it, instead of calculating its own
    //which also doesn't work for this example
    //
    //Under Linux, positioning of letters within pstoedit is very approximate.
    //Using the -nfw option delegates the positioning to the reader, and we
    //will do a proper job.  The option is ignored on Windows.
    OUString arg1(u"-usebbfrominput"_ustr);   //-usebbfrominput use the original ps bounding box
    OUString arg2(u"-f"_ustr);
    OUString arg3(u"emf:-OO -drawbb -nfw"_ustr); //-drawbb mark out the bounding box extent with bg pixels
                                           //-nfw delegate letter placement to us
    rtl_uString *args[] =
    {
        arg1.pData, arg2.pData, arg3.pData, input.pData, output.pData
    };
    oslProcess aProcess;
    oslFileHandle pIn = nullptr;
    oslFileHandle pOut = nullptr;
    oslFileHandle pErr = nullptr;
    oslProcessError eErr = runProcessWithPathSearch(
            u"pstoedit" EXESUFFIX ""_ustr,
            args, SAL_N_ELEMENTS(args),
            &aProcess, &pIn, &pOut, &pErr);

    if (eErr!=osl_Process_E_None)
        return false;

    bool bRet = false;
    if (pIn) osl_closeFile(pIn);
    osl_joinProcess(aProcess);
    osl_freeProcessHandle(aProcess);
    bool bEMFSupported=true;
    if (pOut)
    {
        rtl::ByteSequence seq;
        if (osl_File_E_None == osl_readLine(pOut, reinterpret_cast<sal_Sequence **>(&seq)))
        {
            OString line( reinterpret_cast<const char *>(seq.getConstArray()), seq.getLength() );
            if (line.startsWith("Unsupported output format"))
                bEMFSupported=false;
        }
        osl_closeFile(pOut);
    }
    if (pErr) osl_closeFile(pErr);
    if (nCount == nBytesRead && bEMFSupported)
    {
        SvFileStream aFile(output, StreamMode::READ);
        if (GraphicConverter::Import(aFile, rGraphic, ConvertDataFormat::EMF) == ERRCODE_NONE)
            bRet = true;
    }

    return bRet;
}

namespace {

struct WriteData
{
    oslFileHandle   m_pFile;
    const sal_uInt8 *m_pBuf;
    sal_uInt32      m_nBytesToWrite;
};

}

extern "C" {

static void WriteFileInThread(void *wData)
{
    sal_uInt64 nCount;
    WriteData *wdata = static_cast<WriteData *>(wData);
    osl_writeFile(wdata->m_pFile, wdata->m_pBuf, wdata->m_nBytesToWrite, &nCount);
    // The number of bytes written does not matter.
    // The helper process may close its input stream before reading it all.
    // (e.g. at "showpage" in EPS)

    // File must be closed here.
    // Otherwise, the helper process may wait for the next input,
    // then its stdout is not closed and osl_readFile() blocks.
    if (wdata->m_pFile) osl_closeFile(wdata->m_pFile);
}

}

static bool RenderAsBMPThroughHelper(const sal_uInt8* pBuf, sal_uInt32 nBytesRead,
                                     Graphic& rGraphic,
                                     std::initializer_list<std::u16string_view> aProgNames,
                                     rtl_uString* pArgs[], size_t nArgs)
{
    oslProcess aProcess = nullptr;
    oslFileHandle pIn = nullptr;
    oslFileHandle pOut = nullptr;
    oslFileHandle pErr = nullptr;
    oslProcessError eErr = osl_Process_E_Unknown;
    for (const auto& rProgName : aProgNames)
    {
        eErr = runProcessWithPathSearch(OUString(rProgName), pArgs, nArgs, &aProcess, &pIn, &pOut, &pErr);
        if (eErr == osl_Process_E_None)
            break;
    }
    if (eErr!=osl_Process_E_None)
        return false;

    WriteData Data;
    Data.m_pFile = pIn;
    Data.m_pBuf = pBuf;
    Data.m_nBytesToWrite = nBytesRead;
    oslThread hThread = osl_createThread(WriteFileInThread, &Data);

    bool bRet = false;
    sal_uInt64 nCount;
    {
        SvMemoryStream aMemStm;
        sal_uInt8 aBuf[32000];
        oslFileError eFileErr = osl_readFile(pOut, aBuf, 32000, &nCount);
        while (eFileErr == osl_File_E_None && nCount)
        {
            aMemStm.WriteBytes(aBuf, sal::static_int_cast<std::size_t>(nCount));
            eFileErr = osl_readFile(pOut, aBuf, 32000, &nCount);
        }

        aMemStm.Seek(0);
        if (
            aMemStm.GetEndOfData() &&
            GraphicConverter::Import(aMemStm, rGraphic, ConvertDataFormat::BMP) == ERRCODE_NONE
           )
        {
            MakeAsMeta(rGraphic);
            bRet = true;
        }
    }
    if (pOut) osl_closeFile(pOut);
    if (pErr) osl_closeFile(pErr);
    osl_joinProcess(aProcess);
    osl_freeProcessHandle(aProcess);
    osl_joinWithThread(hThread);
    osl_destroyThread(hThread);
    return bRet;
}

static bool RenderAsBMPThroughConvert(const sal_uInt8* pBuf, sal_uInt32 nBytesRead,
    Graphic &rGraphic)
{
    // density in pixel/inch
    OUString arg1(u"-density"_ustr);
    // since the preview is also used for PDF-Export & printing on non-PS-printers,
    // use some better quality - 300x300 should allow some resizing as well
    OUString arg2(u"300x300"_ustr);
    // read eps from STDIN
    OUString arg3(u"eps:-"_ustr);
    // write bmp to STDOUT
    OUString arg4(u"bmp:-"_ustr);
    rtl_uString *args[] =
    {
        arg1.pData, arg2.pData, arg3.pData, arg4.pData
    };
    return RenderAsBMPThroughHelper(pBuf, nBytesRead, rGraphic,
        { u"convert" EXESUFFIX },
        args,
        SAL_N_ELEMENTS(args));
}

static bool RenderAsBMPThroughGS(const sal_uInt8* pBuf, sal_uInt32 nBytesRead,
    Graphic &rGraphic)
{
    OUString arg1(u"-q"_ustr);
    OUString arg2(u"-dBATCH"_ustr);
    OUString arg3(u"-dNOPAUSE"_ustr);
    OUString arg4(u"-dPARANOIDSAFER"_ustr);
    OUString arg5(u"-dEPSCrop"_ustr);
    OUString arg6(u"-dTextAlphaBits=4"_ustr);
    OUString arg7(u"-dGraphicsAlphaBits=4"_ustr);
    OUString arg8(u"-r300x300"_ustr);
    OUString arg9(u"-sDEVICE=bmp16m"_ustr);
    OUString arg10(u"-sOutputFile=-"_ustr);
    OUString arg11(u"-"_ustr);
    rtl_uString *args[] =
    {
        arg1.pData, arg2.pData, arg3.pData, arg4.pData, arg5.pData,
        arg6.pData, arg7.pData, arg8.pData, arg9.pData, arg10.pData,
        arg11.pData
    };
    return RenderAsBMPThroughHelper(pBuf, nBytesRead, rGraphic,
#ifdef _WIN32
        // Try both 32-bit and 64-bit ghostscript executable name
        {
            u"gswin32c" EXESUFFIX,
            u"gswin64c" EXESUFFIX,
        },
#else
        { u"gs" EXESUFFIX },
#endif
        args,
        SAL_N_ELEMENTS(args));
}

static bool RenderAsBMP(const sal_uInt8* pBuf, sal_uInt32 nBytesRead, Graphic &rGraphic)
{
    if (RenderAsBMPThroughGS(pBuf, nBytesRead, rGraphic))
        return true;
    else
        return RenderAsBMPThroughConvert(pBuf, nBytesRead, rGraphic);
}

// this method adds a replacement action containing the original wmf or tiff replacement,
// so the original eps can be written when storing to ODF.
static void CreateMtfReplacementAction( GDIMetaFile& rMtf, SvStream& rStrm, sal_uInt32 nOrigPos, sal_uInt32 nPSSize,
                                sal_uInt32 nPosWMF, sal_uInt32 nSizeWMF, sal_uInt32 nPosTIFF, sal_uInt32 nSizeTIFF )
{
    OString aComment("EPSReplacementGraphic"_ostr);
    if ( nSizeWMF || nSizeTIFF )
    {
        std::vector<sal_uInt8> aWMFBuf;
        if (nSizeWMF && checkSeek(rStrm, nOrigPos + nPosWMF) && rStrm.remainingSize() >= nSizeWMF)
        {
            aWMFBuf.resize(nSizeWMF);
            aWMFBuf.resize(rStrm.ReadBytes(aWMFBuf.data(), nSizeWMF));
        }
        nSizeWMF = aWMFBuf.size();

        std::vector<sal_uInt8> aTIFFBuf;
        if (nSizeTIFF && checkSeek(rStrm, nOrigPos + nPosTIFF) && rStrm.remainingSize() >= nSizeTIFF)
        {
            aTIFFBuf.resize(nSizeTIFF);
            aTIFFBuf.resize(rStrm.ReadBytes(aTIFFBuf.data(), nSizeTIFF));
        }
        nSizeTIFF = aTIFFBuf.size();

        SvMemoryStream aReplacement( nSizeWMF + nSizeTIFF + 28 );
        sal_uInt32 const nMagic = 0xc6d3d0c5;
        sal_uInt32 nPPos = 28 + nSizeWMF + nSizeTIFF;
        sal_uInt32 nWPos = nSizeWMF ? 28 : 0;
        sal_uInt32 nTPos = nSizeTIFF ? 28 + nSizeWMF : 0;

        aReplacement.WriteUInt32( nMagic ).WriteUInt32( nPPos ).WriteUInt32( nPSSize )
                    .WriteUInt32( nWPos ).WriteUInt32( nSizeWMF )
                    .WriteUInt32( nTPos ).WriteUInt32( nSizeTIFF );

        aReplacement.WriteBytes(aWMFBuf.data(), nSizeWMF);
        aReplacement.WriteBytes(aTIFFBuf.data(), nSizeTIFF);
        rMtf.AddAction( static_cast<MetaAction*>( new MetaCommentAction( aComment, 0, static_cast<const sal_uInt8*>(aReplacement.GetData()), aReplacement.Tell() ) ) );
    }
    else
        rMtf.AddAction( static_cast<MetaAction*>( new MetaCommentAction( aComment, 0, nullptr, 0 ) ) );
}

//there is no preview -> make a red box
static void MakePreview(const sal_uInt8* pBuf, sal_uInt32 nBytesRead,
    tools::Long nWidth, tools::Long nHeight, Graphic &rGraphic)
{
    GDIMetaFile aMtf;
    ScopedVclPtrInstance< VirtualDevice > pVDev;
    vcl::Font       aFont;

    pVDev->EnableOutput( false );
    aMtf.Record( pVDev );
    pVDev->SetLineColor( COL_RED );
    pVDev->SetFillColor();

    aFont.SetColor( COL_LIGHTRED );

    pVDev->Push( vcl::PushFlags::FONT );
    pVDev->SetFont( aFont );

    tools::Rectangle aRect( Point( 1, 1 ), Size( nWidth - 2, nHeight - 2 ) );
    pVDev->DrawRect( aRect );

    OUString aString;
    int nLen;
    const sal_uInt8* pDest = ImplSearchEntry( pBuf, reinterpret_cast<sal_uInt8 const *>("%%Title:"), nBytesRead - 32, 8 );
    sal_uInt32 nRemainingBytes = pDest ? (nBytesRead - (pDest - pBuf)) : 0;
    if (nRemainingBytes >= 8)
    {
        pDest += 8;
        nRemainingBytes -= 8;
        if (nRemainingBytes && *pDest == ' ')
        {
            ++pDest;
            --nRemainingBytes;
        }
        nLen = ImplGetLen(pDest, std::min<sal_uInt32>(nRemainingBytes, 32));
        if (o3tl::make_unsigned(nLen) < nRemainingBytes)
        {
            std::string_view chunk(reinterpret_cast<const char*>(pDest), nLen);
            if (chunk != "none")
            {
                aString += " Title:" + OStringToOUString(chunk, RTL_TEXTENCODING_ASCII_US) + "\n";
            }
        }
    }
    pDest = ImplSearchEntry( pBuf, reinterpret_cast<sal_uInt8 const *>("%%Creator:"), nBytesRead - 32, 10 );
    nRemainingBytes = pDest ? (nBytesRead - (pDest - pBuf)) : 0;
    if (nRemainingBytes >= 10)
    {
        pDest += 10;
        nRemainingBytes -= 10;
        if (nRemainingBytes && *pDest == ' ')
        {
            ++pDest;
            --nRemainingBytes;
        }
        nLen = ImplGetLen(pDest, std::min<sal_uInt32>(nRemainingBytes, 32));
        if (o3tl::make_unsigned(nLen) < nRemainingBytes)
        {
            std::string_view chunk(reinterpret_cast<const char*>(pDest), nLen);
            aString += " Creator:" + OStringToOUString(chunk, RTL_TEXTENCODING_ASCII_US) + "\n";
        }
    }
    pDest = ImplSearchEntry( pBuf, reinterpret_cast<sal_uInt8 const *>("%%CreationDate:"), nBytesRead - 32, 15 );
    nRemainingBytes = pDest ? (nBytesRead - (pDest - pBuf)) : 0;
    if (nRemainingBytes >= 15)
    {
        pDest += 15;
        nRemainingBytes -= 15;
        if (nRemainingBytes && *pDest == ' ')
        {
            ++pDest;
            --nRemainingBytes;
        }
        nLen = ImplGetLen(pDest, std::min<sal_uInt32>(nRemainingBytes, 32));
        if (o3tl::make_unsigned(nLen) < nRemainingBytes)
        {
            std::string_view chunk(reinterpret_cast<const char*>(pDest), nLen);
            if (chunk != "none")
            {
                aString += " CreationDate:" + OStringToOUString(chunk, RTL_TEXTENCODING_ASCII_US) + "\n";
            }
        }
    }
    pDest = ImplSearchEntry( pBuf, reinterpret_cast<sal_uInt8 const *>("%%LanguageLevel:"), nBytesRead - 4, 16 );
    nRemainingBytes = pDest ? (nBytesRead - (pDest - pBuf)) : 0;
    if (nRemainingBytes >= 16)
    {
        pDest += 16;
        nRemainingBytes -= 16;
        sal_uInt32 nCount = std::min<sal_uInt32>(nRemainingBytes, 4U);
        sal_uInt32 nNumber = ImplGetNumber(pDest, nCount);
        if (nCount && nNumber < 10)
        {
            aString += " LanguageLevel:" + OUString::number( nNumber );
        }
    }
    pVDev->DrawText( aRect, aString, DrawTextFlags::Clip | DrawTextFlags::MultiLine );
    pVDev->Pop();
    aMtf.Stop();
    aMtf.WindStart();
    aMtf.SetPrefMapMode(MapMode(MapUnit::MapPoint));
    aMtf.SetPrefSize( Size( nWidth, nHeight ) );
    rGraphic = aMtf;
}

//================== GraphicImport - the exported function ================


bool ImportEpsGraphic( SvStream & rStream, Graphic & rGraphic)
{
    if ( rStream.GetError() )
        return false;

    Graphic     aGraphic;
    bool    bRetValue = false;
    bool    bHasPreview = false;
    sal_uInt32  nSignature = 0, nPSStreamPos, nPSSize = 0;
    sal_uInt32  nSizeWMF = 0;
    sal_uInt32  nPosWMF = 0;
    sal_uInt32  nSizeTIFF = 0;
    sal_uInt32  nPosTIFF = 0;

    auto nOrigPos = nPSStreamPos = rStream.Tell();
    SvStreamEndian nOldFormat = rStream.GetEndian();

    rStream.SetEndian( SvStreamEndian::LITTLE );
    rStream.ReadUInt32( nSignature );
    if ( nSignature == 0xc6d3d0c5 )
    {
        rStream.ReadUInt32( nPSStreamPos ).ReadUInt32( nPSSize ).ReadUInt32( nPosWMF ).ReadUInt32( nSizeWMF );

        // first we try to get the metafile grafix

        if ( nSizeWMF )
        {
            if (nPosWMF && checkSeek(rStream, nOrigPos + nPosWMF))
            {
                if (GraphicConverter::Import(rStream, aGraphic, ConvertDataFormat::WMF) == ERRCODE_NONE)
                    bHasPreview = bRetValue = true;
            }
        }
        else
        {
            rStream.ReadUInt32( nPosTIFF ).ReadUInt32( nSizeTIFF );

            // else we have to get the tiff grafix

            if (nPosTIFF && nSizeTIFF && checkSeek(rStream, nOrigPos + nPosTIFF))
            {
                if ( GraphicConverter::Import( rStream, aGraphic, ConvertDataFormat::TIF ) == ERRCODE_NONE )
                {
                    MakeAsMeta(aGraphic);
                    rStream.Seek( nOrigPos + nPosTIFF );
                    bHasPreview = bRetValue = true;
                }
            }
        }
    }
    else
    {
        nPSStreamPos = nOrigPos;            // no preview available _>so we must get the size manually
        nPSSize = rStream.Seek( STREAM_SEEK_TO_END ) - nOrigPos;
    }

    std::vector<sal_uInt8> aHeader(22, 0);
    rStream.Seek( nPSStreamPos );
    rStream.ReadBytes(aHeader.data(), 22); // check PostScript header
    sal_uInt8* pHeader = aHeader.data();
    bool bOk = ImplSearchEntry(pHeader, reinterpret_cast<sal_uInt8 const *>("%!PS-Adobe"), 10, 10) &&
               ImplSearchEntry(pHeader + 15, reinterpret_cast<sal_uInt8 const *>("EPS"), 3, 3);
    if (bOk)
    {
        rStream.Seek(nPSStreamPos);
        bOk = rStream.remainingSize() >= nPSSize;
        SAL_WARN_IF(!bOk, "filter.eps", "eps claims to be: " << nPSSize << " in size, but only " << rStream.remainingSize() << " remains");
    }
    if (bOk)
    {
        sal_uInt64 nBufStartPos = rStream.Tell();
        BinaryDataContainer aBuf(rStream, nPSSize);
        if (!aBuf.isEmpty())
        {
            sal_uInt32 nBytesRead = aBuf.getSize();
            sal_uInt32 nSecurityCount = 32;
            // if there is no tiff/wmf preview, we will parse for a preview in
            // the eps prolog
            if (!bHasPreview && nBytesRead >= nSecurityCount)
            {
                const sal_uInt8* pDest = ImplSearchEntry( aBuf.getData(), reinterpret_cast<sal_uInt8 const *>("%%BeginPreview:"), nBytesRead - nSecurityCount, 15 );
                sal_uInt32 nRemainingBytes = pDest ? (nBytesRead - (pDest - aBuf.getData())) : 0;
                if (nRemainingBytes >= 15)
                {
                    pDest += 15;
                    nSecurityCount = nRemainingBytes - 15;
                    tools::Long nWidth = ImplGetNumber(pDest, nSecurityCount);
                    tools::Long nHeight = ImplGetNumber(pDest, nSecurityCount);
                    tools::Long nBitDepth = ImplGetNumber(pDest, nSecurityCount);
                    tools::Long nScanLines = ImplGetNumber(pDest, nSecurityCount);
                    pDest = ImplSearchEntry(pDest, reinterpret_cast<sal_uInt8 const *>("%"), nSecurityCount, 1);       // go to the first Scanline
                    bOk = pDest && nWidth > 0 && nHeight > 0 && ( ( nBitDepth == 1 ) || ( nBitDepth == 8 ) ) && nScanLines;
                    if (bOk)
                    {
                        tools::Long nResult;
                        bOk = !o3tl::checked_multiply(nWidth, nHeight, nResult) && nResult <= SAL_MAX_INT32/2/3;
                    }
                    if (bOk)
                    {
                        rStream.Seek( nBufStartPos + ( pDest - aBuf.getData() ) );

                        vcl::bitmap::RawBitmap aBitmap( Size( nWidth, nHeight ), 24 );
                        {
                            bool bIsValid = true;
                            sal_uInt8 nDat = 0;
                            char nByte;
                            for (tools::Long y = 0; bIsValid && y < nHeight; ++y)
                            {
                                int nBitsLeft = 0;
                                for (tools::Long x = 0; x < nWidth; ++x)
                                {
                                    if ( --nBitsLeft < 0 )
                                    {
                                        while ( bIsValid && ( nBitsLeft != 7 ) )
                                        {
                                            rStream.ReadChar(nByte);
                                            bIsValid = rStream.good();
                                            if (!bIsValid)
                                                break;
                                            switch (nByte)
                                            {
                                                case 0x0a :
                                                    if ( --nScanLines < 0 )
                                                        bIsValid = false;
                                                    break;
                                                case 0x09 :
                                                case 0x0d :
                                                case 0x20 :
                                                case 0x25 :
                                                break;
                                                default:
                                                {
                                                    if ( nByte >= '0' )
                                                    {
                                                        if ( nByte > '9' )
                                                        {
                                                            nByte &=~0x20;  // case none sensitive for hexadecimal values
                                                            nByte -= ( 'A' - 10 );
                                                            if ( nByte > 15 )
                                                                bIsValid = false;
                                                        }
                                                        else
                                                            nByte -= '0';
                                                        nBitsLeft += 4;
                                                        nDat <<= 4;
                                                        nDat |= ( nByte ^ 0xf ); // in epsi a zero bit represents white color
                                                    }
                                                    else
                                                        bIsValid = false;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                    if (!bIsValid)
                                        break;
                                    if ( nBitDepth == 1 )
                                        aBitmap.SetPixel( y, x, Color(ColorTransparency, static_cast<sal_uInt8>(nDat >> nBitsLeft) & 1) );
                                    else
                                    {
                                        aBitmap.SetPixel( y, x, nDat ? COL_WHITE : COL_BLACK );  // nBitDepth == 8
                                        nBitsLeft = 0;
                                    }
                                }
                            }
                            if (bIsValid)
                            {
                                ScopedVclPtrInstance<VirtualDevice> pVDev;
                                GDIMetaFile     aMtf;
                                Size            aSize( nWidth, nHeight );
                                pVDev->EnableOutput( false );
                                aMtf.Record( pVDev );
                                aSize = OutputDevice::LogicToLogic(aSize, MapMode(), MapMode(MapUnit::Map100thMM));
                                pVDev->DrawBitmapEx( Point(), aSize, vcl::bitmap::CreateFromData(std::move(aBitmap)) );
                                aMtf.Stop();
                                aMtf.WindStart();
                                aMtf.SetPrefMapMode(MapMode(MapUnit::Map100thMM));
                                aMtf.SetPrefSize( aSize );
                                aGraphic = aMtf;
                                bHasPreview = bRetValue = true;
                            }
                        }
                    }
                }
            }

            const sal_uInt8* pDest = ImplSearchEntry( aBuf.getData(), reinterpret_cast<sal_uInt8 const *>("%%BoundingBox:"), nBytesRead, 14 );
            sal_uInt32 nRemainingBytes = pDest ? (nBytesRead - (pDest - aBuf.getData())) : 0;
            if (nRemainingBytes >= 14)
            {
                pDest += 14;
                nSecurityCount = std::min<sal_uInt32>(nRemainingBytes - 14, 100);
                tools::Long nNumb[4];
                nNumb[0] = nNumb[1] = nNumb[2] = nNumb[3] = 0;
                for ( int i = 0; ( i < 4 ) && nSecurityCount; i++ )
                {
                    nNumb[ i ] = ImplGetNumber(pDest, nSecurityCount);
                }
                bool bFail = nSecurityCount == 0;
                tools::Long nWidth(0), nHeight(0);
                if (!bFail)
                    bFail = o3tl::checked_sub(nNumb[2], nNumb[0], nWidth) || o3tl::checked_add(nWidth, tools::Long(1), nWidth);
                if (!bFail)
                    bFail = o3tl::checked_sub(nNumb[3], nNumb[1], nHeight) || o3tl::checked_add(nHeight, tools::Long(1), nHeight);
                if (!bFail && nWidth > 0 && nHeight > 0)
                {
                    GDIMetaFile aMtf;

                    // if there is no preview -> try with gs to make one
                    if (!bHasPreview && !comphelper::IsFuzzing())
                    {
                        bHasPreview = RenderAsEMF(aBuf.getData(), nBytesRead, aGraphic);
                        if (!bHasPreview)
                            bHasPreview = RenderAsBMP(aBuf.getData(), nBytesRead, aGraphic);
                    }

                    // if there is no preview -> make a red box
                    if( !bHasPreview )
                    {
                        MakePreview(aBuf.getData(), nBytesRead, nWidth, nHeight,
                            aGraphic);
                    }

                    GfxLink aGfxLink(std::move(aBuf), GfxLinkType::EpsBuffer);
                    aMtf.AddAction( static_cast<MetaAction*>( new MetaEPSAction( Point(), Size( nWidth, nHeight ),
                                                                      std::move(aGfxLink), aGraphic.GetGDIMetaFile() ) ) );
                    CreateMtfReplacementAction( aMtf, rStream, nOrigPos, nPSSize, nPosWMF, nSizeWMF, nPosTIFF, nSizeTIFF );
                    aMtf.WindStart();
                    aMtf.SetPrefMapMode(MapMode(MapUnit::MapPoint));
                    aMtf.SetPrefSize( Size( nWidth, nHeight ) );
                    rGraphic = aMtf;
                    bRetValue = true;
                }
            }
        }
    }

    rStream.SetEndian(nOldFormat);
    rStream.Seek( nOrigPos );
    return bRetValue;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

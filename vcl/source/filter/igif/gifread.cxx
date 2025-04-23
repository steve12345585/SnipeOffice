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

#include <sal/log.hxx>
#include <tools/stream.hxx>
#include "decode.hxx"
#include "gifread.hxx"
#include <memory>
#include <vcl/BitmapWriteAccess.hxx>

namespace {

enum GIFAction
{
    GLOBAL_HEADER_READING,
    MARKER_READING,
    EXTENSION_READING,
    LOCAL_HEADER_READING,
    FIRST_BLOCK_READING,
    NEXT_BLOCK_READING,
    ABORT_READING,
    END_READING
};

enum ReadState
{
    GIFREAD_OK,
    GIFREAD_ERROR
};

}

class GIFLZWDecompressor;

class SvStream;

namespace {

class GIFReader
{
    Animation           aAnimation;
    sal_uInt64          nAnimationByteSize;
    sal_uInt64          nAnimationMinFileData;
    Bitmap              aBmp8;
    Bitmap              aBmp1;
    BitmapPalette       aGPalette;
    BitmapPalette       aLPalette;
    SvStream&           rIStm;
    std::vector<sal_uInt8> aSrcBuf;
    std::unique_ptr<GIFLZWDecompressor> pDecomp;
    BitmapScopedWriteAccess pAcc8;
    BitmapScopedWriteAccess pAcc1;
    tools::Long                nYAcc;
    tools::Long                nLastPos;
    sal_uInt64          nMaxStreamData;
    sal_uInt32          nLogWidth100;
    sal_uInt32          nLogHeight100;
    sal_uInt16          nTimer;
    sal_uInt16          nGlobalWidth;           // maximum imagewidth from header
    sal_uInt16          nGlobalHeight;          // maximum imageheight from header
    sal_uInt16          nImageWidth;            // maximum screenwidth from header
    sal_uInt16          nImageHeight;           // maximum screenheight from header
    sal_uInt16          nImagePosX;
    sal_uInt16          nImagePosY;
    sal_uInt16          nImageX;                // maximum screenwidth from header
    sal_uInt16          nImageY;                // maximum screenheight from header
    sal_uInt16          nLastImageY;
    sal_uInt16          nLastInterCount;
    sal_uInt16          nLoops;
    GIFAction           eActAction;
    bool                bStatus;
    bool                bGCTransparent;         // is the image transparent, if yes:
    bool                bInterlaced;
    bool                bOverreadBlock;
    bool                bGlobalPalette;
    sal_uInt8           nBackgroundColor;       // backgroundcolour
    sal_uInt8           nGCTransparentIndex;    // pixels of this index are transparent
    sal_uInt8           nGCDisposalMethod;      // 'Disposal Method' (see GIF docs)
    sal_uInt8           cTransIndex1;
    sal_uInt8           cNonTransIndex1;
    sal_uLong           nPaletteSize;

    void                ReadPaletteEntries( BitmapPalette* pPal, sal_uLong nCount );
    void                ClearImageExtensions();
    void                CreateBitmaps( tools::Long nWidth, tools::Long nHeight, BitmapPalette* pPal, bool bWatchForBackgroundColor );
    bool                ReadGlobalHeader();
    bool                ReadExtension();
    bool                ReadLocalHeader();
    sal_uLong           ReadNextBlock();
    void                FillImages( const sal_uInt8* pBytes, sal_uLong nCount );
    void                CreateNewBitmaps();
    bool                ProcessGIF();

public:

    ReadState           ReadGIF(ImportOutput& rImportOutput);
    bool                ReadIsAnimated();
    void GetLogicSize(Size& rLogicSize);

    explicit            GIFReader( SvStream& rStm );
};

}

GIFReader::GIFReader( SvStream& rStm )
    : nAnimationByteSize(0)
    , nAnimationMinFileData(0)
    , aGPalette ( 256 )
    , aLPalette ( 256 )
    , rIStm ( rStm )
    , nYAcc ( 0 )
    , nLastPos ( rStm.Tell() )
    , nMaxStreamData( rStm.remainingSize() )
    , nLogWidth100 ( 0 )
    , nLogHeight100 ( 0 )
    , nGlobalWidth ( 0 )
    , nGlobalHeight ( 0 )
    , nImageWidth ( 0 )
    , nImageHeight ( 0 )
    , nImagePosX ( 0 )
    , nImagePosY ( 0 )
    , nImageX ( 0 )
    , nImageY ( 0 )
    , nLastImageY ( 0 )
    , nLastInterCount ( 0 )
    , nLoops ( 1 )
    , eActAction ( GLOBAL_HEADER_READING )
    , bStatus ( false )
    , bGCTransparent  ( false )
    , bInterlaced ( false)
    , bOverreadBlock ( false )
    , bGlobalPalette ( false )
    , nBackgroundColor ( 0 )
    , nGCTransparentIndex ( 0 )
    , cTransIndex1 ( 0 )
    , cNonTransIndex1 ( 0 )
    , nPaletteSize( 0 )
{
    aSrcBuf.resize(256);    // Memory buffer for ReadNextBlock
    ClearImageExtensions();
}

void GIFReader::ClearImageExtensions()
{
    nGCDisposalMethod = 0;
    bGCTransparent = false;
    nTimer = 0;
}

void GIFReader::CreateBitmaps(tools::Long nWidth, tools::Long nHeight, BitmapPalette* pPal,
                              bool bWatchForBackgroundColor)
{
    const Size aSize(nWidth, nHeight);

    sal_uInt64 nCombinedPixSize = nWidth * nHeight;
    if (bGCTransparent)
        nCombinedPixSize += (nCombinedPixSize/8);

    // "Overall data compression asymptotically approaches 3839 × 8 / 12 = 2559 1/3"
    // so assume compression of 1:2560 is possible
    // (http://cloudinary.com/blog/a_one_color_image_is_worth_two_thousand_words suggests
    // 1:1472.88 [184.11 x 8] is more realistic)

    sal_uInt64 nMinFileData = nWidth * nHeight / 2560;

    nMinFileData += nAnimationMinFileData;
    nCombinedPixSize += nAnimationByteSize;

    if (nMaxStreamData < nMinFileData)
    {
        //there is nowhere near enough data in this stream to fill the claimed dimensions
        SAL_WARN("vcl.filter", "in gif frame index " << aAnimation.Count() << " gif claims dimensions " << nWidth << " x " << nHeight <<
                               " but filesize of " << nMaxStreamData << " is surely insufficiently large to fill all frame images");
        bStatus = false;
        return;
    }

    // Don't bother allocating a bitmap of a size that would fail on a
    // 32-bit system. We have at least one unit tests that is expected
    // to fail (loading a 65535*65535 size GIF
    // svtools/qa/cppunit/data/gif/fail/CVE-2008-5937-1.gif), but
    // which doesn't fail on 64-bit macOS at least. Why the loading
    // fails on 64-bit Linux, no idea.
    if (nCombinedPixSize >= SAL_MAX_INT32/3*2)
    {
        bStatus = false;
        return;
    }

    if (!aSize.Width() || !aSize.Height())
    {
        bStatus = false;
        return;
    }

    if (bGCTransparent)
    {
        const Color aWhite(COL_WHITE);
        const Color aBlack(COL_BLACK);

        aBmp1 = Bitmap(aSize, vcl::PixelFormat::N8_BPP, &Bitmap::GetGreyPalette(256));

        if (!aAnimation.Count())
            aBmp1.Erase(aBlack);

        pAcc1 = aBmp1;

        if (pAcc1)
        {
            // We have to make an AlphaMask from it, that needs to be inverted from transparency.
            // It is faster to invert it here.
            // So Non-Transparent color should be 0xff , and Transparent should be 0.
            cNonTransIndex1 = static_cast<sal_uInt8>(pAcc1->GetBestPaletteIndex(aWhite));
            cTransIndex1 = static_cast<sal_uInt8>(pAcc1->GetBestPaletteIndex(aBlack));
        }
        else
        {
            bStatus = false;
        }
    }

    if (bStatus)
    {
        aBmp8 = Bitmap(aSize, vcl::PixelFormat::N8_BPP, pPal);

        if (!aBmp8.IsEmpty() && bWatchForBackgroundColor && aAnimation.Count())
            aBmp8.Erase((*pPal)[nBackgroundColor]);
        else
            aBmp8.Erase(COL_WHITE);

        pAcc8 = aBmp8;
        bStatus = bool(pAcc8);
    }
}

bool GIFReader::ReadGlobalHeader()
{
    char    pBuf[ 7 ];
    bool    bRet = false;

    auto nRead = rIStm.ReadBytes(pBuf, 6);
    if (nRead == 6 && rIStm.good())
    {
        pBuf[ 6 ] = 0;
        if( !strcmp( pBuf, "GIF87a" ) || !strcmp( pBuf, "GIF89a" ) )
        {
            nRead = rIStm.ReadBytes(pBuf, 7);
            if (nRead == 7 && rIStm.good())
            {
                sal_uInt8   nAspect;
                sal_uInt8   nRF;
                SvMemoryStream aMemStm;

                aMemStm.SetBuffer( pBuf, 7, 7 );
                aMemStm.ReadUInt16( nGlobalWidth );
                aMemStm.ReadUInt16( nGlobalHeight );
                aMemStm.ReadUChar( nRF );
                aMemStm.ReadUChar( nBackgroundColor );
                aMemStm.ReadUChar( nAspect );

                bGlobalPalette = ( nRF & 0x80 );

                if( bGlobalPalette )
                    ReadPaletteEntries( &aGPalette, sal_uLong(1) << ( ( nRF & 7 ) + 1 ) );
                else
                    nBackgroundColor = 0;

                if (rIStm.good())
                    bRet = true;
            }
        }
        else
            bStatus = false;
    }

    return bRet;
}

void GIFReader::ReadPaletteEntries( BitmapPalette* pPal, sal_uLong nCount )
{
    sal_uLong nLen = 3 * nCount;
    const sal_uInt64 nMaxPossible = rIStm.remainingSize();
    if (nLen > nMaxPossible)
        nLen = nMaxPossible;
    std::unique_ptr<sal_uInt8[]> pBuf(new sal_uInt8[ nLen ]);
    std::size_t nRead = rIStm.ReadBytes(pBuf.get(), nLen);
    nCount = nRead/3UL;
    if (!rIStm.good())
        return;

    sal_uInt8* pTmp = pBuf.get();

    for (sal_uLong i = 0; i < nCount; ++i)
    {
        BitmapColor& rColor = (*pPal)[i];

        rColor.SetRed( *pTmp++ );
        rColor.SetGreen( *pTmp++ );
        rColor.SetBlue( *pTmp++ );
    }

    // if possible accommodate some standard colours
    if( nCount < 256 )
    {
        (*pPal)[ 255UL ] = COL_WHITE;

        if( nCount < 255 )
            (*pPal)[ 254UL ] = COL_BLACK;
    }

    nPaletteSize = nCount;
}

bool GIFReader::ReadExtension()
{
    bool    bRet = false;

    // Extension-Label
    sal_uInt8 cFunction(0);
    rIStm.ReadUChar( cFunction );
    if (rIStm.good())
    {
        bool    bOverreadDataBlocks = false;
        sal_uInt8 cSize(0);
        // Block length
        rIStm.ReadUChar( cSize );
        switch( cFunction )
        {
            // 'Graphic Control Extension'
            case 0xf9 :
            {
                sal_uInt8 cFlags(0);
                rIStm.ReadUChar(cFlags);
                rIStm.ReadUInt16(nTimer);
                rIStm.ReadUChar(nGCTransparentIndex);
                sal_uInt8 cByte(0);
                rIStm.ReadUChar(cByte);

                if (rIStm.good())
                {
                    nGCDisposalMethod = ( cFlags >> 2) & 7;
                    bGCTransparent = ( cFlags & 1 );
                    bStatus = ( cSize == 4 ) && ( cByte == 0 );
                    bRet = true;
                }
            }
            break;

            // Application extension
            case 0xff :
            {
                if (rIStm.good())
                {
                    // by default overread this extension
                    bOverreadDataBlocks = true;

                    // Appl. extension has length 11
                    if ( cSize == 0x0b )
                    {
                        OString aAppId = read_uInt8s_ToOString(rIStm, 8);
                        OString aAppCode = read_uInt8s_ToOString(rIStm, 3);
                        rIStm.ReadUChar( cSize );

                        // NetScape-Extension
                        if( aAppId == "NETSCAPE" && aAppCode == "2.0" && cSize == 3 )
                        {
                            sal_uInt8 cByte(0);
                            rIStm.ReadUChar( cByte );

                            // Loop-Extension
                            if ( cByte == 0x01 )
                            {
                                rIStm.ReadUChar( cByte );
                                nLoops = cByte;
                                rIStm.ReadUChar( cByte );
                                nLoops |= ( static_cast<sal_uInt16>(cByte) << 8 );
                                rIStm.ReadUChar( cByte );

                                bStatus = ( cByte == 0 );
                                bRet = rIStm.good();
                                bOverreadDataBlocks = false;

                                // Netscape interprets the loop count
                                // as pure number of _repeats_;
                                // here it is the total number of loops
                                if( nLoops )
                                    nLoops++;
                            }
                            else
                                rIStm.SeekRel( -1 );
                        }
                        else if ( aAppId == "STARDIV " && aAppCode == "5.0" && cSize == 9 )
                        {
                            sal_uInt8 cByte(0);
                            rIStm.ReadUChar( cByte );

                            // Loop extension
                            if ( cByte == 0x01 )
                            {
                                rIStm.ReadUInt32( nLogWidth100 ).ReadUInt32( nLogHeight100 );
                                rIStm.ReadUChar( cByte );
                                bStatus = ( cByte == 0 );
                                bRet = rIStm.good();
                                bOverreadDataBlocks = false;
                            }
                            else
                                rIStm.SeekRel( -1 );
                        }

                    }
                }
            }
            break;

            // overread everything else
            default:
                bOverreadDataBlocks = true;
            break;
        }

        // overread sub-blocks
        if ( bOverreadDataBlocks )
        {
            bRet = true;
            while( cSize && bStatus && !rIStm.eof() )
            {
                sal_uInt16 nCount = static_cast<sal_uInt16>(cSize) + 1;
                const sal_uInt64 nMaxPossible = rIStm.remainingSize();
                if (nCount > nMaxPossible)
                    nCount = nMaxPossible;

                if (nCount)
                    rIStm.SeekRel( nCount - 1 );    // Skip subblock data

                bRet = false;
                std::size_t nRead = rIStm.ReadBytes(&cSize, 1);
                if (rIStm.good() && nRead == 1)
                {
                    bRet = true;
                }
                else
                    cSize = 0;
            }
        }
    }

    return bRet;
}

bool GIFReader::ReadLocalHeader()
{
    sal_uInt8   pBuf[ 9 ];
    bool    bRet = false;

    std::size_t nRead = rIStm.ReadBytes(pBuf, 9);
    if (rIStm.good() && nRead == 9)
    {
        SvMemoryStream  aMemStm;
        BitmapPalette*  pPal;

        aMemStm.SetBuffer( pBuf, 9, 9 );
        aMemStm.ReadUInt16( nImagePosX );
        aMemStm.ReadUInt16( nImagePosY );
        aMemStm.ReadUInt16( nImageWidth );
        aMemStm.ReadUInt16( nImageHeight );
        sal_uInt8 nFlags(0);
        aMemStm.ReadUChar(nFlags);

        // if interlaced, first define startvalue
        bInterlaced = ( ( nFlags & 0x40 ) == 0x40 );
        nLastInterCount = 7;
        nLastImageY = 0;

        if( nFlags & 0x80 )
        {
            pPal = &aLPalette;
            ReadPaletteEntries( pPal, sal_uLong(1) << ( (nFlags & 7 ) + 1 ) );
        }
        else
            pPal = &aGPalette;

        // if we could read everything, we will create the local image;
        // if the global colour table is valid for the image, we will
        // consider the BackGroundColorIndex.
        if (rIStm.good())
        {
            CreateBitmaps( nImageWidth, nImageHeight, pPal, bGlobalPalette && ( pPal == &aGPalette ) );
            bRet = true;
        }
    }

    return bRet;
}

sal_uLong GIFReader::ReadNextBlock()
{
    sal_uLong   nRet = 0;
    sal_uInt8   cBlockSize;

    rIStm.ReadUChar( cBlockSize );

    if ( rIStm.eof() )
        nRet = 4;
    else if (rIStm.good())
    {
        if ( cBlockSize == 0 )
            nRet = 2;
        else
        {
            rIStm.ReadBytes( aSrcBuf.data(), cBlockSize );

            if (rIStm.good())
            {
                if( bOverreadBlock )
                    nRet = 3;
                else
                {
                    bool       bEOI;
                    sal_uLong  nRead;
                    sal_uInt8* pTarget = pDecomp->DecompressBlock( aSrcBuf.data(), cBlockSize, nRead, bEOI );

                    nRet = ( bEOI ? 3 : 1 );

                    if( nRead && !bOverreadBlock )
                        FillImages( pTarget, nRead );

                    std::free( pTarget );
                }
            }
        }
    }

    return nRet;
}

void GIFReader::FillImages( const sal_uInt8* pBytes, sal_uLong nCount )
{
    for( sal_uLong i = 0; i < nCount; i++ )
    {
        if( nImageX >= nImageWidth )
        {
            if( bInterlaced )
            {
                tools::Long nT1;

                // lines will be copied if interlaced
                if( nLastInterCount )
                {
                    tools::Long nMinY = std::min( static_cast<tools::Long>(nLastImageY) + 1, static_cast<tools::Long>(nImageHeight) - 1 );
                    tools::Long nMaxY = std::min( static_cast<tools::Long>(nLastImageY) + nLastInterCount, static_cast<tools::Long>(nImageHeight) - 1 );

                    // copy last line read, if lines do not coincide
                    // ( happens at the end of the image )
                    if( ( nMinY > nLastImageY ) && ( nLastImageY < ( nImageHeight - 1 ) ) )
                    {
                        sal_uInt8*  pScanline8 = pAcc8->GetScanline( nYAcc );
                        sal_uInt32  nSize8 = pAcc8->GetScanlineSize();
                        sal_uInt8*  pScanline1 = nullptr;
                        sal_uInt32  nSize1 = 0;

                        if( bGCTransparent )
                        {
                            pScanline1 = pAcc1->GetScanline( nYAcc );
                            nSize1 = pAcc1->GetScanlineSize();
                        }

                        for( tools::Long j = nMinY; j <= nMaxY; j++ )
                        {
                            memcpy( pAcc8->GetScanline( j ), pScanline8, nSize8 );

                            if( bGCTransparent )
                                memcpy( pAcc1->GetScanline( j ), pScanline1, nSize1 );
                        }
                    }
                }

                nT1 = ( ++nImageY ) << 3;
                nLastInterCount = 7;

                if( nT1 >= nImageHeight )
                {
                    tools::Long nT2 = nImageY - ( ( nImageHeight + 7 ) >> 3 );
                    nT1 = ( nT2 << 3 ) + 4;
                    nLastInterCount = 3;

                    if( nT1 >= nImageHeight )
                    {
                        nT2 -= ( nImageHeight + 3 ) >> 3;
                        nT1 = ( nT2 << 2 ) + 2;
                        nLastInterCount = 1;

                        if( nT1 >= nImageHeight )
                        {
                            nT2 -= ( nImageHeight + 1 ) >> 2;
                            nT1 = ( nT2 << 1 ) + 1;
                            nLastInterCount = 0;
                        }
                    }
                }

                nLastImageY = static_cast<sal_uInt16>(nT1);
                nYAcc = nT1;
            }
            else
            {
                nLastImageY = ++nImageY;
                nYAcc = nImageY;
            }

            // line starts from the beginning
            nImageX = 0;
        }

        if( nImageY < nImageHeight )
        {
            const sal_uInt8 cTmp = pBytes[ i ];

            if( bGCTransparent )
            {
                if( cTmp == nGCTransparentIndex )
                    pAcc1->SetPixelIndex( nYAcc, nImageX++, cTransIndex1 );
                else
                {
                    pAcc8->SetPixelIndex( nYAcc, nImageX, cTmp );
                    pAcc1->SetPixelIndex( nYAcc, nImageX++, cNonTransIndex1 );
                }
            }
            else
                pAcc8->SetPixelIndex( nYAcc, nImageX++, cTmp );
        }
        else
        {
            bOverreadBlock = true;
            break;
        }
    }
}

void GIFReader::CreateNewBitmaps()
{
    AnimationFrame aAnimationFrame;

    pAcc8.reset();

    if( bGCTransparent )
    {
        pAcc1.reset();
        AlphaMask aAlphaMask(aBmp1);
        // No need to convert from transparency to alpha
        // aBmp1 is already inverted
        aAnimationFrame.maBitmapEx = BitmapEx( aBmp8, aAlphaMask );
    }
    else if( nPaletteSize > 2 )
    {
        // tdf#160690 set an opaque alpha mask for non-transparent frames
        // Due to the switch from transparency to alpha in commit
        // 81994cb2b8b32453a92bcb011830fcb884f22ff3, an empty alpha mask
        // is treated as a completely transparent bitmap. So revert all
        // of the previous commits for tdf#157576, tdf#157635, and tdf#157793
        // and create a completely opaque bitmap instead.
        // Note: this fix also fixes tdf#157576, tdf#157635, and tdf#157793.
        AlphaMask aAlphaMask(aBmp8.GetSizePixel());
        aAnimationFrame.maBitmapEx = BitmapEx( aBmp8, aAlphaMask );
    }
    else
    {
        // Don't apply the fix for tdf#160690 as it will cause 1 bit bitmaps
        // in Word documents like the following test document to fail to be
        // parsed correctly:
        // sw/qa/extras/tiledrendering/data/tdf159626_yellowPatternFill.docx
        aAnimationFrame.maBitmapEx = BitmapEx( aBmp8 );
    }

    aAnimationFrame.maPositionPixel = Point( nImagePosX, nImagePosY );
    aAnimationFrame.maSizePixel = Size( nImageWidth, nImageHeight );
    aAnimationFrame.mnWait = ( nTimer != 65535 ) ? nTimer : ANIMATION_TIMEOUT_ON_CLICK;
    aAnimationFrame.mbUserInput = false;

    // tdf#104121 . Internet Explorer, Firefox, Chrome and Safari all set a minimum default playback speed.
    // IE10 Consumer Preview sets default of 100ms for rates less that 20ms. We do the same
    if (aAnimationFrame.mnWait < 2) // 20ms, specified in 100's of a second
        aAnimationFrame.mnWait = 10;

    if( nGCDisposalMethod == 2 )
        aAnimationFrame.meDisposal = Disposal::Back;
    else if( nGCDisposalMethod == 3 )
        aAnimationFrame.meDisposal = Disposal::Previous;
    else
        aAnimationFrame.meDisposal = Disposal::Not;

    nAnimationByteSize += aAnimationFrame.maBitmapEx.GetSizeBytes();
    nAnimationMinFileData += static_cast<sal_uInt64>(nImageWidth) * nImageHeight / 2560;
    aAnimation.Insert(aAnimationFrame);

    if( aAnimation.Count() == 1 )
    {
        aAnimation.SetDisplaySizePixel( Size( nGlobalWidth, nGlobalHeight ) );
        aAnimation.SetLoopCount( nLoops );
    }
}

bool GIFReader::ProcessGIF()
{
    bool bRead = false;
    bool bEnd = false;

    if ( !bStatus )
        eActAction = ABORT_READING;

    // set stream to right position
    rIStm.Seek( nLastPos );

    switch( eActAction )
    {
        // read next marker
        case MARKER_READING:
        {
            sal_uInt8 cByte;

            rIStm.ReadUChar( cByte );

            if( rIStm.eof() )
                eActAction = END_READING;
            else if (rIStm.good())
            {
                bRead = true;

                if( cByte == '!' )
                    eActAction = EXTENSION_READING;
                else if( cByte == ',' )
                    eActAction = LOCAL_HEADER_READING;
                else if( cByte == ';' )
                    eActAction = END_READING;
                else
                    eActAction = ABORT_READING;
            }
        }
        break;

        // read ScreenDescriptor
        case GLOBAL_HEADER_READING:
        {
            bRead = ReadGlobalHeader();
            if( bRead )
            {
                ClearImageExtensions();
                eActAction = MARKER_READING;
            }
        }
        break;

        // read extension
        case EXTENSION_READING:
        {
            bRead = ReadExtension();
            if( bRead )
                eActAction = MARKER_READING;
        }
        break;

        // read Image-Descriptor
        case LOCAL_HEADER_READING:
        {
            bRead = ReadLocalHeader();
            if( bRead )
            {
                nYAcc = nImageX = nImageY = 0;
                eActAction = FIRST_BLOCK_READING;
            }
        }
        break;

        // read first data block
        case FIRST_BLOCK_READING:
        {
            sal_uInt8 cDataSize;

            rIStm.ReadUChar( cDataSize );

            if( rIStm.eof() )
                eActAction = ABORT_READING;
            else if( cDataSize > 12 )
                bStatus = false;
            else if (rIStm.good())
            {
                bRead = true;
                pDecomp = std::make_unique<GIFLZWDecompressor>( cDataSize );
                eActAction = NEXT_BLOCK_READING;
                bOverreadBlock = false;
            }
            else
                eActAction = FIRST_BLOCK_READING;
        }
        break;

        // read next data block
        case NEXT_BLOCK_READING:
        {
            sal_uInt16  nLastX = nImageX;
            sal_uInt16  nLastY = nImageY;
            sal_uLong   nRet = ReadNextBlock();

            // Return: 0:Pending / 1:OK; / 2:OK and last block: / 3:EOI / 4:HardAbort
            if( nRet )
            {
                bRead = true;

                if ( nRet == 1 )
                {
                    eActAction = NEXT_BLOCK_READING;
                    bOverreadBlock = false;
                }
                else
                {
                    if( nRet == 2 )
                    {
                        pDecomp.reset();
                        CreateNewBitmaps();
                        eActAction = MARKER_READING;
                        ClearImageExtensions();
                    }
                    else if( nRet == 3 )
                    {
                        eActAction = NEXT_BLOCK_READING;
                        bOverreadBlock = true;
                    }
                    else
                    {
                        pDecomp.reset();
                        CreateNewBitmaps();
                        eActAction = ABORT_READING;
                        ClearImageExtensions();
                    }
                }
            }
            else
            {
                nImageX = nLastX;
                nImageY = nLastY;
            }
        }
        break;

        // an error occurred
        case ABORT_READING:
        {
            bEnd = true;
            eActAction = END_READING;
        }
        break;

        default:
        break;
    }

    // set stream to right position,
    // if data could be read put it at the old
    // position otherwise at the actual one
    if( bRead || bEnd )
        nLastPos = rIStm.Tell();

    return bRead;
}

bool GIFReader::ReadIsAnimated()
{
    bStatus = true;
    while (ProcessGIF() && eActAction != END_READING)
    {}

    ReadState eReadState = GIFREAD_ERROR;

    if (!bStatus)
        eReadState = GIFREAD_ERROR;
    else if (eActAction == END_READING)
        eReadState = GIFREAD_OK;

    if (eReadState == GIFREAD_OK)
        return aAnimation.Count() > 1;
    return false;
}

void GIFReader::GetLogicSize(Size& rLogicSize)
{
    rLogicSize.setWidth(nLogWidth100);
    rLogicSize.setHeight(nLogHeight100);
}

ReadState GIFReader::ReadGIF(ImportOutput& rImportOutput)
{
    bStatus = true;

    while (ProcessGIF() && eActAction != END_READING)
    {}

    ReadState eReadState = GIFREAD_ERROR;

    if (!bStatus)
        eReadState = GIFREAD_ERROR;
    else if (eActAction == END_READING)
        eReadState = GIFREAD_OK;

    Size aPrefSize;
    if (nLogWidth100 && nLogHeight100)
    {
        aPrefSize = Size(nLogWidth100, nLogHeight100);
    }

    if (aAnimation.Count() == 1)
    {
        rImportOutput.mbIsAnimated = false;
        rImportOutput.moBitmap = aAnimation.Get(0).maBitmapEx;

        if (aPrefSize.Width() && aPrefSize.Height())
        {
            rImportOutput.moBitmap->SetPrefSize(aPrefSize);
            rImportOutput.moBitmap->SetPrefMapMode(MapMode(MapUnit::Map100thMM));
        }
    }
    else
    {
        rImportOutput.mbIsAnimated = true;
        rImportOutput.moAnimation = aAnimation;

        if (aPrefSize.Width() && aPrefSize.Height())
        {
            BitmapEx& rBitmap = const_cast<BitmapEx&>(rImportOutput.moAnimation->GetBitmapEx());
            rBitmap.SetPrefSize(aPrefSize);
            rBitmap.SetPrefMapMode(MapMode(MapUnit::Map100thMM));
        }
    }

    return eReadState;
}

bool IsGIFAnimated(SvStream& rStream, Size& rLogicSize)
{
    GIFReader aReader(rStream);

    SvStreamEndian nOldFormat = rStream.GetEndian();
    rStream.SetEndian(SvStreamEndian::LITTLE);
    bool bResult = aReader.ReadIsAnimated();
    aReader.GetLogicSize(rLogicSize);
    rStream.SetEndian(nOldFormat);

    return bResult;
}

VCL_DLLPUBLIC bool ImportGIF(SvStream & rStream, ImportOutput& rImportOutput)
{
    bool bReturn = false;
    GIFReader aGIFReader(rStream);

    SvStreamEndian nOldFormat = rStream.GetEndian();
    rStream.SetEndian(SvStreamEndian::LITTLE);

    ReadState eReadState = aGIFReader.ReadGIF(rImportOutput);

    if (eReadState == GIFREAD_OK)
        bReturn = true;

    rStream.SetEndian(nOldFormat);
    return bReturn;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

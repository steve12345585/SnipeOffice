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

#include <vcl/gdimtf.hxx>
#include <vcl/lineinfo.hxx>
#include <vcl/virdev.hxx>
#include <vcl/FilterConfigItem.hxx>
#include <com/sun/star/task/XStatusIndicator.hpp>
#include <tools/stream.hxx>

#define MAXOBJECTHANDLES 16

struct WMFWriterAttrStackMember
{
    struct WMFWriterAttrStackMember * pSucc;
    Color aLineColor;
    Color aFillColor;
    Color aTextColor;
    LineInfo aLineInfo;
    TextAlign eTextAlign;
    RasterOp eRasterOp;
    vcl::Font aFont;
    MapMode aMapMode;
    vcl::Region aClipRegion;
    vcl::PushFlags nFlags;
};

class StarSymbolToMSMultiFont;
class LineInfo;
namespace basegfx { class B2DPolygon; }

class WMFWriter
{
private:

    bool            bStatus;

    sal_uLong                   nLastPercent; // with which number pCallback was called last time.

    css::uno::Reference< css::task::XStatusIndicator > xStatusIndicator;

    SvStream*               pWMF;
    VclPtr<VirtualDevice>   pVirDev;
    MapMode                 aTargetMapMode;
    Size                    aTargetSize;

    sal_uLong nMetafileHeaderPos;
    sal_uInt32 nMaxRecordSize; // in words
    sal_uLong nActRecordPos;

    // actual attribute in source metafile:
    Color     aSrcLineColor;
    Color     aSrcFillColor;
    Color     aSrcTextColor;
    LineInfo  aSrcLineInfo;
    RasterOp  eSrcRasterOp;
    TextAlign eSrcTextAlign;
    vcl::Font aSrcFont;
    MapMode   aSrcMapMode;
    vcl::Region    aSrcClipRegion;
    WMFWriterAttrStackMember * pAttrStack;

    sal_uInt16    eSrcHorTextAlign;

    // actual attribute in destination metafile:
    Color     aDstLineColor;
    Color     aDstFillColor;
    Color     aDstTextColor;
    LineInfo  aDstLineInfo;
    RasterOp  eDstROP2;
    TextAlign eDstTextAlign;
    vcl::Font aDstFont;

    sal_uInt16    eDstHorTextAlign;

    vcl::Region    aDstClipRegion; // ???: not taken into account at the moment
    bool bHandleAllocated[MAXOBJECTHANDLES];             // which handles have been assigned
    sal_uInt16 nDstPenHandle,nDstFontHandle,nDstBrushHandle; // which handles are owned by
                                                         // Selected-Objects
                                                         // 0xFFFF = none:

    // to prevent we have to compare all attributes at each operation:

    sal_uLong nNumberOfActions; // number of actions in the GDIMetafile
    sal_uLong nNumberOfBitmaps; // number of bitmaps
    sal_uLong nWrittenActions;  // number of processed actions while writing the directory
    sal_uLong nWrittenBitmaps;  // number of bitmaps written
    sal_uLong nActBitmapPercent; // percentage of next bitmap written.

    bool bEmbedEMF; // optionally embed EMF data into WMF

    void MayCallback();
        // this function calculates percentage using the above 5 parameters
        // and triggers a callback if needed. Puts bStatus to FALSE if the
        // users wants to abort.

    void CountActionsAndBitmaps(const GDIMetaFile & rMTF);
        // Counts bitmaps and actions (nNumberOfActions and nNumberOfBitmaps should
        // be initialised to 0 at start, as this method is recursive)

    void WritePointXY(const Point & rPoint);
    void WritePointYX(const Point & rPoint);
    sal_Int32 ScaleWidth( sal_Int32 nDX );
    void WriteSize(const Size & rSize);
    void WriteHeightWidth(const Size & rSize);
    void WriteRectangle(const tools::Rectangle & rRect);
    void WriteColor(const Color & rColor);

    void WriteRecordHeader(sal_uInt32 nSizeWords, sal_uInt16 nType);
        // nSizeWords is the size of the all records in number of words.
        // If nSizeWords is unknown, then use 0 (see UpdateRecordHeader())

    void UpdateRecordHeader();
        // returns the size of the record after writing the parameters, if
        // nSizeWords was unknown upon calling WriteRecordHeader(..)
        // if needed it inserts a BYTE 0 to make number of bytes even

    void WMFRecord_Arc(const tools::Rectangle& rRect, const Point& rStartPt, const Point& rEndPt);
    void WMFRecord_Chord(const tools::Rectangle& rRect, const Point& rStartPt, const Point& rEndPt);
    void WMFRecord_CreateBrushIndirect(const Color& rColor);
    void WMFRecord_CreateFontIndirect(const vcl::Font& rFont);
    void WMFRecord_CreatePenIndirect(const Color& rColor, const LineInfo& rLineInfo );
    void WMFRecord_DeleteObject(sal_uInt16 nObjectHandle);
    void WMFRecord_Ellipse(const tools::Rectangle& rRect);
    void WMFRecord_Escape( sal_uInt32 nEsc, sal_uInt32 nLen, const sal_Int8* pData );
    bool WMFRecord_Escape_Unicode( const Point& rPoint, const OUString& rStr, KernArraySpan pDXAry );
    void WMFRecord_ExtTextOut(const Point& rPoint, std::u16string_view rString, KernArraySpan pDXAry);

    void TrueExtTextOut(const Point& rPoint, std::u16string_view rString,
                        const OString& rByteString, KernArraySpan pDXAry);
    void TrueTextOut(const Point& rPoint, const OString& rString);
    void WMFRecord_LineTo(const Point & rPoint);
    void WMFRecord_MoveTo(const Point & rPoint);
    void WMFRecord_Pie(const tools::Rectangle & rRect, const Point & rStartPt, const Point & rEndPt);
    void WMFRecord_Polygon(const tools::Polygon & rPoly);
    void WMFRecord_PolyLine(const tools::Polygon & rPoly);
    void WMFRecord_PolyPolygon(const tools::PolyPolygon & rPolyPoly);
    void WMFRecord_Rectangle(const tools::Rectangle & rRect);
    void WMFRecord_RestoreDC();
    void WMFRecord_RoundRect(const tools::Rectangle & rRect, tools::Long nHorzRound, tools::Long nVertRound);
    void WMFRecord_SaveDC();
    void WMFRecord_SelectObject(sal_uInt16 nObjectHandle);
    void WMFRecord_SetBkMode(bool bTransparent);
    void WMFRecord_SetStretchBltMode();
    void WMFRecord_SetPixel(const Point & rPoint, const Color & rColor);
    void WMFRecord_SetROP2(RasterOp eROP);
    void WMFRecord_SetTextAlign(TextAlign eFontAlign, sal_uInt16 eHorTextAlign);
    void WMFRecord_SetTextColor(const Color & rColor);
    void WMFRecord_SetWindowExt(const Size & rSize);
    void WMFRecord_SetWindowOrg(const Point & rPoint);
    void WMFRecord_StretchDIB(const Point & rPoint, const Size & rSize, const Bitmap & rBitmap, sal_uInt32 nROP = 0 );
    void WMFRecord_TextOut(const Point & rPoint, std::u16string_view rString);
    void WMFRecord_IntersectClipRect( const tools::Rectangle& rRect);

    sal_uInt16 AllocHandle();
    void FreeHandle(sal_uInt16 nObjectHandle);
    void CreateSelectDeletePen( const Color& rColor, const LineInfo& rLineInfo );
    void CreateSelectDeleteFont(const vcl::Font & rFont);
    void CreateSelectDeleteBrush(const Color& rColor);

    void SetLineAndFillAttr();
    void SetAllAttr();

    void HandleLineInfoPolyPolygons(const LineInfo& rInfo, const basegfx::B2DPolygon& rLinePolygon);
    void WriteRecords(const GDIMetaFile & rMTF);

    void WriteHeader(bool bPlaceable);
    void UpdateHeader();

    void WriteEmbeddedEMF( const GDIMetaFile& rMTF );
    void WriteEMFRecord( SvMemoryStream& rStream, sal_uInt32 nCurSize,
                            sal_uInt32 nRemainingSize,
                            sal_uInt32 nTotalSize,
                            sal_uInt32 nRecCounts,
                            sal_uInt16 nCheckSum );

    sal_uInt16 CalcSaveTargetMapMode(MapMode& rMapMode, const Size& rPrefSize);

public:
    WMFWriter();
    bool WriteWMF(const GDIMetaFile & rMTF, SvStream & rTargetStream, FilterConfigItem const * pFilterConfigItem, bool bPlaceable);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

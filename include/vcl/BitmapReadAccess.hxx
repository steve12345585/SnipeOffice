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

#include <vcl/dllapi.h>
#include <vcl/bitmap.hxx>
#include <vcl/Scanline.hxx>
#include <vcl/BitmapBuffer.hxx>
#include <vcl/BitmapColor.hxx>
#include <vcl/BitmapAccessMode.hxx>
#include <vcl/BitmapInfoAccess.hxx>

class SAL_DLLPUBLIC_RTTI BitmapReadAccess : public BitmapInfoAccess
{
    friend class BitmapWriteAccess;

public:
    VCL_DLLPUBLIC BitmapReadAccess(const Bitmap& rBitmap,
                                   BitmapAccessMode nMode = BitmapAccessMode::Read);
    VCL_DLLPUBLIC BitmapReadAccess(const AlphaMask& rBitmap,
                                   BitmapAccessMode nMode = BitmapAccessMode::Read);
    VCL_DLLPUBLIC virtual ~BitmapReadAccess() override;

    Scanline GetBuffer() const
    {
        assert(mpBuffer && "Access is not valid!");

        return mpBuffer ? mpBuffer->mpBits : nullptr;
    }

    Scanline GetScanline(tools::Long nY) const
    {
        assert(mpBuffer && "Access is not valid!");
        assert(nY < mpBuffer->mnHeight && "y-coordinate out of range!");

        if (mpBuffer->meDirection == ScanlineDirection::TopDown)
        {
            return mpBuffer->mpBits + (nY * mpBuffer->mnScanlineSize);
        }
        return mpBuffer->mpBits + ((mpBuffer->mnHeight - 1 - nY) * mpBuffer->mnScanlineSize);
    }

    BitmapColor GetPixelFromData(const sal_uInt8* pData, tools::Long nX) const
    {
        assert(pData && "Access is not valid!");

        return mFncGetPixel(pData, nX);
    }

    sal_uInt8 GetIndexFromData(const sal_uInt8* pData, tools::Long nX) const
    {
        return GetPixelFromData(pData, nX).GetIndex();
    }

    void SetPixelOnData(sal_uInt8* pData, tools::Long nX, const BitmapColor& rBitmapColor)
    {
        assert(pData && "Access is not valid!");

        mFncSetPixel(pData, nX, rBitmapColor);
    }

    BitmapColor GetPixel(tools::Long nY, tools::Long nX) const
    {
        assert(mpBuffer && "Access is not valid!");
        assert(nX < mpBuffer->mnWidth && "x-coordinate out of range!");

        return GetPixelFromData(GetScanline(nY), nX);
    }

    BitmapColor GetPixel(const Point& point) const { return GetPixel(point.Y(), point.X()); }

    BitmapColor GetColorFromData(const sal_uInt8* pData, tools::Long nX) const
    {
        if (HasPalette())
            return GetPaletteColor(GetIndexFromData(pData, nX));
        else
            return GetPixelFromData(pData, nX);
    }

    BitmapColor GetColor(tools::Long nY, tools::Long nX) const
    {
        assert(mpBuffer && "Access is not valid!");
        assert(nX < mpBuffer->mnWidth && "x-coordinate out of range!");
        return GetColorFromData(GetScanline(nY), nX);
    }

    BitmapColor GetColor(const Point& point) const { return GetColor(point.Y(), point.X()); }

    sal_uInt8 GetPixelIndex(tools::Long nY, tools::Long nX) const
    {
        return GetPixel(nY, nX).GetIndex();
    }

    sal_uInt8 GetPixelIndex(const Point& point) const
    {
        return GetPixelIndex(point.Y(), point.X());
    }

    /** Get the interpolated color at coordinates fY, fX; if outside, return rFallback */
    BitmapColor GetInterpolatedColorWithFallback(double fY, double fX,
                                                 const BitmapColor& rFallback) const;

    /** Get the color at coordinates fY, fX; if outside, return rFallback. Automatically does the correct
        inside/outside checks, e.g. static_cast< sal_uInt32 >(-0.25) *is* 0, not -1 and has to be outside */
    BitmapColor GetColorWithFallback(double fY, double fX, const BitmapColor& rFallback) const;

private:
    BitmapReadAccess(const BitmapReadAccess&) = delete;
    BitmapReadAccess& operator=(const BitmapReadAccess&) = delete;

protected:
    FncGetPixel mFncGetPixel;
    FncSetPixel mFncSetPixel;

public:
    BitmapBuffer* ImplGetBitmapBuffer() const { return mpBuffer; }

    static BitmapColor GetPixelForN1BitMsbPal(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN8BitPal(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN24BitTcBgr(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN24BitTcRgb(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcAbgr(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcXbgr(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcArgb(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcXrgb(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcBgra(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcBgrx(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcRgba(ConstScanline pScanline, tools::Long nX);
    static BitmapColor GetPixelForN32BitTcRgbx(ConstScanline pScanline, tools::Long nX);

    static void SetPixelForN1BitMsbPal(Scanline pScanline, tools::Long nX,
                                       const BitmapColor& rBitmapColor);
    static void SetPixelForN8BitPal(Scanline pScanline, tools::Long nX,
                                    const BitmapColor& rBitmapColor);
    static void SetPixelForN24BitTcBgr(Scanline pScanline, tools::Long nX,
                                       const BitmapColor& rBitmapColor);
    static void SetPixelForN24BitTcRgb(Scanline pScanline, tools::Long nX,
                                       const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcAbgr(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcXbgr(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcArgb(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcXrgb(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcBgra(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcBgrx(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcRgba(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);
    static void SetPixelForN32BitTcRgbx(Scanline pScanline, tools::Long nX,
                                        const BitmapColor& rBitmapColor);

    static FncGetPixel GetPixelFunction(ScanlineFormat nFormat);
    static FncSetPixel SetPixelFunction(ScanlineFormat nFormat);
};

class BitmapScopedReadAccess
{
public:
    BitmapScopedReadAccess(const Bitmap& rBitmap)
        : moAccess(rBitmap)
    {
    }
    BitmapScopedReadAccess(const AlphaMask& rBitmap)
        : moAccess(rBitmap)
    {
    }
    BitmapScopedReadAccess() {}

    BitmapScopedReadAccess& operator=(const Bitmap& rBitmap)
    {
        moAccess.emplace(rBitmap);
        return *this;
    }

    BitmapScopedReadAccess& operator=(const AlphaMask& rBitmap)
    {
        moAccess.emplace(rBitmap);
        return *this;
    }

    bool operator!() const { return !moAccess.has_value() || !*moAccess; }
    explicit operator bool() const { return moAccess && bool(*moAccess); }

    void reset() { moAccess.reset(); }

    BitmapReadAccess* get() { return moAccess ? &*moAccess : nullptr; }
    const BitmapReadAccess* get() const { return moAccess ? &*moAccess : nullptr; }

    BitmapReadAccess* operator->() { return &*moAccess; }
    const BitmapReadAccess* operator->() const { return &*moAccess; }

    BitmapReadAccess& operator*() { return *moAccess; }
    const BitmapReadAccess& operator*() const { return *moAccess; }

private:
    std::optional<BitmapReadAccess> moAccess;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

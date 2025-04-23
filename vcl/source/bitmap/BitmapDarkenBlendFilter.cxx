/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <comphelper/diagnose_ex.hxx>

#include <vcl/bitmap/BitmapDarkenBlendFilter.hxx>
#include <vcl/BitmapWriteAccess.hxx>
#include <vcl/BitmapTools.hxx>

BitmapDarkenBlendFilter::BitmapDarkenBlendFilter(BitmapEx const& rBitmapBlendEx)
    : maBlendBitmapBitmapEx(rBitmapBlendEx)
{
}

static sal_uInt8 lcl_calculate(const sal_uInt8 aColor, const sal_uInt8 aAlpha,
                               const sal_uInt8 aColor2, const sal_uInt8 aAlpha2)
{
    const double c1 = aColor / 255.0;
    const double c2 = aColor2 / 255.0;
    const double a1 = aAlpha / 255.0;
    const double a2 = aAlpha2 / 255.0;
    const double result = std::min((1.0 - a1) * c2 + c1, (1.0 - a2) * c1 + c2);
    return result * 255.0;
}

BitmapEx BitmapDarkenBlendFilter::execute(BitmapEx const& rBitmapBlendEx) const
{
    if (rBitmapBlendEx.IsEmpty() || maBlendBitmapBitmapEx.IsEmpty())
        return BitmapEx();

    const Size aSize = rBitmapBlendEx.GetBitmap().GetSizePixel();
    const Size aSize2 = maBlendBitmapBitmapEx.GetBitmap().GetSizePixel();
    const sal_Int32 nHeight = std::min(aSize.getHeight(), aSize2.getHeight());
    const sal_Int32 nWidth = std::min(aSize.getWidth(), aSize2.getWidth());

    Bitmap aDstBitmap(Size(nWidth, nHeight), vcl::PixelFormat::N24_BPP);
    Bitmap aDstAlpha(AlphaMask(Size(nWidth, nHeight)).GetBitmap());

    BitmapScopedWriteAccess pWriteAccess(aDstBitmap);
    BitmapScopedWriteAccess pAlphaWriteAccess(aDstAlpha);

    for (tools::Long y(0); y < nHeight; ++y)
    {
        Scanline pScanline = pWriteAccess->GetScanline(y);
        Scanline pScanAlpha = pAlphaWriteAccess->GetScanline(y);
        for (tools::Long x(0); x < nWidth; ++x)
        {
            const BitmapColor i1 = vcl::bitmap::premultiply(rBitmapBlendEx.GetPixelColor(x, y));
            const BitmapColor i2
                = vcl::bitmap::premultiply(maBlendBitmapBitmapEx.GetPixelColor(x, y));
            const sal_uInt8 r(
                lcl_calculate(i1.GetRed(), i1.GetAlpha(), i2.GetRed(), i2.GetAlpha()));
            const sal_uInt8 g(
                lcl_calculate(i1.GetGreen(), i1.GetAlpha(), i2.GetGreen(), i2.GetAlpha()));
            const sal_uInt8 b(
                lcl_calculate(i1.GetBlue(), i1.GetAlpha(), i2.GetBlue(), i2.GetAlpha()));
            const sal_uInt8 a(
                lcl_calculate(i1.GetAlpha(), i1.GetAlpha(), i2.GetAlpha(), i2.GetAlpha()));

            pWriteAccess->SetPixelOnData(
                pScanline, x, vcl::bitmap::unpremultiply(BitmapColor(ColorAlpha, r, g, b, a)));
            pAlphaWriteAccess->SetPixelOnData(pScanAlpha, x, BitmapColor(a));
        }
    }

    pWriteAccess.reset();
    pAlphaWriteAccess.reset();

    return BitmapEx(aDstBitmap, AlphaMask(aDstAlpha));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

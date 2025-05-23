/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <test/outputdevice.hxx>

namespace vcl::test {

Bitmap OutputDeviceTestAnotherOutDev::setupDrawOutDev()
{
    ScopedVclPtrInstance<VirtualDevice> pSourceDev;
    Size aSourceSize(9, 9);
    pSourceDev->SetOutputSizePixel(aSourceSize);
    pSourceDev->SetBackground(Wallpaper(constFillColor));
    pSourceDev->Erase();

    initialSetup(13, 13, constBackgroundColor);

    mpVirtualDevice->DrawOutDev(Point(2, 2), aSourceSize, Point(), aSourceSize, *pSourceDev);

    return mpVirtualDevice->GetBitmap(maVDRectangle.TopLeft(), maVDRectangle.GetSize());
}


Bitmap OutputDeviceTestAnotherOutDev::setupDrawOutDevScaledClipped()
{
    ScopedVclPtrInstance<VirtualDevice> pSourceDev;
    Size aSourceSize(18, 18);
    pSourceDev->SetOutputSizePixel(aSourceSize);
    pSourceDev->SetBackground(Wallpaper(constFillColor));
    pSourceDev->Erase();

    initialSetup(13, 13, constBackgroundColor);

    tools::Rectangle rectangle = maVDRectangle;
    rectangle.SetLeft(rectangle.GetWidth() / 2);
    mpVirtualDevice->SetClipRegion(vcl::Region(rectangle));

    mpVirtualDevice->DrawOutDev(Point(2, 2), aSourceSize / 2, Point(), aSourceSize, *pSourceDev);

    return mpVirtualDevice->GetBitmap(maVDRectangle.TopLeft(), maVDRectangle.GetSize());
}

Bitmap OutputDeviceTestAnotherOutDev::setupDrawOutDevSelf()
{
    initialSetup(13, 13, constBackgroundColor);

    mpVirtualDevice->SetLineColor();
    mpVirtualDevice->SetFillColor(constFillColor);

    tools::Rectangle aDrawRectangle(maVDRectangle);
    aDrawRectangle.shrink(3);
    aDrawRectangle.Move( 2, -2 );
    mpVirtualDevice->DrawRect(aDrawRectangle);
    mpVirtualDevice->SetLineColor(COL_YELLOW);
    mpVirtualDevice->DrawPixel(aDrawRectangle.TopLeft() + Point(aDrawRectangle.GetWidth() - 1, 0));
    mpVirtualDevice->DrawPixel(aDrawRectangle.TopLeft() + Point(0,aDrawRectangle.GetHeight() - 1));

    // Intentionally overlap a bit.
    mpVirtualDevice->DrawOutDev(Point(1, 5), aDrawRectangle.GetSize(),
                                Point(5,1), aDrawRectangle.GetSize(), *mpVirtualDevice);

    return mpVirtualDevice->GetBitmap(maVDRectangle.TopLeft(), maVDRectangle.GetSize());
}

Bitmap OutputDeviceTestAnotherOutDev::setupXOR()
{
    initialSetup(13, 13, constBackgroundColor);

    tools::Rectangle aDrawRectangle(maVDRectangle);
    aDrawRectangle.shrink(2);

    tools::Rectangle aScissorRectangle(maVDRectangle);
    aScissorRectangle.shrink(4);

    mpVirtualDevice->SetRasterOp(RasterOp::Xor);
    mpVirtualDevice->SetFillColor(constFillColor);
    mpVirtualDevice->DrawRect(aDrawRectangle);

    mpVirtualDevice->SetRasterOp(RasterOp::N0);
    mpVirtualDevice->SetFillColor(COL_BLACK);
    mpVirtualDevice->DrawRect(aScissorRectangle);

    mpVirtualDevice->SetRasterOp(RasterOp::Xor);
    mpVirtualDevice->SetFillColor(constFillColor);
    mpVirtualDevice->DrawRect(aDrawRectangle);

    mpVirtualDevice->SetRasterOp(RasterOp::Xor);
    mpVirtualDevice->SetLineColor(constFillColor);
    mpVirtualDevice->SetFillColor();
    // Rectangle drawn twice is a no-op.
    aDrawRectangle = maVDRectangle;
    mpVirtualDevice->DrawRect(aDrawRectangle);
    mpVirtualDevice->DrawRect(aDrawRectangle);
    // Rectangle drawn three times is like drawing once.
    aDrawRectangle.shrink(1);
    mpVirtualDevice->DrawRect(aDrawRectangle);
    mpVirtualDevice->DrawRect(aDrawRectangle);
    mpVirtualDevice->DrawRect(aDrawRectangle);

    return mpVirtualDevice->GetBitmap(maVDRectangle.TopLeft(), maVDRectangle.GetSize());
}

} // end namespace vcl::test

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

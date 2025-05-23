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

#include <vcl/toolkit/imgctrl.hxx>

#include <com/sun/star/awt/ImageScaleMode.hpp>
#include <osl/diagnose.h>

namespace ImageScaleMode = css::awt::ImageScaleMode;

ImageControl::ImageControl( vcl::Window* pParent, WinBits nStyle )
    :FixedImage( pParent, nStyle )
    ,mnScaleMode( ImageScaleMode::ANISOTROPIC )
{
}

void ImageControl::SetScaleMode( const ::sal_Int16 _nMode )
{
    if ( _nMode != mnScaleMode )
    {
        mnScaleMode = _nMode;
        Invalidate();
    }
}

void ImageControl::Resize()
{
    Invalidate();
}

namespace
{
    Size lcl_calcPaintSize( const tools::Rectangle& _rPaintRect, const Size& _rBitmapSize )
    {
        const Size aPaintSize = _rPaintRect.GetSize();

        const double nRatioX = 1.0 * aPaintSize.Width() / _rBitmapSize.Width();
        const double nRatioY = 1.0 * aPaintSize.Height() / _rBitmapSize.Height();
        const double nRatioMin = ::std::min( nRatioX, nRatioY );

        return Size( tools::Long( _rBitmapSize.Width() * nRatioMin ), tools::Long( _rBitmapSize.Height() * nRatioMin ) );
    }

    Point lcl_centerWithin( const tools::Rectangle& _rArea, const Size& _rObjectSize )
    {
        Point aPos( _rArea.TopLeft() );
        aPos.AdjustX(( _rArea.GetWidth() - _rObjectSize.Width() ) / 2 );
        aPos.AdjustY(( _rArea.GetHeight() - _rObjectSize.Height() ) / 2 );
        return aPos;
    }
}

void ImageControl::ImplDraw(OutputDevice& rDev, const Point& rPos, const Size& rSize) const
{
    DrawImageFlags nStyle = DrawImageFlags::NONE;
    if ( !IsEnabled() )
        nStyle |= DrawImageFlags::Disable;

    const Image& rImage( GetModeImage() );
    const tools::Rectangle aDrawRect( rPos, rSize );
    if (!rImage)
    {
        OUString  sText( GetText() );
        if ( sText.isEmpty() )
            return;

        WinBits nWinStyle = GetStyle();
        DrawTextFlags nTextStyle = FixedText::ImplGetTextStyle( nWinStyle );
        if ( !IsEnabled() )
            nTextStyle |= DrawTextFlags::Disable;

        rDev.DrawText( aDrawRect, sText, nTextStyle );
        return;
    }

    const Size aBitmapSize = rImage.GetSizePixel();

    switch ( mnScaleMode )
    {
    case ImageScaleMode::NONE:
    {
        rDev.DrawImage(lcl_centerWithin( aDrawRect, aBitmapSize ), rImage, nStyle);
    }
    break;

    case ImageScaleMode::ISOTROPIC:
    {
        const Size aPaintSize = lcl_calcPaintSize( aDrawRect, aBitmapSize );
        rDev.DrawImage(lcl_centerWithin(aDrawRect, aPaintSize), aPaintSize, rImage, nStyle);
    }
    break;

    case ImageScaleMode::ANISOTROPIC:
    {
        rDev.DrawImage(
            aDrawRect.TopLeft(),
            aDrawRect.GetSize(),
            rImage, nStyle );
    }
    break;

    default:
        OSL_ENSURE( false, "ImageControl::ImplDraw: unhandled scale mode!" );
        break;

    }   // switch ( mnScaleMode )
}

void ImageControl::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& /*rRect*/)
{
    ImplDraw(rRenderContext, Point(), GetOutputSizePixel());

    if (!HasFocus())
        return;

    vcl::Window* pBorderWindow = GetWindow(GetWindowType::Border);

    bool bFlat = (GetBorderStyle() == WindowBorderStyle::MONO);
    tools::Rectangle aRect(Point(0,0), pBorderWindow->GetOutputSizePixel());
    pBorderWindow->GetOutDev()->Push(vcl::PushFlags::FILLCOLOR | vcl::PushFlags::LINECOLOR);
    pBorderWindow->GetOutDev()->SetFillColor();
    pBorderWindow->GetOutDev()->SetLineColor(bFlat ? COL_WHITE : COL_BLACK);
    pBorderWindow->GetOutDev()->DrawRect(aRect);
    aRect.AdjustLeft( 1 );
    aRect.AdjustRight( -1 );
    aRect.AdjustTop( 1 );
    aRect.AdjustBottom( -1 );
    pBorderWindow->GetOutDev()->SetLineColor(bFlat ? COL_BLACK : COL_WHITE);
    pBorderWindow->GetOutDev()->DrawRect(aRect);
    pBorderWindow->GetOutDev()->Pop();
}

void ImageControl::Draw( OutputDevice* pDev, const Point& rPos, SystemTextColorFlags )
{
    const Point     aPos  = pDev->LogicToPixel( rPos );
    const Size      aSize = GetSizePixel();
    tools::Rectangle aRect( aPos, aSize );

    pDev->Push();
    pDev->SetMapMode();

    // Border
    if ( GetStyle() & WB_BORDER )
    {
        ImplDrawFrame( pDev, aRect );
    }
    pDev->IntersectClipRegion( aRect );
    ImplDraw( *pDev, aRect.TopLeft(), aRect.GetSize() );

    pDev->Pop();
}

void ImageControl::GetFocus()
{
    FixedImage::GetFocus();
    GetWindow( GetWindowType::Border )->Invalidate();
}

void ImageControl::LoseFocus()
{
    FixedImage::GetFocus();
    GetWindow( GetWindowType::Border )->Invalidate();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

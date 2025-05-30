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


#include <memory>
#include <awt/vclxgraphics.hxx>
#include <toolkit/awt/vclxdevice.hxx>
#include <toolkit/helper/vclunohelper.hxx>

#include <vcl/svapp.hxx>
#include <vcl/outdev.hxx>
#include <vcl/image.hxx>
#include <vcl/kernarray.hxx>
#include <vcl/gradient.hxx>
#include <vcl/metric.hxx>
#include <vcl/unohelp.hxx>
#include <tools/debug.hxx>

using namespace com::sun::star;



VCLXGraphics::VCLXGraphics()
    : mpOutputDevice(nullptr)
    , meRasterOp(RasterOp::OverPaint)
{
}

VCLXGraphics::~VCLXGraphics()
{
    std::vector< VCLXGraphics* > *pLst = mpOutputDevice ? mpOutputDevice->GetUnoGraphicsList() : nullptr;
    if ( pLst )
    {
        auto it = std::find(pLst->begin(), pLst->end(), this);
        if (it != pLst->end())
            pLst->erase( it );
    }

    mpClipRegion.reset();

    SolarMutexGuard g;
    mpOutputDevice.reset();
}

void VCLXGraphics::SetOutputDevice( OutputDevice* pOutDev )
{
    mpOutputDevice = pOutDev;
    mxDevice = nullptr;
    initAttrs();
}

void VCLXGraphics::Init( OutputDevice* pOutDev )
{
    DBG_ASSERT( !mpOutputDevice, "VCLXGraphics::Init already has pOutDev !" );
    mpOutputDevice  = pOutDev;

    initAttrs();
    mpClipRegion    = nullptr;

    // Register at OutputDevice
    std::vector< VCLXGraphics* > *pLst = mpOutputDevice->GetUnoGraphicsList();
    if ( !pLst )
        pLst = mpOutputDevice->CreateUnoGraphicsList();
    pLst->push_back( this );
}

void VCLXGraphics::initAttrs()
{
    if ( !mpOutputDevice )
        return;

    maFont          = mpOutputDevice->GetFont();
    maTextColor     = mpOutputDevice->GetTextColor(); /* COL_BLACK */
    maTextFillColor = mpOutputDevice->GetTextFillColor(); /* COL_TRANSPARENT */
    maLineColor     = mpOutputDevice->GetLineColor(); /* COL_BLACK */
    maFillColor     = mpOutputDevice->GetFillColor(); /* COL_WHITE */
    meRasterOp      = mpOutputDevice->GetRasterOp(); /* RasterOp::OverPaint */
}

void VCLXGraphics::InitOutputDevice( InitOutDevFlags nFlags )
{
    if(!mpOutputDevice)
        return;

    SolarMutexGuard aVclGuard;

    if ( nFlags & InitOutDevFlags::FONT )
    {
        mpOutputDevice->SetFont( maFont );
        mpOutputDevice->SetTextColor( maTextColor );
        mpOutputDevice->SetTextFillColor( maTextFillColor );
    }

    if ( nFlags & InitOutDevFlags::COLORS )
    {
        mpOutputDevice->SetLineColor( maLineColor );
        mpOutputDevice->SetFillColor( maFillColor );
    }

    mpOutputDevice->SetRasterOp( meRasterOp );

    if( mpClipRegion )
        mpOutputDevice->SetClipRegion( *mpClipRegion );
    else
        mpOutputDevice->SetClipRegion();
}

uno::Reference< awt::XDevice > VCLXGraphics::getDevice()
{
    SolarMutexGuard aGuard;

    if( !mxDevice.is() && mpOutputDevice )
    {
        rtl::Reference<VCLXDevice> xDev = new VCLXDevice;
        xDev->SetOutputDevice( mpOutputDevice );
        mxDevice = std::move(xDev);
    }
    return mxDevice;
}

awt::SimpleFontMetric VCLXGraphics::getFontMetric()
{
    SolarMutexGuard aGuard;

    awt::SimpleFontMetric aM;
    if( mpOutputDevice )
    {
        mpOutputDevice->SetFont( maFont );
        aM = VCLUnoHelper::CreateFontMetric( mpOutputDevice->GetFontMetric() );
    }
    return aM;
}

void VCLXGraphics::setFont( const uno::Reference< awt::XFont >& rxFont )
{
    SolarMutexGuard aGuard;

    maFont = VCLUnoHelper::CreateFont( rxFont );
}

void VCLXGraphics::selectFont( const awt::FontDescriptor& rDescription )
{
    SolarMutexGuard aGuard;

    maFont = VCLUnoHelper::CreateFont( rDescription, vcl::Font() );
}

void VCLXGraphics::setTextColor( sal_Int32 nColor )
{
    SolarMutexGuard aGuard;

    maTextColor = Color( ColorTransparency, nColor );
}

void VCLXGraphics::setTextFillColor( sal_Int32 nColor )
{
    SolarMutexGuard aGuard;

    maTextFillColor = Color( ColorTransparency, nColor );
}

void VCLXGraphics::setLineColor( sal_Int32 nColor )
{
    SolarMutexGuard aGuard;

    maLineColor = Color( ColorTransparency, nColor );
}

void VCLXGraphics::setFillColor( sal_Int32 nColor )
{
    SolarMutexGuard aGuard;

    maFillColor = Color( ColorTransparency, nColor );
}

void VCLXGraphics::setRasterOp( awt::RasterOperation eROP )
{
    SolarMutexGuard aGuard;

    meRasterOp = static_cast<RasterOp>(eROP);
}

void VCLXGraphics::setClipRegion( const uno::Reference< awt::XRegion >& rxRegion )
{
    SolarMutexGuard aGuard;

    if ( rxRegion.is() )
        mpClipRegion.reset( new vcl::Region( VCLUnoHelper::GetRegion( rxRegion ) ) );
    else
        mpClipRegion.reset();
}

void VCLXGraphics::intersectClipRegion( const uno::Reference< awt::XRegion >& rxRegion )
{
    SolarMutexGuard aGuard;

    if ( rxRegion.is() )
    {
        vcl::Region aRegion( VCLUnoHelper::GetRegion( rxRegion ) );
        if ( !mpClipRegion )
            mpClipRegion.reset( new vcl::Region(std::move(aRegion)) );
        else
            mpClipRegion->Intersect( aRegion );
    }
}

void VCLXGraphics::push(  )
{
    SolarMutexGuard aGuard;


    if( mpOutputDevice )
        mpOutputDevice->Push();
}

void VCLXGraphics::pop(  )
{
    SolarMutexGuard aGuard;


    if( mpOutputDevice )
        mpOutputDevice->Pop();
}

void VCLXGraphics::clear(
    const awt::Rectangle& aRect )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        const ::tools::Rectangle aVCLRect = vcl::unohelper::ConvertToVCLRect( aRect );
        mpOutputDevice->Erase( aVCLRect );
    }
}

void VCLXGraphics::copy( const uno::Reference< awt::XDevice >& rxSource, sal_Int32 nSourceX, sal_Int32 nSourceY, sal_Int32 nSourceWidth, sal_Int32 nSourceHeight, sal_Int32 nDestX, sal_Int32 nDestY, sal_Int32 nDestWidth, sal_Int32 nDestHeight )
{
    SolarMutexGuard aGuard;

    if ( mpOutputDevice )
    {
        VCLXDevice* pFromDev = dynamic_cast<VCLXDevice*>( rxSource.get() );
        DBG_ASSERT( pFromDev, "VCLXGraphics::copy - invalid device" );
        if ( pFromDev )
        {
            InitOutputDevice( InitOutDevFlags::NONE );
            mpOutputDevice->DrawOutDev( Point( nDestX, nDestY ), Size( nDestWidth, nDestHeight ),
                                    Point( nSourceX, nSourceY ), Size( nSourceWidth, nSourceHeight ), *pFromDev->GetOutputDevice() );
        }
    }
}

void VCLXGraphics::draw( const uno::Reference< awt::XDisplayBitmap >& rxBitmapHandle, sal_Int32 nSourceX, sal_Int32 nSourceY, sal_Int32 nSourceWidth, sal_Int32 nSourceHeight, sal_Int32 nDestX, sal_Int32 nDestY, sal_Int32 nDestWidth, sal_Int32 nDestHeight )
{
    SolarMutexGuard aGuard;

    if( !mpOutputDevice )
        return;

    InitOutputDevice( InitOutDevFlags::NONE);
    uno::Reference< awt::XBitmap > xBitmap( rxBitmapHandle, uno::UNO_QUERY );
    BitmapEx aBmpEx = VCLUnoHelper::GetBitmap( xBitmap );

    Point aPos(nDestX - nSourceX, nDestY - nSourceY);
    Size aSz = aBmpEx.GetSizePixel();

    if(nDestWidth != nSourceWidth)
    {
        float zoomX = static_cast<float>(nDestWidth) / static_cast<float>(nSourceWidth);
        aSz.setWidth( static_cast<tools::Long>(static_cast<float>(aSz.Width()) * zoomX) );
    }

    if(nDestHeight != nSourceHeight)
    {
        float zoomY = static_cast<float>(nDestHeight) / static_cast<float>(nSourceHeight);
        aSz.setHeight( static_cast<tools::Long>(static_cast<float>(aSz.Height()) * zoomY) );
    }

    if(nSourceX || nSourceY || aSz.Width() != nSourceWidth || aSz.Height() != nSourceHeight)
        mpOutputDevice->IntersectClipRegion(vcl::Region(tools::Rectangle(nDestX, nDestY, nDestX + nDestWidth - 1, nDestY + nDestHeight - 1)));

    mpOutputDevice->DrawBitmapEx( aPos, aSz, aBmpEx );
}

void VCLXGraphics::drawPixel( sal_Int32 x, sal_Int32 y )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawPixel( Point( x, y ) );
    }
}

void VCLXGraphics::drawLine( sal_Int32 x1, sal_Int32 y1, sal_Int32 x2, sal_Int32 y2 )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawLine( Point( x1, y1 ), Point( x2, y2 ) );
    }
}

void VCLXGraphics::drawRect( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawRect( tools::Rectangle( Point( x, y ), Size( width, height ) ) );
    }
}

void VCLXGraphics::drawRoundedRect( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, sal_Int32 nHorzRound, sal_Int32 nVertRound )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawRect( tools::Rectangle( Point( x, y ), Size( width, height ) ), nHorzRound, nVertRound );
    }
}

void VCLXGraphics::drawPolyLine( const uno::Sequence< sal_Int32 >& DataX, const uno::Sequence< sal_Int32 >& DataY )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawPolyLine( VCLUnoHelper::CreatePolygon( DataX, DataY ) );
    }
}

void VCLXGraphics::drawPolygon( const uno::Sequence< sal_Int32 >& DataX, const uno::Sequence< sal_Int32 >& DataY )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawPolygon( VCLUnoHelper::CreatePolygon( DataX, DataY ) );
    }
}

void VCLXGraphics::drawPolyPolygon( const uno::Sequence< uno::Sequence< sal_Int32 > >& DataX, const uno::Sequence< uno::Sequence< sal_Int32 > >& DataY )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        sal_uInt16 nPolys = static_cast<sal_uInt16>(DataX.getLength());
        tools::PolyPolygon aPolyPoly( nPolys );
        for ( sal_uInt16 n = 0; n < nPolys; n++ )
            aPolyPoly[n] = VCLUnoHelper::CreatePolygon( DataX.getConstArray()[n], DataY.getConstArray()[n] );

        mpOutputDevice->DrawPolyPolygon( aPolyPoly );
    }
}

void VCLXGraphics::drawEllipse( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawEllipse( tools::Rectangle( Point( x, y ), Size( width, height ) ) );
    }
}

void VCLXGraphics::drawArc( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, sal_Int32 x1, sal_Int32 y1, sal_Int32 x2, sal_Int32 y2 )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawArc( tools::Rectangle( Point( x, y ), Size( width, height ) ), Point( x1, y1 ), Point( x2, y2 ) );
    }
}

void VCLXGraphics::drawPie( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, sal_Int32 x1, sal_Int32 y1, sal_Int32 x2, sal_Int32 y2 )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawPie( tools::Rectangle( Point( x, y ), Size( width, height ) ), Point( x1, y1 ), Point( x2, y2 ) );
    }
}

void VCLXGraphics::drawChord( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, sal_Int32 x1, sal_Int32 y1, sal_Int32 x2, sal_Int32 y2 )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS );
        mpOutputDevice->DrawChord( tools::Rectangle( Point( x, y ), Size( width, height ) ), Point( x1, y1 ), Point( x2, y2 ) );
    }
}

void VCLXGraphics::drawGradient( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, const awt::Gradient& rGradient )
{
    SolarMutexGuard aGuard;

    if( !mpOutputDevice )
        return;

    InitOutputDevice( InitOutDevFlags::COLORS );
    Gradient aGradient(rGradient.Style, Color(ColorTransparency, rGradient.StartColor), Color(ColorTransparency, rGradient.EndColor));
    aGradient.SetAngle(Degree10(rGradient.Angle));
    aGradient.SetBorder(rGradient.Border);
    aGradient.SetOfsX(rGradient.XOffset);
    aGradient.SetOfsY(rGradient.YOffset);
    aGradient.SetStartIntensity(rGradient.StartIntensity);
    aGradient.SetEndIntensity(rGradient.EndIntensity);
    aGradient.SetSteps(rGradient.StepCount);
    mpOutputDevice->DrawGradient( tools::Rectangle( Point( x, y ), Size( width, height ) ), aGradient );
}

void VCLXGraphics::drawText( sal_Int32 x, sal_Int32 y, const OUString& rText )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS |InitOutDevFlags::FONT);
        mpOutputDevice->DrawText( Point( x, y ), rText );
    }
}

void VCLXGraphics::drawTextArray( sal_Int32 x, sal_Int32 y, const OUString& rText, const uno::Sequence< sal_Int32 >& rLongs )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice )
    {
        InitOutputDevice( InitOutDevFlags::COLORS|InitOutDevFlags::FONT );
        KernArray aDXA;
        aDXA.reserve(rText.getLength());
        for(int i = 0; i < rText.getLength(); ++i)
            aDXA.push_back(rLongs[i]);
        mpOutputDevice->DrawTextArray( Point( x, y ), rText, aDXA, {}, 0, rText.getLength());
    }
}


void VCLXGraphics::drawImage( sal_Int32 x, sal_Int32 y, sal_Int32 width, sal_Int32 height, sal_Int16 nStyle, const uno::Reference< graphic::XGraphic >& xGraphic )
{
    SolarMutexGuard aGuard;

    if( mpOutputDevice && xGraphic.is() )
    {
        Image aImage( xGraphic );
        if ( !!aImage )
        {
            InitOutputDevice( InitOutDevFlags::COLORS );
            mpOutputDevice->DrawImage( Point( x, y ), Size( width, height ), aImage, static_cast<DrawImageFlags>(nStyle) );
        }
    }
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <uielement/imagebuttontoolbarcontroller.hxx>

#include <framework/addonsoptions.hxx>

#include <com/sun/star/uno/XComponentContext.hpp>

#include <comphelper/getexpandeduri.hxx>
#include <comphelper/processfactory.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/graph.hxx>
#include <vcl/graphicfilter.hxx>
#include <vcl/toolbox.hxx>
#include <svtools/miscopt.hxx>
#include <memory>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;

const ::Size  aImageSizeSmall( 16, 16 );
const ::Size  aImageSizeBig( 26, 26 );

namespace framework
{

static void SubstituteVariables( OUString& aURL )
{
    aURL = comphelper::getExpandedUri(
        comphelper::getProcessComponentContext(), aURL);
}

ImageButtonToolbarController::ImageButtonToolbarController(
    const Reference< XComponentContext >&    rxContext,
    const Reference< XFrame >&               rFrame,
    ToolBox*                                 pToolbar,
    ToolBoxItemId                            nID,
    const OUString&                          aCommand ) :
    ComplexToolbarController( rxContext, rFrame, pToolbar, nID, aCommand )
{
    bool bBigImages( SvtMiscOptions::AreCurrentSymbolsLarge() );

    Image aImage(AddonsOptions().GetImageFromURL(aCommand, bBigImages, true));

    // Height will be controlled by scaling according to button height
    m_xToolbar->SetItemImage( m_nID, aImage );
}

ImageButtonToolbarController::~ImageButtonToolbarController()
{
}

void SAL_CALL ImageButtonToolbarController::dispose()
{
    SolarMutexGuard aSolarMutexGuard;
    ComplexToolbarController::dispose();
}

void ImageButtonToolbarController::executeControlCommand( const css::frame::ControlCommand& rControlCommand )
{
    SolarMutexGuard aSolarMutexGuard;
    // i73486 to be downward compatible use old and "wrong" also!
    if( rControlCommand.Command != "SetImag" &&
        rControlCommand.Command != "SetImage" )
        return;

    for ( const NamedValue& rArg : rControlCommand.Arguments )
    {
        if ( rArg.Name == "URL" )
        {
            OUString aURL;
            rArg.Value >>= aURL;

            SubstituteVariables( aURL );

            Image aImage;
            if ( ReadImageFromURL( SvtMiscOptions::AreCurrentSymbolsLarge(),
                                   aURL,
                                   aImage ))
            {
                m_xToolbar->SetItemImage( m_nID, aImage );

                // send notification
                uno::Sequence< beans::NamedValue > aInfo { { u"URL"_ustr, css::uno::Any(aURL) } };
                addNotifyInfo( u"ImageChanged"_ustr,
                            getDispatchFromCommand( m_aCommandURL ),
                            aInfo );
                break;
            }
        }
    }
}

// static
bool ImageButtonToolbarController::ReadImageFromURL( bool bBigImage, const OUString& aImageURL, Image& aImage )
{
    std::unique_ptr<SvStream> pStream(utl::UcbStreamHelper::CreateStream( aImageURL, StreamMode::STD_READ ));
    if ( !pStream || ( pStream->GetErrorCode() != ERRCODE_NONE ))
        return false;

    // Use graphic class to also support more graphic formats (bmp,png,...)
    Graphic aGraphic;

    GraphicFilter& rGF = GraphicFilter::GetGraphicFilter();
    rGF.ImportGraphic( aGraphic, u"", *pStream );

    BitmapEx aBitmapEx = aGraphic.GetBitmapEx();

    const ::Size aSize = bBigImage ? aImageSizeBig : aImageSizeSmall; // Sizes used for toolbar images

    ::Size aBmpSize = aBitmapEx.GetSizePixel();
    if ( !aBmpSize.IsEmpty() )
    {
        ::Size aNoScaleSize( aBmpSize.Width(), aSize.Height() );
        if ( aBmpSize != aNoScaleSize )
            aBitmapEx.Scale( aNoScaleSize, BmpScaleFlag::BestQuality );
        aImage = Image( aBitmapEx );
        return true;
    }

    return false;
}

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

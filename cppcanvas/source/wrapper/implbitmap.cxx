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


#include "implbitmap.hxx"
#include "implbitmapcanvas.hxx"

#include <osl/diagnose.h>


using namespace ::com::sun::star;

namespace cppcanvas::internal
{

        ImplBitmap::ImplBitmap( const CanvasSharedPtr&                      rParentCanvas,
                                const uno::Reference< rendering::XBitmap >& rBitmap ) :
            CanvasGraphicHelper( rParentCanvas ),
            mxBitmap( rBitmap )
        {
            OSL_ENSURE( mxBitmap.is(), "ImplBitmap::ImplBitmap: no valid bitmap" );

            uno::Reference< rendering::XBitmapCanvas > xBitmapCanvas( rBitmap,
                                                                      uno::UNO_QUERY );
            if( xBitmapCanvas.is() )
                mpBitmapCanvas = std::make_shared<ImplBitmapCanvas>(
                                          uno::Reference< rendering::XBitmapCanvas >(rBitmap,
                                                                                     uno::UNO_QUERY) );
        }

        ImplBitmap::~ImplBitmap()
        {
        }

        bool ImplBitmap::draw() const
        {
            CanvasSharedPtr pCanvas( getCanvas() );

            OSL_ENSURE( pCanvas && pCanvas->getUNOCanvas().is(),
                        "ImplBitmap::draw: invalid canvas" );

            if( !pCanvas ||
                !pCanvas->getUNOCanvas().is() )
            {
                return false;
            }

            // TODO(P1): implement caching
            pCanvas->getUNOCanvas()->drawBitmap( mxBitmap,
                                                 pCanvas->getViewState(),
                                                 getRenderState() );

            return true;
        }

        void ImplBitmap::drawAlphaModulated( double nAlphaModulation ) const
        {
            CanvasSharedPtr pCanvas( getCanvas() );

            OSL_ENSURE( pCanvas && pCanvas->getUNOCanvas().is(),
                        "ImplBitmap::drawAlphaModulated(): invalid canvas" );

            if( !pCanvas ||
                !pCanvas->getUNOCanvas().is() )
            {
                return;
            }

            rendering::RenderState aLocalState( getRenderState() );
            uno::Sequence<rendering::ARGBColor> aCol { { nAlphaModulation, 1.0, 1.0, 1.0 } };
            aLocalState.DeviceColor =
                pCanvas->getUNOCanvas()->getDevice()->getDeviceColorSpace()->convertFromARGB(aCol);

            // TODO(P1): implement caching
            pCanvas->getUNOCanvas()->drawBitmapModulated( mxBitmap,
                                                          pCanvas->getViewState(),
                                                          aLocalState );
        }

        BitmapCanvasSharedPtr ImplBitmap::getBitmapCanvas() const
        {
            return mpBitmapCanvas;
        }

        uno::Reference< rendering::XBitmap > ImplBitmap::getUNOBitmap() const
        {
            return mxBitmap;
        }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

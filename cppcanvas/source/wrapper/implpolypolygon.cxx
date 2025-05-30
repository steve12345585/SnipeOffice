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


#include <rtl/math.hxx>
#include <osl/diagnose.h>

#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/rendering/PathJoinType.hpp>
#include <com/sun/star/rendering/PathCapType.hpp>

#include "implpolypolygon.hxx"
#include <tools.hxx>
#include <utility>


using namespace ::com::sun::star;


namespace cppcanvas::internal
{
        ImplPolyPolygon::ImplPolyPolygon( const CanvasSharedPtr& rParentCanvas,
                                          uno::Reference< rendering::XPolyPolygon2D > xPolyPoly ) :
            CanvasGraphicHelper( rParentCanvas ),
            mxPolyPoly(std::move( xPolyPoly )),
            maStrokeAttributes(1.0,
                               10.0,
                               uno::Sequence< double >(),
                               uno::Sequence< double >(),
                               rendering::PathCapType::ROUND,
                               rendering::PathCapType::ROUND,
                               rendering::PathJoinType::ROUND ),
            mbFillColorSet( false ),
            mbStrokeColorSet( false )
        {
            OSL_ENSURE( mxPolyPoly.is(), "PolyPolygonImpl::PolyPolygonImpl: no valid polygon" );
        }

        ImplPolyPolygon::~ImplPolyPolygon()
        {
        }

        void ImplPolyPolygon::setRGBAFillColor( IntSRGBA aColor )
        {
            maFillColor = tools::intSRGBAToDoubleSequence( aColor );
            mbFillColorSet = true;
        }

        void ImplPolyPolygon::setRGBALineColor( IntSRGBA aColor )
        {
            maStrokeColor = tools::intSRGBAToDoubleSequence( aColor );
            mbStrokeColorSet = true;
        }

        IntSRGBA ImplPolyPolygon::getRGBALineColor() const
        {
            return tools::doubleSequenceToIntSRGBA( maStrokeColor );
        }

        void ImplPolyPolygon::setStrokeWidth( const double& rStrokeWidth )
        {
            maStrokeAttributes.StrokeWidth = rStrokeWidth;
        }

        double ImplPolyPolygon::getStrokeWidth() const
        {
            return maStrokeAttributes.StrokeWidth;
        }

        bool ImplPolyPolygon::draw() const
        {
            CanvasSharedPtr pCanvas( getCanvas() );

            OSL_ENSURE( pCanvas && pCanvas->getUNOCanvas().is(),
                        "ImplBitmap::draw: invalid canvas" );

            if( !pCanvas ||
                !pCanvas->getUNOCanvas().is() )
                return false;

            if( mbFillColorSet )
            {
                rendering::RenderState aLocalState( getRenderState() );
                aLocalState.DeviceColor = maFillColor;

                pCanvas->getUNOCanvas()->fillPolyPolygon( mxPolyPoly,
                                                          pCanvas->getViewState(),
                                                          aLocalState );
            }

            if( mbStrokeColorSet )
            {
                rendering::RenderState aLocalState( getRenderState() );
                aLocalState.DeviceColor = maStrokeColor;

                if( ::rtl::math::approxEqual(maStrokeAttributes.StrokeWidth, 1.0) )
                    pCanvas->getUNOCanvas()->drawPolyPolygon( mxPolyPoly,
                                                              pCanvas->getViewState(),
                                                              aLocalState );
                else
                    pCanvas->getUNOCanvas()->strokePolyPolygon( mxPolyPoly,
                                                                pCanvas->getViewState(),
                                                                aLocalState,
                                                                maStrokeAttributes );
            }

            return true;
        }

        uno::Reference< rendering::XPolyPolygon2D > ImplPolyPolygon::getUNOPolyPolygon() const
        {
            return mxPolyPoly;
        }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

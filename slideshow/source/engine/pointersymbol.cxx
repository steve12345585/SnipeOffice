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


#include <canvas/canvastools.hxx>

#include <cppcanvas/customsprite.hxx>

#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/vector/b2dsize.hxx>
#include <basegfx/vector/b2dvector.hxx>

#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/presentation/XSlideShowView.hpp>
#include <comphelper/diagnose_ex.hxx>

#include "pointersymbol.hxx"
#include <eventmultiplexer.hxx>

#include <algorithm>
#include <utility>


using namespace com::sun::star;

namespace slideshow::internal {

PointerSymbolSharedPtr PointerSymbol::create( const uno::Reference<rendering::XBitmap>& xBitmap,
                                              ScreenUpdater&                            rScreenUpdater,
                                              EventMultiplexer&                         rEventMultiplexer,
                                              const UnoViewContainer&                   rViewContainer )
{
    PointerSymbolSharedPtr pRet(
        new PointerSymbol( xBitmap,
                           rScreenUpdater,
                           rViewContainer ));

    rEventMultiplexer.addViewHandler( pRet );

    return pRet;
}

PointerSymbol::PointerSymbol( uno::Reference<rendering::XBitmap>           xBitmap,
                              ScreenUpdater&                               rScreenUpdater,
                              const UnoViewContainer&                      rViewContainer ) :
    mxBitmap(std::move(xBitmap)),
    maViews(),
    mrScreenUpdater( rScreenUpdater ),
    maPos(),
    mbVisible(false)
{
    for( const auto& rView : rViewContainer )
        viewAdded( rView );
}

void PointerSymbol::setVisible( const bool bVisible )
{
    if( mbVisible == bVisible )
        return;

    mbVisible = bVisible;

    for( const auto& rView : maViews )
    {
        if( rView.second )
        {
            if( bVisible )
                rView.second->show();
            else
                rView.second->hide();
        }
    }

    // sprites changed, need a screen update for this frame.
    mrScreenUpdater.requestImmediateUpdate();
}

basegfx::B2DPoint PointerSymbol::calcSpritePos(UnoViewSharedPtr const & rView) const
{
    const awt::Rectangle aViewArea( rView->getUnoView()->getCanvasArea() );
    const geometry::IntegerSize2D realTranslationOffset ( rView->getTranslationOffset() );

    return basegfx::B2DPoint(
        realTranslationOffset.Width + (aViewArea.Width * maPos.X),
        realTranslationOffset.Height + (aViewArea.Height * maPos.Y));
}

void PointerSymbol::viewAdded( const UnoViewSharedPtr& rView )
{
    cppcanvas::CustomSpriteSharedPtr sprite;

    try
    {
        const geometry::IntegerSize2D spriteSize( mxBitmap->getSize() );
        sprite = rView->createSprite( basegfx::B2DSize( spriteSize.Width,
                                                          spriteSize.Height ),
                                      1000.0 ); // sprite should be in front of all
                                                // other sprites
        rendering::ViewState viewState;
        canvas::tools::initViewState( viewState );
        rendering::RenderState renderState;
        canvas::tools::initRenderState( renderState );
        sprite->getContentCanvas()->getUNOCanvas()->drawBitmap(
            mxBitmap, viewState, renderState );

        sprite->setAlpha( 0.9 );
        sprite->movePixel( calcSpritePos( rView ) );
        if( mbVisible )
            sprite->show();
    }
    catch( uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION( "slideshow", "" );
    }

    maViews.emplace_back( rView, sprite );
}

void PointerSymbol::viewRemoved( const UnoViewSharedPtr& rView )
{
    std::erase_if(
            maViews,
            [&rView]
            ( const ViewsVecT::value_type& cp )
            { return rView == cp.first; } );
}

void PointerSymbol::viewChanged( const UnoViewSharedPtr& rView )
{
    // find entry corresponding to modified view
    ViewsVecT::iterator aModifiedEntry(
        std::find_if(
            maViews.begin(),
            maViews.end(),
            [&rView]
            ( const ViewsVecT::value_type& cp )
            { return rView == cp.first; } ) );

    OSL_ASSERT( aModifiedEntry != maViews.end() );
    if( aModifiedEntry == maViews.end() )
        return;

    if( aModifiedEntry->second )
        aModifiedEntry->second->movePixel(
            calcSpritePos(aModifiedEntry->first) );
}

void PointerSymbol::viewsChanged()
{
    // reposition sprites on all views
    for( const auto& rView : maViews )
    {
        if( rView.second )
            rView.second->movePixel(
                calcSpritePos( rView.first ) );
    }
}

void PointerSymbol::viewsChanged(const geometry::RealPoint2D pos)
{
    if( pos.X == maPos.X && pos.Y == maPos.Y )
        return;

    maPos = pos;

    // reposition sprites on all views
    for( const auto& rView : maViews )
    {
        if( rView.second )
        {
            rView.second->movePixel(
                calcSpritePos( rView.first ) );
            mrScreenUpdater.notifyUpdate();
            mrScreenUpdater.commitUpdates();
        }
    }
}

} // namespace slideshow::internal

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <sal/types.h>

#include <cppuhelper/compbase.hxx>
#include <cppuhelper/basemutex.hxx>
#include <comphelper/make_shared_from_uno.hxx>

#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/range/b1drange.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/vector/b2dsize.hxx>

#include "tests.hxx"
#include <view.hxx>
#include <com/sun/star/presentation/XSlideShowView.hpp>

#include <vector>
#include <exception>


namespace target = slideshow::internal;
using namespace ::com::sun::star;

// our test view subject
typedef ::cppu::WeakComponentImplHelper< presentation::XSlideShowView > ViewBase;

namespace {

class ImplTestView : public TestView,
                     private cppu::BaseMutex,
                     public ViewBase
{
    mutable std::vector<std::pair<basegfx::B2DVector,double> > maCreatedSprites;
    mutable std::vector<TestViewSharedPtr>                     maViewLayers;
    basegfx::B2DRange                                  maBounds;
    basegfx::B1DRange                                  maPriority;
    bool                                               mbIsClipSet;
    bool                                               mbIsClipEmptied;
    bool                                               mbDisposed;


public:
    ImplTestView() :
        ViewBase(m_aMutex),
        maCreatedSprites(),
        maViewLayers(),
        maBounds(),
        maPriority(),
        mbIsClipSet(false),
        mbIsClipEmptied(false),
        mbDisposed( false )
    {
    }

    // XSlideShowView
    virtual uno::Reference< rendering::XSpriteCanvas > SAL_CALL getCanvas(  ) override
    {
        return uno::Reference< rendering::XSpriteCanvas >();
    }

    virtual void SAL_CALL clear(  ) override
    {
    }

    virtual geometry::AffineMatrix2D SAL_CALL getTransformation(  ) override
    {
        return geometry::AffineMatrix2D();
    }

    virtual ::css::geometry::IntegerSize2D SAL_CALL getTranslationOffset() override
    {
        return geometry::IntegerSize2D();
    }

    virtual geometry::IntegerSize2D getTranslationOffset() const override
    {
        return geometry::IntegerSize2D();
    }

    virtual void SAL_CALL addTransformationChangedListener( const uno::Reference< util::XModifyListener >& ) override
    {
    }

    virtual void SAL_CALL removeTransformationChangedListener( const uno::Reference< util::XModifyListener >& ) override
    {
    }

    virtual void SAL_CALL addPaintListener( const uno::Reference< awt::XPaintListener >& ) override
    {
    }

    virtual void SAL_CALL removePaintListener( const uno::Reference< awt::XPaintListener >& ) override
    {
    }

    virtual void SAL_CALL addMouseListener( const uno::Reference< awt::XMouseListener >& ) override
    {
    }

    virtual void SAL_CALL removeMouseListener( const uno::Reference< awt::XMouseListener >& ) override
    {
    }

    virtual void SAL_CALL addMouseMotionListener( const uno::Reference< awt::XMouseMotionListener >& ) override
    {
    }

    virtual void SAL_CALL removeMouseMotionListener( const uno::Reference< awt::XMouseMotionListener >& ) override
    {
    }

    virtual void SAL_CALL setMouseCursor( ::sal_Int16 ) override
    {
    }

    virtual awt::Rectangle SAL_CALL getCanvasArea(  ) override
    {
        return awt::Rectangle(0,0,100,100);
    }

    virtual basegfx::B2DRange getBounds() const override
    {
        return maBounds;
    }

    virtual std::vector<std::shared_ptr<TestView> > getViewLayers() const override
    {
        return maViewLayers;
    }

    // ViewLayer
    virtual bool isOnView(target::ViewSharedPtr const& /*rView*/) const override
    {
        return true;
    }

    virtual ::cppcanvas::CanvasSharedPtr getCanvas() const override
    {
        return ::cppcanvas::CanvasSharedPtr();
    }

    virtual ::cppcanvas::CustomSpriteSharedPtr createSprite( const ::basegfx::B2DSize& rSpriteSizePixel,
                                                             double                    nPriority ) const override
    {
        basegfx::B2DVector aSpriteSizeVector(rSpriteSizePixel.getWidth(), rSpriteSizePixel.getHeight());
        maCreatedSprites.emplace_back(aSpriteSizeVector, nPriority);

        return ::cppcanvas::CustomSpriteSharedPtr();
    }

    virtual void setPriority( const basegfx::B1DRange& rRange ) override
    {
        maPriority = rRange;
    }

    virtual ::basegfx::B2DHomMatrix getTransformation() const override
    {
        return ::basegfx::B2DHomMatrix();
    }

    virtual ::basegfx::B2DHomMatrix getSpriteTransformation() const override
    {
        return ::basegfx::B2DHomMatrix();
    }

    virtual void setClip( const ::basegfx::B2DPolyPolygon& rClip ) override
    {
        if( !mbIsClipSet )
        {
            if( rClip.count() > 0 )
                mbIsClipSet = true;
        }
        else if( !mbIsClipEmptied )
        {
            if( rClip.count() == 0 )
                mbIsClipEmptied = true;
        }
        else if( rClip.count() > 0 )
        {
            mbIsClipSet = true;
            mbIsClipEmptied = false;
        }
        else
        {
            // unexpected call
            throw std::exception();
        }
    }

    virtual bool resize( const basegfx::B2DRange& rArea ) override
    {
        const bool bRet( maBounds != rArea );
        maBounds = rArea;
        return bRet;
    }

    virtual target::ViewLayerSharedPtr createViewLayer(
        const basegfx::B2DRange& rLayerBounds ) const override
    {
        maViewLayers.push_back( std::make_shared<ImplTestView>());
        maViewLayers.back()->resize( rLayerBounds );

        return maViewLayers.back();
    }

    virtual bool updateScreen() const override
    {
        // misusing updateScreen for state reporting
        return !mbDisposed;
    }

    virtual bool paintScreen() const override
    {
        // misusing updateScreen for state reporting
        return !mbDisposed;
    }

    virtual void clear() const override
    {
    }

    virtual void clearAll() const override
    {
    }

    virtual void setViewSize( const ::basegfx::B2DSize& ) override
    {
    }

    virtual void setCursorShape( sal_Int16 /*nPointerShape*/ ) override
    {
    }

    virtual uno::Reference< presentation::XSlideShowView > getUnoView() const override
    {
        return uno::Reference< presentation::XSlideShowView >( const_cast<ImplTestView*>(this) );
    }

    virtual void _dispose() override
    {
        mbDisposed = true;
    }

    virtual bool isSoundEnabled() const override
    {
        return true;
    }

    virtual void setIsSoundEnabled (const bool /*bValue*/) override
    {
    }
};

}

TestViewSharedPtr createTestView()
{
    return TestViewSharedPtr(
        comphelper::make_shared_from_UNO(
            new ImplTestView()) );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

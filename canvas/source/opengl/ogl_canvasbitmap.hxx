/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cppuhelper/compbase.hxx>

#include <com/sun/star/rendering/XBitmapCanvas.hpp>
#include <com/sun/star/rendering/XIntegerBitmap.hpp>

#include <base/integerbitmapbase.hxx>
#include <base/basemutexhelper.hxx>
#include <base/bitmapcanvasbase.hxx>

#include "ogl_bitmapcanvashelper.hxx"
#include "ogl_spritecanvas.hxx"


/* Definition of CanvasBitmap class */

namespace oglcanvas
{
    typedef ::cppu::WeakComponentImplHelper< css::rendering::XBitmapCanvas,
                                             css::rendering::XIntegerBitmap > CanvasBitmapBase_Base;
    typedef ::canvas::IntegerBitmapBase<
        canvas::BitmapCanvasBase2<
            ::canvas::BaseMutexHelper< CanvasBitmapBase_Base >,
            BitmapCanvasHelper,
            ::osl::MutexGuard,
            ::cppu::OWeakObject> > CanvasBitmapBaseT;

    class CanvasBitmap : public CanvasBitmapBaseT
    {
    public:
        /** Create a canvas bitmap for the given surface

            @param rSize
            Size of the bitmap

            @param rDevice
            Reference device, with which bitmap should be compatible
         */
        CanvasBitmap( const css::geometry::IntegerSize2D&        rSize,
                      SpriteCanvasRef                            rDevice,
                      SpriteDeviceHelper&                        rDeviceHelper );

        /** Create verbatim copy (including all recorded actions)
         */
        CanvasBitmap( const CanvasBitmap& rSrc );

        /// Dispose all internal references
        virtual void disposeThis() override;

        /** Write out recorded actions
         */
        bool renderRecordedActions() const;

    private:
        /** MUST hold here, too, since CanvasHelper only contains a
            raw pointer (without refcounting)
        */
        SpriteCanvasRef mpDevice;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

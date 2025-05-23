/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <comphelper/diagnose_ex.hxx>

#include <utility>

#include "ogl_canvasbitmap.hxx"


using namespace ::com::sun::star;

namespace oglcanvas
{
    CanvasBitmap::CanvasBitmap( const geometry::IntegerSize2D& rSize,
                                SpriteCanvasRef                rDevice,
                                SpriteDeviceHelper&            rDeviceHelper ) :
        mpDevice(std::move( rDevice ))
    {
        ENSURE_OR_THROW( mpDevice.is(),
                         "CanvasBitmap::CanvasBitmap(): Invalid surface or device" );

        maCanvasHelper.init( *mpDevice, rDeviceHelper, rSize );
    }

    CanvasBitmap::CanvasBitmap( const CanvasBitmap& rSrc ) :
        mpDevice( rSrc.mpDevice )
    {
        maCanvasHelper = rSrc.maCanvasHelper;
    }

    void CanvasBitmap::disposeThis()
    {
        mpDevice.clear();

        // forward to parent
        CanvasBitmapBaseT::disposeThis();
    }

    bool CanvasBitmap::renderRecordedActions() const
    {
        return maCanvasHelper.renderRecordedActions();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

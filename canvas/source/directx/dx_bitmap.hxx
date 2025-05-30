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

#pragma once

#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/rendering/XIntegerBitmap.hpp>
#include <basegfx/vector/b2ivector.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/range/b2drange.hxx>
#include <memory>
#include "dx_winstuff.hxx"
#include "dx_ibitmap.hxx"
#include "dx_graphicsprovider.hxx"
#include "dx_gdiplususer.hxx"

namespace dxcanvas
{
    class DXBitmap : public IBitmap
    {
    public:
        DXBitmap( const BitmapSharedPtr& rBitmap, bool bWithAlpha );
        DXBitmap( const ::basegfx::B2ISize& rSize, bool bWithAlpha );

        virtual GraphicsSharedPtr         getGraphics() override;

        virtual BitmapSharedPtr           getBitmap() const override;
        virtual ::basegfx::B2ISize getSize() const override;
        virtual bool                      hasAlpha() const override;

        css::uno::Sequence< sal_Int8 > getData(
            css::rendering::IntegerBitmapLayout&       bitmapLayout,
            const css::geometry::IntegerRectangle2D&   rect ) override;

        void setData(
            const css::uno::Sequence< sal_Int8 >&      data,
            const css::rendering::IntegerBitmapLayout& bitmapLayout,
            const css::geometry::IntegerRectangle2D&   rect ) override;

        void setPixel(
            const css::uno::Sequence< sal_Int8 >&      color,
            const css::rendering::IntegerBitmapLayout& bitmapLayout,
            const css::geometry::IntegerPoint2D&       pos ) override;

        css::uno::Sequence< sal_Int8 > getPixel(
            css::rendering::IntegerBitmapLayout&       bitmapLayout,
            const css::geometry::IntegerPoint2D&       pos ) override;

    private:
        // Refcounted global GDI+ state container
        GDIPlusUserSharedPtr mpGdiPlusUser;

        // size of this image in pixels [integral unit]
        ::basegfx::B2ISize maSize;

        BitmapSharedPtr      mpBitmap;
        GraphicsSharedPtr    mpGraphics;

        // true if the bitmap contains an alpha channel
        bool                 mbAlpha;
    };

    typedef std::shared_ptr< DXBitmap > DXBitmapSharedPtr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

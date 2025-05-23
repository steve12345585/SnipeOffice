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


#include <basegfx/numeric/ftools.hxx>
#include <sdr/overlay/overlayobjectcell.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>
#include <drawinglayer/primitive2d/unifiedtransparenceprimitive2d.hxx>

using namespace ::basegfx;

namespace sdr::overlay
{
        OverlayObjectCell::OverlayObjectCell( const Color& rColor, RangeVector&& rRects )
        :   OverlayObject( rColor ),
            maRectangles( std::move(rRects) )
        {
            // no AA for selection overlays
            allowAntiAliase(false);
        }

        OverlayObjectCell::~OverlayObjectCell()
        {
        }

        drawinglayer::primitive2d::Primitive2DContainer OverlayObjectCell::createOverlayObjectPrimitive2DSequence()
        {
            drawinglayer::primitive2d::Primitive2DContainer aRetval;
            const sal_uInt32 nCount(maRectangles.size());

            if(nCount)
            {
                const basegfx::BColor aRGBColor(getBaseColor().getBColor());
                aRetval.resize(nCount);

                // create primitives for all ranges
                for(sal_uInt32 a(0); a < nCount; a++)
                {
                    const basegfx::B2DRange& rRange(maRectangles[a]);
                    const basegfx::B2DPolygon aPolygon(basegfx::utils::createPolygonFromRect(rRange));

                    aRetval[a] =
                        new drawinglayer::primitive2d::PolyPolygonColorPrimitive2D(
                            basegfx::B2DPolyPolygon(aPolygon),
                            aRGBColor);
                }


                aRetval = drawinglayer::primitive2d::Primitive2DContainer {
                    // embed in 50% transparent paint
                        new drawinglayer::primitive2d::UnifiedTransparencePrimitive2D(
                            std::move(aRetval),
                            0.5)
                };
            }

            return aRetval;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

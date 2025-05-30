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

#include <svx/sdr/overlay/overlayselection.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <drawinglayer/primitive2d/PolyPolygonColorPrimitive2D.hxx>
#include <drawinglayer/primitive2d/PolyPolygonHairlinePrimitive2D.hxx>
#include <svtools/optionsdrawinglayer.hxx>
#include <vcl/svapp.hxx>
#include <vcl/outdev.hxx>
#include <vcl/settings.hxx>
#include <drawinglayer/primitive2d/invertprimitive2d.hxx>
#include <drawinglayer/primitive2d/unifiedtransparenceprimitive2d.hxx>
#include <basegfx/polygon/b2dpolypolygoncutter.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <officecfg/Office/Common.hxx>
#include <o3tl/sorted_vector.hxx>
#include <map>
#include <tools/fract.hxx>

namespace sdr::overlay
{

        // combine ranges geometrically to a single, ORed polygon
        static basegfx::B2DPolyPolygon impCombineRangesToPolyPolygon(const std::vector< basegfx::B2DRange >& rRanges, bool bOffset, double fOffset)
        {
            const sal_uInt32 nCount(rRanges.size());
            basegfx::B2DPolyPolygon aRetval;

            for(sal_uInt32 a(0); a < nCount; a++)
            {
                basegfx::B2DRange aRange(rRanges[a]);
                if (bOffset)
                    aRange.grow(fOffset);
                const basegfx::B2DPolygon aDiscretePolygon(basegfx::utils::createPolygonFromRect(aRange));

                if(0 == a)
                {
                    aRetval.append(aDiscretePolygon);
                }
                else
                {
                    aRetval = basegfx::utils::solvePolygonOperationOr(aRetval, basegfx::B2DPolyPolygon(aDiscretePolygon));
                }
            }

            return aRetval;
        }

        // tdf#161204 Creates a poly-polygon using white hairline to provide contrast
        static basegfx::B2DPolyPolygon impCombineRangesToContrastPolyPolygon(const std::vector< basegfx::B2DRange >& rRanges)
        {
            const sal_uInt32 nCount(rRanges.size());
            basegfx::B2DPolyPolygon aRetval;

            for(sal_uInt32 a(0); a < nCount; a++)
            {
                const basegfx::B2DPolygon aDiscretePolygon(basegfx::utils::createPolygonFromRect(rRanges[a]));

                if(0 == a)
                {
                    aRetval.append(aDiscretePolygon);
                }
                else
                {
                    aRetval = basegfx::utils::solvePolygonOperationOr(aRetval, basegfx::B2DPolyPolygon(aDiscretePolygon));
                }
            }

            return aRetval;
        }

        // check if wanted type OverlayType::Transparent or OverlayType::Solid
        // is possible. If not, fallback to invert mode (classic mode)
        static OverlayType impCheckPossibleOverlayType(OverlayType aOverlayType)
        {
            if(OverlayType::Invert != aOverlayType)
            {
                if(!officecfg::Office::Common::Drawinglayer::TransparentSelection::get())
                {
                    // not possible when switched off by user
                    return OverlayType::Invert;
                }
                else if (const OutputDevice* pOut = Application::GetDefaultDevice())
                {

                    if(pOut->GetSettings().GetStyleSettings().GetHighContrastMode())
                    {
                        // not possible when in high contrast mode
                        return  OverlayType::Invert;
                    }
                }
            }

            return aOverlayType;
        }

        drawinglayer::primitive2d::Primitive2DContainer OverlaySelection::createOverlayObjectPrimitive2DSequence()
        {
            drawinglayer::primitive2d::Primitive2DContainer aRetval;
            const sal_uInt32 nCount(getRanges().size());

            if(nCount)
            {
                // create range primitives
                const bool bInvert(OverlayType::Invert == maLastOverlayType);
                basegfx::BColor aRGBColor(getBaseColor().getBColor());
                aRetval.resize(nCount);

                if(bInvert)
                {
                    // force color to white for invert to get a full invert
                    aRGBColor = basegfx::BColor(1.0, 1.0, 1.0);
                }

                for(sal_uInt32 a(0);a < nCount; a++)
                {
                    const basegfx::B2DPolygon aPolygon(basegfx::utils::createPolygonFromRect(maRanges[a]));
                    aRetval[a] =
                        new drawinglayer::primitive2d::PolyPolygonColorPrimitive2D(
                            basegfx::B2DPolyPolygon(aPolygon),
                            aRGBColor);
                }

                if(bInvert)
                {
                    // embed all in invert primitive
                    aRetval = drawinglayer::primitive2d::Primitive2DContainer {
                            new drawinglayer::primitive2d::InvertPrimitive2D(
                                std::move(aRetval))
                    };
                }
                else if(maLastOverlayType == OverlayType::Transparent || maLastOverlayType == OverlayType::NoFill)
                {
                    // Determine transparency level
                    double fTransparence;
                    if (maLastOverlayType == OverlayType::NoFill)
                        fTransparence = 1;
                    else
                        fTransparence = mnLastTransparence / 100.0;

                    // embed all rectangles in transparent paint
                    const drawinglayer::primitive2d::Primitive2DReference aUnifiedTransparence(
                        new drawinglayer::primitive2d::UnifiedTransparencePrimitive2D(
                            std::move(aRetval),
                            fTransparence));

                    if(mbBorder)
                    {
                        aRetval = drawinglayer::primitive2d::Primitive2DContainer {aUnifiedTransparence};

                        // tdf#161204 Outline with white color to provide contrast
                        if (mbContrastOutline)
                        {
                            basegfx::B2DPolyPolygon aContrastPolyPolygon(impCombineRangesToContrastPolyPolygon(getRanges()));
                            const drawinglayer::primitive2d::Primitive2DReference aContrastSelectionOutline(
                                new drawinglayer::primitive2d::PolyPolygonHairlinePrimitive2D(
                                    std::move(aContrastPolyPolygon),
                                    basegfx::BColor(1.0, 1.0, 1.0)));
                            aRetval.append(drawinglayer::primitive2d::Primitive2DContainer{aContrastSelectionOutline});
                        }

                        // Offset to be applied to the external outline
                        double fOffset(0);
                        if (getOverlayManager())
                            fOffset = getOverlayManager()->getOutputDevice().PixelToLogic(Size(1, 1)).getWidth();

                        // External outline using themed color
                        basegfx::B2DPolyPolygon aPolyPolygon(impCombineRangesToPolyPolygon(getRanges(), mbContrastOutline, fOffset));
                        const drawinglayer::primitive2d::Primitive2DReference aSelectionOutline(
                            new drawinglayer::primitive2d::PolyPolygonHairlinePrimitive2D(
                                std::move(aPolyPolygon),
                                aRGBColor));

                        // Add to result
                        aRetval.append(drawinglayer::primitive2d::Primitive2DContainer {aSelectionOutline});
                    }
                    else
                    {
                        // just add transparent part
                        aRetval = drawinglayer::primitive2d::Primitive2DContainer { aUnifiedTransparence };
                    }
                }
            }

            return aRetval;
        }

        OverlaySelection::OverlaySelection(
            OverlayType eType,
            const Color& rColor,
            std::vector< basegfx::B2DRange >&& rRanges,
            bool bBorder,
            bool bContrastOutline)
        :   OverlayObject(rColor),
            meOverlayType(eType),
            maRanges(std::move(rRanges)),
            maLastOverlayType(eType),
            mnLastTransparence(0),
            mbBorder(bBorder),
            mbContrastOutline(bContrastOutline)
        {
            // no AA for selection overlays
            allowAntiAliase(false);
        }

        OverlaySelection::~OverlaySelection()
        {
            if(getOverlayManager())
            {
                getOverlayManager()->remove(*this);
            }
        }

        drawinglayer::primitive2d::Primitive2DContainer OverlaySelection::getOverlayObjectPrimitive2DSequence() const
        {
            // get current values
            const OverlayType aNewOverlayType(impCheckPossibleOverlayType(meOverlayType));
            const sal_uInt16 nNewTransparence(SvtOptionsDrawinglayer::GetTransparentSelectionPercent());

            if(!getPrimitive2DSequence().empty())
            {
                if(aNewOverlayType != maLastOverlayType
                    || nNewTransparence != mnLastTransparence)
                {
                    // conditions of last local decomposition have changed, delete
                    const_cast< OverlaySelection* >(this)->resetPrimitive2DSequence();
                }
            }

            if(getPrimitive2DSequence().empty())
            {
                // remember new values
                const_cast< OverlaySelection* >(this)->maLastOverlayType = aNewOverlayType;
                const_cast< OverlaySelection* >(this)->mnLastTransparence = nNewTransparence;
            }

            // call base implementation
            return OverlayObject::getOverlayObjectPrimitive2DSequence();
        }

        void OverlaySelection::setRanges(std::vector< basegfx::B2DRange >&& rNew)
        {
            if(rNew != maRanges)
            {
                maRanges = std::move(rNew);
                objectChange();
            }
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

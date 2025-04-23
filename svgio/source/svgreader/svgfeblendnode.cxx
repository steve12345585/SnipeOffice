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

#include <svgfeblendnode.hxx>
#include <o3tl/string_view.hxx>

#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/primitive2d/bitmapprimitive2d.hxx>
#include <drawinglayer/converters.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <vcl/bitmap/BitmapDarkenBlendFilter.hxx>
#include <vcl/bitmap/BitmapLightenBlendFilter.hxx>
#include <vcl/bitmap/BitmapMultiplyBlendFilter.hxx>
#include <vcl/bitmap/BitmapNormalBlendFilter.hxx>
#include <vcl/bitmap/BitmapScreenBlendFilter.hxx>
#include <vcl/BitmapTools.hxx>

namespace svgio::svgreader
{
SvgFeBlendNode::SvgFeBlendNode(SvgDocument& rDocument, SvgNode* pParent)
    : SvgFilterNode(SVGToken::FeBlend, rDocument, pParent)
    , maMode(Mode::Normal)
{
}

SvgFeBlendNode::~SvgFeBlendNode() {}

void SvgFeBlendNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
{
    // parse own
    switch (aSVGToken)
    {
        case SVGToken::Style:
        {
            readLocalCssStyle(aContent);
            break;
        }
        case SVGToken::In:
        {
            maIn = aContent.trim();
            break;
        }
        case SVGToken::In2:
        {
            maIn2 = aContent.trim();
            break;
        }
        case SVGToken::Result:
        {
            maResult = aContent.trim();
            break;
        }
        case SVGToken::Mode:
        {
            if (!aContent.isEmpty())
            {
                if (o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), u"normal"))
                {
                    maMode = Mode::Normal;
                }
                else if (o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), u"screen"))
                {
                    maMode = Mode::Screen;
                }
                else if (o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), u"multiply"))
                {
                    maMode = Mode::Multiply;
                }
                else if (o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), u"darken"))
                {
                    maMode = Mode::Darken;
                }
                else if (o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), u"lighten"))
                {
                    maMode = Mode::Lighten;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void SvgFeBlendNode::apply(drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                           const SvgFilterNode* pParent) const
{
    const drawinglayer::primitive2d::Primitive2DContainer* pSource
        = pParent->findGraphicSource(maIn);
    const drawinglayer::primitive2d::Primitive2DContainer* pSource2
        = pParent->findGraphicSource(maIn2);

    basegfx::B2DRange aRange, aRange2;
    const drawinglayer::geometry::ViewInformation2D aViewInformation2D;
    if (pSource)
    {
        aRange = pSource->getB2DRange(aViewInformation2D);
    }

    if (pSource2)
    {
        aRange2 = pSource2->getB2DRange(aViewInformation2D);
    }

    const basegfx::B2DRange aBaseRange(0, 0, std::max(aRange.getMaxX(), aRange2.getMaxX()),
                                       std::max(aRange.getMaxY(), aRange2.getMaxY()));

    BitmapEx aBmpEx, aBmpEx2;

    if (pSource)
    {
        drawinglayer::primitive2d::Primitive2DContainer aSource(*pSource);
        aBmpEx = drawinglayer::convertToBitmapEx(std::move(aSource), aViewInformation2D,
                                                 aBaseRange.getWidth(), aBaseRange.getHeight(),
                                                 aBaseRange.getWidth() * aBaseRange.getHeight());
    }
    else
    {
        aBmpEx = drawinglayer::convertToBitmapEx(std::move(rTarget), aViewInformation2D,
                                                 aBaseRange.getWidth(), aBaseRange.getHeight(),
                                                 aBaseRange.getWidth() * aBaseRange.getHeight());
    }

    if (pSource2)
    {
        drawinglayer::primitive2d::Primitive2DContainer aSource(*pSource2);
        aBmpEx2 = drawinglayer::convertToBitmapEx(std::move(aSource), aViewInformation2D,
                                                  aBaseRange.getWidth(), aBaseRange.getHeight(),
                                                  aBaseRange.getWidth() * aBaseRange.getHeight());
    }
    else
    {
        aBmpEx2 = drawinglayer::convertToBitmapEx(std::move(rTarget), aViewInformation2D,
                                                  aBaseRange.getWidth(), aBaseRange.getHeight(),
                                                  aBaseRange.getWidth() * aBaseRange.getHeight());
    }

    BitmapEx aResBmpEx;
    switch (maMode)
    {
        case Mode::Screen:
        {
            BitmapScreenBlendFilter aScreenBlendFilter(aBmpEx, aBmpEx2);
            aResBmpEx = aScreenBlendFilter.execute();
            break;
        }
        case Mode::Multiply:
        {
            BitmapMultiplyBlendFilter aMultiplyBlendFilter(aBmpEx, aBmpEx2);
            aResBmpEx = aMultiplyBlendFilter.execute();
            break;
        }
        case Mode::Darken:
        {
            BitmapDarkenBlendFilter aDarkenBlendFilter(aBmpEx2);
            aResBmpEx = aDarkenBlendFilter.execute(aBmpEx);
            break;
        }
        case Mode::Lighten:
        {
            BitmapLightenBlendFilter aLightenBlendFilter(aBmpEx, aBmpEx2);
            aResBmpEx = aLightenBlendFilter.execute();
            break;
        }
        case Mode::Normal:
        {
            BitmapNormalBlendFilter aNormalBlendFilter(aBmpEx, aBmpEx2);
            aResBmpEx = aNormalBlendFilter.execute();
            break;
        }
    }

    const drawinglayer::primitive2d::Primitive2DReference xRef(
        new drawinglayer::primitive2d::BitmapPrimitive2D(
            aResBmpEx, basegfx::utils::createScaleTranslateB2DHomMatrix(aBaseRange.getRange(),
                                                                        aBaseRange.getMinimum())));
    rTarget = drawinglayer::primitive2d::Primitive2DContainer{ xRef };

    pParent->addGraphicSourceToMapper(maResult, rTarget);
}

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

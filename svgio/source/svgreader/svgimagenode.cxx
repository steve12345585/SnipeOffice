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

#include <svgimagenode.hxx>
#include <svgdocument.hxx>
#include <tools/stream.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/graphicfilter.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <drawinglayer/primitive2d/bitmapprimitive2d.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/primitive2d/maskprimitive2d.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <rtl/uri.hxx>
#include <sal/log.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <comphelper/base64.hxx>
#include <toolkit/helper/vclunohelper.hxx>

namespace svgio::svgreader
{
        SvgImageNode::SvgImageNode(
            SvgDocument& rDocument,
            SvgNode* pParent)
        :   SvgNode(SVGToken::Rect, rDocument, pParent),
            maSvgStyleAttributes(*this),
            maX(0),
            maY(0),
            maWidth(0),
            maHeight(0)
        {
        }

        SvgImageNode::~SvgImageNode()
        {
        }

        const SvgStyleAttributes* SvgImageNode::getSvgStyleAttributes() const
        {
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgImageNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
        {
            // call parent
            SvgNode::parseAttribute(aSVGToken, aContent);

            // read style attributes
            maSvgStyleAttributes.parseStyleAttribute(aSVGToken, aContent);

            // parse own
            switch(aSVGToken)
            {
                case SVGToken::Style:
                {
                    readLocalCssStyle(aContent);
                    break;
                }
                case SVGToken::PreserveAspectRatio:
                {
                    maSvgAspectRatio = readSvgAspectRatio(aContent);
                    break;
                }
                case SVGToken::Transform:
                {
                    const basegfx::B2DHomMatrix aMatrix(readTransform(aContent, *this));

                    if(!aMatrix.isIdentity())
                    {
                        setTransform(aMatrix);
                    }
                    break;
                }
                case SVGToken::X:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maX = aNum;
                    }
                    break;
                }
                case SVGToken::Y:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maY = aNum;
                    }
                    break;
                }
                case SVGToken::Width:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        if(aNum.isPositive())
                        {
                            maWidth = aNum;
                        }
                    }
                    break;
                }
                case SVGToken::Height:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        if(aNum.isPositive())
                        {
                            maHeight = aNum;
                        }
                    }
                    break;
                }
                case SVGToken::Href:
                case SVGToken::XlinkHref:
                {
                    const sal_Int32 nLen(aContent.getLength());

                    if(nLen)
                    {
                        readImageLink(aContent, maXLink, maUrl, maData);
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        static void extractFromGraphic(
            const Graphic& rGraphic,
            drawinglayer::primitive2d::Primitive2DContainer& rEmbedded,
            basegfx::B2DRange& rViewBox,
            BitmapEx& rBitmapEx)
        {
            if(GraphicType::Bitmap == rGraphic.GetType())
            {
                if(rGraphic.getVectorGraphicData())
                {
                    // embedded Svg
                    rEmbedded = rGraphic.getVectorGraphicData()->getPrimitive2DSequence();

                    // fill aViewBox
                    rViewBox = rGraphic.getVectorGraphicData()->getRange();
                }
                else
                {
                    // get bitmap
                    rBitmapEx = rGraphic.GetBitmapEx();
                }
            }
            else
            {
                // evtl. convert to bitmap
                rBitmapEx = rGraphic.GetBitmapEx();
            }
        }

        void SvgImageNode::decomposeSvgNode(drawinglayer::primitive2d::Primitive2DContainer& rTarget, bool /*bReferenced*/) const
        {
            // get size range and create path
            const SvgStyleAttributes* pStyle = getSvgStyleAttributes();

            if(!(pStyle && getWidth().isSet() && getHeight().isSet()))
                return;

            const double fWidth(getWidth().solve(*this, NumberType::xcoordinate));
            const double fHeight(getHeight().solve(*this, NumberType::ycoordinate));

            if(fWidth <= 0.0 || fHeight <= 0.0)
                return;

            BitmapEx aBitmapEx;
            drawinglayer::primitive2d::Primitive2DContainer aNewTarget;

            // prepare Target and ViewBox for evtl. AspectRatio mappings
            const double fX(getX().isSet() ? getX().solve(*this, NumberType::xcoordinate) : 0.0);
            const double fY(getY().isSet() ? getY().solve(*this, NumberType::ycoordinate) : 0.0);
            const basegfx::B2DRange aTarget(fX, fY, fX + fWidth, fY + fHeight);
            basegfx::B2DRange aViewBox(aTarget);

            if(!maData.isEmpty())
            {
                // use embedded base64 encoded data
                css::uno::Sequence< sal_Int8 > aPass;
                ::comphelper::Base64::decode(aPass, maData);

                if(aPass.hasElements())
                {
                    SvMemoryStream aStream(aPass.getArray(), aPass.getLength(), StreamMode::READ);
                    Graphic aGraphic;

                    if(ERRCODE_NONE == GraphicFilter::GetGraphicFilter().ImportGraphic(
                        aGraphic,
                        u"",
                        aStream))
                    {
                        extractFromGraphic(aGraphic, aNewTarget, aViewBox, aBitmapEx);
                    }
                }
            }
            else if(!maUrl.isEmpty())
            {
                const OUString& rPath = getDocument().getAbsolutePath();
                OUString aAbsUrl;
                try {
                    aAbsUrl = rtl::Uri::convertRelToAbs(rPath, maUrl);
                } catch (rtl::MalformedUriException & e) {
                    SAL_WARN(
                        "svg",
                        "caught rtl::MalformedUriException \""
                            << e.getMessage() << "\"");
                }

                if (!aAbsUrl.isEmpty() && rPath != aAbsUrl)
                {
                    SvFileStream aStream(aAbsUrl, StreamMode::STD_READ);
                    Graphic aGraphic;

                    if(ERRCODE_NONE == GraphicFilter::GetGraphicFilter().ImportGraphic(
                           aGraphic,
                           aAbsUrl,
                           aStream))
                    {
                        extractFromGraphic(aGraphic, aNewTarget, aViewBox, aBitmapEx);
                    }
                }
            }
            else if(!maXLink.isEmpty())
            {
                const SvgNode* pXLink = getDocument().findSvgNodeById(maXLink);

                if(pXLink && Display::None != pXLink->getDisplay())
                {
                    pXLink->decomposeSvgNode(aNewTarget, true);

                    if(!aNewTarget.empty())
                    {
                        aViewBox = aNewTarget.getB2DRange(drawinglayer::geometry::ViewInformation2D());
                    }
                }
            }

            if(!aBitmapEx.IsEmpty() && 0 != aBitmapEx.GetSizePixel().Width()  && 0 != aBitmapEx.GetSizePixel().Height())
            {
                // calculate centered unit size
                const double fAspectRatio = static_cast<double>(aBitmapEx.GetSizePixel().Width()) / static_cast<double>(aBitmapEx.GetSizePixel().Height());

                if (basegfx::fTools::equalZero(fAspectRatio))
                {
                    // use unit range
                    aViewBox = basegfx::B2DRange(0.0, 0.0, 1.0, 1.0);
                }
                else if (fAspectRatio > 0.0)
                {
                    // width bigger height
                    const double fHalfHeight((1.0 / fAspectRatio) * 0.5);
                    aViewBox = basegfx::B2DRange(
                        0.0,
                        0.5 - fHalfHeight,
                        1.0,
                        0.5 + fHalfHeight);
                }
                else
                {
                    // height bigger width
                    const double fHalfWidth(fAspectRatio * 0.5);
                    aViewBox = basegfx::B2DRange(
                        0.5 - fHalfWidth,
                        0.0,
                        0.5 + fHalfWidth,
                        1.0);
                }

                // create content from created bitmap, use calculated unit range size
                // as transformation to map the picture data correctly
                aNewTarget.resize(1);
                aNewTarget[0] = new drawinglayer::primitive2d::BitmapPrimitive2D(
                    aBitmapEx,
                    basegfx::utils::createScaleTranslateB2DHomMatrix(
                        aViewBox.getRange(),
                        aViewBox.getMinimum()));
            }

            if(aNewTarget.empty())
                return;

            // create mapping
            const SvgAspectRatio& rRatio = maSvgAspectRatio;

            // even when ratio is not set, use the defaults
            // let mapping be created from SvgAspectRatio
            const basegfx::B2DHomMatrix aEmbeddingTransform(rRatio.createMapping(aTarget, aViewBox));

            if(!aEmbeddingTransform.isIdentity())
            {
                const drawinglayer::primitive2d::Primitive2DReference xRef(
                    new drawinglayer::primitive2d::TransformPrimitive2D(
                        aEmbeddingTransform,
                        std::move(aNewTarget)));

                aNewTarget = drawinglayer::primitive2d::Primitive2DContainer { xRef };
            }

            if(!rRatio.isMeetOrSlice())
            {
                // need to embed in MaskPrimitive2D to ensure clipping
                const drawinglayer::primitive2d::Primitive2DReference xMask(
                    new drawinglayer::primitive2d::MaskPrimitive2D(
                        basegfx::B2DPolyPolygon(
                            basegfx::utils::createPolygonFromRect(aTarget)),
                        std::move(aNewTarget)));

                aNewTarget = drawinglayer::primitive2d::Primitive2DContainer { xMask };
            }

            // embed and add to rTarget, take local extra-transform into account
            pStyle->add_postProcess(rTarget, std::move(aNewTarget), getTransform());
        }

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

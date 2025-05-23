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

#include <svgrectnode.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

namespace svgio::svgreader
{
        SvgRectNode::SvgRectNode(
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

        SvgRectNode::~SvgRectNode()
        {
        }

        const SvgStyleAttributes* SvgRectNode::getSvgStyleAttributes() const
        {
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgRectNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
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
                case SVGToken::Rx:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        if(aNum.isPositive())
                        {
                            maRx = aNum;
                        }
                    }
                    break;
                }
                case SVGToken::Ry:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        if(aNum.isPositive())
                        {
                            maRy = aNum;
                        }
                    }
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
                default:
                {
                    break;
                }
            }
        }

        void SvgRectNode::decomposeSvgNode(drawinglayer::primitive2d::Primitive2DContainer& rTarget, bool /*bReferenced*/) const
        {
            // get size range and create path
            const SvgStyleAttributes* pStyle = getSvgStyleAttributes();

            if(!(pStyle && getWidth().isSet() && getHeight().isSet()))
                return;

            const double fWidth(getWidth().solve(*this, NumberType::xcoordinate));
            const double fHeight(getHeight().solve(*this, NumberType::ycoordinate));

            if(fWidth <= 0.0 || fHeight <= 0.0)
                return;

            const double fX(getX().isSet() ? getX().solve(*this, NumberType::xcoordinate) : 0.0);
            const double fY(getY().isSet() ? getY().solve(*this, NumberType::ycoordinate) : 0.0);
            const basegfx::B2DRange aRange(fX, fY, fX + fWidth, fY + fHeight);
            basegfx::B2DPolygon aPath;

            if(getRx().isSet() || getRy().isSet())
            {
                double frX(getRx().isSet() ? getRx().solve(*this, NumberType::xcoordinate) : 0.0);
                double frY(getRy().isSet() ? getRy().solve(*this, NumberType::ycoordinate) : 0.0);

                if(!getRy().isSet() && 0.0 == frY && frX > 0.0)
                {
                    frY = frX;
                }
                else if(!getRx().isSet() && 0.0 == frX && frY > 0.0)
                {
                    frX = frY;
                }

                frX /= fWidth;
                frY /= fHeight;

                frX = std::min(0.5, frX);
                frY = std::min(0.5, frY);

                aPath = basegfx::utils::createPolygonFromRect(aRange, frX * 2.0, frY * 2.0);
            }
            else
            {
                aPath = basegfx::utils::createPolygonFromRect(aRange);
            }

            drawinglayer::primitive2d::Primitive2DContainer aNewTarget;

            pStyle->add_path(basegfx::B2DPolyPolygon(aPath), aNewTarget, nullptr);

            if(!aNewTarget.empty())
            {
                pStyle->add_postProcess(rTarget, std::move(aNewTarget), getTransform());
            }
        }
} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

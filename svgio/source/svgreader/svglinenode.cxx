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

#include <svglinenode.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

namespace svgio::svgreader
{
        SvgLineNode::SvgLineNode(
            SvgDocument& rDocument,
            SvgNode* pParent)
        :   SvgNode(SVGToken::Line, rDocument, pParent),
            maSvgStyleAttributes(*this),
            maX1(0),
            maY1(0),
            maX2(0),
            maY2(0)
        {
        }

        SvgLineNode::~SvgLineNode()
        {
        }

        const SvgStyleAttributes* SvgLineNode::getSvgStyleAttributes() const
        {
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgLineNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
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
                case SVGToken::X1:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maX1 = aNum;
                    }
                    break;
                }
                case SVGToken::Y1:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maY1 = aNum;
                    }
                    break;
                }
                case SVGToken::X2:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maX2 = aNum;
                    }
                    break;
                }
                case SVGToken::Y2:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maY2 = aNum;
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

        void SvgLineNode::decomposeSvgNode(drawinglayer::primitive2d::Primitive2DContainer& rTarget, bool /*bReferenced*/) const
        {
            const SvgStyleAttributes* pStyle = getSvgStyleAttributes();

            if(!pStyle)
                return;

            const basegfx::B2DPoint X(
                getX1().isSet() ? getX1().solve(*this, NumberType::xcoordinate) : 0.0,
                getY1().isSet() ? getY1().solve(*this, NumberType::ycoordinate) : 0.0);
            const basegfx::B2DPoint Y(
                getX2().isSet() ? getX2().solve(*this, NumberType::xcoordinate) : 0.0,
                getY2().isSet() ? getY2().solve(*this, NumberType::ycoordinate) : 0.0);

            // X and Y may be equal, do not drop them. Markers or linecaps 'round' and 'square'
            // need to be drawn for zero-length lines too.

            basegfx::B2DPolygon aPath;

            aPath.append(X);
            aPath.append(Y);

            drawinglayer::primitive2d::Primitive2DContainer aNewTarget;

            pStyle->add_path(basegfx::B2DPolyPolygon(aPath), aNewTarget, nullptr);

            if(!aNewTarget.empty())
            {
                pStyle->add_postProcess(rTarget, std::move(aNewTarget), getTransform());
            }
        }
} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <svgcirclenode.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

namespace svgio::svgreader
{
        SvgCircleNode::SvgCircleNode(
            SvgDocument& rDocument,
            SvgNode* pParent)
        :   SvgNode(SVGToken::Circle, rDocument, pParent),
            maSvgStyleAttributes(*this),
            maCx(0),
            maCy(0),
            maR(0)
        {
        }

        SvgCircleNode::~SvgCircleNode()
        {
        }

        const SvgStyleAttributes* SvgCircleNode::getSvgStyleAttributes() const
        {
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgCircleNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
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
                case SVGToken::Cx:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maCx = aNum;
                    }
                    break;
                }
                case SVGToken::Cy:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        maCy = aNum;
                    }
                    break;
                }
                case SVGToken::R:
                {
                    SvgNumber aNum;

                    if(readSingleNumber(aContent, aNum))
                    {
                        if(aNum.isPositive())
                        {
                            maR = aNum;
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

        void SvgCircleNode::decomposeSvgNode(drawinglayer::primitive2d::Primitive2DContainer& rTarget, bool /*bReferenced*/) const
        {
            const SvgStyleAttributes* pStyle = getSvgStyleAttributes();

            if(!(pStyle && getR().isSet()))
                return;

            const double fR(getR().solve(*this));

            if(fR <= 0.0)
                return;

            const basegfx::B2DPolygon aPath(
                basegfx::utils::createPolygonFromCircle(
                    basegfx::B2DPoint(
                        getCx().isSet() ? getCx().solve(*this, NumberType::xcoordinate) : 0.0,
                        getCy().isSet() ? getCy().solve(*this, NumberType::ycoordinate) : 0.0),
                    fR));

            drawinglayer::primitive2d::Primitive2DContainer aNewTarget;

            pStyle->add_path(basegfx::B2DPolyPolygon(aPath), aNewTarget, nullptr);

            if(!aNewTarget.empty())
            {
                pStyle->add_postProcess(rTarget, std::move(aNewTarget), getTransform());
            }
        }
} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

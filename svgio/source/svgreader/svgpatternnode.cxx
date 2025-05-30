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

#include <svgpatternnode.hxx>
#include <svgdocument.hxx>
#include <o3tl/string_view.hxx>

namespace svgio::svgreader
{
        void SvgPatternNode::tryToFindLink()
        {
            if(!mpXLink && !maXLink.isEmpty())
            {
                mpXLink = dynamic_cast< const SvgPatternNode* >(getDocument().findSvgNodeById(maXLink));
            }
        }

        SvgPatternNode::SvgPatternNode(
            SvgDocument& rDocument,
            SvgNode* pParent)
        :   SvgNode(SVGToken::Pattern, rDocument, pParent),
            maSvgStyleAttributes(*this),
            mbResolvingLink(false),
            mpXLink(nullptr)
        {
        }

        SvgPatternNode::~SvgPatternNode()
        {
        }

        const SvgStyleAttributes* SvgPatternNode::getSvgStyleAttributes() const
        {
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgPatternNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
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
                case SVGToken::ViewBox:
                {
                    const basegfx::B2DRange aRange(readViewBox(aContent, *this));

                    if(!aRange.isEmpty())
                    {
                        setViewBox(&aRange);
                    }
                    break;
                }
                case SVGToken::PreserveAspectRatio:
                {
                    maSvgAspectRatio = readSvgAspectRatio(aContent);
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
                case SVGToken::PatternUnits:
                {
                    if(!aContent.isEmpty())
                    {
                        if(o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), commonStrings::aStrUserSpaceOnUse))
                        {
                            setPatternUnits(SvgUnits::userSpaceOnUse);
                        }
                        else if(o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), commonStrings::aStrObjectBoundingBox))
                        {
                            setPatternUnits(SvgUnits::objectBoundingBox);
                        }
                    }
                    break;
                }
                case SVGToken::PatternContentUnits:
                {
                    if(!aContent.isEmpty())
                    {
                        if(o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), commonStrings::aStrUserSpaceOnUse))
                        {
                            setPatternContentUnits(SvgUnits::userSpaceOnUse);
                        }
                        else if(o3tl::equalsIgnoreAsciiCase(o3tl::trim(aContent), commonStrings::aStrObjectBoundingBox))
                        {
                            setPatternContentUnits(SvgUnits::objectBoundingBox);
                        }
                    }
                    break;
                }
                case SVGToken::PatternTransform:
                {
                    const basegfx::B2DHomMatrix aMatrix(readTransform(aContent, *this));

                    if(!aMatrix.isIdentity())
                    {
                        setPatternTransform(aMatrix);
                    }
                    break;
                }
                case SVGToken::Href:
                case SVGToken::XlinkHref:
                {
                    readLocalLink(aContent, maXLink);
                    tryToFindLink();
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        void SvgPatternNode::getValuesRelative(double& rfX, double& rfY, double& rfW, double& rfH, const basegfx::B2DRange& rGeoRange, SvgNode const & rUser) const
        {
            double fTargetWidth(rGeoRange.getWidth());
            double fTargetHeight(rGeoRange.getHeight());

            if(fTargetWidth <= 0.0 || fTargetHeight <= 0.0)
                return;

            const SvgUnits aPatternUnits(getPatternUnits() ? *getPatternUnits() : SvgUnits::objectBoundingBox);

            if (SvgUnits::objectBoundingBox == aPatternUnits)
            {
                rfW = (getWidth().isSet()) ? getWidth().getNumber() : 0.0;
                rfH = (getHeight().isSet()) ? getHeight().getNumber() : 0.0;

                if(SvgUnit::percent == getWidth().getUnit())
                {
                    rfW *= 0.01;
                }

                if(SvgUnit::percent == getHeight().getUnit())
                {
                    rfH *= 0.01;
                }
            }
            else
            {
                rfW = (getWidth().isSet()) ? getWidth().solve(rUser, NumberType::xcoordinate) : 0.0;
                rfH = (getHeight().isSet()) ? getHeight().solve(rUser, NumberType::ycoordinate) : 0.0;

                // make relative to rGeoRange
                rfW /= fTargetWidth;
                rfH /= fTargetHeight;
            }

            if(rfW <= 0.0 || rfH <= 0.0)
                return;

            if (SvgUnits::objectBoundingBox == aPatternUnits)
            {
                rfX = (getX().isSet()) ? getX().getNumber() : 0.0;
                rfY = (getY().isSet()) ? getY().getNumber() : 0.0;

                if(SvgUnit::percent == getX().getUnit())
                {
                    rfX *= 0.01;
                }

                if(SvgUnit::percent == getY().getUnit())
                {
                    rfY *= 0.01;
                }
            }
            else
            {
                rfX = (getX().isSet()) ? getX().solve(rUser, NumberType::xcoordinate) : 0.0;
                rfY = (getY().isSet()) ? getY().solve(rUser, NumberType::ycoordinate) : 0.0;

                // make relative to rGeoRange
                rfX = (rfX - rGeoRange.getMinX()) / fTargetWidth;
                rfY = (rfY - rGeoRange.getMinY()) / fTargetHeight;
            }
        }

        const drawinglayer::primitive2d::Primitive2DContainer& SvgPatternNode::getPatternPrimitives() const
        {
            if(aPrimitives.empty() && Display::None != getDisplay())
            {
                decomposeSvgNode(const_cast< SvgPatternNode* >(this)->aPrimitives, true);
            }

            if(aPrimitives.empty() && !maXLink.isEmpty())
            {
                const_cast< SvgPatternNode* >(this)->tryToFindLink();

                if (mpXLink && !mbResolvingLink)
                {
                    mbResolvingLink = true;
                    const drawinglayer::primitive2d::Primitive2DContainer& ret = mpXLink->getPatternPrimitives();
                    mbResolvingLink = false;
                    return ret;
                }
            }

            return aPrimitives;
        }

        basegfx::B2DRange SvgPatternNode::getCurrentViewPort() const
        {
            if(getViewBox())
            {
                return *(getViewBox());
            }
            else
            {
                return SvgNode::getCurrentViewPort();
            }
        }

        const basegfx::B2DRange* SvgPatternNode::getViewBox() const
        {
            if(mpViewBox)
            {
                return mpViewBox.get();
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                auto ret = mpXLink->getViewBox();
                mbResolvingLink = false;
                return ret;
            }

            return nullptr;
        }

        const SvgAspectRatio& SvgPatternNode::getSvgAspectRatio() const
        {
            if(maSvgAspectRatio.isSet())
            {
                return maSvgAspectRatio;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                const SvgAspectRatio& ret = mpXLink->getSvgAspectRatio();
                mbResolvingLink = false;
                return ret;
            }

            return maSvgAspectRatio;
        }

        const SvgNumber& SvgPatternNode::getX() const
        {
            if(maX.isSet())
            {
                return maX;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                const SvgNumber& ret = mpXLink->getX();
                mbResolvingLink = false;
                return ret;
            }

            return maX;
        }

        const SvgNumber& SvgPatternNode::getY() const
        {
            if(maY.isSet())
            {
                return maY;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                const SvgNumber& ret = mpXLink->getY();
                mbResolvingLink = false;
                return ret;
            }

            return maY;
        }

        const SvgNumber& SvgPatternNode::getWidth() const
        {
            if(maWidth.isSet())
            {
                return maWidth;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                const SvgNumber& ret = mpXLink->getWidth();
                mbResolvingLink = false;
                return ret;
            }

            return maWidth;
        }

        const SvgNumber& SvgPatternNode::getHeight() const
        {
            if(maHeight.isSet())
            {
                return maHeight;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                const SvgNumber& ret = mpXLink->getHeight();
                mbResolvingLink = false;
                return ret;
            }

            return maHeight;
        }

        const SvgUnits* SvgPatternNode::getPatternUnits() const
        {
            if(moPatternUnits)
            {
                return &*moPatternUnits;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                auto ret = mpXLink->getPatternUnits();
                mbResolvingLink = false;
                return ret;
            }

            return nullptr;
        }

        const SvgUnits* SvgPatternNode::getPatternContentUnits() const
        {
            if(moPatternContentUnits)
            {
                return &*moPatternContentUnits;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                auto ret = mpXLink->getPatternContentUnits();
                mbResolvingLink = false;
                return ret;
            }

            return nullptr;
        }

        std::optional<basegfx::B2DHomMatrix> SvgPatternNode::getPatternTransform() const
        {
            if(mpaPatternTransform)
            {
                return mpaPatternTransform;
            }

            const_cast< SvgPatternNode* >(this)->tryToFindLink();

            if (mpXLink && !mbResolvingLink)
            {
                mbResolvingLink = true;
                auto ret = mpXLink->getPatternTransform();
                mbResolvingLink = false;
                return ret;
            }

            return std::nullopt;
        }

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <svgfemergenode.hxx>
#include <o3tl/string_view.hxx>

namespace svgio::svgreader
{
SvgFeMergeNode::SvgFeMergeNode(SVGToken aType, SvgDocument& rDocument, SvgNode* pParent)
    : SvgFilterNode(aType, rDocument, pParent)
{
}

SvgFeMergeNode::~SvgFeMergeNode() {}

void SvgFeMergeNode::parseAttribute(SVGToken aSVGToken, const OUString& aContent)
{
    // parse own
    switch (aSVGToken)
    {
        case SVGToken::Style:
        {
            readLocalCssStyle(aContent);
            break;
        }
        case SVGToken::Result:
        {
            maResult = aContent.trim();
            break;
        }
        default:
        {
            break;
        }
    }
}

void SvgFeMergeNode::apply(drawinglayer::primitive2d::Primitive2DContainer& rTarget,
                           const SvgFilterNode* pParent) const
{
    const auto& rChildren = getChildren();
    const sal_uInt32 nCount(rChildren.size());

    // apply children's filters
    for (sal_uInt32 a(0); a < nCount; a++)
    {
        SvgFeMergeNode* pFeMergeNode = dynamic_cast<SvgFeMergeNode*>(rChildren[a].get());
        if (pFeMergeNode)
        {
            pFeMergeNode->apply(rTarget, pParent);
        }
    }

    pParent->addGraphicSourceToMapper(maResult, rTarget);
}

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

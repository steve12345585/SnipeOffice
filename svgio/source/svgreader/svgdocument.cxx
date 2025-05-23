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

#include <svgdocument.hxx>
#include <utility>

namespace svgio::svgreader
{
        SvgDocument::SvgDocument(OUString aAbsolutePath)
        : maAbsolutePath(std::move(aAbsolutePath))
        {
        }

        SvgDocument::~SvgDocument()
        {
        }

        void SvgDocument::appendNode(std::unique_ptr<SvgNode> pNode)
        {
            assert(pNode);
            maNodes.push_back(std::move(pNode));
        }

        void SvgDocument::addSvgNodeToMapper(const OUString& rStr, const SvgNode& rNode)
        {
            if(!rStr.isEmpty())
            {
                maIdTokenMapperList.emplace(rStr, &rNode);
            }
        }

        void SvgDocument::removeSvgNodeFromMapper(const OUString& rStr)
        {
            if(!rStr.isEmpty())
            {
                maIdTokenMapperList.erase(rStr);
            }
        }

        const SvgNode* SvgDocument::findSvgNodeById(const OUString& rStr) const
        {
            const IdTokenMapper::const_iterator aResult(maIdTokenMapperList.find(rStr));

            if(aResult == maIdTokenMapperList.end())
            {
                return nullptr;
            }
            else
            {
                return aResult->second;
            }
        }

        void SvgDocument::addSvgStyleAttributesToMapper(const OUString& rStr, const SvgStyleAttributes& rSvgStyleAttributes)
        {
            if(!rStr.isEmpty())
            {
                maIdStyleTokenMapperList.emplace(rStr, &rSvgStyleAttributes);
            }
        }

        const SvgStyleAttributes* SvgDocument::findGlobalCssStyleAttributes(const OUString& rStr) const
        {
            const IdStyleTokenMapper::const_iterator aResult(maIdStyleTokenMapperList.find(rStr));

            if(aResult == maIdStyleTokenMapperList.end())
            {
                return nullptr;
            }
            else
            {
                return aResult->second;
            }
        }

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

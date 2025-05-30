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

#pragma once

#include <unordered_map>
#include "svgnode.hxx"
#include "svgstyleattributes.hxx"

namespace svgio::svgreader
    {
        class SvgStyleNode final : public SvgNode
        {
        private:
            /// use styles
            std::unordered_map< OUString, std::unique_ptr<SvgStyleAttributes> > maSvgStyleAttributes;

            bool                                    mbTextCss : 1; // true == type is 'text/css'

        public:
            SvgStyleNode(
                SvgDocument& rDocument,
                SvgNode* pParent);

            /// #i125258# tell if this node is allowed to have a parent style (e.g. defs do not)
            virtual bool supportsParentStyle() const override;

            virtual void parseAttribute(SVGToken aSVGToken, const OUString& aContent) override;

            /// CssStyleSheet add helpers
            void addCssStyleSheet(std::u16string_view aSelectors, const SvgStyleAttributes& rNewStyle);
            void addCssStyleSheet(std::u16string_view aSelectors, std::u16string_view aContent);
            void addCssStyleSheet(std::u16string_view aSelectorsAndContent);

            /// textCss access
            bool isTextCss() const { return mbTextCss; }
            void setTextCss(bool bNew) { mbTextCss = bNew; }
        };

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

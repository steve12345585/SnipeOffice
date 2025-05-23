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

#include <drawinglayer/drawinglayerdllapi.h>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <rtl/ustring.hxx>

namespace drawinglayer::primitive2d
{
        /** ObjectInfoPrimitive2D class

            Info hierarchy helper class to hold contents like Name, Title and
            Description which are valid for the child content, e.g. created for
            primitives based on DrawingLayer objects or SVG parts. It decomposes
            to its content, so all direct renderers may ignore it. May e.g.
            be used when re-creating graphical content from a sequence of primitives
         */
        class DRAWINGLAYER_DLLPUBLIC ObjectInfoPrimitive2D final : public GroupPrimitive2D
        {
        private:
            OUString                           maName;
            OUString                           maTitle;
            OUString                           maDesc;

        public:
            /// constructor
            ObjectInfoPrimitive2D(
                Primitive2DContainer&& aChildren,
                OUString aName,
                OUString aTitle,
                OUString aDesc);

            /// data read access
            const OUString& getName() const { return maName; }
            const OUString& getTitle() const { return maTitle; }
            const OUString& getDesc() const { return maDesc; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

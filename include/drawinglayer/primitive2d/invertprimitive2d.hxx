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


namespace drawinglayer::primitive2d
{
        /** InvertPrimitive2D class

            This is a helper class which encapsulates geometry that should be
            painted XOR, e.g. old cursor visualisations. It decomposes to
            its content, so when not supporting it, the contained geometry
            will be visualized normally.
            Unfortunately this is still needed, but hard to support on various
            systems. XOR painting needs read access to the target, so modern
            graphic systems tend to not support it anymore.
         */
        class DRAWINGLAYER_DLLPUBLIC InvertPrimitive2D final : public GroupPrimitive2D
        {
        public:
            /// constructor
            explicit InvertPrimitive2D(Primitive2DContainer&& aChildren);

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

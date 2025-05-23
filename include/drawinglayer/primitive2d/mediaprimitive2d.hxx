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

#include <drawinglayer/primitive2d/BufferedDecompositionPrimitive2D.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/color/bcolor.hxx>
#include <vcl/graph.hxx>


namespace drawinglayer::primitive2d
{
        /** MediaPrimitive2D class

            This is a unified primitive for Media visualisation, e.g. animations
            or sounds. It's geometry is defined by Transform. For convenience,
            it also contains a discrete border size (aka Pixels) which will be added
            if used. This makes it a view-dependent primitive. It also gets a filled
            background and the decomposition will try to create a graphic representation
            if the content (defined by the URL), e.g. a still frame for animated stuff.
         */
        class DRAWINGLAYER_DLLPUBLIC MediaPrimitive2D final : public BufferedDecompositionPrimitive2D
        {
        private:
            /// the geometry definition
            basegfx::B2DHomMatrix                       maTransform;

            /// the content definition
            OUString                               maURL;

            /// style: background color
            basegfx::BColor                             maBackgroundColor;

            /// discrete border (in 'pixels')
            sal_uInt32                                  mnDiscreteBorder;

            const Graphic                               maSnapshot;

            /// local decomposition
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            MediaPrimitive2D(
                basegfx::B2DHomMatrix aTransform,
                OUString aURL,
                const basegfx::BColor& rBackgroundColor,
                sal_uInt32 nDiscreteBorder,
                Graphic aSnapshot);

            /// data read access
            const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
            const basegfx::BColor& getBackgroundColor() const { return maBackgroundColor; }
            sal_uInt32 getDiscreteBorder() const { return mnDiscreteBorder; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// get range
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <drawinglayer/primitive2d/primitivetools2d.hxx>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>


// WrongSpellPrimitive2D class

namespace drawinglayer::primitive2d
{
        /** WrongSpellPrimitive2D class

            This is a helper primitive to hold evtl. WrongSpell visualisations
            in the sequence of primitives. The primitive holds this information
            separated from the TextPortions to where it belongs to, to expand the
            TextSimplePortionPrimitive2D more as needed.

            A renderer who does not want to visualize this (if contained at all)
            can detect and ignore this primitive. If its decomposition is used,
            it will be visualized as red wavelines.

            The geometric definition defines a line on the X-Axis (no Y-coordinates)
            which will when transformed by Transformation, create the coordinate data.
         */
        class DRAWINGLAYER_DLLPUBLIC WrongSpellPrimitive2D final : public DiscreteMetricDependentPrimitive2D
        {
        private:
            /// geometry definition
            basegfx::B2DHomMatrix                           maTransformation;
            double                                          mfStart;
            double                                          mfStop;

            /// color (usually red)
            basegfx::BColor                                 maColor;

            /// create local decomposition
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            WrongSpellPrimitive2D(
                basegfx::B2DHomMatrix aTransformation,
                double fStart,
                double fStop,
                const basegfx::BColor& rColor);

            /// data read access
            const basegfx::B2DHomMatrix& getTransformation() const { return maTransformation; }
            double getStart() const { return mfStart; }
            double getStop() const { return mfStop; }
            const basegfx::BColor& getColor() const { return maColor; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

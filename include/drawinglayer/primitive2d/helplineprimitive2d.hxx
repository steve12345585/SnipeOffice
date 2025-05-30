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
#include <basegfx/color/bcolor.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>


// HelplinePrimitive2D class

namespace drawinglayer::primitive2d
{
        /** HelplineStyle2D definition

            The available styles of Helplines
        */
        enum class HelplineStyle2D
        {
            Point,
            Line
        };

        /** HelplinePrimitive2D class

            This primitive provides a view-dependent helpline definition. The Helpline
            is defined by a line equation (Point and vector) and a style. When the style
            is a line, dependent from Viewport the visible part of that Helpline is
            constructed. For Point, a cross is constructed. This primitive is highly
            view-dependent.

            The visualisation uses the two given colors to create a dashed line with
            the given dash length.
         */
        class DRAWINGLAYER_DLLPUBLIC HelplinePrimitive2D final : public BufferedDecompositionPrimitive2D
        {
        private:
            /// Helpline geometry definition
            basegfx::B2DPoint                               maPosition;
            basegfx::B2DVector                              maDirection;
            HelplineStyle2D                                 meStyle;

            /// Helpline style definition
            basegfx::BColor                                 maRGBColA;
            basegfx::BColor                                 maRGBColB;
            double                                          mfDiscreteDashLength;

            /** the last used object to view transformtion and the last Viewport,
                used from getDecomposition for decide buffering
             */
            basegfx::B2DHomMatrix                           maLastObjectToViewTransformation;
            basegfx::B2DRange                               maLastViewport;

            /// create local decomposition
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            HelplinePrimitive2D(
                const basegfx::B2DPoint& rPosition,
                const basegfx::B2DVector& rDirection,
                HelplineStyle2D eStyle,
                const basegfx::BColor& rRGBColA,
                const basegfx::BColor& aRGBColB,
                double fDiscreteDashLength);

            /// data read access
            const basegfx::B2DPoint& getPosition() const { return maPosition; }
            const basegfx::B2DVector& getDirection() const { return maDirection; }
            HelplineStyle2D getStyle() const { return meStyle; }
            const basegfx::BColor& getRGBColA() const { return maRGBColA; }
            const basegfx::BColor& getRGBColB() const { return maRGBColB; }
            double getDiscreteDashLength() const { return mfDiscreteDashLength; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;

            /// Override standard getDecomposition to be view-dependent here
            virtual void get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

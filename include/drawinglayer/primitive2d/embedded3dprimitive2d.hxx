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
#include <drawinglayer/primitive3d/baseprimitive3d.hxx>
#include <drawinglayer/geometry/viewinformation3d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>


// Embedded3DPrimitive2D class

namespace drawinglayer::primitive2d
{
        /** Embedded3DPrimitive2D class

            This is a helper primitive which allows embedding of single 3D
            primitives to the 2D primitive logic. It will get the scene it's
            involved and thus the 3D transformation. With this information it
            is able to provide 2D range data for a 3D primitive.

            This primitive will not be visualized and decomposes to a yellow
            2D rectangle to visualize that this should never be visualized
         */
        class DRAWINGLAYER_DLLPUBLIC Embedded3DPrimitive2D final : public BufferedDecompositionPrimitive2D
        {
        private:
            /// the sequence of 3d primitives
            primitive3d::Primitive3DContainer                    mxChildren3D;

            /// the 2D scene object transformation
            basegfx::B2DHomMatrix                               maObjectTransformation;

            /// the 3D transformations
            geometry::ViewInformation3D                         maViewInformation3D;

            /** if the embedded 3D primitives contain shadow, these parameters are needed
                to extract the shadow which is a sequence of 2D primitives and may expand
                the 2D range. Since every single 3D object in a scene may individually
                have shadow or not, these values need to be provided and prepared. The shadow
                distance itself (a 2D transformation) is part of the 3D shadow definition
             */
            basegfx::B3DVector                                  maLightNormal;
            double                                              mfShadowSlant;
            basegfx::B3DRange                                   maScene3DRange;

            /// the primitiveSequence for on-demand created shadow primitives (see mbShadow3DChecked)
            Primitive2DContainer                                   maShadowPrimitives;

            /// #i96669# add simple range buffering for this primitive
            basegfx::B2DRange                                   maB2DRange;

            /** flag if given 3D geometry is already checked for shadow definitions and 2d shadows
                are created in maShadowPrimitives
             */
            bool                                                mbShadow3DChecked : 1;

            /// private helpers
            bool impGetShadow3D() const;

            /// local decomposition.
            virtual Primitive2DReference create2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const override;

        public:
            /// constructor
            Embedded3DPrimitive2D(
                primitive3d::Primitive3DContainer xChildren3D,
                basegfx::B2DHomMatrix aObjectTransformation,
                geometry::ViewInformation3D aViewInformation3D,
                const basegfx::B3DVector& rLightNormal,
                double fShadowSlant,
                const basegfx::B3DRange& rScene3DRange);

            /// data read access
            const primitive3d::Primitive3DContainer& getChildren3D() const { return mxChildren3D; }
            const basegfx::B2DHomMatrix& getObjectTransformation() const { return maObjectTransformation; }
            const geometry::ViewInformation3D& getViewInformation3D() const { return maViewInformation3D; }
            const basegfx::B3DVector& getLightNormal() const { return maLightNormal; }
            double getShadowSlant() const { return mfShadowSlant; }
            const basegfx::B3DRange& getScene3DRange() const { return maScene3DRange; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// get range
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override;
        };
} // end of namespace drawinglayer::primitive2d


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

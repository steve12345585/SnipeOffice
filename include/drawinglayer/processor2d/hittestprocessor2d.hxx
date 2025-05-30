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

#ifndef INCLUDED_DRAWINGLAYER_PROCESSOR2D_HITTESTPROCESSOR2D_HXX
#define INCLUDED_DRAWINGLAYER_PROCESSOR2D_HITTESTPROCESSOR2D_HXX

#include <drawinglayer/drawinglayerdllapi.h>
#include <drawinglayer/primitive2d/Primitive2DContainer.hxx>
#include <drawinglayer/processor2d/baseprocessor2d.hxx>

namespace basegfx { class B2DPolygon; }
namespace basegfx { class B2DPolyPolygon; }
namespace drawinglayer::primitive2d { class ScenePrimitive2D; }
class BitmapEx;

namespace drawinglayer::processor2d
    {
        /** HitTestProcessor2D class

            This processor implements a HitTest with the fed primitives,
            given tolerance and extras
         */
        class DRAWINGLAYER_DLLPUBLIC HitTestProcessor2D final : public BaseProcessor2D
        {
        private:
            /// discrete HitTest position
            basegfx::B2DPoint           maDiscreteHitPosition;

            /// discrete HitTolerance
            basegfx::B2DVector          maDiscreteHitTolerancePerAxis;

            /// stack of HitPrimitives, taken care of during HitTest run
            primitive2d::Primitive2DContainer        maHitStack;

            /// flag if HitStack shall be collected as part of the result, default is false
            bool                        mbCollectHitStack : 1;

            /// Boolean to flag if a hit was found. If yes, fast exit is taken
            bool                        mbHit : 1;

            /// flag to concentrate on text hits only
            bool                        mbHitTextOnly : 1;

            /// tooling methods
            void processBasePrimitive2D(const primitive2d::BasePrimitive2D& rCandidate) override;
            bool checkHairlineHitWithTolerance(
                const basegfx::B2DPolygon& rPolygon,
                const basegfx::B2DVector& rDiscreteHitTolerancePerAxis) const;
            bool checkFillHitWithTolerance(
                const basegfx::B2DPolyPolygon& rPolyPolygon,
                const basegfx::B2DVector& rDiscreteHitTolerancePerAxis) const;
            void check3DHit(const primitive2d::ScenePrimitive2D& rCandidate);
            void checkBitmapHit(basegfx::B2DRange aRange, const BitmapEx& rBitmapEx, const basegfx::B2DHomMatrix& rTransform);

        public:
            HitTestProcessor2D(
                const geometry::ViewInformation2D& rViewInformation,
                const basegfx::B2DPoint& rLogicHitPosition,
                const basegfx::B2DVector& rLogicHitTolerancePerAxis,
                bool bHitTextOnly);
            virtual ~HitTestProcessor2D() override;

            /// switch on collecting primitives for a found hit on maHitStack, default is off
            void collectHitStack(bool bCollect) { mbCollectHitStack = bCollect; }

            /// get HitStack of primitives, first is the one that created the hit, last is the
            /// top-most
            const primitive2d::Primitive2DContainer& getHitStack() const { return maHitStack; }

            /// data read access
            const basegfx::B2DPoint& getDiscreteHitPosition() const { return maDiscreteHitPosition; }
            const basegfx::B2DVector& getDiscreteHitTolerance() const { return maDiscreteHitTolerancePerAxis; }
            bool getCollectHitStack() const { return mbCollectHitStack; }
            bool getHit() const { return mbHit; }
            bool getHitTextOnly() const { return mbHitTextOnly; }
        };

} // end of namespace drawinglayer::processor2d

#endif // INCLUDED_DRAWINGLAYER_PROCESSOR2D_HITTESTPROCESSOR2D_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

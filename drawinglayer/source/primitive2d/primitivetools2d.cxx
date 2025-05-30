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

#include <drawinglayer/primitive2d/primitivetools2d.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>


namespace drawinglayer::primitive2d
{
        void DiscreteMetricDependentPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            // get the current DiscreteUnit, look at X and Y and use the maximum
            const basegfx::B2DVector aDiscreteVector(rViewInformation.getInverseObjectToViewTransformation() * basegfx::B2DVector(1.0, 1.0));
            const double fDiscreteUnit(std::min(fabs(aDiscreteVector.getX()), fabs(aDiscreteVector.getY())));

            if(hasBuffered2DDecomposition() && !basegfx::fTools::equal(fDiscreteUnit, getDiscreteUnit()))
            {
                // conditions of last local decomposition have changed, delete
                const_cast< DiscreteMetricDependentPrimitive2D* >(this)->setBuffered2DDecomposition(nullptr);
            }

            if(!hasBuffered2DDecomposition())
            {
                // remember new valid DiscreteUnit
                const_cast< DiscreteMetricDependentPrimitive2D* >(this)->mfDiscreteUnit = fDiscreteUnit;
            }

            // call base implementation
            BufferedDecompositionPrimitive2D::get2DDecomposition(rVisitor, rViewInformation);
        }




        void ViewportDependentPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            // get the current Viewport
            const basegfx::B2DRange& rViewport = rViewInformation.getViewport();

            if(hasBuffered2DDecomposition() && !rViewport.equal(getViewport()))
            {
                // conditions of last local decomposition have changed, delete
                const_cast< ViewportDependentPrimitive2D* >(this)->setBuffered2DDecomposition(nullptr);
            }

            if(!hasBuffered2DDecomposition())
            {
                // remember new valid DiscreteUnit
                const_cast< ViewportDependentPrimitive2D* >(this)->maViewport = rViewport;
            }

            // call base implementation
            BufferedDecompositionPrimitive2D::get2DDecomposition(rVisitor, rViewInformation);
        }

        void ViewTransformationDependentPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            // get the current ViewTransformation
            const basegfx::B2DHomMatrix& rViewTransformation = rViewInformation.getViewTransformation();

            if(hasBuffered2DDecomposition() && rViewTransformation != getViewTransformation())
            {
                // conditions of last local decomposition have changed, delete
                const_cast< ViewTransformationDependentPrimitive2D* >(this)->setBuffered2DDecomposition(nullptr);
            }

            if(!hasBuffered2DDecomposition())
            {
                // remember new valid ViewTransformation
                const_cast< ViewTransformationDependentPrimitive2D* >(this)->maViewTransformation = rViewTransformation;
            }

            // call base implementation
            BufferedDecompositionPrimitive2D::get2DDecomposition(rVisitor, rViewInformation);
        }

        void ObjectAndViewTransformationDependentPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            // get the current ViewTransformation
            const basegfx::B2DHomMatrix& rViewTransformation = rViewInformation.getViewTransformation();

            if(hasBuffered2DDecomposition() && rViewTransformation != getViewTransformation())
            {
                // conditions of last local decomposition have changed, delete
                const_cast< ObjectAndViewTransformationDependentPrimitive2D* >(this)->setBuffered2DDecomposition(nullptr);
            }

            // get the current ObjectTransformation
            const basegfx::B2DHomMatrix& rObjectTransformation = rViewInformation.getObjectTransformation();

            if(hasBuffered2DDecomposition() && rObjectTransformation != getObjectTransformation())
            {
                // conditions of last local decomposition have changed, delete
                const_cast< ObjectAndViewTransformationDependentPrimitive2D* >(this)->setBuffered2DDecomposition(nullptr);
            }

            if(!hasBuffered2DDecomposition())
            {
                // remember new valid ViewTransformation, and ObjectTransformation
                const_cast< ObjectAndViewTransformationDependentPrimitive2D* >(this)->maViewTransformation = rViewTransformation;
                const_cast< ObjectAndViewTransformationDependentPrimitive2D* >(this)->maObjectTransformation = rObjectTransformation;
            }

            // call base implementation
            BufferedDecompositionPrimitive2D::get2DDecomposition(rVisitor, rViewInformation);
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

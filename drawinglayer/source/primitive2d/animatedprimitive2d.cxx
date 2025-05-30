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

#include <drawinglayer/primitive2d/animatedprimitive2d.hxx>
#include <drawinglayer/animation/animationtiming.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>


using namespace com::sun::star;


namespace drawinglayer::primitive2d
{
        void AnimatedSwitchPrimitive2D::setAnimationEntry(const animation::AnimationEntry& rNew)
        {
            // clone given animation description
            mpAnimationEntry = rNew.clone();
        }

        AnimatedSwitchPrimitive2D::AnimatedSwitchPrimitive2D(
            const animation::AnimationEntry& rAnimationEntry,
            Primitive2DContainer&& aChildren,
            bool bIsTextAnimation)
        :   GroupPrimitive2D(std::move(aChildren)),
            mbIsTextAnimation(bIsTextAnimation)
        {
            // clone given animation description
            mpAnimationEntry = rAnimationEntry.clone();
        }

        AnimatedSwitchPrimitive2D::~AnimatedSwitchPrimitive2D()
        {
        }

        bool AnimatedSwitchPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            if(GroupPrimitive2D::operator==(rPrimitive))
            {
                const AnimatedSwitchPrimitive2D& rCompare = static_cast< const AnimatedSwitchPrimitive2D& >(rPrimitive);

                return (getAnimationEntry() == rCompare.getAnimationEntry());
            }

            return false;
        }

        void AnimatedSwitchPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            if(getChildren().empty())
                return;

            const double fState(getAnimationEntry().getStateAtTime(rViewInformation.getViewTime()));
            const sal_uInt32 nLen(getChildren().size());
            sal_uInt32 nIndex(basegfx::fround(fState * static_cast<double>(nLen)));

            if(nIndex >= nLen)
            {
                nIndex = nLen - 1;
            }

            rVisitor.visit(getChildren()[nIndex]);
        }

        // provide unique ID
        sal_uInt32 AnimatedSwitchPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_ANIMATEDSWITCHPRIMITIVE2D;
        }

} // end of namespace drawinglayer::primitive2d


namespace drawinglayer::primitive2d
{
        AnimatedBlinkPrimitive2D::AnimatedBlinkPrimitive2D(
            const animation::AnimationEntry& rAnimationEntry,
            Primitive2DContainer&& aChildren)
        :   AnimatedSwitchPrimitive2D(rAnimationEntry, std::move(aChildren), true/*bIsTextAnimation*/)
        {
        }

        void AnimatedBlinkPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            if(!getChildren().empty())
            {
                const double fState(getAnimationEntry().getStateAtTime(rViewInformation.getViewTime()));

                if(fState < 0.5)
                {
                    getChildren(rVisitor);
                }
            }
        }

        // provide unique ID
        sal_uInt32 AnimatedBlinkPrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_ANIMATEDBLINKPRIMITIVE2D;
        }

} // end of namespace drawinglayer::primitive2d


namespace drawinglayer::primitive2d
{
        AnimatedInterpolatePrimitive2D::AnimatedInterpolatePrimitive2D(
            const std::vector< basegfx::B2DHomMatrix >& rmMatrixStack,
            const animation::AnimationEntry& rAnimationEntry,
            Primitive2DContainer&& aChildren)
        :   AnimatedSwitchPrimitive2D(rAnimationEntry, std::move(aChildren), true/*bIsTextAnimation*/)
        {
            // copy matrices to locally pre-decomposed matrix stack
            const sal_uInt32 nCount(rmMatrixStack.size());
            maMatrixStack.reserve(nCount);

            for(const auto& a : rmMatrixStack)
            {
                maMatrixStack.emplace_back(a);
            }
        }

        void AnimatedInterpolatePrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            const sal_uInt32 nSize(maMatrixStack.size());

            if(nSize)
            {
                double fState(getAnimationEntry().getStateAtTime(rViewInformation.getViewTime()));

                if(fState < 0.0)
                {
                    fState = 0.0;
                }
                else if(fState > 1.0)
                {
                    fState = 1.0;
                }

                const double fIndex(fState * static_cast<double>(nSize - 1));
                const sal_uInt32 nIndA(sal_uInt32(floor(fIndex)));
                const double fOffset(fIndex - static_cast<double>(nIndA));
                basegfx::B2DHomMatrix aTargetTransform;
                std::vector< basegfx::utils::B2DHomMatrixBufferedDecompose >::const_iterator aMatA(maMatrixStack.begin() + nIndA);

                if(basegfx::fTools::equalZero(fOffset))
                {
                    // use matrix from nIndA directly
                    aTargetTransform = aMatA->getB2DHomMatrix();
                }
                else
                {
                    // interpolate. Get involved buffered decomposed matrices
                    const sal_uInt32 nIndB((nIndA + 1) % nSize);
                    std::vector< basegfx::utils::B2DHomMatrixBufferedDecompose >::const_iterator aMatB(maMatrixStack.begin() + nIndB);

                    // interpolate for fOffset [0.0 .. 1.0[
                    const basegfx::B2DVector aScale(basegfx::interpolate(aMatA->getScale(), aMatB->getScale(), fOffset));
                    const basegfx::B2DVector aTranslate(basegfx::interpolate(aMatA->getTranslate(), aMatB->getTranslate(), fOffset));
                    const double fRotate(((aMatB->getRotate() - aMatA->getRotate()) * fOffset) + aMatA->getRotate());
                    const double fShearX(((aMatB->getShearX() - aMatA->getShearX()) * fOffset) + aMatA->getShearX());

                    // build matrix for state
                    aTargetTransform = basegfx::utils::createScaleShearXRotateTranslateB2DHomMatrix(
                        aScale, fShearX, fRotate, aTranslate);
                }

                // create new transform primitive reference, return new sequence
                Primitive2DReference xRef(new TransformPrimitive2D(aTargetTransform, Primitive2DContainer(getChildren())));
                rVisitor.visit(xRef);
            }
            else
            {
                getChildren(rVisitor);
            }
        }

        // provide unique ID
        sal_uInt32 AnimatedInterpolatePrimitive2D::getPrimitive2DID() const
        {
            return PRIMITIVE2D_ID_ANIMATEDINTERPOLATEPRIMITIVE2D;
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

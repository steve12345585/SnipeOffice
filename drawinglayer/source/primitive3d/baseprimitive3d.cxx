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

#include <drawinglayer/primitive3d/baseprimitive3d.hxx>
#include <drawinglayer/geometry/viewinformation3d.hxx>
#include <basegfx/utils/canvastools.hxx>
#include <comphelper/sequence.hxx>


using namespace com::sun::star;


namespace drawinglayer::primitive3d
{
        BasePrimitive3D::BasePrimitive3D()
        {
        }

        BasePrimitive3D::~BasePrimitive3D()
        {
        }

        bool BasePrimitive3D::operator==( const BasePrimitive3D& rPrimitive ) const
        {
            return (getPrimitive3DID() == rPrimitive.getPrimitive3DID());
        }

        basegfx::B3DRange BasePrimitive3D::getB3DRange(const geometry::ViewInformation3D& rViewInformation) const
        {
            return get3DDecomposition(rViewInformation).getB3DRange(rViewInformation);
        }

        Primitive3DContainer BasePrimitive3D::get3DDecomposition(const geometry::ViewInformation3D& /*rViewInformation*/) const
        {
            return Primitive3DContainer();
        }

        css::uno::Sequence< ::css::uno::Reference< ::css::graphic::XPrimitive3D > > SAL_CALL BasePrimitive3D::getDecomposition( const uno::Sequence< beans::PropertyValue >& rViewParameters )
        {
            const geometry::ViewInformation3D aViewInformation(rViewParameters);
            return comphelper::containerToSequence(get3DDecomposition(aViewInformation));
        }

        css::geometry::RealRectangle3D SAL_CALL BasePrimitive3D::getRange( const uno::Sequence< beans::PropertyValue >& rViewParameters )
        {
            const geometry::ViewInformation3D aViewInformation(rViewParameters);
            return basegfx::unotools::rectangle3DFromB3DRectangle(getB3DRange(aViewInformation));
        }


        Primitive3DContainer BufferedDecompositionPrimitive3D::create3DDecomposition(const geometry::ViewInformation3D& /*rViewInformation*/) const
        {
            return Primitive3DContainer();
        }

        BufferedDecompositionPrimitive3D::BufferedDecompositionPrimitive3D()
        {
        }

        Primitive3DContainer BufferedDecompositionPrimitive3D::get3DDecomposition(const geometry::ViewInformation3D& rViewInformation) const
        {
            std::unique_lock aGuard( m_aMutex );

            if(getBuffered3DDecomposition().empty())
            {
                const Primitive3DContainer aNewSequence(create3DDecomposition(rViewInformation));
                const_cast< BufferedDecompositionPrimitive3D* >(this)->setBuffered3DDecomposition(aNewSequence);
            }

            return getBuffered3DDecomposition();
        }

// tooling

        // get range3D from a given Primitive3DReference
        basegfx::B3DRange getB3DRangeFromPrimitive3DReference(const Primitive3DReference& rCandidate, const geometry::ViewInformation3D& aViewInformation)
        {
            basegfx::B3DRange aRetval;

            if(rCandidate.is())
            {
                const BasePrimitive3D* pCandidate(static_cast< BasePrimitive3D* >(rCandidate.get()));
                aRetval.expand(pCandidate->getB3DRange(aViewInformation));
            }

            return aRetval;
        }

        // get range3D from a given Primitive3DContainer
        basegfx::B3DRange Primitive3DContainer::getB3DRange(const geometry::ViewInformation3D& aViewInformation) const
        {
            basegfx::B3DRange aRetval;

            if(!empty())
            {
                const size_t nCount(size());

                for(size_t a(0); a < nCount; a++)
                {
                    aRetval.expand(getB3DRangeFromPrimitive3DReference((*this)[a], aViewInformation));
                }
            }

            return aRetval;
        }

        bool arePrimitive3DReferencesEqual(const Primitive3DReference& rxA, const Primitive3DReference& rxB)
        {
            const bool bAIs(rxA.is());

            if(bAIs != rxB.is())
            {
                return false;
            }

            if(!bAIs)
            {
                return true;
            }

            const BasePrimitive3D* pA(static_cast< const BasePrimitive3D* >(rxA.get()));
            const BasePrimitive3D* pB(static_cast< const BasePrimitive3D* >(rxB.get()));

            return pA->operator==(*pB);
        }

        bool Primitive3DContainer::operator==(const Primitive3DContainer& rB) const
        {
            const bool bAHasElements(!empty());

            if(bAHasElements != !rB.empty())
            {
                return false;
            }

            if(!bAHasElements)
            {
                return true;
            }

            const size_t nCount(size());

            if(nCount != rB.size())
            {
                return false;
            }

            for(size_t a(0); a < nCount; a++)
            {
                if(!arePrimitive3DReferencesEqual((*this)[a], rB[a]))
                {
                    return false;
                }
            }

            return true;
        }

        void Primitive3DContainer::append(const Primitive3DContainer& rSource)
        {
            insert(end(), rSource.begin(), rSource.end());
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

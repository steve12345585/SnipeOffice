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

#include "ShadowOverlayObject.hxx"

#include <view.hxx>
#include <svx/sdrpaintwindow.hxx>
#include <svx/svdview.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>

#include <sw_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/primitivetools2d.hxx>
#include <drawinglayer/primitive2d/fillgradientprimitive2d.hxx>
#include <basegfx/utils/gradienttools.hxx>

namespace sw::sidebarwindows {

// helper SwPostItShadowPrimitive

namespace {

// Used to allow view-dependent primitive definition. For that purpose, the
// initially created primitive (this one) always has to be view-independent,
// but the decomposition is made view-dependent. Very simple primitive which
// just remembers the discrete data and applies it at decomposition time.
class ShadowPrimitive : public drawinglayer::primitive2d::DiscreteMetricDependentPrimitive2D
{
private:
    basegfx::B2DPoint           maBasePosition;
    basegfx::B2DPoint           maSecondPosition;
    ShadowState                 maShadowState;

protected:
    virtual drawinglayer::primitive2d::Primitive2DReference create2DDecomposition(
        const drawinglayer::geometry::ViewInformation2D& rViewInformation) const override;

public:
    ShadowPrimitive(
        const basegfx::B2DPoint& rBasePosition,
        const basegfx::B2DPoint& rSecondPosition,
        ShadowState aShadowState)
    :   maBasePosition(rBasePosition),
        maSecondPosition(rSecondPosition),
        maShadowState(aShadowState)
    {}

    // data access
    const basegfx::B2DPoint& getSecondPosition() const { return maSecondPosition; }

    virtual bool operator==( const drawinglayer::primitive2d::BasePrimitive2D& rPrimitive ) const override;

    virtual sal_uInt32 getPrimitive2DID() const override;
};

}

drawinglayer::primitive2d::Primitive2DReference ShadowPrimitive::create2DDecomposition(
    const drawinglayer::geometry::ViewInformation2D& /*rViewInformation*/) const
{
    const Color aBgCol = svtools::ColorConfig().GetColorValue(svtools::WRITERSECTIONBOUNDARIES).nColor;
    const Color aBgColInv = aBgCol.IsDark() ? COL_WHITE : COL_BLACK;
    // get logic sizes in object coordinate system
    basegfx::B2DRange aRange(maBasePosition);
    drawinglayer::primitive2d::Primitive2DReference xRet;
    switch(maShadowState)
    {
        case SS_NORMAL:
        {
            aRange.expand(basegfx::B2DTuple(getSecondPosition().getX(), getSecondPosition().getY() + (1.0 * getDiscreteUnit())));

            ::drawinglayer::attribute::FillGradientAttribute aFillGradientAttribute(
                css::awt::GradientStyle_LINEAR,
                0.0,
                0.5,
                0.5,
                M_PI,
                basegfx::BColorStops(aBgCol.getBColor(), aBgColInv.getBColor()));
            xRet =
                new drawinglayer::primitive2d::FillGradientPrimitive2D(
                    aRange,
                    std::move(aFillGradientAttribute));
            break;
        }
        case SS_VIEW:
        {
            aRange.expand(basegfx::B2DTuple(getSecondPosition().getX(), getSecondPosition().getY() + (2.0 * getDiscreteUnit())));
            drawinglayer::attribute::FillGradientAttribute aFillGradientAttribute(
                css::awt::GradientStyle_LINEAR,
                0.0,
                0.5,
                0.5,
                M_PI,
                basegfx::BColorStops(aBgCol.getBColor(), aBgColInv.getBColor()));
            xRet =
                new drawinglayer::primitive2d::FillGradientPrimitive2D(
                    aRange,
                    std::move(aFillGradientAttribute));
            break;
        }
        case SS_EDIT:
        {
            aRange.expand(basegfx::B2DTuple(getSecondPosition().getX(), getSecondPosition().getY() + (4.0 * getDiscreteUnit())));
            drawinglayer::attribute::FillGradientAttribute aFillGradientAttribute(
                css::awt::GradientStyle_LINEAR,
                0.0,
                0.5,
                0.5,
                M_PI,
                basegfx::BColorStops(aBgCol.getBColor(), aBgColInv.getBColor()));
            xRet =
                new drawinglayer::primitive2d::FillGradientPrimitive2D(
                    aRange,
                    std::move(aFillGradientAttribute));
            break;
        }
        default:
        {
            break;
        }
    }
    return xRet;
}

bool ShadowPrimitive::operator==( const drawinglayer::primitive2d::BasePrimitive2D& rPrimitive ) const
{
    if(drawinglayer::primitive2d::DiscreteMetricDependentPrimitive2D::operator==(rPrimitive))
    {
        const ShadowPrimitive& rCompare = static_cast< const ShadowPrimitive& >(rPrimitive);

        return (maBasePosition == rCompare.maBasePosition
            && getSecondPosition() == rCompare.getSecondPosition()
            && maShadowState == rCompare.maShadowState);
    }

    return false;
}

sal_uInt32 ShadowPrimitive::getPrimitive2DID() const
{
    return PRIMITIVE2D_ID_SWSIDEBARSHADOWPRIMITIVE;
}

/* static */ std::unique_ptr<ShadowOverlayObject> ShadowOverlayObject::CreateShadowOverlayObject( SwView const & rDocView )
{
    std::unique_ptr<ShadowOverlayObject> pShadowOverlayObject;

    if ( rDocView.GetDrawView() )
    {
        SdrPaintWindow* pPaintWindow = rDocView.GetDrawView()->GetPaintWindow(0);
        if( pPaintWindow )
        {
            const rtl::Reference< sdr::overlay::OverlayManager >& xOverlayManager = pPaintWindow->GetOverlayManager();

            if ( xOverlayManager.is() )
            {
                pShadowOverlayObject.reset( new ShadowOverlayObject( basegfx::B2DPoint(0,0),
                                                                basegfx::B2DPoint(0,0),
                                                                Color(0,0,0) ) );
                xOverlayManager->add(*pShadowOverlayObject);
            }
        }
    }

    return pShadowOverlayObject;
}

ShadowOverlayObject::ShadowOverlayObject( const basegfx::B2DPoint& rBasePos,
                                          const basegfx::B2DPoint& rSecondPosition,
                                          Color aBaseColor )
    : OverlayObjectWithBasePosition(rBasePos, aBaseColor)
    , maSecondPosition(rSecondPosition)
    , mShadowState(SS_NORMAL)
{
}

ShadowOverlayObject::~ShadowOverlayObject()
{
    if ( getOverlayManager() )
    {
        getOverlayManager()->remove(*this);
    }
}

drawinglayer::primitive2d::Primitive2DContainer ShadowOverlayObject::createOverlayObjectPrimitive2DSequence()
{
    const drawinglayer::primitive2d::Primitive2DReference aReference(
        new ShadowPrimitive( getBasePosition(),
                             maSecondPosition,
                             GetShadowState() ) );
    return drawinglayer::primitive2d::Primitive2DContainer { aReference };
}

void ShadowOverlayObject::SetShadowState(ShadowState aState)
{
    if (mShadowState != aState)
    {
        mShadowState = aState;

        objectChange();
    }
}

void ShadowOverlayObject::SetPosition( const basegfx::B2DPoint& rPoint1,
                                       const basegfx::B2DPoint& rPoint2)
{
    if(!rPoint1.equal(getBasePosition()) || !rPoint2.equal(maSecondPosition))
    {
        maBasePosition = rPoint1;
        maSecondPosition = rPoint2;

        objectChange();
    }
}

} // end of namespace sw::sidebarwindows

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

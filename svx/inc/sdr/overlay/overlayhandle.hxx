/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#ifndef INCLUDED_SVX_SDR_OVERLAY_OVERLAYHANDLE_HXX
#define INCLUDED_SVX_SDR_OVERLAY_OVERLAYHANDLE_HXX

#include <svx/sdr/overlay/overlayobject.hxx>
#include <basegfx/vector/b2dsize.hxx>

namespace sdr::overlay {


class OverlayHandle final : public OverlayObjectWithBasePosition
{
    basegfx::B2DSize maSize;
    Color maStrokeColor;

    // geometry creation for OverlayObject
    virtual drawinglayer::primitive2d::Primitive2DContainer createOverlayObjectPrimitive2DSequence() override;

public:
    OverlayHandle(const basegfx::B2DPoint& rBasePos,
                  const basegfx::B2DSize& rSize,
                  Color const & rStrokeColor,
                  Color const & rFillColor);

    virtual ~OverlayHandle() override;
};

} // end of namespace sdr::overlay

#endif // INCLUDED_SVX_SDR_OVERLAY_OVERLAYHANDLE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

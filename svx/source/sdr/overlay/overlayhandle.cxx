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

#include <sdr/overlay/overlayhandle.hxx>
#include <sdr/overlay/overlaytools.hxx>

namespace sdr::overlay {

using namespace drawinglayer;
using namespace basegfx;

primitive2d::Primitive2DContainer OverlayHandle::createOverlayObjectPrimitive2DSequence()
{
    basegfx::BColor aStrokeColor = maStrokeColor.getBColor();
    basegfx::BColor aFillColor = getBaseColor().getBColor();

    const primitive2d::Primitive2DReference aReference(
        new primitive2d::OverlayStaticRectanglePrimitive(maBasePosition, maSize, aStrokeColor, aFillColor, 0.3f, 0.0f));

    return primitive2d::Primitive2DContainer { aReference };
}

OverlayHandle::OverlayHandle(const B2DPoint& rBasePos,
                             const B2DSize& rSize,
                             Color const & rStrokeColor,
                             Color const & rFillColor)
    : OverlayObjectWithBasePosition(rBasePos, rFillColor)
    , maSize(rSize)
    , maStrokeColor(rStrokeColor)
{
}

OverlayHandle::~OverlayHandle()
{
}

} // end of namespace sdr::overlay

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

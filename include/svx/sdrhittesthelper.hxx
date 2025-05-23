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

#ifndef INCLUDED_SVX_SDRHITTESTHELPER_HXX
#define INCLUDED_SVX_SDRHITTESTHELPER_HXX

#include <svx/svxdllapi.h>


// #i101872# new Object HitTest as View-tooling


class Point;
class SdrObject;
class SdrPageView;
class SdrLayerIDSet;
class SdrObjList;
namespace sdr::contact { class ViewObjectContact; }
namespace basegfx
{
class B2DPoint;
class B2DVector;
}
namespace drawinglayer::primitive2d { class Primitive2DContainer; }


// Wrappers for classic Sdr* Mode/View classes

SVXCORE_DLLPUBLIC SdrObject* SdrObjectPrimitiveHit(
    const SdrObject& rObject,
    const Point& rPnt,
    const basegfx::B2DVector& rHitTolerance,
    const SdrPageView& rSdrPageView,
    const SdrLayerIDSet* pVisiLayer,
    bool bTextOnly,
    /// allow getting back an evtl. resulting primitive stack which lead to a hit
    drawinglayer::primitive2d::Primitive2DContainer* pHitContainer = nullptr);

SVXCORE_DLLPUBLIC SdrObject* SdrObjListPrimitiveHit(
    const SdrObjList& rList,
    const Point& rPnt,
    const basegfx::B2DVector& rHitTolerance,
    const SdrPageView& rSdrPageView,
    const SdrLayerIDSet* pVisiLayer,
    bool bTextOnly);


// the pure HitTest based on a VOC

bool ViewObjectContactPrimitiveHit(
    const sdr::contact::ViewObjectContact& rVOC,
    const basegfx::B2DPoint& rHitPosition,
    const basegfx::B2DVector& rLogicHitTolerance,
    bool bTextOnly,
    /// allow to get back the stack of primitives that lead to the hit
    drawinglayer::primitive2d::Primitive2DContainer* pHitContainer);


#endif // INCLUDED_SVX_SDRHITTESTHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

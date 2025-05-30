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

#ifndef INCLUDED_SVX_SDR_OVERLAY_OVERLAYOBJECTLIST_HXX
#define INCLUDED_SVX_SDR_OVERLAY_OVERLAYOBJECTLIST_HXX

#include <svx/sdr/overlay/overlayobject.hxx>
#include <sal/types.h>
#include <svx/svxdllapi.h>
#include <memory>
#include <vector>


class Point;

namespace sdr::overlay
    {
        class SVXCORE_DLLPUBLIC OverlayObjectList final
        {
            // the vector of OverlayObjects
            ::std::vector< std::unique_ptr<OverlayObject> > maVector;

        public:
            OverlayObjectList() {}
            OverlayObjectList(const OverlayObjectList&) = delete;
            OverlayObjectList& operator=(const OverlayObjectList&) = delete;
            ~OverlayObjectList();

            // clear list, this includes deletion of all contained objects
            void clear();

            // append objects (takes ownership)
            void append(std::unique_ptr<OverlayObject> pOverlayObject);

            // access to objects
            sal_uInt32 count() const { return maVector.size(); }
            OverlayObject& getOverlayObject(sal_uInt32 nIndex) const { return *(maVector[nIndex]); }

            // Hittest with logical coordinates
            bool isHitLogic(const basegfx::B2DPoint& rLogicPosition, double fLogicTolerance = 0.0) const;

            // Hittest with pixel coordinates
            bool isHitPixel(const Point& rDiscretePosition) const;

            // calculate BaseRange of all included OverlayObjects and return
            basegfx::B2DRange getBaseRange() const;
        };

} // end of namespace sdr::overlay


#endif // INCLUDED_SVX_SDR_OVERLAY_OVERLAYOBJECTLIST_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

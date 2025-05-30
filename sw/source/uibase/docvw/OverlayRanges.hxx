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

#include <svx/sdr/overlay/overlayobject.hxx>
#include <basegfx/range/b2drange.hxx>

#include <memory>
#include <vector>

class SwView;

namespace sw::overlay
    {
        class OverlayRanges final : public sdr::overlay::OverlayObject
        {
        public:
            static std::unique_ptr<OverlayRanges> CreateOverlayRange(
                SwView const & rDocView,
                const Color& rColor,
                std::vector< basegfx::B2DRange >&& rRanges,
                const bool bShowSolidBorder );

            virtual ~OverlayRanges() override;

            // data read access
            const std::vector< basegfx::B2DRange >& getRanges() const
            {
                return maRanges;
            }

            // data write access
            void setRanges(std::vector< basegfx::B2DRange >&& rNew);

            void ShowSolidBorder();
            void HideSolidBorder();

        private:
            OverlayRanges(
                const Color& rColor,
                std::vector< basegfx::B2DRange >&& rRanges,
                const bool bShowSolidBorder );

            // geometry creation for OverlayObject
            virtual drawinglayer::primitive2d::Primitive2DContainer createOverlayObjectPrimitive2DSequence() override;

            // geometry of overlay
            std::vector< basegfx::B2DRange > maRanges;
            bool mbShowSolidBorder;
        };
} // end of namespace sw::overlay

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

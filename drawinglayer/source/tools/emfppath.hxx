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

#include "emfphelperdata.hxx"

namespace emfplushelper
{
    class EMFPPath : public EMFPObject
    {
        ::basegfx::B2DPolyPolygon    aPolygon;
        sal_uInt32                   nPoints;
        std::deque<float>            xPoints, yPoints;
        std::unique_ptr<sal_uInt8[]> pPointTypes;

    public:
        EMFPPath(sal_uInt32 _nPoints, bool bLines = false);

        virtual ~EMFPPath() override;

        void Read(SvStream& s, sal_uInt32 pathFlags);

        ::basegfx::B2DPolyPolygon& GetPolygon(EmfPlusHelperData const & rR, bool bMapIt = true, bool bAddLineToCloseShape = false);
        ::basegfx::B2DPolyPolygon& GetCardinalSpline(EmfPlusHelperData const& rR, float fTension,
                                                     sal_uInt32 aOffset, sal_uInt32 aNumSegments);
        ::basegfx::B2DPolyPolygon& GetClosedCardinalSpline(EmfPlusHelperData const& rR, float fTension);
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

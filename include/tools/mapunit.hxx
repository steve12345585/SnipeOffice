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

#include <sal/config.h>

#include <o3tl/unit_conversion.hxx>
#include <sal/types.h>

enum class MapUnit : sal_uInt8
{
    Map100thMM, Map10thMM, MapMM, MapCM,
    Map1000thInch, Map100thInch, Map10thInch, MapInch,
    MapPoint, MapTwip,
    MapPixel,
    MapSysFont, MapAppFont,
    MapRelative,
    LAST = MapRelative,
    LASTENUMDUMMY // used as an error return
};

constexpr o3tl::Length MapToO3tlLength(MapUnit eU, o3tl::Length ePixelValue = o3tl::Length::px)
{
    switch (eU)
    {
        case MapUnit::Map100thMM:
            return o3tl::Length::mm100;
        case MapUnit::Map10thMM:
            return o3tl::Length::mm10;
        case MapUnit::MapMM:
            return o3tl::Length::mm;
        case MapUnit::MapCM:
            return o3tl::Length::cm;
        case MapUnit::Map1000thInch:
            return o3tl::Length::in1000;
        case MapUnit::Map100thInch:
            return o3tl::Length::in100;
        case MapUnit::Map10thInch:
            return o3tl::Length::in10;
        case MapUnit::MapInch:
            return o3tl::Length::in;
        case MapUnit::MapPoint:
            return o3tl::Length::pt;
        case MapUnit::MapTwip:
            return o3tl::Length::twip;
        case MapUnit::MapPixel:
            return ePixelValue;
        default:
            return o3tl::Length::invalid;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

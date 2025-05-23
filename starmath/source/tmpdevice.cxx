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

#include <smmod.hxx>
#include <utility.hxx>

#include "tmpdevice.hxx"

#include <svtools/colorcfg.hxx>
#include <sal/log.hxx>

// SmTmpDevice
// Allows for font and color changes. The original settings will be restored
// in the destructor.
// It's main purpose is to allow for the "const" in the 'OutputDevice'
// argument in the 'Arrange' functions and restore changes made in the 'Draw'
// functions.
// Usually a MapMode of 1/100th mm will be used.

SmTmpDevice::SmTmpDevice(OutputDevice &rTheDev, bool bUseMap100th_mm) :
    rOutDev(rTheDev)
{
    rOutDev.Push(vcl::PushFlags::FONT | vcl::PushFlags::MAPMODE |
                 vcl::PushFlags::LINECOLOR | vcl::PushFlags::FILLCOLOR | vcl::PushFlags::TEXTCOLOR);
    if (bUseMap100th_mm  &&  SmMapUnit() != rOutDev.GetMapMode().GetMapUnit())
    {
        SAL_WARN("starmath", "incorrect MapMode?");
        rOutDev.SetMapMode(MapMode(SmMapUnit())); // format for 100% always
    }
}


Color SmTmpDevice::GetTextColor(const Color& rTextColor)
{
    if (rTextColor == COL_AUTO)
    {
        auto& config = SmModule::get()->GetColorConfig();
        Color aConfigFontColor = config.GetColorValue(svtools::FONTCOLOR).nColor;
        Color aConfigDocColor = config.GetColorValue(svtools::DOCCOLOR).nColor;
        return rOutDev.GetReadableFontColor(aConfigFontColor, aConfigDocColor);
    }

    return rTextColor;
}


void SmTmpDevice::SetFont(const vcl::Font &rNewFont)
{
    rOutDev.SetFont(rNewFont);
    rOutDev.SetTextColor(GetTextColor(rNewFont.GetColor()));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

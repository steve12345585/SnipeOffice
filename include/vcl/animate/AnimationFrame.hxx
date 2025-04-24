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

#include <vcl/bitmapex.hxx>

enum class Disposal
{
    Not,
    Back,
    Previous
};

enum class Blend
{
    Source,
    Over
};

struct AnimationFrame
{
    BitmapEx maBitmapEx;
    Point maPositionPixel;
    Size maSizePixel;
    tools::Long mnWait;
    Disposal meDisposal;
    Blend meBlend;
    bool mbUserInput;

    AnimationFrame()
        : mnWait(0)
        , meDisposal(Disposal::Not)
        , meBlend(Blend::Over)
        , mbUserInput(false)
    {
    }

    AnimationFrame(const BitmapEx& rBitmapEx, const Point& rPositionPixel, const Size& rSizePixel,
                   tools::Long nWait = 0, Disposal eDisposal = Disposal::Not,
                   Blend eBlend = Blend::Over)
        : maBitmapEx(rBitmapEx)
        , maPositionPixel(rPositionPixel)
        , maSizePixel(rSizePixel)
        , mnWait(nWait)
        , meDisposal(eDisposal)
        , meBlend(eBlend)
        , mbUserInput(false)
    {
    }

    bool operator==(const AnimationFrame& rAnimationFrame) const
    {
        return (rAnimationFrame.maBitmapEx == maBitmapEx
                && rAnimationFrame.maPositionPixel == maPositionPixel
                && rAnimationFrame.maSizePixel == maSizePixel && rAnimationFrame.mnWait == mnWait
                && rAnimationFrame.meDisposal == meDisposal && rAnimationFrame.meBlend == meBlend
                && rAnimationFrame.mbUserInput == mbUserInput);
    }

    bool operator!=(const AnimationFrame& rAnimationFrame) const
    {
        return !(*this == rAnimationFrame);
    }

    BitmapChecksum GetChecksum() const;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

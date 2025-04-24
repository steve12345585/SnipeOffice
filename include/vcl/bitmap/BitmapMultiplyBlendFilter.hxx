/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

#include <vcl/bitmapex.hxx>

class VCL_DLLPUBLIC BitmapMultiplyBlendFilter
{
private:
    BitmapEx maBitmapEx;
    BitmapEx maBitmapEx2;

public:
    BitmapMultiplyBlendFilter(BitmapEx const& rBmpEx, BitmapEx const& rBmpEx2);

    ~BitmapMultiplyBlendFilter();
    BitmapEx execute();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

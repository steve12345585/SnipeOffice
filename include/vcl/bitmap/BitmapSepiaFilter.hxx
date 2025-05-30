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

#include <vcl/bitmap/BitmapFilter.hxx>

class VCL_DLLPUBLIC BitmapSepiaFilter final : public BitmapFilter
{
public:
    BitmapSepiaFilter(double nSepiaPercent)
    {
        // clamp value to 100%
        if (nSepiaPercent <= 100)
            mnSepiaPercent = nSepiaPercent;
        else
            mnSepiaPercent = 100;
    }

    virtual BitmapEx execute(BitmapEx const& rBitmapEx) const override;

private:
    sal_uInt16 mnSepiaPercent;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

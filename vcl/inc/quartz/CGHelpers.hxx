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

#include <premac.h>
#include <CoreGraphics/CoreGraphics.h>
#include <postmac.h>

#include <quartz/utils.h>

class CGLayerHolder
{
private:
    CGLayerRef mpLayer;

    // Layer's scaling factor
    float mfScale;

public:
    CGLayerHolder()
        : mpLayer(nullptr)
        , mfScale(1.0)
    {
    }

    CGLayerHolder(CGLayerRef pLayer, float fScale = 1.0)
        : mpLayer(pLayer)
        , mfScale(fScale)
    {
    }

    // Just the size of the layer in pixels
    CGSize getSizePixels() const
    {
        CGSize aSize;
        if (mpLayer)
        {
            aSize = CGLayerGetSize(mpLayer);
        }
        return aSize;
    }

    // Size in points is size in pixels divided by the scaling factor
    CGSize getSizePoints() const
    {
        CGSize aSize;
        if (mpLayer)
        {
            const CGSize aLayerSize = getSizePixels();
            aSize.width = aLayerSize.width / mfScale;
            aSize.height = aLayerSize.height / mfScale;
        }
        return aSize;
    }

    CGLayerRef get() const { return mpLayer; }

    bool isSet() const { return mpLayer != nullptr; }

    void set(CGLayerRef const& pLayer) { mpLayer = pLayer; }

    float getScale() const { return mfScale; }

    void setScale(float fScale) { mfScale = fScale; }
};

class CGContextHolder
{
private:
    CGContextRef mpContext;

public:
    CGContextHolder()
        : mpContext(nullptr)
    {
    }

    CGContextHolder(CGContextRef pContext)
        : mpContext(pContext)
    {
    }

    CGContextRef get() const { return mpContext; }

    bool isSet() const { return mpContext != nullptr; }

    void set(CGContextRef const& pContext) { mpContext = pContext; }

    void saveState() { CGContextSaveGState(mpContext); }

    void restoreState() { CGContextRestoreGState(mpContext); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

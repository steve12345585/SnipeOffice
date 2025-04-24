/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <win/salgdi.h>
#include <ControlCacheKey.hxx>

class ControlCacheKey;

// Base class for some functionality that OpenGL/Skia/GDI backends must each implement.
class WinSalGraphicsImplBase
{
public:
    virtual ~WinSalGraphicsImplBase() {}

    // If true is returned, the following functions are used for drawing controls.
    virtual bool UseRenderNativeControl() const { return false; }
    virtual bool TryRenderCachedNativeControl(const ControlCacheKey& /*rControlCacheKey*/,
                                              int /*nX*/, int /*nY*/)
    {
        abort();
    }
    virtual bool RenderAndCacheNativeControl(CompatibleDC& /*rWhite*/, CompatibleDC& /*rBlack*/,
                                             int /*nX*/, int /*nY*/,
                                             ControlCacheKey& /*aControlCacheKey*/)
    {
        abort();
    }

    virtual void ClearDevFontCache() {}

    virtual void Flush() {}

    // Implementation for WinSalGraphics::DrawTextLayout().
    // Returns true if handled, if false, then WinSalGraphics will handle it itself.
    virtual bool DrawTextLayout(const GenericSalLayout&) { return false; }

    virtual void ClearNativeControlCache() {}
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <view/SlsILayerPainter.hxx>

#include <vcl/vclptr.hxx>
#include <vcl/mapmod.hxx>

#include <memory>

namespace sd { class Window; }
namespace tools { class Rectangle; }
namespace vcl { class Region; }

class VirtualDevice;

namespace sd::slidesorter::view {

/** A simple wrapper around an OutputDevice that provides support for
    independent layers and buffering.
    Each layer may contain any number of painters.
*/
class LayeredDevice
    : public std::enable_shared_from_this<LayeredDevice>

{
public:
    explicit LayeredDevice (const VclPtr<sd::Window>& pTargetWindow);
    ~LayeredDevice ();

    void Invalidate (
        const ::tools::Rectangle& rInvalidationBox,
        const sal_Int32 nLayer);
    void InvalidateAllLayers (
        const ::tools::Rectangle& rInvalidationBox);
    void InvalidateAllLayers (
        const vcl::Region& rInvalidationRegion);

    void RegisterPainter (
        const SharedILayerPainter& rPainter,
        const sal_Int32 nLayer);

    void RemovePainter (
        const SharedILayerPainter& rPainter,
        const sal_Int32 nLayer);

    bool HandleMapModeChange();
    void Repaint (const vcl::Region& rRepaintRegion);

    void Resize();

    void Dispose();

private:
    VclPtr<sd::Window> mpTargetWindow;
    class LayerContainer;
    std::unique_ptr<LayerContainer> mpLayers;
    ScopedVclPtr<VirtualDevice> mpBackBuffer;
    MapMode maSavedMapMode;

    void RepaintRectangle (const ::tools::Rectangle& rRepaintRectangle);
};

} // end of namespace ::sd::slidesorter::view

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

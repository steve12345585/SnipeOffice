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

#include <svx/sdr/contact/displayinfo.hxx>


namespace sdr::contact
{
        DisplayInfo::DisplayInfo()
        :   maProcessLayers(true), // init layer info with all bits set to draw everything on default
            mbControlLayerProcessingActive(false),
            mbGhostedDrawModeActive(false),
            mbSubContentActive(false)
        {
        }

        // Access to LayerInfos (which layers to process)
        void DisplayInfo::SetProcessLayers(const SdrLayerIDSet& rSet)
        {
            maProcessLayers = rSet;
        }

        // access to RedrawArea
        void DisplayInfo::SetRedrawArea(const vcl::Region& rRegion)
        {
            maRedrawArea = rRegion;
        }

        void DisplayInfo::SetWriterPageFrame(basegfx::B2IRectangle const& rPageFrame)
        {
            m_WriterPageFrame = rPageFrame;
        }

        void DisplayInfo::SetControlLayerProcessingActive(bool bDoProcess)
        {
            if(mbControlLayerProcessingActive != bDoProcess)
            {
                mbControlLayerProcessingActive = bDoProcess;
            }
        }

        void DisplayInfo::ClearGhostedDrawMode()
        {
            mbGhostedDrawModeActive = false;
        }

        void DisplayInfo::SetGhostedDrawMode()
        {
            mbGhostedDrawModeActive = true;
        }

        void DisplayInfo::SetSubContentActive(bool bNew)
        {
            if(mbSubContentActive != bNew)
            {
                mbSubContentActive = bNew;
            }
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

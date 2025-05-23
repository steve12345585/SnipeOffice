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

#include "mtftools.hxx"
#include <tools/stream.hxx>

// predefines
struct WmfExternal;

namespace emfio
{
    class WmfReader : public MtfTools
    {
    private:
        sal_uInt16      mnUnitsPerInch;
        sal_uInt32      mnRecSize;
        bool            mbPlaceable;

        // embedded EMF data
        std::optional<std::vector<sal_uInt8>> mpEMFStream;

        // total number of comment records containing EMF data
        sal_uInt32      mnEMFRecCount;

        // number of EMF records read
        sal_uInt32      mnEMFRec;

        // total size of embedded EMF data
        sal_uInt32      mnEMFSize;

        sal_uInt32      mnSkipActions;

        // eventually handed over external header
        const WmfExternal* mpExternalHeader;

        bool mbEnableEMFPlus = true;

        // reads header of the WMF-Datei
        bool            ReadHeader();

        // reads parameters of the record with the functionnumber nFunction.
        void            ReadRecordParams(sal_uInt32 nRecordSize, sal_uInt16 nFunction);

        Point           ReadPoint();                // reads and converts a point (first X then Y)
        Point           ReadYX();                   // reads and converts a point (first Y then X)
        tools::Rectangle       ReadRectangle();            // reads and converts a rectangle
        Size            ReadYXExt();
        void            GetPlaceableBound(tools::Rectangle& rSize, SvStream* pStrm);

    public:
        WmfReader(SvStream& rStreamWMF, GDIMetaFile& rGDIMetaFile, const WmfExternal* pExternalHeader);

        // read WMF file from stream and fill the GDIMetaFile
        void ReadWMF();

        // Allows disabling EMF+ if EMF is embedded in this WMF.
        void SetEnableEMFPlus(bool bEnableEMFPlus) { mbEnableEMFPlus = bEnableEMFPlus; }
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

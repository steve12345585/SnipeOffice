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

#include "cgm.hxx"
#include <vcl/bitmapex.hxx>
#include <vector>

class CGM;

class CGMBitmapDescriptor
{
    public:
        sal_uInt8*              mpBuf;
        sal_uInt8*              mpEndBuf;
        BitmapEx                mxBitmap;
        bool                    mbStatus;
        bool                    mbVMirror;
        sal_uInt32              mnDstBitsPerPixel;
        sal_uInt32              mnScanSize;         // bytes per line
        FloatPoint              mnP, mnQ, mnR;

        FloatPoint              mnOrigin;
        double                  mndx, mndy;
        double                  mnOrientation;

        sal_uInt32              mnX, mnY;
        tools::Long             mnLocalColorPrecision;
        sal_uInt32              mnCompressionMode;

        CGMBitmapDescriptor()
            : mpBuf(nullptr)
            , mpEndBuf(nullptr)
            , mbStatus(false)
            , mbVMirror(false)
            , mnDstBitsPerPixel(0)
            , mnScanSize(0)
            , mndx(0.0)
            , mndy(0.0)
            , mnOrientation(0.0)
            , mnX(0)
            , mnY(0)
            , mnLocalColorPrecision(0)
            , mnCompressionMode(0)
            { };
};

class CGMBitmap
{
    CGM*                    mpCGM;
    std::unique_ptr<CGMBitmapDescriptor>
                            pCGMBitmapDescriptor;
    bool                    ImplGetDimensions( CGMBitmapDescriptor& );
    std::vector<Color>      ImplGeneratePalette( CGMBitmapDescriptor const & );
    void                    ImplGetBitmap( CGMBitmapDescriptor& );
    void                    ImplInsert( CGMBitmapDescriptor const & rSource, CGMBitmapDescriptor& rDest );
public:
    explicit CGMBitmap( CGM& rCGM );
    ~CGMBitmap();
    CGMBitmapDescriptor*    GetBitmap() { return pCGMBitmapDescriptor.get();}
    std::unique_ptr<CGMBitmap> GetNext();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

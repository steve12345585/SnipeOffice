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

#include <sal/config.h>

#include <string_view>

#include "dxfreprd.hxx"
#include <vcl/font.hxx>
#include <vcl/lineinfo.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/virdev.hxx>

class DXF2GDIMetaFile {
private:

    VclPtr<VirtualDevice> pVirDev;
    const DXFRepresentation * pDXF;
    bool bStatus;

    sal_uInt16 OptPointsPerCircle;

    sal_uInt16 nMinPercent;
    sal_uInt16 nMaxPercent;
    sal_uInt16 nLastPercent;
    sal_uInt16 nMainEntitiesCount;

    tools::Long        nBlockColor;
    DXFLineInfo aBlockDXFLineInfo;
    tools::Long        nParentLayerColor;
    DXFLineInfo aParentLayerDXFLineInfo;
    Color       aActLineColor;
    Color       aActFillColor;
    vcl::Font   aActFont;
    const LineInfo aDefaultLineInfo; // to share between lines to reduce memory

    static sal_uInt64 CountEntities(const DXFEntities & rEntities);

    Color ConvertColor(sal_uInt8 nColor) const;

    tools::Long GetEntityColor(const DXFBasicEntity & rE) const;

    DXFLineInfo LTypeToDXFLineInfo(std::string_view rLineType) const;

    DXFLineInfo GetEntityDXFLineInfo(const DXFBasicEntity & rE);

    bool SetLineAttribute(const DXFBasicEntity & rE);

    bool SetAreaAttribute(const DXFBasicEntity & rE);

    bool SetFontAttribute(const DXFBasicEntity & rE, short nAngle,
                          sal_uInt16 nHeight);

    void DrawLineEntity(const DXFLineEntity & rE, const DXFTransform & rTransform);

    void DrawPointEntity(const DXFPointEntity & rE, const DXFTransform & rTransform);

    void DrawCircleEntity(const DXFCircleEntity & rE, const DXFTransform & rTransform);

    void DrawArcEntity(const DXFArcEntity & rE, const DXFTransform & rTransform);

    void DrawTraceEntity(const DXFTraceEntity & rE, const DXFTransform & rTransform);

    void DrawSolidEntity(const DXFSolidEntity & rE, const DXFTransform & rTransform);

    void DrawTextEntity(const DXFTextEntity & rE, const DXFTransform & rTransform);

    void DrawInsertEntity(const DXFInsertEntity & rE, const DXFTransform & rTransform);

    void DrawAttribEntity(const DXFAttribEntity & rE, const DXFTransform & rTransform);

    void DrawPolyLineEntity(const DXFPolyLineEntity & rE, const DXFTransform & rTransform);

    void Draw3DFaceEntity(const DXF3DFaceEntity & rE, const DXFTransform & rTransform);

    void DrawDimensionEntity(const DXFDimensionEntity & rE, const DXFTransform & rTransform);

    void DrawLWPolyLineEntity( const DXFLWPolyLineEntity & rE, const DXFTransform & rTransform );

    void DrawHatchEntity( const DXFHatchEntity & rE, const DXFTransform & rTransform );

    void DrawEntities(const DXFEntities & rEntities,
                      const DXFTransform & rTransform);

    void DrawLine(const Point& rA, const Point& rB);

public:

    DXF2GDIMetaFile();
    ~DXF2GDIMetaFile();

    bool Convert( const DXFRepresentation & rDXF, GDIMetaFile & rMTF, sal_uInt16 nMinPercent, sal_uInt16 nMaxPercent);

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

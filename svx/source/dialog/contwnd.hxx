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

#include <tools/poly.hxx>
#include <svx/graphctl.hxx>

class ContourWindow final : public GraphCtrl
{
    tools::PolyPolygon         aPolyPoly;
    Color               aPipetteColor;
    tools::Rectangle           aWorkRect;
    Link<ContourWindow&,void>  aPipetteLink;
    Link<ContourWindow&,void>  aPipetteClickLink;
    Link<ContourWindow&,void>  aWorkplaceClickLink;
    bool                bPipetteMode;
    bool                bWorkplaceMode;
    bool                bClickValid;

    virtual bool        MouseButtonDown(const MouseEvent& rMEvt) override;
    virtual bool        MouseMove(const MouseEvent& rMEvt) override;
    virtual bool        MouseButtonUp(const MouseEvent& rMEvt) override;
    virtual void        SdrObjCreated( const SdrObject& rObj ) override;
    virtual void        InitSdrModel() override;
    virtual void        Paint( vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect ) override;
    virtual void        SetDrawingArea(weld::DrawingArea* pDrawingArea) override;

public:

    ContourWindow(weld::Dialog* pDialog);

    void                SetPolyPolygon( const tools::PolyPolygon& rPolyPoly );
    const tools::PolyPolygon&  GetPolyPolygon();

    void                SetPipetteMode( const bool bPipette ) { bPipetteMode = bPipette; }
    const Color&        GetPipetteColor() const { return aPipetteColor; }

    bool                IsClickValid() const { return bClickValid; }
    bool                IsContourChanged() const;

    void                SetWorkplaceMode( const bool bWorkplace ) { bWorkplaceMode = bWorkplace; }
    const tools::Rectangle&    GetWorkRect() const { return aWorkRect; }

    void                SetPipetteHdl( const Link<ContourWindow&,void>& rLink ) { aPipetteLink = rLink; }
    void                SetPipetteClickHdl( const Link<ContourWindow&,void>& rLink ) { aPipetteClickLink = rLink; }
    void                SetWorkplaceClickHdl( const Link<ContourWindow&,void>& rLink ) { aWorkplaceClickLink = rLink; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

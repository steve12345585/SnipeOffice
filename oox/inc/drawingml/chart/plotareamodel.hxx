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

#ifndef INCLUDED_OOX_DRAWINGML_CHART_PLOTAREAMODEL_HXX
#define INCLUDED_OOX_DRAWINGML_CHART_PLOTAREAMODEL_HXX

#include <oox/drawingml/shape.hxx>
#include <drawingml/chart/axismodel.hxx>
#include <drawingml/chart/seriesmodel.hxx>
#include <drawingml/chart/typegroupmodel.hxx>
#include <drawingml/chart/datatablemodel.hxx>

namespace oox::drawingml::chart {

struct View3DModel
{
    std::optional< sal_Int32 > monHeightPercent; /// Height of the 3D view, relative to chart width.
    std::optional< sal_Int32 > monRotationX;     /// Horizontal rotation in degrees.
    std::optional< sal_Int32 > monRotationY;     /// Vertical rotation in degrees.
    sal_Int32           mnDepthPercent;     /// Depth of the 3D view, relative to chart width.
    sal_Int32           mnPerspective;      /// Eye distance to the 3D objects.
    bool                mbRightAngled;      /// True = right-angled axes in 3D view.

    explicit            View3DModel(bool bMSO2007Doc);
};

struct WallFloorModel
{
    typedef ModelRef< Shape >               ShapeRef;
    typedef ModelRef< PictureOptionsModel > PictureOptionsRef;

    ShapeRef            mxShapeProp;        /// Wall/floor frame formatting.
    PictureOptionsRef   mxPicOptions;       /// Fill bitmap settings.

    explicit            WallFloorModel();
                        ~WallFloorModel();
};

struct PlotAreaModel
{
    typedef ModelVector< TypeGroupModel >   TypeGroupVector;
    typedef ModelVector< AxisModel >        AxisVector;
    typedef ModelRef< Shape >               ShapeRef;
    typedef ModelRef< LayoutModel >         LayoutRef;
    typedef ModelRef< DataTableModel >         DataTableRef;

    TypeGroupVector     maTypeGroups;       /// All chart type groups contained in the chart.
    AxisVector          maAxes;             /// All axes contained in the chart.
    ShapeRef            mxShapeProp;        /// Plot area frame formatting.
    LayoutRef           mxLayout;           /// Layout/position of the plot area.
    DataTableRef        mxDataTable;        /// Data table of the plot area.

    explicit            PlotAreaModel();
                        ~PlotAreaModel();
};

} // namespace oox::drawingml::chart

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

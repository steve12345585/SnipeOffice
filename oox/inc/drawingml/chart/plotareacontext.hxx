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

#ifndef INCLUDED_OOX_DRAWINGML_CHART_PLOTAREACONTEXT_HXX
#define INCLUDED_OOX_DRAWINGML_CHART_PLOTAREACONTEXT_HXX

#include <drawingml/chart/chartcontextbase.hxx>

namespace oox::drawingml::chart {


struct View3DModel;

/** Handler for a chart plot area context (c:plotArea element).
 */
class View3DContext final : public ContextBase< View3DModel >
{
public:
    explicit            View3DContext( ::oox::core::ContextHandler2Helper& rParent, View3DModel& rModel );
    virtual             ~View3DContext() override;

    virtual ::oox::core::ContextHandlerRef onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;
};


struct WallFloorModel;

/** Handler for a chart wall/floor context (c:backWall, c:floor, c:sideWall
    elements).
 */
class WallFloorContext final : public ContextBase< WallFloorModel >
{
public:
    explicit            WallFloorContext( ::oox::core::ContextHandler2Helper& rParent, WallFloorModel& rModel );
    virtual             ~WallFloorContext() override;

    virtual ::oox::core::ContextHandlerRef onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;
};

struct PlotAreaModel;

/** Handler for a chart plot area context (c:plotArea element).
 */
class PlotAreaContext final : public ContextBase< PlotAreaModel >
{
public:
    explicit            PlotAreaContext( ::oox::core::ContextHandler2Helper& rParent, PlotAreaModel& rModel );
    virtual             ~PlotAreaContext() override;

    virtual ::oox::core::ContextHandlerRef onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;
};


} // namespace oox::drawingml::chart

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <vcl/weld.hxx>

namespace chart { class ControllerLockHelper; }

namespace chart
{
class ChartModel;

class ThreeD_SceneAppearance_TabPage
{
public:
    ThreeD_SceneAppearance_TabPage(
        weld::Container* pParent,
        rtl::Reference<::chart::ChartModel> xChartModel,
        ControllerLockHelper & rControllerLockHelper );
    void ActivatePage();
    ~ThreeD_SceneAppearance_TabPage();

private:
    DECL_LINK( SelectSchemeHdl, weld::ComboBox&, void );
    DECL_LINK( SelectShading, weld::Toggleable&, void );
    DECL_LINK( SelectRoundedEdgeOrObjectLines, weld::Toggleable&, void );

    void initControlsFromModel();
    void applyShadeModeToModel();
    void applyRoundedEdgeAndObjectLinesToModel();
    void updateScheme();

private:
    rtl::Reference<::chart::ChartModel> m_xChartModel;

    bool            m_bUpdateOtherControls;
    bool            m_bCommitToModel;
    OUString        m_aCustom;

    ControllerLockHelper& m_rControllerLockHelper;

    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xContainer;
    std::unique_ptr<weld::ComboBox> m_xLB_Scheme;
    std::unique_ptr<weld::CheckButton> m_xCB_Shading;
    std::unique_ptr<weld::CheckButton> m_xCB_ObjectLines;
    std::unique_ptr<weld::CheckButton> m_xCB_RoundedEdge;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

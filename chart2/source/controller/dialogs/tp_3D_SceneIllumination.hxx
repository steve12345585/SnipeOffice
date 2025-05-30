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

#include <ModifyListenerCallBack.hxx>
#include <TimerTriggeredControllerLock.hxx>
#include <vcl/weld.hxx>
#include <svx/dlgctl3d.hxx>
#include <svx/float3d.hxx>

namespace com::sun::star::beans
{
class XPropertySet;
}

class ColorListBox;

namespace chart
{
struct LightSourceInfo;
class ChartModel;

class ThreeD_SceneIllumination_TabPage
{
public:
    ThreeD_SceneIllumination_TabPage(weld::Container* pParent, weld::Window* pTopLevel,
                                     css::uno::Reference<css::beans::XPropertySet> xSceneProperties,
                                     const rtl::Reference<::chart::ChartModel>& xChartModel);
    ~ThreeD_SceneIllumination_TabPage();

private:
    DECL_LINK(ClickLightSourceButtonHdl, weld::Button&, void);
    DECL_LINK(SelectColorHdl, ColorListBox&, void);
    DECL_LINK(ColorDialogHdl, weld::Button&, void);
    DECL_LINK(PreviewChangeHdl, SvxLightCtl3D*, void);
    DECL_LINK(PreviewSelectHdl, SvxLightCtl3D*, void);

    void updatePreview();

private:
    DECL_LINK(fillControlsFromModel, void*, void);

    void applyLightSourceToModel(sal_uInt32 nLightNumber);
    void applyLightSourcesToModel();

    std::unique_ptr<LightSourceInfo[]> m_pLightSourceInfoList;

    css::uno::Reference<css::beans::XPropertySet> m_xSceneProperties;

    TimerTriggeredControllerLock m_aTimerTriggeredControllerLock;

    bool m_bInCommitToModel;

    ModifyListenerCallBack m_aModelChangeListener;
    rtl::Reference<::chart::ChartModel> m_xChartModel;

    weld::Window* m_pTopLevel;
    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xContainer;
    LightButton m_aBtn_Light1;
    LightButton m_aBtn_Light2;
    LightButton m_aBtn_Light3;
    LightButton m_aBtn_Light4;
    LightButton m_aBtn_Light5;
    LightButton m_aBtn_Light6;
    LightButton m_aBtn_Light7;
    LightButton m_aBtn_Light8;
    std::unique_ptr<ColorListBox> m_xLB_LightSource;
    std::unique_ptr<weld::Button> m_xBtn_LightSource_Color;
    std::unique_ptr<ColorListBox> m_xLB_AmbientLight;
    std::unique_ptr<weld::Button> m_xBtn_AmbientLight_Color;
    std::unique_ptr<weld::Scale> m_xHoriScale;
    std::unique_ptr<weld::Scale> m_xVertScale;
    std::unique_ptr<weld::Button> m_xBtn_Corner;
    std::unique_ptr<Svx3DLightControl> m_xPreview;
    std::unique_ptr<weld::CustomWeld> m_xPreviewWnd;
    std::unique_ptr<SvxLightCtl3D> m_xCtl_Preview;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#ifndef INCLUDED_SVX_SOURCE_SIDEBAR_AREA_AREATRANSPARENCYGRADIENTPOPUP_HXX
#define INCLUDED_SVX_SOURCE_SIDEBAR_AREA_AREATRANSPARENCYGRADIENTPOPUP_HXX

#include <basegfx/utils/bgradient.hxx>
#include <vcl/weld.hxx>
#include <svtools/toolbarmenu.hxx>

class XFillFloatTransparenceItem;

namespace svx::sidebar
{
class AreaPropertyPanelBase;

class AreaTransparencyGradientPopup final : public WeldToolbarPopup
{
private:
    AreaPropertyPanelBase& mrAreaPropertyPanel;
    std::unique_ptr<weld::Widget> mxCenterGrid;
    std::unique_ptr<weld::Widget> mxAngleGrid;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrCenterX;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrCenterY;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrAngle;
    std::unique_ptr<weld::Toolbar> mxBtnLeft45;
    std::unique_ptr<weld::Toolbar> mxBtnRight45;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrStartValue;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrEndValue;
    std::unique_ptr<weld::MetricSpinButton> mxMtrTrgrBorder;

    // MCGR: Preserve ColorStops until we have a UI to edit these
    basegfx::BColorStops maColorStops;

    void InitStatus(XFillFloatTransparenceItem const* pGradientItem);
    void ExecuteValueModify();
    DECL_LINK(ModifiedTrgrHdl_Impl, weld::MetricSpinButton&, void);
    DECL_LINK(Left_Click45_Impl, const OUString&, void);
    DECL_LINK(Right_Click45_Impl, const OUString&, void);

public:
    AreaTransparencyGradientPopup(const css::uno::Reference<css::frame::XFrame>& rFrame,
                                  AreaPropertyPanelBase& rPanel, weld::Widget* pParent);
    ~AreaTransparencyGradientPopup();

    void Rearrange(XFillFloatTransparenceItem const* pItem);
    virtual void GrabFocus() override;
};

} // end of namespace svx::sidebar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

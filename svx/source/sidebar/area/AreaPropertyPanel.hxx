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
#ifndef INCLUDED_SVX_SOURCE_SIDEBAR_AREA_AREAPROPERTYPANEL_HXX
#define INCLUDED_SVX_SOURCE_SIDEBAR_AREA_AREAPROPERTYPANEL_HXX

#include <sfx2/sidebar/ControllerItem.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xflclit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xbtmpit.hxx>

#include <svx/sidebar/AreaPropertyPanelBase.hxx>

class XFillFloatTransparenceItem;
class XFillTransparenceItem;
class XFillUseSlideBackgroundItem;
class XFillStyleItem;
class XFillGradientItem;
class XFillColorItem;
class XFillHatchItem;
class XFillBitmapItem;

namespace svx::sidebar {

class AreaTransparencyGradientControl;

class AreaPropertyPanel : public AreaPropertyPanelBase
{
public:
    static std::unique_ptr<PanelLayout> Create(
        weld::Widget* pParent,
        const css::uno::Reference<css::frame::XFrame>& rxFrame,
        SfxBindings* pBindings);

    SfxBindings* GetBindings() { return mpBindings;}

    // constructor/destructor
    AreaPropertyPanel(
        weld::Widget* pParent,
        const css::uno::Reference<css::frame::XFrame>& rxFrame,
        SfxBindings* pBindings);

    virtual ~AreaPropertyPanel() override;

    virtual void setFillTransparence(const XFillTransparenceItem& rItem) override;
    virtual void setFillUseBackground(const XFillStyleItem* pStyleItem, const XFillUseSlideBackgroundItem& rItem) override;
    virtual void setFillFloatTransparence(const XFillFloatTransparenceItem& rItem) override;
    virtual void setFillStyle(const XFillStyleItem& rItem) override;
    virtual void setFillStyleAndColor(const XFillStyleItem* pStyleItem, const XFillColorItem& aColorItem) override;
    virtual void setFillStyleAndGradient(const XFillStyleItem* pStyleItem, const XFillGradientItem& aGradientItem) override;
    virtual void setFillStyleAndHatch(const XFillStyleItem* pStyleItem, const XFillHatchItem& aHatchItem) override;
    virtual void setFillStyleAndBitmap(const XFillStyleItem* pStyleItem, const XFillBitmapItem& aHatchItem) override;

private:
    ::sfx2::sidebar::ControllerItem maStyleControl;
    ::sfx2::sidebar::ControllerItem maColorControl;
    ::sfx2::sidebar::ControllerItem maGradientControl;
    ::sfx2::sidebar::ControllerItem maHatchControl;
    ::sfx2::sidebar::ControllerItem maBitmapControl;
    ::sfx2::sidebar::ControllerItem maGradientListControl;
    ::sfx2::sidebar::ControllerItem maHatchListControl;
    ::sfx2::sidebar::ControllerItem maBitmapListControl;
    ::sfx2::sidebar::ControllerItem maPatternListControl;
    ::sfx2::sidebar::ControllerItem maFillTransparenceController;
    ::sfx2::sidebar::ControllerItem maFillFloatTransparenceController;
    ::sfx2::sidebar::ControllerItem maFillUseSlideBackgroundController;

    SfxBindings* mpBindings;
    virtual void HandleContextChange(const vcl::EnumContext& rContext) override;

};


} // end of namespace svx::sidebar


#endif // INCLUDED_SVX_SOURCE_SIDEBAR_AREA_AREAPROPERTYPANEL_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

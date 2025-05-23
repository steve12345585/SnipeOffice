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

#include <tools/solar.h>
#include <vcl/sysdata.hxx>
#include <salobj.hxx>
#include <unx/gtk/gtkframe.hxx>

class GtkSalObjectBase : public SalObject
{
protected:
    SystemEnvData       m_aSystemData;
    GtkWidget*          m_pSocket;
    GtkSalFrame*        m_pParent;
    cairo_region_t*     m_pRegion;

    void Init();

public:
    GtkSalObjectBase(GtkSalFrame* pParent);
    virtual ~GtkSalObjectBase() override;

    virtual void                    BeginSetClipRegion( sal_uInt32 nRects ) override;
    virtual void                    UnionClipRegion( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight ) override;

    virtual void                    SetForwardKey( bool bEnable ) override;

    virtual const SystemEnvData&    GetSystemData() const override;

    virtual Size                    GetOptimalSize() const override;

private:
    // signals
#if !GTK_CHECK_VERSION(4, 0, 0)
    static gboolean     signalButton( GtkWidget*, GdkEventButton*, gpointer );
    static gboolean     signalFocus( GtkWidget*, GdkEventFocus*, gpointer );
#endif
};

// this attempts to clip the hosted native window using gdk_window_shape_combine_region
class GtkSalObject final : public GtkSalObjectBase
{
    // signals
    static void         signalDestroy( GtkWidget*, gpointer );

public:
    GtkSalObject(GtkSalFrame* pParent, bool bShow);
    virtual ~GtkSalObject() override;

    // override all pure virtual methods
    virtual void                    ResetClipRegion() override;
    virtual void                    EndSetClipRegion() override;

    virtual void                    SetPosSize( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight ) override;
    virtual void                    Show( bool bVisible ) override;
    virtual void                    Reparent(SalFrame* pFrame) override;
};

// this attempts to clip the hosted native GtkWidget by using a GtkScrolledWindow as a viewport
// only a rectangular area is going to work
class GtkSalObjectWidgetClip final : public GtkSalObjectBase
{
    tools::Rectangle m_aRect;
    tools::Rectangle m_aClipRect;
    GtkWidget* m_pScrolledWindow;
    GtkWidget* m_pViewPort;
    GtkCssProvider* m_pBgCssProvider;

    // signals
#if !GTK_CHECK_VERSION(4, 0, 0)
    static gboolean     signalScroll(GtkWidget*, GdkEvent*, gpointer);
#else
    static gboolean     signalScroll(GtkEventControllerScroll* pController, double delta_x, double delta_y, gpointer object);
#endif
    static void         signalDestroy( GtkWidget*, gpointer );

#if !GTK_CHECK_VERSION(4, 0, 0)
    bool signal_scroll(GtkWidget* pScrolledWindow, GdkEvent* pEvent);
#else
    bool signal_scroll(GtkEventControllerScroll* pController, double delta_x, double delta_y);
#endif

    void ApplyClipRegion();

    void SetViewPortBackground();

    DECL_LINK(SettingsChangedHdl, VclWindowEvent&, void);

public:
    GtkSalObjectWidgetClip(GtkSalFrame* pParent, bool bShow);
    virtual ~GtkSalObjectWidgetClip() override;

    // override all pure virtual methods
    virtual void                    ResetClipRegion() override;
    virtual void                    EndSetClipRegion() override;

    virtual void                    SetPosSize( tools::Long nX, tools::Long nY, tools::Long nWidth, tools::Long nHeight ) override;
    virtual void                    Show( bool bVisible ) override;
    virtual void                    Reparent(SalFrame* pFrame) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

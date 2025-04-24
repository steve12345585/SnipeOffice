/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "QtInstanceWidget.hxx"

#include <QtWidgets/QScrollArea>

class QtInstanceScrolledWindow : public QtInstanceWidget, public virtual weld::ScrolledWindow
{
    Q_OBJECT

    QScrollArea* m_pScrollArea;

public:
    QtInstanceScrolledWindow(QScrollArea* pScrollArea);

public:
    virtual void move(weld::Widget* pWidget, weld::Container* pNewParent) override;
    virtual css::uno::Reference<css::awt::XWindow> CreateChildFrame() override;
    virtual void child_grab_focus() override;

    virtual void hadjustment_configure(int nValue, int nLower, int nUpper, int nStepIncrement,
                                       int nPageIncrement, int nPageSize) override;
    virtual int hadjustment_get_value() const override;
    virtual void hadjustment_set_value(int nValue) override;
    virtual int hadjustment_get_upper() const override;
    virtual void hadjustment_set_upper(int nUpper) override;
    virtual int hadjustment_get_page_size() const override;
    virtual void hadjustment_set_page_size(int nSize) override;
    virtual void hadjustment_set_page_increment(int nSize) override;
    virtual void hadjustment_set_step_increment(int nSize) override;
    virtual void set_hpolicy(VclPolicyType eHPolicy) override;
    virtual VclPolicyType get_hpolicy() const override;

    virtual void vadjustment_configure(int nValue, int nLower, int nUpper, int nStepIncrement,
                                       int nPageIncrement, int nPageSize) override;
    virtual int vadjustment_get_value() const override;
    virtual void vadjustment_set_value(int nValue) override;
    virtual int vadjustment_get_upper() const override;
    virtual void vadjustment_set_upper(int nUpper) override;
    virtual int vadjustment_get_page_size() const override;
    virtual void vadjustment_set_page_size(int nSize) override;
    virtual void vadjustment_set_page_increment(int nSize) override;
    virtual void vadjustment_set_step_increment(int nSize) override;
    virtual int vadjustment_get_lower() const override;
    virtual void vadjustment_set_lower(int nLower) override;
    virtual void set_vpolicy(VclPolicyType eVPolicy) override;
    virtual VclPolicyType get_vpolicy() const override;
    virtual int get_scroll_thickness() const override;
    virtual void set_scroll_thickness(int nThickness) override;

    virtual void customize_scrollbars(const Color& rBackgroundColor, const Color& rShadowColor,
                                      const Color& rFaceColor) override;

private:
    static Qt::ScrollBarPolicy toQtPolicy(VclPolicyType eVclPolicy);
    static VclPolicyType toVclPolicy(Qt::ScrollBarPolicy eQtPolicy);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

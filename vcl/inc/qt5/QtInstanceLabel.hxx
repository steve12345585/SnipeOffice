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

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

class QtInstanceLabel : public QtInstanceWidget, public virtual weld::Label
{
    Q_OBJECT

    QLabel* m_pLabel;

public:
    QtInstanceLabel(QLabel* pLabel);

    virtual void set_label(const OUString& rText) override;
    virtual OUString get_label() const override;
    virtual void set_mnemonic_widget(Widget* pTarget) override;
    virtual void set_font(const vcl::Font& rFont) override;
    virtual void set_label_type(weld::LabelType eType) override;
    virtual void set_font_color(const Color& rColor) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

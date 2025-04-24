/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <tools/gen.hxx>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

#include <optional>

class QtDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

    std::function<std::optional<QString>(double)> m_aFormatValueFunction;
    // see weld::SpinButton::parse_text for semantics
    std::function<TriState(const QString&, double*)> m_aParseTextFunction;

public:
    QtDoubleSpinBox(QWidget* pParent);

    // public override of the protected base class method
    QLineEdit* lineEdit() const;

    virtual QString textFromValue(double fValue) const override;
    virtual double valueFromText(const QString& rText) const override;
    virtual QValidator::State validate(QString& rInput, int& rPos) const override;

    void setFormatValueFunction(std::function<std::optional<QString>(double)> aFunction)
    {
        m_aFormatValueFunction = aFunction;
    }

    void setParseTextFunction(std::function<TriState(const QString&, double*)> aFunction)
    {
        m_aParseTextFunction = aFunction;
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

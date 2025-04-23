/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QtInstanceProgressBar.hxx>
#include <QtInstanceProgressBar.moc>

#include <vcl/qt/QtUtils.hxx>

QtInstanceProgressBar::QtInstanceProgressBar(QProgressBar* pProgressBar)
    : QtInstanceWidget(pProgressBar)
    , m_pProgressBar(pProgressBar)
{
    assert(pProgressBar);
}

void QtInstanceProgressBar::set_percentage(int nValue)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] { m_pProgressBar->setValue(nValue); });
}

OUString QtInstanceProgressBar::get_text() const
{
    SolarMutexGuard g;
    OUString sText;
    GetQtInstance().RunInMainThread([&] { sText = toOUString(m_pProgressBar->text()); });
    return sText;
}

void QtInstanceProgressBar::set_text(const OUString& rText)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] {
        m_pProgressBar->setFormat(toQString(rText));
        m_pProgressBar->setTextVisible(!rText.isEmpty());
    });
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

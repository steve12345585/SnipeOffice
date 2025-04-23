/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QtInstanceMenuButton.hxx>
#include <QtInstanceMenuButton.moc>

#include <QtTools.hxx>

#include <vcl/qt/QtUtils.hxx>

#include <QtWidgets/QMenu>

QtInstanceMenuButton::QtInstanceMenuButton(QToolButton* pButton)
    : QtInstanceToggleButton(pButton)
    , m_pToolButton(pButton)
    , m_pPopover(nullptr)
{
    assert(m_pToolButton);

    if (m_pToolButton->menu())
        connect(m_pToolButton->menu(), &QMenu::triggered, this,
                &QtInstanceMenuButton::handleMenuItemTriggered);

    connect(m_pToolButton, &QToolButton::clicked, this, &QtInstanceMenuButton::handleButtonClicked);
}

void QtInstanceMenuButton::insert_item(int nPos, const OUString& rId, const OUString& rStr,
                                       const OUString* pIconName, VirtualDevice* pImageSurface,
                                       TriState eCheckRadioFalse)
{
    SolarMutexGuard g;

    assert(eCheckRadioFalse == TRISTATE_INDET && "Param not handled yet");
    (void)eCheckRadioFalse;

    GetQtInstance().RunInMainThread([&] {
        QAction* pAction = new QAction(vclToQtStringWithAccelerator(rStr), &getMenu());
        pAction->setObjectName(toQString(rId));

        if (pIconName)
            pAction->setIcon(loadQPixmapIcon(*pIconName));
        else if (pImageSurface)
            pAction->setIcon(toQPixmap(*pImageSurface));

        insertAction(pAction, nPos);
    });
}

void QtInstanceMenuButton::insert_separator(int nPos, const OUString& rId)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QAction* pAction = new QAction(&getMenu());
        pAction->setSeparator(true);
        pAction->setObjectName(toQString(rId));

        insertAction(pAction, nPos);
    });
}

void QtInstanceMenuButton::remove_item(const OUString& rId)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rId))
            getMenu().removeAction(pAction);
    });
}

void QtInstanceMenuButton::clear()
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] { getMenu().clear(); });
}

void QtInstanceMenuButton::set_item_sensitive(const OUString& rIdent, bool bSensitive)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rIdent))
            pAction->setEnabled(bSensitive);
    });
}

void QtInstanceMenuButton::set_item_active(const OUString& rIdent, bool bActive)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rIdent))
            pAction->setChecked(bActive);
    });
}

void QtInstanceMenuButton::set_item_label(const OUString& rIdent, const OUString& rLabel)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rIdent))
            pAction->setText(toQString(rLabel));
    });
}

OUString QtInstanceMenuButton::get_item_label(const OUString& rIdent) const
{
    SolarMutexGuard g;

    OUString sLabel;
    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rIdent))
            sLabel = toOUString(pAction->text());
    });

    return sLabel;
}

void QtInstanceMenuButton::set_item_visible(const OUString& rIdent, bool bVisible)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (QAction* pAction = getAction(rIdent))
            pAction->setVisible(bVisible);
    });
}

void QtInstanceMenuButton::set_popover(weld::Widget* pPopover)
{
    QtInstanceWidget* pPopoverWidget = dynamic_cast<QtInstanceWidget*>(pPopover);
    m_pPopover = pPopoverWidget ? pPopoverWidget->getQWidget() : nullptr;
}

QMenu& QtInstanceMenuButton::getMenu() const
{
    QMenu* pMenu = m_pToolButton->menu();
    assert(pMenu);
    return *pMenu;
}

QAction* QtInstanceMenuButton::getAction(const OUString& rIdent) const
{
    const QList<QAction*> aActions = getMenu().actions();
    for (QAction* pAction : aActions)
    {
        if (pAction && pAction->objectName() == toQString(rIdent))
            return pAction;
    }

    return nullptr;
}

void QtInstanceMenuButton::insertAction(QAction* pAction, int nPos)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QAction* pNextAction = nullptr;
        QList<QAction*> pActions = getMenu().actions();
        if (nPos >= 0 && nPos < pActions.count())
            pNextAction = pActions.at(nPos);
        getMenu().insertAction(pNextAction, pAction);
    });
}

void QtInstanceMenuButton::handleButtonClicked()
{
    if (m_pPopover)
        m_pPopover->show();
    else
        m_pToolButton->showMenu();
}

void QtInstanceMenuButton::handleMenuItemTriggered(QAction* pAction)
{
    SolarMutexGuard g;

    assert(pAction);
    signal_selected(toOUString(pAction->objectName()));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

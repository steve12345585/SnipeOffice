/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QtInstanceWidget.hxx>
#include <QtInstanceWidget.moc>

#include <QtInstanceContainer.hxx>

#include <i18nlangtag/languagetag.hxx>
#include <vcl/event.hxx>
#include <vcl/transfer.hxx>
#include <vcl/qt/QtUtils.hxx>

#include <QtGui/QMouseEvent>

/** Name of QObject property used for the help ID. */
const char* const PROPERTY_HELP_ID = "help-id";

QtInstanceWidget::QtInstanceWidget(QWidget* pWidget)
    : m_pWidget(pWidget)
{
    assert(pWidget);

    connect(qApp, &QApplication::focusChanged, this, &QtInstanceWidget::applicationFocusChanged);
    pWidget->installEventFilter(this);
}

void QtInstanceWidget::connect_mouse_move(const Link<const MouseEvent&, bool>& rLink)
{
    getQWidget()->setMouseTracking(rLink.IsSet());

    weld::Widget::connect_mouse_move(rLink);
}

void QtInstanceWidget::set_sensitive(bool bSensitive)
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { set_sensitive(bSensitive); });
        return;
    }

    getQWidget()->setEnabled(bSensitive);
}

bool QtInstanceWidget::get_sensitive() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        bool bSensitive = false;
        rQtInstance.RunInMainThread([&] { bSensitive = get_sensitive(); });
        return bSensitive;
    }

    return getQWidget()->isEnabled();
}

bool QtInstanceWidget::get_visible() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        bool bVisible = false;
        rQtInstance.RunInMainThread([&] { bVisible = get_visible(); });
        return bVisible;
    }

    return getQWidget()->isVisible();
}

bool QtInstanceWidget::is_visible() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        bool bVisible = false;
        rQtInstance.RunInMainThread([&] { bVisible = is_visible(); });
        return bVisible;
    }

    QWidget* pTopLevel = getQWidget()->topLevelWidget();
    assert(pTopLevel);
    return getQWidget()->isVisibleTo(pTopLevel) && pTopLevel->isVisible();
}

void QtInstanceWidget::set_can_focus(bool bCanFocus)
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { set_can_focus(bCanFocus); });
        return;
    }

    if (bCanFocus)
        getQWidget()->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    else
        getQWidget()->setFocusPolicy(Qt::FocusPolicy::NoFocus);
}

void QtInstanceWidget::grab_focus()
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { grab_focus(); });
        return;
    }

    getQWidget()->setFocus();
}

bool QtInstanceWidget::has_focus() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        bool bFocus = false;
        rQtInstance.RunInMainThread([&] { bFocus = has_focus(); });
        return bFocus;
    }

    return getQWidget()->hasFocus();
}

bool QtInstanceWidget::is_active() const { return has_focus(); }

bool QtInstanceWidget::has_child_focus() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        bool bChildFocus = false;
        rQtInstance.RunInMainThread([&] { bChildFocus = has_child_focus(); });
        return bChildFocus;
    }

    QWidget* pFocusWidget = QApplication::focusWidget();
    if (!pFocusWidget)
        return false;

    QWidget* pParent = pFocusWidget->parentWidget();
    while (pParent)
    {
        if (pParent == getQWidget())
            return true;
        pParent = pParent->parentWidget();
    }
    return false;
}

void QtInstanceWidget::show()
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { show(); });
        return;
    }

    getQWidget()->show();
}

void QtInstanceWidget::hide()
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { hide(); });
        return;
    }

    getQWidget()->hide();
}

void QtInstanceWidget::set_size_request(int nWidth, int nHeight)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread(
        [&] { getQWidget()->setMinimumSize(std::max(0, nWidth), std::max(0, nHeight)); });
}

Size QtInstanceWidget::get_size_request() const
{
    SolarMutexGuard g;

    Size aSize;
    GetQtInstance().RunInMainThread([&] { aSize = toSize(getQWidget()->minimumSize()); });
    return aSize;
}

Size QtInstanceWidget::get_preferred_size() const
{
    SolarMutexGuard g;

    Size aPreferredSize;
    GetQtInstance().RunInMainThread([&] { aPreferredSize = toSize(getQWidget()->sizeHint()); });

    return aPreferredSize;
}

float QtInstanceWidget::get_approximate_digit_width() const
{
    SolarMutexGuard g;

    float fWidth = 0;
    GetQtInstance().RunInMainThread(
        [&] { fWidth = getQWidget()->fontMetrics().horizontalAdvance("0123456789") / 10.0; });
    return fWidth;
}

int QtInstanceWidget::get_text_height() const
{
    SolarMutexGuard g;

    int nHeight = 0;
    GetQtInstance().RunInMainThread([&] { nHeight = getQWidget()->fontMetrics().height(); });
    return nHeight;
}

Size QtInstanceWidget::get_pixel_size(const OUString& rText) const
{
    SolarMutexGuard g;

    Size aSize;
    GetQtInstance().RunInMainThread(
        [&] { aSize = toSize(getQWidget()->fontMetrics().boundingRect(toQString(rText)).size()); });

    return aSize;
}

vcl::Font QtInstanceWidget::get_font()
{
    SolarMutexGuard g;

    vcl::Font aFont;
    GetQtInstance().RunInMainThread([&] {
        const QFont& rWidgetFont = getQWidget()->font();
        const css::lang::Locale& rLocale
            = Application::GetSettings().GetUILanguageTag().getLocale();
        if (toVclFont(rWidgetFont, rLocale, aFont))
            return;

        aFont = Application::GetSettings().GetStyleSettings().GetAppFont();
    });

    return aFont;
}

OUString QtInstanceWidget::get_buildable_name() const { return OUString(); }

void QtInstanceWidget::set_buildable_name(const OUString&) {}

bool QtInstanceWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
    SolarMutexGuard g;
    assert(GetQtInstance().IsMainThread());

    if (pObject != getQWidget())
        return false;

    switch (pEvent->type())
    {
        case QEvent::KeyPress:
        {
            QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
            return signal_key_press(toVclKeyEvent(*pKeyEvent));
        }
        case QEvent::KeyRelease:
        {
            QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
            return signal_key_release(toVclKeyEvent(*pKeyEvent));
        }
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
            return signal_mouse_press(toVclMouseEvent(*pMouseEvent));
        }
        case QEvent::MouseButtonRelease:
        {
            QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
            return signal_mouse_release(toVclMouseEvent(*pMouseEvent));
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
            return signal_mouse_motion(toVclMouseEvent(*pMouseEvent));
        }
        default:
            return QObject::eventFilter(pObject, pEvent);
    }
}

void QtInstanceWidget::setHelpId(QWidget& rWidget, const OUString& rHelpId)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread(
        [&] { rWidget.setProperty(PROPERTY_HELP_ID, toQString(rHelpId)); });
}

void QtInstanceWidget::set_help_id(const OUString& rHelpId) { setHelpId(*getQWidget(), rHelpId); }

OUString QtInstanceWidget::get_help_id() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        OUString sHelpId;
        rQtInstance.RunInMainThread([&] { sHelpId = get_help_id(); });
        return sHelpId;
    }

    const QVariant aHelpIdVariant = getQWidget()->property(PROPERTY_HELP_ID);
    if (!aHelpIdVariant.isValid())
        return OUString();

    assert(aHelpIdVariant.canConvert<QString>());
    return toOUString(aHelpIdVariant.toString());
}

void QtInstanceWidget::set_hexpand(bool) { assert(false && "Not implemented yet"); }

bool QtInstanceWidget::get_hexpand() const
{
    assert(false && "Not implemented yet");
    return true;
}

void QtInstanceWidget::set_vexpand(bool) { assert(false && "Not implemented yet"); }

bool QtInstanceWidget::get_vexpand() const
{
    assert(false && "Not implemented yet");
    return true;
}

void QtInstanceWidget::set_margin_top(int nMargin)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QMargins aMargins = m_pWidget->contentsMargins();
        aMargins.setTop(nMargin);
        m_pWidget->setContentsMargins(aMargins);
    });
}

void QtInstanceWidget::set_margin_bottom(int nMargin)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QMargins aMargins = m_pWidget->contentsMargins();
        aMargins.setBottom(nMargin);
        m_pWidget->setContentsMargins(aMargins);
    });
}

void QtInstanceWidget::set_margin_start(int nMargin)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QMargins aMargins = m_pWidget->contentsMargins();
        if (m_pWidget->isRightToLeft())
            aMargins.setRight(nMargin);
        else
            aMargins.setLeft(nMargin);
        m_pWidget->setContentsMargins(aMargins);
    });
}

void QtInstanceWidget::set_margin_end(int nMargin)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QMargins aMargins = m_pWidget->contentsMargins();
        if (m_pWidget->isRightToLeft())
            aMargins.setLeft(nMargin);
        else
            aMargins.setRight(nMargin);
        m_pWidget->setContentsMargins(aMargins);
    });
}

int QtInstanceWidget::get_margin_top() const
{
    SolarMutexGuard g;

    int nMargin = 0;
    GetQtInstance().RunInMainThread([&] { nMargin = m_pWidget->contentsMargins().top(); });

    return nMargin;
}

int QtInstanceWidget::get_margin_bottom() const
{
    SolarMutexGuard g;

    int nMargin = 0;
    GetQtInstance().RunInMainThread([&] { nMargin = m_pWidget->contentsMargins().bottom(); });

    return nMargin;
}

int QtInstanceWidget::get_margin_start() const
{
    SolarMutexGuard g;

    int nMargin = 0;
    GetQtInstance().RunInMainThread([&] {
        if (m_pWidget->isRightToLeft())
            nMargin = m_pWidget->contentsMargins().right();
        else
            nMargin = m_pWidget->contentsMargins().left();
    });

    return nMargin;
}

int QtInstanceWidget::get_margin_end() const
{
    SolarMutexGuard g;

    int nMargin = 0;
    GetQtInstance().RunInMainThread([&] {
        if (m_pWidget->isRightToLeft())
            nMargin = m_pWidget->contentsMargins().left();
        else
            nMargin = m_pWidget->contentsMargins().right();
    });

    return nMargin;
}

void QtInstanceWidget::set_accessible_name(const OUString& rName)
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { set_accessible_name(rName); });
        return;
    }

    getQWidget()->setAccessibleName(toQString(rName));
}

void QtInstanceWidget::set_accessible_description(const OUString& rDescription)
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { set_accessible_description(rDescription); });
        return;
    }

    getQWidget()->setAccessibleDescription(toQString(rDescription));
}

OUString QtInstanceWidget::get_accessible_name() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        OUString sName;
        rQtInstance.RunInMainThread([&] { sName = get_accessible_name(); });
        return sName;
    }

    return toOUString(getQWidget()->accessibleName());
}

OUString QtInstanceWidget::get_accessible_description() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        OUString sDescription;
        rQtInstance.RunInMainThread([&] { sDescription = get_accessible_description(); });
        return sDescription;
    }

    return toOUString(getQWidget()->accessibleDescription());
}

OUString QtInstanceWidget::get_accessible_id() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        OUString sId;
        rQtInstance.RunInMainThread([&] { sId = get_accessible_id(); });
        return sId;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    return toOUString(getQWidget()->accessibleIdentifier());
#else
    return OUString();
#endif
}

void QtInstanceWidget::set_accessible_relation_labeled_by(weld::Widget*)
{
    assert(false && "Not implemented yet");
}

void QtInstanceWidget::set_tooltip_text(const OUString& rTip)
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        rQtInstance.RunInMainThread([&] { set_tooltip_text(rTip); });
        return;
    }

    getQWidget()->setToolTip(toQString(rTip));
}

OUString QtInstanceWidget::get_tooltip_text() const
{
    SolarMutexGuard g;
    QtInstance& rQtInstance = GetQtInstance();
    if (!rQtInstance.IsMainThread())
    {
        OUString sToolTipText;
        rQtInstance.RunInMainThread([&] { sToolTipText = get_tooltip_text(); });
        return sToolTipText;
    }

    return toOUString(getQWidget()->toolTip());
}

void QtInstanceWidget::set_cursor_data(void*) { assert(false && "Not implemented yet"); }

void QtInstanceWidget::grab_add() { assert(false && "Not implemented yet"); }

bool QtInstanceWidget::has_grab() const
{
    assert(false && "Not implemented yet");
    return false;
}

void QtInstanceWidget::grab_remove() { assert(false && "Not implemented yet"); }

bool QtInstanceWidget::get_extents_relative_to(const Widget& rRelative, int& rX, int& rY,
                                               int& rWidth, int& rHeight) const
{
    SolarMutexGuard g;

    bool bRet = false;
    GetQtInstance().RunInMainThread([&] {
        QRect aGeometry = getQWidget()->geometry();
        rWidth = aGeometry.width();
        rHeight = aGeometry.height();
        const QtInstanceWidget* pRelativeWidget = dynamic_cast<const QtInstanceWidget*>(&rRelative);
        if (!pRelativeWidget)
            return;

        QPoint aRelativePos = getQWidget()->mapTo(pRelativeWidget->getQWidget(), QPoint(0, 0));
        rX = aRelativePos.x();
        rY = aRelativePos.y();
        bRet = true;
    });

    return bRet;
}

bool QtInstanceWidget::get_direction() const
{
    SolarMutexGuard g;

    bool bRTL = false;
    GetQtInstance().RunInMainThread(
        [&] { bRTL = getQWidget()->layoutDirection() == Qt::LayoutDirection::RightToLeft; });
    return bRTL;
}

void QtInstanceWidget::set_direction(bool bRTL)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        getQWidget()->setLayoutDirection(bRTL ? Qt::LayoutDirection::RightToLeft
                                              : Qt::LayoutDirection::LeftToRight);
    });
}

void QtInstanceWidget::freeze(){};

void QtInstanceWidget::thaw(){};

void QtInstanceWidget::set_busy_cursor(bool bBusy)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        if (bBusy)
            ++m_nBusyCount;
        else
            --m_nBusyCount;
        assert(m_nBusyCount >= 0);

        if (m_nBusyCount == 1)
            getQWidget()->setCursor(Qt::BusyCursor);
        else if (m_nBusyCount == 0)
            getQWidget()->unsetCursor();
    });
}

std::unique_ptr<weld::Container> QtInstanceWidget::weld_parent() const
{
    QWidget* pParentWidget = getQWidget()->parentWidget();
    if (!pParentWidget)
        return nullptr;

    return std::make_unique<QtInstanceContainer>(pParentWidget);
}

void QtInstanceWidget::queue_resize()
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] { getQWidget()->adjustSize(); });
}

void QtInstanceWidget::help_hierarchy_foreach(const std::function<bool(const OUString&)>&)
{
    assert(false && "Not implemented yet");
}

OUString QtInstanceWidget::strip_mnemonic(const OUString& rLabel) const
{
    return rLabel.replaceFirst("&", "");
}

OUString QtInstanceWidget::escape_ui_str(const OUString& rLabel) const
{
    // preserve literal '&'
    return rLabel.replaceAll("&", "&&");
}

VclPtr<VirtualDevice> QtInstanceWidget::create_virtual_device() const
{
    VclPtr<VirtualDevice> xRet = VclPtr<VirtualDevice>::Create();
    xRet->SetBackground(COL_TRANSPARENT);
    return xRet;
}

css::uno::Reference<css::datatransfer::dnd::XDropTarget> QtInstanceWidget::get_drop_target()
{
    assert(false && "Not implemented yet");
    return nullptr;
}

css::uno::Reference<css::datatransfer::clipboard::XClipboard>
QtInstanceWidget::get_clipboard() const
{
    return GetSystemClipboard();
}

void QtInstanceWidget::connect_get_property_tree(const Link<tools::JsonWriter&, void>&)
{
    // not implemented for the Qt variant
}

void QtInstanceWidget::get_property_tree(tools::JsonWriter&)
{
    // not implemented for the Qt variant
}

void QtInstanceWidget::call_attention_to() { assert(false && "Not implemented yet"); }

void QtInstanceWidget::set_stack_background() { assert(false && "Not implemented yet"); }

void QtInstanceWidget::set_title_background() { assert(false && "Not implemented yet"); }

void QtInstanceWidget::set_toolbar_background() { assert(false && "Not implemented yet"); }

void QtInstanceWidget::set_highlight_background() { assert(false && "Not implemented yet"); }

void QtInstanceWidget::setFontColor(const Color& rFontColor)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QPalette aPalette = getQWidget()->palette();
        aPalette.setColor(getQWidget()->foregroundRole(), toQColor(rFontColor));
        getQWidget()->setPalette(aPalette);
    });
}

void QtInstanceWidget::set_background(const Color& rBackColor)
{
    SolarMutexGuard g;

    GetQtInstance().RunInMainThread([&] {
        QPalette aPalette = getQWidget()->palette();
        aPalette.setColor(getQWidget()->backgroundRole(), toQColor(rBackColor));
        getQWidget()->setPalette(aPalette);
        getQWidget()->setAutoFillBackground(true);
    });
}

void QtInstanceWidget::draw(OutputDevice&, const Point&, const Size&)
{
    assert(false && "Not implemented yet");
}

void QtInstanceWidget::applicationFocusChanged(QWidget* pOldFocus, QWidget* pNewFocus)
{
    SolarMutexGuard g;

    if (pOldFocus == getQWidget())
        signal_focus_out();
    else if (pNewFocus == getQWidget())
        signal_focus_in();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

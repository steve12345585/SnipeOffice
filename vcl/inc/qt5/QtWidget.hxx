/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <QtCore/QRect>
#include <QtWidgets/QGestureEvent>
#include <QtWidgets/QWidget>
#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/accessibility/XAccessibleEditableText.hpp>

class QInputEvent;
class QtFrame;
class QtObject;
struct SalAbstractMouseEvent;

class QtWidget : public QWidget
{
    Q_OBJECT

    QtFrame& m_rFrame;
    bool m_bNonEmptyIMPreeditSeen;
    mutable bool m_bInInputMethodQueryCursorRectangle;
    mutable QRect m_aImCursorRectangle;
    int m_nDeltaX;
    int m_nDeltaY;

    void commitText(const QString& aText) const;
    void deleteReplacementText(int nReplacementStart, int nReplacementLength) const;
    bool handleEvent(QEvent* pEvent);
    // mouse events are always accepted
    void handleMouseButtonEvent(const QMouseEvent*) const;
    bool handleGestureEvent(QGestureEvent* pGestureEvent) const;
    bool handleKeyEvent(QKeyEvent*) const;
    void handleMouseEnterLeaveEvent(QEvent*) const;
    void fillSalAbstractMouseEvent(const QInputEvent* pQEvent, const QPoint& rPos,
                                   Qt::MouseButtons eButtons,
                                   SalAbstractMouseEvent& aSalEvent) const;

    virtual bool event(QEvent*) override;

    virtual void focusInEvent(QFocusEvent*) override;
    virtual void focusOutEvent(QFocusEvent*) override;
    // keyPressEvent(QKeyEvent*) is handled via event(QEvent*); see comment
    virtual void keyReleaseEvent(QKeyEvent*) override;
    virtual void mouseMoveEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseReleaseEvent(QMouseEvent*) override;
    virtual void dragEnterEvent(QDragEnterEvent*) override;
    virtual void dragLeaveEvent(QDragLeaveEvent*) override;
    virtual void dragMoveEvent(QDragMoveEvent*) override;
    virtual void dropEvent(QDropEvent*) override;
    virtual void moveEvent(QMoveEvent*) override;
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;
    virtual void showEvent(QShowEvent*) override;
    virtual void hideEvent(QHideEvent*) override;
    virtual void wheelEvent(QWheelEvent*) override;
    virtual void closeEvent(QCloseEvent*) override;
    virtual void changeEvent(QEvent*) override;
    virtual void leaveEvent(QEvent*) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    virtual void enterEvent(QEnterEvent*) override;
#else
    virtual void enterEvent(QEvent*) override;
#endif

    void inputMethodEvent(QInputMethodEvent*) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery) const override;
    static void closePopup();

public:
    QtWidget(QtFrame& rFrame, Qt::WindowFlags f = Qt::WindowFlags());

    QtFrame& frame() const { return m_rFrame; }
    void endExtTextInput();
    void fakeResize();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

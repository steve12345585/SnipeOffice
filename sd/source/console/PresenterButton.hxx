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

#ifndef INCLUDED_SDEXT_SOURCE_PRESENTER_PRESENTERBUTTON_HXX
#define INCLUDED_SDEXT_SOURCE_PRESENTER_PRESENTERBUTTON_HXX

#include "PresenterBitmapContainer.hxx"
#include "PresenterTheme.hxx"
#include <PresenterHelper.hxx>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/XPaintListener.hpp>
#include <com/sun/star/awt/XMouseListener.hpp>
#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/rendering/XBitmap.hpp>
#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase.hxx>
#include <rtl/ref.hxx>

namespace sdext::presenter {

class PresenterController;

typedef ::cppu::WeakComponentImplHelper <
    css::awt::XPaintListener,
    css::awt::XMouseListener
> PresenterButtonInterfaceBase;

/** Button for the presenter screen.  It displays a text surrounded by a
    frame.
*/
class PresenterButton
    : private ::cppu::BaseMutex,
      public PresenterButtonInterfaceBase
{
public:
    static ::rtl::Reference<PresenterButton> Create (
        const css::uno::Reference<css::uno::XComponentContext>& rxComponentContext,
        const ::rtl::Reference<PresenterController>& rpPresenterController,
        const std::shared_ptr<PresenterTheme>& rpTheme,
        const css::uno::Reference<css::awt::XWindow>& rxParentWindow,
        const css::uno::Reference<css::rendering::XCanvas>& rxParentCanvas,
        const OUString& rsConfigurationName);
    virtual ~PresenterButton() override;
    PresenterButton(const PresenterButton&) = delete;
    PresenterButton& operator=(const PresenterButton&) = delete;

    virtual void SAL_CALL disposing() override;

    void SetCenter (const css::geometry::RealPoint2D& rLocation);
    void SetCanvas (
        const css::uno::Reference<css::rendering::XCanvas>& rxParentCanvas,
        const css::uno::Reference<css::awt::XWindow>& rxParentWindow);
    css::geometry::IntegerSize2D const & GetSize();

    // XPaintListener

    virtual void SAL_CALL windowPaint (const css::awt::PaintEvent& rEvent) override;

    // XMouseListener

    virtual void SAL_CALL mousePressed (const css::awt::MouseEvent& rEvent) override;

    virtual void SAL_CALL mouseReleased (const css::awt::MouseEvent& rEvent) override;

    virtual void SAL_CALL mouseEntered (const css::awt::MouseEvent& rEvent) override;

    virtual void SAL_CALL mouseExited (const css::awt::MouseEvent& rEvent) override;

    // lang::XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject& rEvent) override;

private:
    ::rtl::Reference<PresenterController> mpPresenterController;
    std::shared_ptr<PresenterTheme> mpTheme;
    css::uno::Reference<css::awt::XWindow> mxWindow;
    css::uno::Reference<css::rendering::XCanvas> mxCanvas;
    const OUString msText;
    const PresenterTheme::SharedFontDescriptor mpFont;
    const PresenterTheme::SharedFontDescriptor mpMouseOverFont;
    const OUString msAction;
    css::geometry::RealPoint2D maCenter;
    css::geometry::IntegerSize2D maButtonSize;
    PresenterBitmapDescriptor::Mode meState;
    css::uno::Reference<css::rendering::XBitmap> mxNormalBitmap;
    css::uno::Reference<css::rendering::XBitmap> mxMouseOverBitmap;

    PresenterButton(
        ::rtl::Reference<PresenterController> xPresenterController,
        std::shared_ptr<PresenterTheme> xTheme,
        const css::uno::Reference<css::awt::XWindow>& rxParentWindow,
        PresenterTheme::SharedFontDescriptor aFont,
        PresenterTheme::SharedFontDescriptor aMouseOverFont,
        OUString sText,
        OUString sAction);
    void RenderButton (
        const css::uno::Reference<css::rendering::XCanvas>& rxCanvas,
        const css::geometry::IntegerSize2D& rSize,
        const PresenterTheme::SharedFontDescriptor& rFont,
        const PresenterBitmapDescriptor::Mode eMode,
        const SharedBitmapDescriptor& rpLeft,
        const SharedBitmapDescriptor& rpCenter,
        const SharedBitmapDescriptor& rpRight);
    css::geometry::IntegerSize2D CalculateButtonSize();
    void Invalidate();
    static css::uno::Reference<css::rendering::XBitmap> GetBitmap (
        const SharedBitmapDescriptor& mpIcon,
        const PresenterBitmapDescriptor::Mode eMode);
    void SetupButtonBitmaps();
    static css::uno::Reference<css::beans::XPropertySet> GetConfigurationProperties (
        const css::uno::Reference<css::uno::XComponentContext>& rxComponentContext,
        const OUString& rsConfigurationName);

    /// @throws css::lang::DisposedException
    void ThrowIfDisposed() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

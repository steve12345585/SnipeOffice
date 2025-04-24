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

#include <PresenterHelper.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/rendering/XBitmap.hpp>
#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/util/Color.hpp>
#include <rtl/ref.hxx>
#include <map>
#include <memory>

namespace sdext::presenter {

/** Manage a set of bitmap groups as they are used for buttons: three
    bitmaps, one for the normal state, one for a mouse over effect and one
    to show that the button has been pressed.
    A bitmap group is defined by some entries in the configuration.
*/
class PresenterBitmapContainer
{
public:
    /** There is one bitmap for the normal state, one for a mouse over effect and one
        to show that a button has been pressed.
    */
    class BitmapDescriptor
    {
    public:
        BitmapDescriptor();
        explicit BitmapDescriptor (const std::shared_ptr<BitmapDescriptor>& rpDefault);

        enum Mode {Normal, MouseOver, ButtonDown, Disabled, Mask};
        const css::uno::Reference<css::rendering::XBitmap>& GetNormalBitmap() const;
        css::uno::Reference<css::rendering::XBitmap> const & GetBitmap(const Mode eMode) const;
        void SetBitmap (
            const Mode eMode,
            const css::uno::Reference<css::rendering::XBitmap>& rxBitmap);

        sal_Int32 mnWidth;
        sal_Int32 mnHeight;
        sal_Int32 mnXOffset;
        sal_Int32 mnYOffset;
        sal_Int32 mnXHotSpot;
        sal_Int32 mnYHotSpot;
        css::util::Color maReplacementColor;
        enum TexturingMode { Once, Repeat, Stretch };
        TexturingMode meHorizontalTexturingMode;
        TexturingMode meVerticalTexturingMode;

    private:
        css::uno::Reference<css::rendering::XBitmap> mxNormalBitmap;
        css::uno::Reference<css::rendering::XBitmap> mxMouseOverBitmap;
        css::uno::Reference<css::rendering::XBitmap> mxButtonDownBitmap;
        css::uno::Reference<css::rendering::XBitmap> mxDisabledBitmap;
        css::uno::Reference<css::rendering::XBitmap> mxMaskBitmap;
    };

    /** Create a new bitmap container from a section of the configuration.
        @param rxComponentContext
            The component context is used to create new API objects.
        @param rxCanvas
            Bitmaps are created specifically for this canvas.
        @param rsConfigurationBase
            The name of a configuration node whose sub-tree defines the
            bitmap sets.
    */
    PresenterBitmapContainer (
        const OUString& rsConfigurationBase,
        std::shared_ptr<PresenterBitmapContainer> xParentContainer,
        const css::uno::Reference<css::uno::XComponentContext>& rxComponentContext,
        css::uno::Reference<css::rendering::XCanvas> xCanvas);
    PresenterBitmapContainer (
        const css::uno::Reference<css::container::XNameAccess>& rsRootNode,
        std::shared_ptr<PresenterBitmapContainer> xParentContainer,
        css::uno::Reference<css::rendering::XCanvas> xCanvas);
    ~PresenterBitmapContainer();
    PresenterBitmapContainer(const PresenterBitmapContainer&) = delete;
    PresenterBitmapContainer& operator=(const PresenterBitmapContainer&) = delete;

    /** Return the bitmap set that is associated with the given name.
    */
    std::shared_ptr<BitmapDescriptor> GetBitmap (const OUString& rsName) const;

    static std::shared_ptr<BitmapDescriptor> LoadBitmap (
        const css::uno::Reference<css::container::XHierarchicalNameAccess>& rxNode,
        const OUString& rsPathToBitmapNode,
        const css::uno::Reference<css::rendering::XCanvas>& rxCanvas,
        const std::shared_ptr<BitmapDescriptor>& rpDefaultBitmap);

private:
    std::shared_ptr<PresenterBitmapContainer> mpParentContainer;
    typedef ::std::map<OUString, std::shared_ptr<BitmapDescriptor> > BitmapContainer;
    BitmapContainer maIconContainer;
    css::uno::Reference<css::rendering::XCanvas> mxCanvas;

    void LoadBitmaps (
        const css::uno::Reference<css::container::XNameAccess>& rsRootNode);
    void ProcessBitmap (
        const OUString& rsKey,
        const css::uno::Reference<css::beans::XPropertySet>& rProperties);
    static std::shared_ptr<BitmapDescriptor> LoadBitmap (
        const css::uno::Reference<css::beans::XPropertySet>& rxProperties,
        const css::uno::Reference<css::rendering::XCanvas>& rxCanvas,
        const std::shared_ptr<PresenterBitmapContainer::BitmapDescriptor>& rpDefault);
    static BitmapDescriptor::TexturingMode
        StringToTexturingMode (std::u16string_view rsTexturingMode);
};

typedef PresenterBitmapContainer::BitmapDescriptor PresenterBitmapDescriptor;
typedef std::shared_ptr<PresenterBitmapContainer::BitmapDescriptor> SharedBitmapDescriptor;

} // end of namespace ::sdext::presenter

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

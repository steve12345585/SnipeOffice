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

#include <com/sun/star/view/XRenderable.hpp>
#include <cppuhelper/compbase.hxx>

#include <vcl/print.hxx>

namespace basctl
{

class BaseWindow;

class Renderable :
    public cppu::WeakComponentImplHelper< css::view::XRenderable >,
    public vcl::PrinterOptionsHelper
{
    VclPtr<BaseWindow>      mpWindow;
    osl::Mutex              maMutex;
    std::vector<sal_Int32>  maValidPages;

    VclPtr<Printer> getPrinter() const;
    bool isPrintOddPages() const;
    bool isPrintEvenPages() const;
    static bool isOnEvenPage( sal_Int32 nPage ) { return nPage % 2 == 0; };
public:
    explicit Renderable (BaseWindow*);
    virtual ~Renderable() override;

    // XRenderable
    virtual sal_Int32 SAL_CALL getRendererCount (
        const css::uno::Any& aSelection,
        const css::uno::Sequence<css::beans::PropertyValue >& xOptions) override;

    virtual css::uno::Sequence<css::beans::PropertyValue> SAL_CALL getRenderer (
        sal_Int32 nRenderer,
        const css::uno::Any& rSelection,
        const css::uno::Sequence<css::beans::PropertyValue>& rxOptions) override;

    virtual void SAL_CALL render (
        sal_Int32 nRenderer,
        const css::uno::Any& rSelection,
        const css::uno::Sequence<css::beans::PropertyValue>& rxOptions) override;

};

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#ifndef INCLUDED_SW_SOURCE_UI_VBA_VBADIALOGS_HXX
#define INCLUDED_SW_SOURCE_UI_VBA_VBADIALOGS_HXX

#include <com/sun/star/uno/XComponentContext.hpp>
#include <ooo/vba/word/XDialogs.hpp>
#include <vbahelper/vbadialogsbase.hxx>
#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>

class SwXTextDocument;

typedef cppu::ImplInheritanceHelper< VbaDialogsBase, ov::word::XDialogs > SwVbaDialogs_BASE;

class SwVbaDialogs : public SwVbaDialogs_BASE
{
public:
    SwVbaDialogs( const css::uno::Reference< ov::XHelperInterface >& xParent,
                  const css::uno::Reference< css::uno::XComponentContext > &xContext,
                  const rtl::Reference< SwXTextDocument >& xModel );

    // XCollection
    virtual css::uno::Any SAL_CALL Item( const css::uno::Any& Index ) override;

    // XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
private:
    rtl::Reference< SwXTextDocument > m_xModel;
};

#endif // INCLUDED_SW_SOURCE_UI_VBA_VBADIALOGS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

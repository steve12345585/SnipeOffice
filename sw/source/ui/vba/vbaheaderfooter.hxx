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
#ifndef INCLUDED_SW_SOURCE_UI_VBA_VBAHEADERFOOTER_HXX
#define INCLUDED_SW_SOURCE_UI_VBA_VBAHEADERFOOTER_HXX

#include <ooo/vba/word/XHeaderFooter.hpp>
#include <ooo/vba/word/XRange.hpp>
#include <vbahelper/vbahelperinterface.hxx>
#include <rtl/ref.hxx>

class SwXTextDocument;

typedef InheritedHelperInterfaceWeakImpl< ooo::vba::word::XHeaderFooter > SwVbaHeaderFooter_BASE;

class SwVbaHeaderFooter : public SwVbaHeaderFooter_BASE
{
private:
    rtl::Reference< SwXTextDocument > mxModel;
    css::uno::Reference< css::beans::XPropertySet > mxPageStyleProps;
    bool mbHeader;
    sal_Int32 mnIndex;

public:
    /// @throws css::uno::RuntimeException
    SwVbaHeaderFooter( const css::uno::Reference< ooo::vba::XHelperInterface >& rParent,
                       const css::uno::Reference< css::uno::XComponentContext >& rContext,
                       rtl::Reference< SwXTextDocument > xModel,
                       css::uno::Reference< css::beans::XPropertySet > xProps,
                       bool isHeader, sal_Int32 index );

    // Attributes
    virtual sal_Bool SAL_CALL getIsHeader() override;
    virtual sal_Bool SAL_CALL getLinkToPrevious() override;
    virtual void SAL_CALL setLinkToPrevious( sal_Bool _linktoprevious ) override;
    virtual css::uno::Reference< ::ooo::vba::word::XRange > SAL_CALL getRange() override;
    virtual css::uno::Any SAL_CALL Shapes( const css::uno::Any& aIndex ) override;

    // XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
};
#endif // INCLUDED_SW_SOURCE_UI_VBA_VBAHEADERFOOTER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

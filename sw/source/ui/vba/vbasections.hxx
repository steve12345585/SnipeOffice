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
#ifndef INCLUDED_SW_SOURCE_UI_VBA_VBASECTIONS_HXX
#define INCLUDED_SW_SOURCE_UI_VBA_VBASECTIONS_HXX

#include <vbahelper/vbacollectionimpl.hxx>
#include <ooo/vba/word/XSections.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <rtl/ref.hxx>

class SwXTextDocument;

typedef CollTestImplHelper< ooo::vba::word::XSections > SwVbaSections_BASE;

class SwVbaSections : public SwVbaSections_BASE
{
private:
    rtl::Reference< SwXTextDocument > mxModel;

public:
    SwVbaSections( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext > & xContext, const rtl::Reference< SwXTextDocument >& xModel );
    SwVbaSections( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext > & xContext, const rtl::Reference< SwXTextDocument >& xModel, const css::uno::Reference< css::text::XTextRange >& xTextRange );

    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() override;

    virtual css::uno::Any SAL_CALL PageSetup(  ) override;

    // SwVbaSections_BASE
    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource ) override;
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
};

#endif // INCLUDED_SW_SOURCE_UI_VBA_VBASECTIONS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

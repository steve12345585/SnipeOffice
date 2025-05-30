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
#ifndef INCLUDED_SW_SOURCE_UI_VBA_VBAINFORMATIONHELPER_HXX
#define INCLUDED_SW_SOURCE_UI_VBA_VBAINFORMATIONHELPER_HXX

#include <com/sun/star/text/XTextViewCursor.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <rtl/ref.hxx>

class SwXTextDocument;

class SwVbaInformationHelper
{
public:
    /// @throws css::uno::RuntimeException
    static sal_Int32
    handleWdActiveEndPageNumber(const css::uno::Reference<css::text::XTextViewCursor>& xTVCursor);
    /// @throws css::uno::RuntimeException
    static sal_Int32 handleWdNumberOfPagesInDocument(const rtl::Reference<SwXTextDocument>& xModel);
    /// @throws css::uno::RuntimeException
    static double handleWdVerticalPositionRelativeToPage(
        const rtl::Reference<SwXTextDocument>& xModel,
        const css::uno::Reference<css::text::XTextViewCursor>& xTVCursor);
    //static double verticalPositionRelativeToPageBoundary( const css::uno::Reference< css::frame::XModel >& xModel, const css::uno::Reference< css::text::XTextViewCursor >& xTVCursor, const css::uno::Reference< css::beans::XPropertySet >& xStyleProps ) throw( css::uno::RuntimeException );
};
#endif // INCLUDED_SW_SOURCE_UI_VBA_VBAINFORMATIONHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#ifndef INCLUDED_SW_SOURCE_UI_VBA_VBABOOKMARK_HXX
#define INCLUDED_SW_SOURCE_UI_VBA_VBABOOKMARK_HXX

#include <ooo/vba/word/XBookmark.hpp>
#include <vbahelper/vbahelperinterface.hxx>
#include <com/sun/star/text/XTextContent.hpp>
#include <rtl/ref.hxx>

class SwXTextDocument;

typedef InheritedHelperInterfaceWeakImpl<ooo::vba::word::XBookmark> SwVbaBookmark_BASE;

class SwVbaBookmark : public SwVbaBookmark_BASE
{
private:
    rtl::Reference<SwXTextDocument> mxModel;
    css::uno::Reference<css::text::XTextContent> mxBookmark;
    OUString maBookmarkName;
    bool mbValid;

private:
    /// @throws css::uno::RuntimeException
    void checkVality();

public:
    /// @throws css::uno::RuntimeException
    SwVbaBookmark(const css::uno::Reference<ooo::vba::XHelperInterface>& rParent,
                  const css::uno::Reference<css::uno::XComponentContext>& rContext,
                  rtl::Reference<SwXTextDocument> xModel, OUString aName);
    virtual ~SwVbaBookmark() override;

    // Methods
    virtual OUString SAL_CALL getName() override;
    virtual void SAL_CALL setName(const OUString&) override;
    virtual void SAL_CALL Delete() override;
    virtual void SAL_CALL Select() override;
    virtual css::uno::Any SAL_CALL Range() override;

    // XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
};
#endif // INCLUDED_SW_SOURCE_UI_VBA_VBABOOKMARK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

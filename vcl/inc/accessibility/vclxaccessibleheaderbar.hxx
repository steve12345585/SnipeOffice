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

#include <vcl/accessibility/vclxaccessiblecomponent.hxx>
#include <vcl/headbar.hxx>
#include <unotools/weakref.hxx>

class VCLXAccessibleHeaderBarItem;

typedef std::vector<unotools::WeakReference<VCLXAccessibleHeaderBarItem>> ListItems;

class VCLXAccessibleHeaderBar final : public VCLXAccessibleComponent
{
    VclPtr<HeaderBar> m_pHeadBar;

public:
    virtual ~VCLXAccessibleHeaderBar() override;

    VCLXAccessibleHeaderBar(HeaderBar* pHeaderBar);

    // XAccessibleContext
    virtual sal_Int64 SAL_CALL getAccessibleChildCount() override;
    virtual css::uno::Reference<css::accessibility::XAccessible>
        SAL_CALL getAccessibleChild(sal_Int64 i) override;
    virtual sal_Int16 SAL_CALL getAccessibleRole() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

public:
    virtual void SAL_CALL disposing() override;
    rtl::Reference<VCLXAccessibleHeaderBarItem> CreateChild(sal_Int32 i);

private:
    ListItems m_aAccessibleChildren;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

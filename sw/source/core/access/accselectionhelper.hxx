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

#ifndef INCLUDED_SW_SOURCE_CORE_ACCESS_ACCSELECTIONHELPER_HXX
#define INCLUDED_SW_SOURCE_CORE_ACCESS_ACCSELECTIONHELPER_HXX

#include <sal/types.h>
#include <com/sun/star/uno/Reference.h>

namespace com::sun::star::accessibility { class XAccessible; }

class SwAccessibleContext;
class SwFEShell;

class SwAccessibleSelectionHelper
{
    /// the context on which this helper works
    SwAccessibleContext& m_rContext;

    /// get FE-Shell
    SwFEShell* GetFEShell();

    /// @throws css::lang::IndexOutOfBoundsException
    void throwIndexOutOfBoundsException();

public:
    SwAccessibleSelectionHelper( SwAccessibleContext& rContext );

    // XAccessibleSelection

    /// @throws css::lang::IndexOutOfBoundsException
    /// @throws css::uno::RuntimeException
    void selectAccessibleChild(
        sal_Int64 nChildIndex );

    /// @throws css::lang::IndexOutOfBoundsException
    /// @throws css::uno::RuntimeException
    bool isAccessibleChildSelected(
        sal_Int64 nChildIndex );
    /// @throws css::uno::RuntimeException
    void selectAllAccessibleChildren(  );
    /// @throws css::uno::RuntimeException
    sal_Int64 getSelectedAccessibleChildCount(  );
    /// @throws css::lang::IndexOutOfBoundsException
    /// @throws css::uno::RuntimeException
    css::uno::Reference< css::accessibility::XAccessible > getSelectedAccessibleChild(
        sal_Int64 nSelectedChildIndex );
    // index has to be treated as global child index.
    /// @throws css::lang::IndexOutOfBoundsException
    /// @throws css::uno::RuntimeException
    void deselectAccessibleChild(
        sal_Int64 nChildIndex );
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/accessibility/XAccessibleSelection.hpp>
#include <cppuhelper/implbase.hxx>
#include <vcl/accessibility/vclxaccessiblecomponent.hxx>
#include <vcl/tabctrl.hxx>
#include <vcl/vclptr.hxx>

#include <vector>

class VCLXAccessibleTabPage;


class VCLXAccessibleTabControl final : public cppu::ImplInheritanceHelper<
                                           VCLXAccessibleComponent,
                                           css::accessibility::XAccessibleSelection>
{
private:
    typedef std::vector< rtl::Reference< VCLXAccessibleTabPage > > AccessibleChildren;

    AccessibleChildren      m_aAccessibleChildren;
    VclPtr<TabControl>      m_pTabControl;

    css::uno::Reference< css::accessibility::XAccessible > implGetAccessibleChild( sal_Int64 i );
    bool implIsAccessibleChildSelected( sal_Int32 nChildIndex );


    void                    UpdateFocused();
    void                    UpdateSelected( sal_Int32 i, bool bSelected );
    void                    UpdatePageText( sal_Int32 i );
    void                    UpdateTabPage( sal_Int32 i, bool bNew );

    void                    InsertChild( sal_Int32 i );
    void                    RemoveChild( sal_Int32 i );

    virtual void            ProcessWindowEvent( const VclWindowEvent& rVclWindowEvent ) override;
    virtual void            ProcessWindowChildEvent( const VclWindowEvent& rVclWindowEvent ) override;
    virtual void            FillAccessibleStateSet( sal_Int64& rStateSet ) override;

    // XComponent
    virtual void SAL_CALL   disposing() override;

public:
    VCLXAccessibleTabControl(vcl::Window* pWindow);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XAccessibleContext
    virtual sal_Int64 SAL_CALL getAccessibleChildCount(  ) override;
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL getAccessibleChild( sal_Int64 i ) override;
    virtual sal_Int16 SAL_CALL getAccessibleRole(  ) override;
    virtual OUString SAL_CALL getAccessibleName(  ) override;

    // XAccessibleSelection
    virtual void SAL_CALL selectAccessibleChild( sal_Int64 nChildIndex ) override;
    virtual sal_Bool SAL_CALL isAccessibleChildSelected( sal_Int64 nChildIndex ) override;
    virtual void SAL_CALL clearAccessibleSelection(  ) override;
    virtual void SAL_CALL selectAllAccessibleChildren(  ) override;
    virtual sal_Int64 SAL_CALL getSelectedAccessibleChildCount(  ) override;
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL getSelectedAccessibleChild( sal_Int64 nSelectedChildIndex ) override;
    virtual void SAL_CALL deselectAccessibleChild( sal_Int64 nChildIndex ) override;
};



/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

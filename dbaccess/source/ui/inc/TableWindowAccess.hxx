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

#include "TableWindow.hxx"
#include <com/sun/star/accessibility/AccessibleRelationType.hpp>
#include <com/sun/star/accessibility/XAccessibleRelationSet.hpp>
#include <cppuhelper/implbase.hxx>
#include <vcl/accessibility/vclxaccessiblecomponent.hxx>
#include <vcl/vclptr.hxx>

using css::accessibility::AccessibleRelationType;

namespace dbaui
{
    /** the class OTableWindowAccess represents the accessible object for table windows
        like they are used in the QueryDesign and the RelationDesign
    */
    class OTableWindowAccess    :   public cppu::ImplInheritanceHelper<
                                        VCLXAccessibleComponent,
                                        css::accessibility::XAccessibleRelationSet,
                                        css::accessibility::XAccessible>
    {
        VclPtr<OTableWindow>   m_pTable; // the window which I should give accessibility to

        css::uno::Reference< css::accessibility::XAccessible > getParentChild(sal_Int64 _nIndex);
    protected:
        /** this function is called upon disposing the component
        */
        virtual void SAL_CALL disposing() override;

        virtual void ProcessWindowEvent( const VclWindowEvent& rVclWindowEvent ) override;
    public:
        OTableWindowAccess( OTableWindow* _pTable);

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName() override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

        // XAccessible
        virtual css::uno::Reference< css::accessibility::XAccessibleContext > SAL_CALL getAccessibleContext(  ) override;

        // XAccessibleContext
        virtual sal_Int64 SAL_CALL getAccessibleChildCount(  ) override;
        virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL getAccessibleChild( sal_Int64 i ) override;
        virtual sal_Int64 SAL_CALL getAccessibleIndexInParent(  ) override;
        virtual sal_Int16 SAL_CALL getAccessibleRole(  ) override;
        virtual OUString SAL_CALL getAccessibleName(  ) override;
        virtual css::uno::Reference< css::accessibility::XAccessibleRelationSet > SAL_CALL getAccessibleRelationSet(  ) override;

        // XAccessibleComponent
        virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL getAccessibleAtPoint( const css::awt::Point& aPoint ) override;

        // XAccessibleExtendedComponent
        virtual OUString SAL_CALL getTitledBorderText(  ) override;

        // XAccessibleRelationSet
        virtual sal_Int32 SAL_CALL getRelationCount(  ) override;
        virtual css::accessibility::AccessibleRelation SAL_CALL getRelation( sal_Int32 nIndex ) override;
        virtual sal_Bool SAL_CALL containsRelation(AccessibleRelationType eRelationType) override;
        virtual css::accessibility::AccessibleRelation SAL_CALL getRelationByType(AccessibleRelationType eRelationType) override;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

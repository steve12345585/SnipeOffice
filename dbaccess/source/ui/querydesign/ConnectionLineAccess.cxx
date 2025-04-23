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

#include <ConnectionLineAccess.hxx>
#include <ConnectionLine.hxx>
#include <JoinTableView.hxx>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/AccessibleRelationType.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <TableConnection.hxx>
#include <TableWindow.hxx>

namespace dbaui
{
    using namespace ::com::sun::star::accessibility;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star;

    OConnectionLineAccess::OConnectionLineAccess(OTableConnection* _pLine)
        : ImplInheritanceHelper(_pLine)
        ,m_pLine(_pLine)
    {
    }
    void SAL_CALL OConnectionLineAccess::disposing()
    {
        m_pLine = nullptr;
        VCLXAccessibleComponent::disposing();
    }
    OUString SAL_CALL OConnectionLineAccess::getImplementationName()
    {
        return u"org.openoffice.comp.dbu.ConnectionLineAccessibility"_ustr;
    }
    // XAccessibleContext
    sal_Int64 SAL_CALL OConnectionLineAccess::getAccessibleChildCount(  )
    {
        return 0;
    }
    Reference< XAccessible > SAL_CALL OConnectionLineAccess::getAccessibleChild( sal_Int64 /*i*/ )
    {
        return Reference< XAccessible >();
    }
    sal_Int64 SAL_CALL OConnectionLineAccess::getAccessibleIndexInParent(  )
    {
        ::osl::MutexGuard aGuard( m_aMutex  );
        sal_Int64 nIndex = -1;
        if( m_pLine )
        {
            // search the position of our table window in the table window map
            // TODO JNA Shouldn't nIndex begin at 0?
            nIndex = m_pLine->GetParent()->GetTabWinMap().size();
            const auto& rVec = m_pLine->GetParent()->getTableConnections();
            bool bFound = false;
            for (auto const& elem : rVec)
            {
                if (elem.get() == m_pLine)
                {
                    bFound = true;
                    break;
                }
                ++nIndex;
            }
            nIndex = bFound ? nIndex : -1;
        }
        return nIndex;
    }
    sal_Int16 SAL_CALL OConnectionLineAccess::getAccessibleRole(  )
    {
        return AccessibleRole::UNKNOWN; // ? or may be an AccessibleRole::WINDOW
    }
    OUString SAL_CALL OConnectionLineAccess::getAccessibleDescription(  )
    {
        return u"Relation"_ustr;
    }
    Reference< XAccessibleRelationSet > SAL_CALL OConnectionLineAccess::getAccessibleRelationSet(  )
    {
        ::osl::MutexGuard aGuard( m_aMutex  );
        return this;
    }
    // XAccessibleComponent
    Reference< XAccessible > SAL_CALL OConnectionLineAccess::getAccessibleAtPoint( const awt::Point& /*_aPoint*/ )
    {
        return Reference< XAccessible >();
    }

    awt::Rectangle OConnectionLineAccess::implGetBounds()
    {
        tools::Rectangle aRect(m_pLine ? m_pLine->GetBoundingRect() : tools::Rectangle());
        return awt::Rectangle(aRect.Left(),aRect.Top(),aRect.getOpenWidth(),aRect.getOpenHeight());
    }

    // XAccessibleRelationSet
    sal_Int32 SAL_CALL OConnectionLineAccess::getRelationCount(  )
    {
        return 1;
    }
    AccessibleRelation SAL_CALL OConnectionLineAccess::getRelation( sal_Int32 nIndex )
    {
        ::osl::MutexGuard aGuard( m_aMutex  );
        if( nIndex < 0 || nIndex >= getRelationCount() )
            throw IndexOutOfBoundsException();

        Sequence<Reference<XAccessible>> aSeq;
        if( m_pLine )
        {
            aSeq = { m_pLine->GetSourceWin()->GetAccessible(),
                     m_pLine->GetDestWin()->GetAccessible() };
        }

        return AccessibleRelation(AccessibleRelationType_CONTROLLED_BY,aSeq);
    }
    sal_Bool SAL_CALL OConnectionLineAccess::containsRelation(AccessibleRelationType eRelationType)
    {
        return AccessibleRelationType_CONTROLLED_BY == eRelationType;
    }
    AccessibleRelation SAL_CALL OConnectionLineAccess::getRelationByType(AccessibleRelationType eRelationType)
    {
        if (AccessibleRelationType_CONTROLLED_BY == eRelationType)
            return getRelation(0);
        return AccessibleRelation();
    }
    Reference< XAccessible > OTableConnection::CreateAccessible()
    {
        return new OConnectionLineAccess(this);
    }
    OTableConnection::~OTableConnection()
    {
        disposeOnce();
    }
    void OTableConnection::dispose()
    {
        // clear vector
        clearLineData();
        m_pParent.clear();
        vcl::Window::dispose();
    }
    Reference< XAccessibleContext > SAL_CALL OConnectionLineAccess::getAccessibleContext(  )
    {
        return this;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

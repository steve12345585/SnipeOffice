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

#include <editeng/AccessibleContextBase.hxx>

#include <com/sun/star/accessibility/XAccessibleEventListener.hpp>
#include <com/sun/star/accessibility/XAccessibleSelection.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleRelationType.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/IllegalAccessibleComponentStateException.hpp>

#include <unotools/accessiblerelationsethelper.hxx>
#include <comphelper/accessibleeventnotifier.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/mutex.hxx>
#include <rtl/ref.hxx>
#include <tools/color.hxx>

#include <utility>

using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;

namespace accessibility {

// internal

AccessibleContextBase::AccessibleContextBase (
        uno::Reference<XAccessible> xParent,
        const sal_Int16 aRole)
    :   mxParent(std::move(xParent)),
        meDescriptionOrigin(NotSet),
        meNameOrigin(NotSet),
        maRole(aRole)
{
    // Create the state set.
    mnStateSet = 0;

    // Set some states.  Don't use the SetState method because no events
    // shall be broadcasted (that is not yet initialized anyway).
    mnStateSet |= AccessibleStateType::ENABLED;
    mnStateSet |= AccessibleStateType::SENSITIVE;
    mnStateSet |= AccessibleStateType::SHOWING;
    mnStateSet |= AccessibleStateType::VISIBLE;
    mnStateSet |= AccessibleStateType::FOCUSABLE;
    mnStateSet |= AccessibleStateType::SELECTABLE;

    // Create the relation set.
    mxRelationSet = new ::utl::AccessibleRelationSetHelper ();
}

AccessibleContextBase::~AccessibleContextBase()
{
}

bool AccessibleContextBase::SetState (sal_Int64 aState)
{
    ::osl::ClearableMutexGuard aGuard (m_aMutex);
    if (!(mnStateSet & aState))
    {
        mnStateSet |= aState;
        // Clear the mutex guard so that it is not locked during calls to
        // listeners.
        aGuard.clear();

        // Send event for all states except the DEFUNC state.
        if (aState != AccessibleStateType::DEFUNC)
        {
            uno::Any aNewValue;
            aNewValue <<= aState;
            CommitChange(
                AccessibleEventId::STATE_CHANGED,
                aNewValue,
                uno::Any(), -1);
        }
        return true;
    }
    else
        return false;
}


bool AccessibleContextBase::ResetState (sal_Int64 aState)
{
    ::osl::ClearableMutexGuard aGuard (m_aMutex);
    if (mnStateSet & aState)
    {
        mnStateSet &= ~aState;
        // Clear the mutex guard so that it is not locked during calls to listeners.
        aGuard.clear();

        uno::Any aOldValue;
        aOldValue <<= aState;
        CommitChange(
            AccessibleEventId::STATE_CHANGED,
            uno::Any(),
            aOldValue, -1);
        return true;
    }
    else
        return false;
}


bool AccessibleContextBase::GetState (sal_Int64 aState)
{
    ::osl::MutexGuard aGuard (m_aMutex);
    return mnStateSet & aState;
}


void AccessibleContextBase::SetRelationSet (
    const rtl::Reference<utl::AccessibleRelationSetHelper>& rxNewRelationSet)
{
    // Try to emit some meaningful events indicating differing relations in
    // both sets.
    const std::pair<AccessibleRelationType, short int> aRelationDescriptors[] = {
        { AccessibleRelationType_CONTROLLED_BY, AccessibleEventId::CONTROLLED_BY_RELATION_CHANGED },
        { AccessibleRelationType_CONTROLLER_FOR, AccessibleEventId::CONTROLLER_FOR_RELATION_CHANGED },
        { AccessibleRelationType_LABELED_BY, AccessibleEventId::LABELED_BY_RELATION_CHANGED },
        { AccessibleRelationType_LABEL_FOR, AccessibleEventId::LABEL_FOR_RELATION_CHANGED },
        { AccessibleRelationType_MEMBER_OF, AccessibleEventId::MEMBER_OF_RELATION_CHANGED },
    };
    for (const std::pair<AccessibleRelationType, short int>& rPair : aRelationDescriptors)
    {
        if (mxRelationSet->containsRelation(rPair.first)
            != rxNewRelationSet->containsRelation(rPair.first))
            CommitChange(rPair.second, uno::Any(), uno::Any(), -1);
    }

    mxRelationSet = rxNewRelationSet;
}


// XAccessible

uno::Reference< XAccessibleContext> SAL_CALL
    AccessibleContextBase::getAccessibleContext()
{
    return this;
}


// XAccessibleContext

/** No children.
*/
sal_Int64 SAL_CALL
       AccessibleContextBase::getAccessibleChildCount()
{
    return 0;
}


/** Forward the request to the shape.  Return the requested shape or throw
    an exception for a wrong index.
*/
uno::Reference<XAccessible> SAL_CALL
    AccessibleContextBase::getAccessibleChild (sal_Int64 nIndex)
{
    ensureAlive();
    throw lang::IndexOutOfBoundsException (
        "no child with index " + OUString::number(nIndex),
        nullptr);
}


uno::Reference<XAccessible> SAL_CALL
       AccessibleContextBase::getAccessibleParent()
{
    ensureAlive();
    return mxParent;
}

sal_Int16 SAL_CALL
    AccessibleContextBase::getAccessibleRole()
{
    ensureAlive();
    return maRole;
}


OUString SAL_CALL
       AccessibleContextBase::getAccessibleDescription()
{
    ensureAlive();

    return msDescription;
}


OUString SAL_CALL
       AccessibleContextBase::getAccessibleName()
{
    ensureAlive();

    if (meNameOrigin == NotSet)
    {
        // Do not send an event because this is the first time it has been
        // requested.
        msName = CreateAccessibleName();
        meNameOrigin = AutomaticallyCreated;
    }

    return msName;
}


/** Return a copy of the relation set.
*/
uno::Reference<XAccessibleRelationSet> SAL_CALL
       AccessibleContextBase::getAccessibleRelationSet()
{
    ensureAlive();

    // Create a copy of the relation set and return it.
    if (mxRelationSet)
    {
        return mxRelationSet->Clone();
    }
    else
        return uno::Reference<XAccessibleRelationSet>(nullptr);
}


/** Return a copy of the state set.
    Possible states are:
        ENABLED
        SHOWING
        VISIBLE
*/
sal_Int64 SAL_CALL
    AccessibleContextBase::getAccessibleStateSet()
{
    if (rBHelper.bDisposed)
    {
        // We are already disposed!
        // Create a new state set that has only set the DEFUNC state.
        return AccessibleStateType::DEFUNC;
    }
    else
    {
        return mnStateSet;
    }
}


lang::Locale SAL_CALL
       AccessibleContextBase::getLocale()
{
    ensureAlive();
    // Delegate request to parent.
    if (mxParent.is())
    {
        uno::Reference<XAccessibleContext> xParentContext (
            mxParent->getAccessibleContext());
        if (xParentContext.is())
            return xParentContext->getLocale ();
    }

    //  No locale and no parent.  Therefore throw exception to indicate this
    //  cluelessness.
    throw IllegalAccessibleComponentStateException ();
}

// XAccessibleComponent

uno::Reference<XAccessible > SAL_CALL
AccessibleContextBase::getAccessibleAtPoint (
    const awt::Point& /*aPoint*/)
{
    return uno::Reference<XAccessible>();
}

void SAL_CALL AccessibleContextBase::grabFocus()
{
    uno::Reference<XAccessibleSelection> xSelection(getAccessibleParent(), uno::UNO_QUERY);
    if (xSelection.is())
    {
        // Do a single selection on this object.
        xSelection->clearAccessibleSelection();
        xSelection->selectAccessibleChild (getAccessibleIndexInParent());
    }
}


sal_Int32 SAL_CALL AccessibleContextBase::getForeground()
{
    return sal_Int32(COL_BLACK);
}


sal_Int32 SAL_CALL AccessibleContextBase::getBackground()
{
    return sal_Int32(COL_WHITE);
}

// XServiceInfo
OUString SAL_CALL AccessibleContextBase::getImplementationName()
{
    return u"AccessibleContextBase"_ustr;
}

sal_Bool SAL_CALL AccessibleContextBase::supportsService (const OUString& sServiceName)
{
    return cppu::supportsService(this, sServiceName);
}

uno::Sequence< OUString > SAL_CALL
       AccessibleContextBase::getSupportedServiceNames()
{
    return {
        u"com.sun.star.accessibility.Accessible"_ustr,
        u"com.sun.star.accessibility.AccessibleContext"_ustr};
}


// XTypeProvider

uno::Sequence<sal_Int8> SAL_CALL
    AccessibleContextBase::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

// internal

void SAL_CALL AccessibleContextBase::disposing()
{
    SetState (AccessibleStateType::DEFUNC);

    ::osl::MutexGuard aGuard (m_aMutex);

    comphelper::OAccessibleComponentHelper::disposing();

    mxParent.clear();
    mxRelationSet.clear();
}


void AccessibleContextBase::SetAccessibleDescription (
    const OUString& rDescription,
    StringOrigin eDescriptionOrigin)
{
    if (!(eDescriptionOrigin < meDescriptionOrigin
        || (eDescriptionOrigin == meDescriptionOrigin && msDescription != rDescription)))
        return;

    uno::Any aOldValue, aNewValue;
    aOldValue <<= msDescription;
    aNewValue <<= rDescription;

    msDescription = rDescription;
    meDescriptionOrigin = eDescriptionOrigin;

    CommitChange(
        AccessibleEventId::DESCRIPTION_CHANGED,
        aNewValue,
        aOldValue, -1);
}


void AccessibleContextBase::SetAccessibleName (
    const OUString& rName,
    StringOrigin eNameOrigin)
{
    if (!(eNameOrigin < meNameOrigin
        || (eNameOrigin == meNameOrigin && msName != rName)))
        return;

    uno::Any aOldValue, aNewValue;
    aOldValue <<= msName;
    aNewValue <<= rName;

    msName = rName;
    meNameOrigin = eNameOrigin;

    CommitChange(
        AccessibleEventId::NAME_CHANGED,
        aNewValue,
        aOldValue, -1);
}


OUString AccessibleContextBase::CreateAccessibleName()
{
    return u"Empty Name"_ustr;
}


void AccessibleContextBase::CommitChange (
    sal_Int16 nEventId,
    const uno::Any& rNewValue,
    const uno::Any& rOldValue,
    sal_Int32 nValueIndex)
{
    NotifyAccessibleEvent(nEventId, rOldValue, rNewValue, nValueIndex);
}

bool AccessibleContextBase::IsDisposed() const
{
    return (rBHelper.bDisposed || rBHelper.bInDispose);
}


void AccessibleContextBase::SetAccessibleRole( sal_Int16 _nRole )
{
    maRole = _nRole;
}


} // end of namespace accessibility

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

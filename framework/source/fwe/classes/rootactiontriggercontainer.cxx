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

#include <classes/rootactiontriggercontainer.hxx>
#include <classes/actiontriggercontainer.hxx>
#include <classes/actiontriggerpropertyset.hxx>
#include <classes/actiontriggerseparatorpropertyset.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <framework/actiontriggerhelper.hxx>
#include <utility>
#include <vcl/svapp.hxx>

using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::beans;

namespace framework
{

RootActionTriggerContainer::RootActionTriggerContainer(css::uno::Reference<css::awt::XPopupMenu> xMenu,
                                                       const OUString* pMenuIdentifier)
    : m_bContainerCreated(false)
    , m_xMenu(std::move(xMenu))
    , m_pMenuIdentifier(pMenuIdentifier)
{
}

RootActionTriggerContainer::~RootActionTriggerContainer()
{
}

// XMultiServiceFactory
Reference< XInterface > SAL_CALL RootActionTriggerContainer::createInstance( const OUString& aServiceSpecifier )
{
    if ( aServiceSpecifier == SERVICENAME_ACTIONTRIGGER )
        return static_cast<OWeakObject *>( new ActionTriggerPropertySet());
    else if ( aServiceSpecifier == SERVICENAME_ACTIONTRIGGERCONTAINER )
        return static_cast<OWeakObject *>( new ActionTriggerContainer());
    else if ( aServiceSpecifier == SERVICENAME_ACTIONTRIGGERSEPARATOR )
        return static_cast<OWeakObject *>( new ActionTriggerSeparatorPropertySet());
    else
        throw css::uno::RuntimeException(u"Unknown service specifier!"_ustr, static_cast<OWeakObject *>(this) );
}

Reference< XInterface > SAL_CALL RootActionTriggerContainer::createInstanceWithArguments( const OUString& ServiceSpecifier, const Sequence< Any >& /*Arguments*/ )
{
    return createInstance( ServiceSpecifier );
}

Sequence< OUString > SAL_CALL RootActionTriggerContainer::getAvailableServiceNames()
{
    Sequence< OUString > aSeq{ SERVICENAME_ACTIONTRIGGER,
                               SERVICENAME_ACTIONTRIGGERCONTAINER,
                               SERVICENAME_ACTIONTRIGGERSEPARATOR };
    return aSeq;
}

// XIndexContainer
void SAL_CALL RootActionTriggerContainer::insertByIndex( sal_Int32 Index, const Any& Element )
{
    SolarMutexGuard g;

    if ( !m_bContainerCreated )
        FillContainer();

    PropertySetContainer::insertByIndex( Index, Element );
}

void SAL_CALL RootActionTriggerContainer::removeByIndex( sal_Int32 Index )
{
    SolarMutexGuard g;

    if ( !m_bContainerCreated )
        FillContainer();

    PropertySetContainer::removeByIndex( Index );
}

// XIndexReplace
void SAL_CALL RootActionTriggerContainer::replaceByIndex( sal_Int32 Index, const Any& Element )
{
    SolarMutexGuard g;

    if ( !m_bContainerCreated )
        FillContainer();

    PropertySetContainer::replaceByIndex( Index, Element );
}

// XIndexAccess
sal_Int32 SAL_CALL RootActionTriggerContainer::getCount()
{
    SolarMutexGuard g;

    if ( !m_bContainerCreated )
    {
        if ( m_xMenu )
            return m_xMenu->getItemCount();
        else
            return 0;
    }
    else
    {
        return PropertySetContainer::getCount();
    }
}

Any SAL_CALL RootActionTriggerContainer::getByIndex( sal_Int32 Index )
{
    SolarMutexGuard g;

    if ( !m_bContainerCreated )
        FillContainer();

    return PropertySetContainer::getByIndex( Index );
}

// XElementAccess
Type SAL_CALL RootActionTriggerContainer::getElementType()
{
    return cppu::UnoType<XPropertySet>::get();
}

sal_Bool SAL_CALL RootActionTriggerContainer::hasElements()
{
    if (m_xMenu)
        return m_xMenu->getItemCount() > 0;
    return false;
}

// XServiceInfo
OUString SAL_CALL RootActionTriggerContainer::getImplementationName()
{
    return IMPLEMENTATIONNAME_ROOTACTIONTRIGGERCONTAINER;
}

sal_Bool SAL_CALL RootActionTriggerContainer::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL RootActionTriggerContainer::getSupportedServiceNames()
{
    return { SERVICENAME_ACTIONTRIGGERCONTAINER };
}

// private implementation helper
void RootActionTriggerContainer::FillContainer()
{
    m_bContainerCreated = true;
    ActionTriggerHelper::FillActionTriggerContainerFromMenu(
        this, m_xMenu);
}
OUString RootActionTriggerContainer::getName()
{
    OUString sRet;
    if( m_pMenuIdentifier )
        sRet = *m_pMenuIdentifier;
    return sRet;
}

void RootActionTriggerContainer::setName( const OUString& )
{
    throw RuntimeException();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

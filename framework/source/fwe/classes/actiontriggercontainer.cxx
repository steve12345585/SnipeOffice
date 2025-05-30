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

#include <classes/actiontriggercontainer.hxx>
#include <classes/actiontriggerpropertyset.hxx>
#include <classes/actiontriggerseparatorpropertyset.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/typeprovider.hxx>

using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;

namespace framework
{

ActionTriggerContainer::ActionTriggerContainer()
{
}

ActionTriggerContainer::~ActionTriggerContainer()
{
}

// XMultiServiceFactory
Reference< XInterface > SAL_CALL ActionTriggerContainer::createInstance( const OUString& aServiceSpecifier )
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

Reference< XInterface > SAL_CALL ActionTriggerContainer::createInstanceWithArguments( const OUString& ServiceSpecifier, const Sequence< Any >& /*Arguments*/ )
{
    return createInstance( ServiceSpecifier );
}

Sequence< OUString > SAL_CALL ActionTriggerContainer::getAvailableServiceNames()
{
    Sequence< OUString > aSeq{ SERVICENAME_ACTIONTRIGGER,
                               SERVICENAME_ACTIONTRIGGERCONTAINER,
                               SERVICENAME_ACTIONTRIGGERSEPARATOR };

    return aSeq;
}

// XServiceInfo
OUString SAL_CALL ActionTriggerContainer::getImplementationName()
{
    return IMPLEMENTATIONNAME_ACTIONTRIGGERCONTAINER;
}

sal_Bool SAL_CALL ActionTriggerContainer::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL ActionTriggerContainer::getSupportedServiceNames()
{
    Sequence< OUString > seqServiceNames { SERVICENAME_ACTIONTRIGGERCONTAINER };
    return seqServiceNames;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#ifndef INCLUDED_COMPHELPER_PROCESSFACTORY_HXX
#define INCLUDED_COMPHELPER_PROCESSFACTORY_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <comphelper/comphelperdllapi.h>

namespace com::sun::star::lang {
    class XMultiServiceFactory;
}
namespace com::sun::star::uno { class XComponentContext; }

namespace comphelper
{

/**
 * This function set the process service factory.
 */
COMPHELPER_DLLPUBLIC void setProcessServiceFactory(const css::uno::Reference< css::lang::XMultiServiceFactory >& xSMgr);

/**
 * This function gets the process service factory.
 *
 * If no service factory is set the function throws a RuntimeException.
 */
COMPHELPER_DLLPUBLIC css::uno::Reference< css::lang::XMultiServiceFactory > getProcessServiceFactory();

/** Obtains a component context from a service factory.

    Throws a RuntimeException if no component context can be obtained.

    @param factory may be null
    @return may be null
 */
COMPHELPER_DLLPUBLIC
css::uno::Reference< css::uno::XComponentContext >
getComponentContext(
    css::uno::Reference< css::lang::XMultiServiceFactory >
        const & factory);

/**
 * This function gets the process service factory's default component context.
 *
 * Throws a RuntimeException if no component context can be obtained.
 */
COMPHELPER_DLLPUBLIC
const css::uno::Reference< css::uno::XComponentContext > &
getProcessComponentContext();

}

#endif // INCLUDED_COMPHELPER_PROCESSFACTORY_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

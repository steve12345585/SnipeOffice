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

#include <classes/taskcreator.hxx>
#include <services.h>
#include <taskcreatordefs.hxx>

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/TaskCreator.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <utility>

namespace framework{

/*-****************************************************************************************************
    @short      initialize instance with necessary information
    @descr      We need a valid uno service manager to create or instantiate new services.
                All other information to create frames or tasks come in on right interface methods.

    @param      xContext
                    points to the valid uno service manager
*//*-*****************************************************************************************************/
TaskCreator::TaskCreator( css::uno::Reference< css::uno::XComponentContext > xContext )
    : m_xContext    (std::move( xContext ))
{
}

/*-****************************************************************************************************
    @short      deinitialize instance
    @descr      We should release all used resource which are not needed any longer.
*//*-*****************************************************************************************************/
TaskCreator::~TaskCreator()
{
}

/*-****************************************************************************************************
    TODO document me
*//*-*****************************************************************************************************/
css::uno::Reference< css::frame::XFrame > TaskCreator::createTask( const OUString& sName, const utl::MediaDescriptor& rDescriptor )
{
    css::uno::Reference< css::lang::XSingleServiceFactory > xCreator;

    try
    {
        xCreator.set( m_xContext->getServiceManager()->createInstanceWithContext(IMPLEMENTATIONNAME_FWK_TASKCREATOR, m_xContext), css::uno::UNO_QUERY_THROW);
    }
    catch(const css::uno::Exception&)
    {}

    // no catch here ... without a task creator service we can't open ANY document window within the office.
    // That's IMHO not a good idea. Then we should accept the stacktrace showing us the real problem.
    // BTW: The used fallback creator service (IMPLEMENTATIONNAME_FWK_TASKCREATOR) is implemented in the same
    // library then these class here ... Why we should not be able to create it ?
    if ( ! xCreator.is())
        xCreator = css::frame::TaskCreator::create(m_xContext);

    css::uno::Sequence< css::uno::Any > lArgs
    {
        css::uno::Any(css::beans::NamedValue(ARGUMENT_PARENTFRAME, css::uno::Any(css::uno::Reference< css::frame::XFrame >( css::frame::Desktop::create( m_xContext ), css::uno::UNO_QUERY_THROW)))) ,
        css::uno::Any(css::beans::NamedValue(ARGUMENT_CREATETOPWINDOW, css::uno::Any(true))),
        css::uno::Any(css::beans::NamedValue(ARGUMENT_MAKEVISIBLE, css::uno::Any(false))),
        css::uno::Any(css::beans::NamedValue(ARGUMENT_SUPPORTPERSISTENTWINDOWSTATE, css::uno::Any(true))),
        css::uno::Any(css::beans::NamedValue(ARGUMENT_FRAMENAME, css::uno::Any(sName))),
        css::uno::Any(css::beans::NamedValue(ARGUMENT_HIDDENFORCONVERSION, css::uno::Any(rDescriptor.getUnpackedValueOrDefault(ARGUMENT_HIDDENFORCONVERSION, false))))
    };
    css::uno::Reference< css::frame::XFrame > xTask(xCreator->createInstanceWithArguments(lArgs), css::uno::UNO_QUERY_THROW);
    return xTask;
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

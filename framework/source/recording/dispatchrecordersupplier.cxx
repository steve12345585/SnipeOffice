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

#include <recording/dispatchrecordersupplier.hxx>

#include <com/sun/star/frame/XRecordableDispatch.hpp>

#include <vcl/svapp.hxx>
#include <cppuhelper/supportsservice.hxx>

namespace framework{

// XInterface, XTypeProvider, XServiceInfo

OUString SAL_CALL DispatchRecorderSupplier::getImplementationName()
{
    return u"com.sun.star.comp.framework.DispatchRecorderSupplier"_ustr;
}

sal_Bool SAL_CALL DispatchRecorderSupplier::supportsService( const OUString& sServiceName )
{
    return cppu::supportsService(this, sServiceName);
}

css::uno::Sequence< OUString > SAL_CALL DispatchRecorderSupplier::getSupportedServiceNames()
{
    return { u"com.sun.star.frame.DispatchRecorderSupplier"_ustr };
}


DispatchRecorderSupplier::DispatchRecorderSupplier()
{
}

/**
    @short  standard destructor
    @descr  We are a helper and not a real service. So we don't provide
            dispose() functionality. This supplier dies by ref count mechanism
            and should release all internal used ones too.
 */
DispatchRecorderSupplier::~DispatchRecorderSupplier()
{
    m_xDispatchRecorder = nullptr;
}

/**
    @short      set a new dispatch recorder on this supplier
    @descr      Because there can exist more than one recorder implementations
                (to generate java/basic/... scripts from recorded data) it must
                be possible to set it on a supplier.

    @see        getDispatchRecorder()

    @param      xRecorder
                the new recorder to set it
                <br><NULL/> isn't recommended, because recording without a
                valid recorder can't work. But it's not checked here. So user
                of this supplier can decide that without changing this
                implementation.

    @change     09.04.2002 by Andreas Schluens
 */
void SAL_CALL DispatchRecorderSupplier::setDispatchRecorder( const css::uno::Reference< css::frame::XDispatchRecorder >& xRecorder )
{
    SolarMutexGuard g;
    m_xDispatchRecorder=xRecorder;
}

/**
    @short      provides access to the dispatch recorder of this supplier
    @descr      Such recorder can be used outside to record dispatches.
                But normally he is used internally only. Of course he must used
                from outside to get the recorded data e.g. for saving it as a
                script.

    @see        setDispatchRecorder()

    @return     the internal used dispatch recorder
                <br>May it can be <NULL/> if no one was set before.

    @change     09.04.2002 by Andreas Schluens
 */
css::uno::Reference< css::frame::XDispatchRecorder > SAL_CALL DispatchRecorderSupplier::getDispatchRecorder()
{
    SolarMutexGuard g;
    return m_xDispatchRecorder;
}

/**
    @short      execute a dispatch request and record it
    @descr      If given dispatch object provides right recording interface it
                will be used. If it's not supported it record the pure dispatch
                parameters only. There is no code neither the possibility to
                check if recording is enabled or not.

    @param      aURL            the command URL
    @param      lArguments      optional arguments (see com.sun.star.document.MediaDescriptor for further information)
    @param      xDispatcher     the original dispatch object which should be recorded

    @change     09.04.2002 by Andreas Schluens
 */
void SAL_CALL DispatchRecorderSupplier::dispatchAndRecord( const css::util::URL&                                  aURL        ,
                                                           const css::uno::Sequence< css::beans::PropertyValue >& lArguments  ,
                                                           const css::uno::Reference< css::frame::XDispatch >&    xDispatcher )
{
    SolarMutexClearableGuard aReadLock;
    css::uno::Reference< css::frame::XDispatchRecorder > xRecorder = m_xDispatchRecorder;
    aReadLock.clear();

    // clear unspecific situations
    if (!xDispatcher.is())
        throw css::uno::RuntimeException(u"specification violation: dispatcher is NULL"_ustr, static_cast< ::cppu::OWeakObject* >(this));

    if (!xRecorder.is())
        throw css::uno::RuntimeException(u"specification violation: no valid dispatch recorder available"_ustr, static_cast< ::cppu::OWeakObject* >(this));

    // check, if given dispatch supports record functionality by itself ...
    // or must be wrapped.
    css::uno::Reference< css::frame::XRecordableDispatch > xRecordable(
        xDispatcher,
        css::uno::UNO_QUERY);

    if (xRecordable.is())
        xRecordable->dispatchAndRecord(aURL,lArguments,xRecorder);
    else
    {
        // There is no reason to wait for information about success
        // of this request. Because status information of a dispatch
        // are not guaranteed. So we execute it and record used
        // parameters only.
        xDispatcher->dispatch(aURL,lArguments);
        xRecorder->recordDispatch(aURL,lArguments);
    }
}

}   // namespace framework

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
framework_DispatchRecorderSupplier_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new framework::DispatchRecorderSupplier());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

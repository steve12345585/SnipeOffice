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

#include <com/sun/star/frame/XDispatchProviderInterception.hpp>
#include <com/sun/star/frame/XDispatchProviderInterceptor.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/DispatchDescriptor.hpp>

#include <rtl/ref.hxx>
#include <tools/wldcrd.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weakref.hxx>

#include <deque>
#include <string_view>

namespace framework{

class DispatchProvider;

/** @short      implements a helper to support interception with additional functionality.

    @descr      This helper implements the complete XDispatchProviderInterception interface with
                master/slave functionality AND using of optional features like registration of URL pattern!

    @attention  Don't use this class as direct member - use it dynamically. Do not derive from this class.
                We hold a weakreference to our owner not to our superclass.
 */
class InterceptionHelper final : public  ::cppu::WeakImplHelper<
                                     css::frame::XDispatchProvider,
                                     css::frame::XDispatchProviderInterception,
                                     css::lang::XEventListener >
{

    // structs, helper

    /** @short bind an interceptor component to its URL pattern registration. */
    struct InterceptorInfo
    {
        /** @short reference to the interceptor component. */
        css::uno::Reference< css::frame::XDispatchProvider > xInterceptor;

        /** @short it's registration for URL patterns.

            @descr If the interceptor component does not support the optional interface
                   XInterceptorInfo, it will be registered for one pattern "*" by default.
                   That would make it possible to handle it in the same manner then real
                   registered interceptor objects and we must not implement any special code. */
        css::uno::Sequence< OUString > lURLPattern;
    };

    /** @short implements a list of items of type InterceptorInfo, and provides some special
               functions on it.

        @descr Because interceptor objects can be registered for URL patterns,
               it supports a wildcard search on all list items.
     */
    class InterceptorList : public ::std::deque< InterceptorInfo >
    {
        public:

            /** @short search for an interceptor inside this list using it's reference.

                @param xInterceptor
                        points to the interceptor object, which should be located inside this list.

                @return An iterator object, which points directly to the located item inside this list.
                        In case no interceptor could be found, it points to the end of this list!
              */
            iterator findByReference(const css::uno::Reference< css::frame::XDispatchProviderInterceptor >& xInterceptor)
            {
                iterator pIt;
                for (pIt=begin(); pIt!=end(); ++pIt)
                {
                    if (pIt->xInterceptor == xInterceptor)
                        return pIt;
                }
                return end();
            }

            /** @short search for an interceptor inside this list using it's reference.

                @param sURL
                        URL which should match with a registered pattern.

                @return An iterator object, which points directly to the located item inside this list.
                        In case no interceptor could be found, it points to the end of this list!
              */
            iterator findByPattern(std::u16string_view sURL)
            {
                for (iterator pIt=begin(); pIt!=end(); ++pIt)
                {
                    for (const OUString& pattern : pIt->lURLPattern)
                    {
                        WildCard aPattern(pattern);
                        if (aPattern.Matches(sURL))
                            return pIt;
                    }
                }
                return end();
            }
    };

    // member

    private:

        /** @short reference to the frame, which uses this instance to implement its own interception.

            @descr We hold a weak reference only, to make disposing operations easy. */
        css::uno::WeakReference< css::frame::XFrame > m_xOwnerWeak;

        /** @short this interception helper implements the top level master of an interceptor list ...
                   but this member is the lowest possible slave! */
        rtl::Reference< DispatchProvider > m_xSlave;

        /** @short contains all registered interceptor objects. */
        InterceptorList m_lInterceptionRegs;

    // native interface

    public:

        /** @short creates a new interception helper instance.

            @param xOwner
                    points to the frame, which use this instances to support its own interception interfaces.

            @param xSlave
                    an outside creates dispatch provider, which has to be used here as lowest slave "interceptor".
         */
        InterceptionHelper(const css::uno::Reference< css::frame::XFrame >&            xOwner,
                           rtl::Reference< DispatchProvider >  xSlave);

    private:

        /** @short standard destructor.

            @descr This method destruct an instance of this class and clear some member.
                   This method is protected, because it's not allowed to use this class as a direct member!
                   You MUST use a dynamical instance (pointer). That's the reason for a protected dtor.
         */
        virtual ~InterceptionHelper() override;

    // uno interface

    public:

        // XDispatchProvider

        /** @short  query for a dispatch, which implements the requested feature.

            @descr  We search inside our list of interception registrations, to locate
                    any interested interceptor. In case no interceptor exists or nobody is
                    interested on this URL our lowest slave will be used.

            @param  aURL
                        describes the requested dispatch functionality.

            @param  sTargetFrameName
                        the name of the target frame or a special name like "_blank", "_top" ...
                        Won't be used here ... but may by one of our registered interceptor objects
                        or our slave.

            @param  nSearchFlags
                        optional search parameter for targeting, if sTargetFrameName isn't a special one.

            @return A valid dispatch object, if any interceptor or at least our slave is interested on the given URL;
                    or NULL otherwise.
         */
        virtual css::uno::Reference< css::frame::XDispatch > SAL_CALL queryDispatch(const css::util::URL&  aURL            ,
                                                                                    const OUString& sTargetFrameName,
                                                                                          sal_Int32        nSearchFlags    ) override;

        // XDispatchProvider

        /** @short implements an optimized queryDispatch() for remote.

            @descr It capsulate more than one queryDispatch() requests and return a list of dispatch objects
                   as result. Because both lists (in and out) correspond together, it's not allowed to
                   pack it - means suppress NULL references!

            @param lDescriptor
                    a list of queryDispatch() arguments.

            @return A list of dispatch objects.
         */
        virtual css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > > SAL_CALL queryDispatches(const css::uno::Sequence< css::frame::DispatchDescriptor >& lDescriptor) override;

        // XDispatchProviderInterception

        /** @short      register an interceptor.

            @descr      Somebody can register himself to intercept all or some special dispatches.
                        It's depend from his supported interfaces. If he implement XInterceptorInfo
                        he his called for some special URLs only - otherwise we call it for every request!

            @attention  We don't check for double registrations here!

            @param      xInterceptor
                        reference to interceptor, which wishes to be registered here.

            @throw      A RuntimeException if the given reference is NULL!
         */
        virtual void SAL_CALL registerDispatchProviderInterceptor(const css::uno::Reference< css::frame::XDispatchProviderInterceptor >& xInterceptor) override;

        // XDispatchProviderInterception

        /** @short      release an interceptor.

            @descr      Remove the registered interceptor from our internal list
                        and delete all special information about it.

            @param      xInterceptor
                        reference to the interceptor, which wishes to be deregistered.

            @throw      A RuntimeException if the given reference is NULL!
         */
        virtual void SAL_CALL releaseDispatchProviderInterceptor( const css::uno::Reference< css::frame::XDispatchProviderInterceptor >& xInterceptor ) override;

        // XEventListener

        /** @short      Is called from our owner frame, in case he will be disposed.

            @descr      We have to release all references to him then.
                        Normally we will die by ref count too...
         */
        virtual void SAL_CALL disposing(const css::lang::EventObject& aEvent) override;

        const rtl::Reference<DispatchProvider> & GetSlave() const { return m_xSlave; }

}; // class InterceptionHelper

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

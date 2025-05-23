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

#include <ucbhelper/interceptedinteraction.hxx>

#include <osl/diagnose.h>

namespace ucbhelper{

InterceptedInteraction::InterceptedInteraction()
{
}

void InterceptedInteraction::setInterceptedHandler(const css::uno::Reference< css::task::XInteractionHandler >& xInterceptedHandler)
{
    m_xInterceptedHandler = xInterceptedHandler;
}

void InterceptedInteraction::setInterceptions(::std::vector< InterceptedRequest >&& lInterceptions)
{
    m_lInterceptions = std::move(lInterceptions);
}

InterceptedInteraction::EInterceptionState InterceptedInteraction::intercepted(
    const InterceptedRequest&,
    const css::uno::Reference< css::task::XInteractionRequest >&)
{
    // default behaviour! see impl_interceptRequest() for further information ...
    return E_NOT_INTERCEPTED;
}

css::uno::Reference< css::task::XInteractionContinuation > InterceptedInteraction::extractContinuation(const css::uno::Sequence< css::uno::Reference< css::task::XInteractionContinuation > >& lContinuations,
                                                                                                       const css::uno::Type&                                                                   aType         )
{
    const css::uno::Reference< css::task::XInteractionContinuation >* pContinuations = std::find_if(lContinuations.begin(), lContinuations.end(),
        [&aType](const css::uno::Reference< css::task::XInteractionContinuation >& rContinuation) {
            css::uno::Reference< css::uno::XInterface > xCheck(rContinuation, css::uno::UNO_QUERY);
            return xCheck->queryInterface(aType).hasValue();
        });
    if (pContinuations != lContinuations.end())
        return *pContinuations;

    return css::uno::Reference< css::task::XInteractionContinuation >();
}

void SAL_CALL InterceptedInteraction::handle(const css::uno::Reference< css::task::XInteractionRequest >& xRequest)
{
    impl_handleDefault(xRequest);
}

void InterceptedInteraction::impl_handleDefault(const css::uno::Reference< css::task::XInteractionRequest >& xRequest)
{
    EInterceptionState eState = impl_interceptRequest(xRequest);

    switch(eState)
    {
        case E_NOT_INTERCEPTED:
        {
            // Non of the intercepted requests match to the given one.
            // => forward request to the internal wrapped handler - if there is one.
            if (m_xInterceptedHandler.is())
                m_xInterceptedHandler->handle(xRequest);
        }
        break;

        case E_NO_CONTINUATION_FOUND:
        {
            // Runtime error! The defined continuation could not be located
            // inside the set of available continuations of the incoming request.
            // What's wrong - the interception list or the request?
            OSL_FAIL("InterceptedInteraction::handle()\nCould intercept this interaction request - but can't locate the right continuation!");
        }
        break;

        case E_INTERCEPTED:
        break;
    }
}

InterceptedInteraction::EInterceptionState InterceptedInteraction::impl_interceptRequest(const css::uno::Reference< css::task::XInteractionRequest >& xRequest)
{
    css::uno::Any                                                                    aRequest       = xRequest->getRequest();
    const css::uno::Type&                                                            aRequestType   = aRequest.getValueType();
    css::uno::Sequence< css::uno::Reference< css::task::XInteractionContinuation > > lContinuations = xRequest->getContinuations();

    // check against the list of static requests
    auto pIt = std::find_if(m_lInterceptions.begin(), m_lInterceptions.end(),
        [&aRequestType](const InterceptedRequest& rInterception) {
            // check the request
            // don't change intercepted and request type here -> it will check the wrong direction!
            return rInterception.Request.getValueType().isAssignableFrom(aRequestType);
        });

    if (pIt != m_lInterceptions.end()) // intercepted ...
    {
        const InterceptedRequest& rInterception = *pIt;

        // Call they might existing derived class, so they can handle that by its own.
        // If it's not interested on that (maybe it's not overwritten and the default implementation
        // returns E_NOT_INTERCEPTED as default) -> search required continuation
        EInterceptionState eState = intercepted(rInterception, xRequest);
        if (eState != E_NOT_INTERCEPTED)
            return eState;

        css::uno::Reference< css::task::XInteractionContinuation > xContinuation = InterceptedInteraction::extractContinuation(lContinuations, rInterception.Continuation);
        if (xContinuation.is())
        {
            xContinuation->select();
            return E_INTERCEPTED;
        }

        // Can be reached only, if the request does not support the given continuation!
        // => RuntimeError!?
        return E_NO_CONTINUATION_FOUND;
    }

    return E_NOT_INTERCEPTED;
}

} // namespace ucbhelper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

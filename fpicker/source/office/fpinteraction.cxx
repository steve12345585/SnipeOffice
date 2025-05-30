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

#include "fpinteraction.hxx"
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <com/sun/star/task/XInteractionAbort.hpp>
#include <com/sun/star/task/XInteractionApprove.hpp>
#include <com/sun/star/task/XInteractionDisapprove.hpp>
#include <com/sun/star/task/XInteractionRetry.hpp>

#include <sal/log.hxx>
#include <utility>


namespace svt
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::task;
    using namespace ::com::sun::star::ucb;

    OFilePickerInteractionHandler::OFilePickerInteractionHandler( css::uno::Reference< css::task::XInteractionHandler > _xMaster )
        :m_xMaster(std::move( _xMaster ))
        ,m_bUsed( false )
        ,m_eInterceptions( OFilePickerInteractionHandler::E_NOINTERCEPTION )
    {
        SAL_WARN_IF( !m_xMaster.is(), "fpicker.office", "OFilePickerInteractionHandler::OFilePickerInteractionHandler: invalid master handler!" );
    }


    OFilePickerInteractionHandler::~OFilePickerInteractionHandler( )
    {
    }


    void SAL_CALL OFilePickerInteractionHandler::handle( const Reference< XInteractionRequest >& _rxRequest )
    {
        if (!_rxRequest.is())
            return;

        m_bUsed = true;

        // extract some generic continuations ... might we need it later
        // if something goes wrong.
        Reference< XInteractionAbort >       xAbort;
        Reference< XInteractionApprove >     xApprove;
        Reference< XInteractionDisapprove >  xDisapprove;
        Reference< XInteractionRetry >       xRetry;

        const Sequence< Reference< XInteractionContinuation > > lConts = _rxRequest->getContinuations();
        for (const Reference< XInteractionContinuation >& rCont : lConts)
        {
            if (!xAbort.is())
                xAbort.set(rCont, UNO_QUERY);
            if (!xApprove.is())
                xApprove.set(rCont, UNO_QUERY);
            if (!xDisapprove.is())
                xDisapprove.set(rCont, UNO_QUERY);
            if (!xRetry.is())
                xRetry.set(rCont, UNO_QUERY);
        }

        // safe the original request for later analyzing!
        m_aException = _rxRequest->getRequest();

        // intercept some interesting interactions

        // The "does not exist" interaction will be suppressed here completely.
        if (m_eInterceptions & OFilePickerInteractionHandler::E_DOESNOTEXIST)
        {
            InteractiveIOException aIoException;
            if (
                (m_aException             >>= aIoException     ) &&
                (IOErrorCode_NOT_EXISTING  == aIoException.Code)
               )
            {
                if (xAbort.is())
                    xAbort->select();
                return;
            }
        }

        // no master => abort this operation ...
        if (!m_xMaster.is())
        {
            if (xAbort.is())
                xAbort->select();
            return;
        }

        // forward it to our master - so he can handle all
        // not interesting interactions :-)
        m_xMaster->handle(_rxRequest);
    }


    void OFilePickerInteractionHandler::enableInterceptions( EInterceptedInteractions eInterceptions )
    {
        m_eInterceptions = eInterceptions;
    }


    void OFilePickerInteractionHandler::resetUseState()
    {
        m_bUsed = false;
    }


    void OFilePickerInteractionHandler::forgetRequest()
    {
        m_aException = Any();
    }


    bool OFilePickerInteractionHandler::wasAccessDenied() const
    {
        InteractiveIOException aIoException;
        return (m_aException              >>= aIoException     ) &&
               (IOErrorCode_ACCESS_DENIED  == aIoException.Code);
    }


}   // namespace svt


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

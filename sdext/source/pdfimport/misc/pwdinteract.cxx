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

#include <sal/config.h>

#include <cassert>
#include <mutex>

#include <pdfihelper.hxx>

#include <com/sun/star/task/ErrorCodeRequest.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/task/XInteractionRequest.hpp>
#include <com/sun/star/task/XInteractionPassword.hpp>
#include <com/sun/star/task/DocumentPasswordRequest2.hpp>

#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>
#include <comphelper/errcode.hxx>

using namespace com::sun::star;

namespace
{

class PDFPasswordRequest:
    public cppu::WeakImplHelper<
        task::XInteractionRequest, task::XInteractionPassword >
{
private:
    mutable std::mutex            m_aMutex;
    uno::Any                      m_aRequest;
    OUString                 m_aPassword;
    bool                          m_bSelected;

public:
    explicit PDFPasswordRequest(bool bFirstTry, const OUString& rName);
    PDFPasswordRequest(const PDFPasswordRequest&) = delete;
    PDFPasswordRequest& operator=(const PDFPasswordRequest&) = delete;

    // XInteractionRequest
    virtual uno::Any SAL_CALL getRequest(  ) override;
    virtual uno::Sequence< uno::Reference< task::XInteractionContinuation > > SAL_CALL getContinuations(  ) override;

    // XInteractionPassword
    virtual void SAL_CALL setPassword( const OUString& rPwd ) override;
    virtual OUString SAL_CALL getPassword() override;

    // XInteractionContinuation
    virtual void SAL_CALL select() override;

    bool isSelected() const { std::scoped_lock const guard( m_aMutex ); return m_bSelected; }

private:
    virtual ~PDFPasswordRequest() override {}
};

PDFPasswordRequest::PDFPasswordRequest( bool bFirstTry, const OUString& rName ) :
    m_aRequest(
        uno::Any(
            task::DocumentPasswordRequest2(
                OUString(), uno::Reference< uno::XInterface >(),
                task::InteractionClassification_QUERY,
                (bFirstTry
                 ? task::PasswordRequestMode_PASSWORD_ENTER
                 : task::PasswordRequestMode_PASSWORD_REENTER),
                rName, false))),
    m_bSelected(false)
{}

uno::Any PDFPasswordRequest::getRequest()
{
    return m_aRequest;
}

uno::Sequence< uno::Reference< task::XInteractionContinuation > > PDFPasswordRequest::getContinuations()
{
    return { this };
}

void PDFPasswordRequest::setPassword( const OUString& rPwd )
{
    std::scoped_lock const guard( m_aMutex );

    m_aPassword = rPwd;
}

OUString PDFPasswordRequest::getPassword()
{
    std::scoped_lock const guard( m_aMutex );

    return m_aPassword;
}

void PDFPasswordRequest::select()
{
    std::scoped_lock const guard( m_aMutex );

    m_bSelected = true;
}

} // namespace

namespace pdfi
{

bool getPassword( const uno::Reference< task::XInteractionHandler >& xHandler,
                  OUString&                                     rOutPwd,
                  bool                                               bFirstTry,
                  const OUString&                               rDocName
                  )
{
    bool bSuccess = false;

    rtl::Reference< PDFPasswordRequest > xReq(
        new PDFPasswordRequest( bFirstTry, rDocName ) );
    try
    {
        xHandler->handle( xReq );
    }
    catch( uno::Exception& )
    {
    }

    if( xReq->isSelected() )
    {
        bSuccess = true;
        rOutPwd = xReq->getPassword();
    }

    return bSuccess;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

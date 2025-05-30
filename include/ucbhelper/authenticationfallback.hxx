/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ref.hxx>
#include <ucbhelper/interactionrequest.hxx>
#include <ucbhelper/ucbhelperdllapi.h>


namespace ucbhelper {

/**
  * This class implements a simple authentication interaction request used
  * when programmatically authentication cannot succeed.
  *
  * Read-only values : instructions, url
  * Read-write values: code
  */
class UCBHELPER_DLLPUBLIC AuthenticationFallbackRequest final : public ucbhelper::InteractionRequest
{
private:
    rtl::Reference< ucbhelper::InteractionAuthFallback > m_xAuthFallback;

public:
    /**
      * Constructor.
      *
      * @param rInstructions instructions to be followed by the user
      * @param rURL contains a URL for which authentication is requested.
      */
    AuthenticationFallbackRequest( const OUString & rInstructions,
                                 const OUString & rURL );

    const rtl::Reference< ucbhelper::InteractionAuthFallback >&
        getAuthFallbackInter( ) const { return m_xAuthFallback; }

};

} // namespace ucbhelper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include "submission_get.hxx"
#include "serialization_urlencoded.hxx"

#include <rtl/strbuf.hxx>
#include <osl/diagnose.h>
#include <ucbhelper/content.hxx>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <comphelper/diagnose_ex.hxx>

using namespace css::uno;
using namespace css::ucb;
using namespace css::task;
using namespace css::io;
using namespace ucbhelper;


CSubmissionGet::CSubmissionGet(std::u16string_view aURL, const css::uno::Reference< css::xml::dom::XDocumentFragment >& aFragment)
    : CSubmission(aURL, aFragment)
{
}

CSubmission::SubmissionResult CSubmissionGet::submit(const css::uno::Reference< css::task::XInteractionHandler >& aInteractionHandler)
{
    // GET always uses application/x-www-formurlencoded
    CSerializationURLEncoded aSerialization;
    aSerialization.setSource(m_aFragment);
    aSerialization.serialize();

    css::uno::Reference< XInputStream > aInStream = aSerialization.getInputStream();

    // create a commandEnvironment and use the default interaction handler
    rtl::Reference<CCommandEnvironmentHelper> pHelper = new CCommandEnvironmentHelper;
    if( aInteractionHandler.is() )
        pHelper->m_aInteractionHandler = aInteractionHandler;
    else
        pHelper->m_aInteractionHandler.set(
            css::task::InteractionHandler::createWithParent(m_xContext, nullptr), UNO_QUERY_THROW);
    rtl::Reference<CProgressHandlerHelper> pProgressHelper = new CProgressHandlerHelper;
    pHelper->m_aProgressHandler.set(pProgressHelper);

    // UCB has ownership of environment...

    // append query string to the URL
    try {
        OStringBuffer aUTF8QueryURL(OUStringToOString(m_aURLObj.GetMainURL(INetURLObject::DecodeMechanism::NONE),
            RTL_TEXTENCODING_UTF8));
        OStringBuffer aQueryString;

        const sal_Int32 size = 1024;
        sal_Int32 n = 0;
        Sequence< sal_Int8 > aByteBuffer(size);
        while ((n = aInStream->readSomeBytes(aByteBuffer, size-1)) != 0)
            aQueryString.append(reinterpret_cast<char const *>(aByteBuffer.getConstArray()), n);
        if (!aQueryString.isEmpty() && m_aURLObj.GetProtocol() != INetProtocol::File)
        {
            aUTF8QueryURL.append("?" + aQueryString);
        }
        OUString aQueryURL = OStringToOUString(aUTF8QueryURL, RTL_TEXTENCODING_UTF8);
        ucbhelper::Content aContent(aQueryURL, pHelper, m_xContext);
        // get reply
        try {
            m_aResultStream = aContent.openStream();
        } catch (const Exception&) {
            OSL_FAIL("Cannot open reply stream from content");
        }
    } catch (const Exception&)
    {
        // XXX
        TOOLS_WARN_EXCEPTION( "forms.misc", "Exception during UCB operation.");
        return UNKNOWN_ERROR;
    }

    return SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

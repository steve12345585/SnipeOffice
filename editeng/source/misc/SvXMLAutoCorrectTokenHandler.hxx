/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/types.h>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <com/sun/star/xml/sax/FastToken.hpp>
#include <sax/fastattribs.hxx>

using namespace css::xml::sax;
using namespace ::xmloff::token;

enum SvXMLAutoCorrectToken : sal_Int32
{
    NAMESPACE = FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST, //65553
    ABBREVIATED_NAME = FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST | XML_ABBREVIATED_NAME,   //65655
    BLOCK = FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST | XML_BLOCK, //65791
    BLOCKLIST = FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST | XML_BLOCK_LIST, //65792
    NAME = FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST | XML_NAME    //66737
};

class SvXMLAutoCorrectTokenHandler :
        public sax_fastparser::FastTokenHandlerBase
{
public:
    explicit SvXMLAutoCorrectTokenHandler();
    virtual ~SvXMLAutoCorrectTokenHandler() override;

    //XFastTokenHandler
    virtual sal_Int32 SAL_CALL getTokenFromUTF8( const css::uno::Sequence< sal_Int8 >& Identifier ) override;
    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getUTF8Identifier( sal_Int32 Token ) override;

    // Much faster direct C++ shortcut to the method that matters
    virtual sal_Int32 getTokenDirect(std::string_view token) const override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include "querytokenizer.hxx"

#include <osl/diagnose.h>
#include <o3tl/string_view.hxx>


namespace filter::config{


QueryTokenizer::QueryTokenizer(std::u16string_view sQuery)
    : m_bValid(true)
{
    sal_Int32 token = 0;
    while(token != -1)
    {
        std::u16string_view sToken = o3tl::getToken(sQuery,0, ':', token);
        if (!sToken.empty())
        {
            sal_Int32 nIdx{ 0 };
            const OUString sKey{ o3tl::getToken(sToken, 0, '=', nIdx) };
            const OUString sVal{ o3tl::getToken(sToken, 0, ':', nIdx) };

            if (sKey.isEmpty())
                m_bValid = false;
            OSL_ENSURE(m_bValid, "QueryTokenizer::QueryTokenizer() Found non boolean query parameter ... but its key is empty. Will be ignored!");

            if (find(sKey) != end())
                m_bValid = false;
            OSL_ENSURE(m_bValid, "QueryTokenizer::QueryTokenizer() Query contains same param more than once. Last one wins :-)");

            (*this)[sKey] = sVal;
        }
    }
}


QueryTokenizer::~QueryTokenizer()
{
    /*TODO*/
}


bool QueryTokenizer::valid() const
{
    return m_bValid;
}

} // namespace filter::config

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

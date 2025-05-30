
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

#include <comphelper/string.hxx>
#include <comphelper/processfactory.hxx>
#include <connectivity/dbexception.hxx>
#include <sal/log.hxx>
#include <o3tl/numeric.hxx>

#include "utils.hxx"

using namespace dbahsql;

//Convert ascii escaped unicode to utf-8
OUString utils::convertToUTF8(std::string_view original)
{
    OUString res = OStringToOUString(original, RTL_TEXTENCODING_UTF8);
    for (sal_Int32 i = 0;;)
    {
        i = res.indexOf("\\u", i);
        if (i == -1)
        {
            break;
        }
        i += 2;
        if (res.getLength() - i >= 4)
        {
            bool escape = true;
            sal_Unicode c = 0;
            for (sal_Int32 j = 0; j != 4; ++j)
            {
                auto const n = o3tl::convertToHex<int>(res[i + j]);
                if (n == -1)
                {
                    escape = false;
                    break;
                }
                c = (c << 4) | n;
            }
            if (escape)
            {
                i -= 2;
                res = res.replaceAt(i, 6, rtl::OUStringChar(c));
                ++i;
            }
        }
    }
    return res;
}

OUString utils::getTableNameFromStmt(std::u16string_view sSql)
{
    std::vector<OUString> stmtComponents = comphelper::string::split(sSql, sal_Unicode(u' '));
    assert(stmtComponents.size() > 2);
    auto wordIter = stmtComponents.begin();

    if (*wordIter == "CREATE" || *wordIter == "ALTER")
        ++wordIter;
    if (*wordIter == "CACHED")
        ++wordIter;
    if (*wordIter == "TABLE")
        ++wordIter;

    // it may contain spaces if it's put into apostrophes.
    if (wordIter->indexOf("\"") >= 0)
    {
        size_t nAposBegin = sSql.find('"');
        assert(nAposBegin != std::u16string_view::npos); // make coverity happy
        size_t nAposEnd = nAposBegin;
        bool bProperEndAposFound = false;
        while (!bProperEndAposFound)
        {
            nAposEnd = sSql.find('"', nAposEnd + 1);
            if (nAposEnd == std::u16string_view::npos)
            {
                SAL_WARN("dbaccess", "no matching \"");
                return OUString();
            }
            if (sSql[nAposEnd - 1] != u'\\')
                bProperEndAposFound = true;
        }
        std::u16string_view result = sSql.substr(nAposBegin, nAposEnd - nAposBegin + 1);
        return OUString(result);
    }

    // next word is the table's name
    // it might stuck together with the column definitions.
    sal_Int32 nParenPos = wordIter->indexOf("(");
    if (nParenPos > 0)
        return wordIter->copy(0, nParenPos);
    else
        return *wordIter;
}

void utils::ensureFirebirdTableLength(std::u16string_view sName)
{
    if (sName.size() > 30) // Firebird limitation
    {
        static constexpr OUStringLiteral NAME_TOO_LONG
            = u"Firebird 3 doesn't support object (table, field) names "
              "of  more than 30 characters; please shorten your object "
              "names in the original file and try again.";
        dbtools::throwGenericSQLException(NAME_TOO_LONG,
                                          ::comphelper::getProcessComponentContext());
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

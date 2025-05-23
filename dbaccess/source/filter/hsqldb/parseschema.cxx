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

#include <string_view>

#include "parseschema.hxx"
#include "fbcreateparser.hxx"
#include "fbalterparser.hxx"
#include "utils.hxx"

#include <com/sun/star/io/TextInputStream.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <sal/log.hxx>
#include <connectivity/dbexception.hxx>
#include <utility>

namespace
{
using namespace ::comphelper;

using IndexVector = std::vector<sal_Int32>;

class IndexStmtParser
{
private:
    OUString m_sql;

public:
    IndexStmtParser(OUString sSql)
        : m_sql(std::move(sSql))
    {
    }

    bool isIndexStatement() const
    {
        return m_sql.startsWith("SET TABLE") && m_sql.indexOf("INDEX") >= 0;
    }

    IndexVector getIndexes() const
    {
        assert(isIndexStatement());

        std::u16string_view sIndexPart = m_sql.subView(m_sql.indexOf("INDEX") + 5);
        size_t nQuotePos = sIndexPart.find('\'');
        if (nQuotePos == std::u16string_view::npos)
            nQuotePos = 0;
        else
            ++nQuotePos;
        std::u16string_view sIndexNums
            = sIndexPart.substr(nQuotePos, sIndexPart.rfind('\'') - nQuotePos);

        std::vector<OUString> sIndexes = string::split(sIndexNums, u' ');
        IndexVector indexes;
        for (const auto& sIndex : sIndexes)
            indexes.push_back(sIndex.toInt32());

        // ignore last element
        // TODO this is an identity peek, which indicates the value of the next
        // identity. At the current state all migrated identities start with 0.
        indexes.pop_back();

        return indexes;
    }

    OUString getTableName() const
    {
        // SET TABLE <tableName> or SET TABLE "<multi word table name>"
        OUString sName = string::split(m_sql, u' ')[2];
        if (sName.indexOf('"') >= 0)
        {
            // Table name with string delimiter
            sName = "\"" + string::split(m_sql, u'"')[1] + "\"";
        }
        return sName;
    }
};

OUString lcl_createAlterForeign(std::u16string_view sForeignPart, std::u16string_view sTableName)
{
    return OUString::Concat("ALTER TABLE ") + sTableName + " ADD " + sForeignPart;
}

} // anonymous namespace

namespace dbahsql
{
using namespace css::io;
using namespace css::uno;
using namespace css::embed;

SchemaParser::SchemaParser(Reference<XStorage>& rStorage)
    : m_rStorage(rStorage)
{
}

void SchemaParser::parseSchema()
{
    assert(m_rStorage);

    static constexpr OUString SCHEMA_FILENAME = u"script"_ustr;
    if (!m_rStorage->hasByName(SCHEMA_FILENAME))
    {
        SAL_WARN("dbaccess", "script file does not exist in storage during hsqldb import");
        return;
    }

    Reference<XStream> xStream(m_rStorage->openStreamElement(SCHEMA_FILENAME, ElementModes::READ));

    const Reference<XComponentContext>& rContext = comphelper::getProcessComponentContext();
    Reference<XTextInputStream2> xTextInput = TextInputStream::create(rContext);
    xTextInput->setEncoding(u"UTF-8"_ustr);
    xTextInput->setInputStream(xStream->getInputStream());

    while (!xTextInput->isEOF())
    {
        // every line contains exactly one DDL statement
        OUString sSql = utils::convertToUTF8(
            OUStringToOString(xTextInput->readLine(), RTL_TEXTENCODING_UTF8));

        IndexStmtParser indexParser{ sSql };
        if (indexParser.isIndexStatement())
        {
            m_Indexes[indexParser.getTableName()] = indexParser.getIndexes();
        }
        else if (sSql.startsWith("SET") || sSql.startsWith("CREATE USER")
                 || sSql.startsWith("CREATE SCHEMA") || sSql.startsWith("GRANT"))
            continue;
        else if (sSql.startsWith("CREATE CACHED TABLE") || sSql.startsWith("CREATE TABLE"))
        {
            FbCreateStmtParser aCreateParser;
            aCreateParser.parse(sSql);

            for (const auto& foreignParts : aCreateParser.getForeignParts())
            {
                m_sAlterStatements.push_back(
                    lcl_createAlterForeign(foreignParts, aCreateParser.getTableName()));
            }

            sSql = aCreateParser.compose();

            // save column definitions
            m_ColumnTypes[aCreateParser.getTableName()] = aCreateParser.getColumnDef();

            m_sCreateStatements.push_back(sSql);
        }
        else if (sSql.startsWith("ALTER"))
        {
            FbAlterStmtParser aAlterParser;
            aAlterParser.parse(sSql);
            OUString parsedStmt = aAlterParser.compose();

            if (!parsedStmt.isEmpty())
                m_sAlterStatements.push_back(parsedStmt);
        }
        else if (sSql.startsWith("CREATE VIEW"))
            m_sCreateStatements.push_back(sSql);
    }
}

std::vector<ColumnDefinition> SchemaParser::getTableColumnTypes(const OUString& sTableName) const
{
    if (m_ColumnTypes.count(sTableName) < 1)
    {
        static constexpr OUString NOT_EXIST
            = u"Internal error while getting column information of table"_ustr;
        SAL_WARN("dbaccess", NOT_EXIST << ". Table name is: " << sTableName);
        dbtools::throwGenericSQLException(NOT_EXIST, ::comphelper::getProcessComponentContext());
    }
    return m_ColumnTypes.at(sTableName);
}

const std::map<OUString, std::vector<sal_Int32>>& SchemaParser::getTableIndexes() const
{
    return m_Indexes;
}

const std::map<OUString, std::vector<OUString>>& SchemaParser::getPrimaryKeys() const
{
    return m_PrimaryKeys;
}

} // namespace dbahsql

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

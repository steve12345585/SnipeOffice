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


#include "NTable.hxx"
#include "NColumns.hxx"

#include <com/sun/star/sdbc/XRow.hpp>

using namespace connectivity;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdbc;
using namespace connectivity::evoab;

OEvoabTable::OEvoabTable( sdbcx::OCollection* _pTables,
                OEvoabConnection* _pConnection,
                const OUString& Name,
                const OUString& Type,
                const OUString& Description ,
                const OUString& SchemaName,
                const OUString& CatalogName
                ) : OEvoabTable_TYPEDEF(_pTables,true,
                                  Name,
                                  Type,
                                  Description,
                                  SchemaName,
                                  CatalogName),
                    m_pConnection(_pConnection)
{
    construct();
}

void OEvoabTable::refreshColumns()
{
    ::std::vector< OUString> aVector;

    if (!isNew())
    {
        Reference< XResultSet > xResult = m_pConnection->getMetaData()->getColumns(
                Any(), m_SchemaName, m_Name, u"%"_ustr);

        if (xResult.is())
        {
            Reference< XRow > xRow(xResult, UNO_QUERY);
            while (xResult->next())
                    aVector.push_back(xRow->getString(4));
        }
    }
    if (m_xColumns)
        m_xColumns->reFill(aVector);
    else
        m_xColumns.reset(new OEvoabColumns(this,m_aMutex,aVector));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

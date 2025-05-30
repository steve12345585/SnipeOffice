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

#include <flat/ECatalog.hxx>

#include <flat/EConnection.hxx>
#include <flat/ETables.hxx>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;

using namespace connectivity::flat;

OFlatCatalog::OFlatCatalog(OFlatConnection* _pCon)
    : file::OFileCatalog(_pCon)
{
}

void OFlatCatalog::refreshTables()
{
    ::std::vector<OUString> aVector;
    Sequence<OUString> aTypes;
    Reference<XResultSet> xResult = m_xMetaData->getTables(Any(), u"%"_ustr, u"%"_ustr, aTypes);

    if (xResult.is())
    {
        Reference<XRow> xRow(xResult, UNO_QUERY);
        while (xResult->next())
            aVector.push_back(xRow->getString(3));
    }
    if (m_pTables)
        m_pTables->reFill(aVector);
    else
        m_pTables.reset(new OFlatTables(m_xMetaData, *this, m_aMutex, aVector));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

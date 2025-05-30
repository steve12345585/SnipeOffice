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


#include "MacabCatalog.hxx"
#include "MacabConnection.hxx"
#include "MacabTables.hxx"
#include <com/sun/star/sdbc/XRow.hpp>

using namespace connectivity::macab;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
using namespace ::cppu;


MacabCatalog::MacabCatalog(MacabConnection* _pCon)
        : connectivity::sdbcx::OCatalog(_pCon),
          m_pConnection(_pCon)
{
}

void MacabCatalog::refreshTables()
{
    ::std::vector< OUString> aVector;
    Sequence< OUString > aTypes { "%" };
    Reference< XResultSet > xResult = m_xMetaData->getTables(
        Any(), "%", "%", aTypes);

    if (xResult.is())
    {
        Reference< XRow > xRow(xResult,UNO_QUERY);
        OUString aName;
        // const OUString& sDot = MacabCatalog::getDot();

        while (xResult->next())
        {
            // aName = xRow->getString(2);
            // aName += sDot;
            aName = xRow->getString(3);
            aVector.push_back(aName);
        }
    }
    if (m_pTables)
        m_pTables->reFill(aVector);
    else
        m_pTables.reset( new MacabTables(m_xMetaData,*this,m_aMutex,aVector) );
}

void MacabCatalog::refreshViews()
{
}

void MacabCatalog::refreshGroups()
{
}

void MacabCatalog::refreshUsers()
{
}

const OUString& MacabCatalog::getDot()
{
    static constexpr OUString sDot = u"."_ustr;
    return sDot;
}


// XTablesSupplier
Reference< XNameAccess > SAL_CALL MacabCatalog::getTables(  )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(rBHelper.bDisposed);

    try
    {
        if (!m_pTables)
            refreshTables();
    }
    catch( const RuntimeException& )
    {
        // allowed to leave this method
        throw;
    }
    catch( const Exception& )
    {
        // allowed
    }

    return m_pTables.get();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

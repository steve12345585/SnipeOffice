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

#include "NCatalog.hxx"
#include "NConnection.hxx"
#include "NTables.hxx"
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>


using namespace connectivity::evoab;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;

OEvoabCatalog::OEvoabCatalog(OEvoabConnection* _pCon) :
    connectivity::sdbcx::OCatalog(_pCon)
    ,m_pConnection(_pCon)
{
}
void OEvoabCatalog::refreshTables()
{
    ::std::vector< OUString> aVector;
    Sequence< OUString > aTypes { u"TABLE"_ustr };
    Reference< XResultSet > xResult = m_xMetaData->getTables(
        Any(), u"%"_ustr, u"%"_ustr, aTypes);

    if(xResult.is())
    {
        Reference< XRow > xRow(xResult,UNO_QUERY);
        OUString aName;

        while(xResult->next())
        {
            aName = xRow->getString(3);
            aVector.push_back(aName);
        }
    }
    if(m_pTables)
        m_pTables->reFill(aVector);
    else
        m_pTables.reset( new OEvoabTables(m_xMetaData,*this,m_aMutex,aVector) );
}
// XTablesSupplier
Reference< XNameAccess > SAL_CALL  OEvoabCatalog::getTables(  )
{
        ::osl::MutexGuard aGuard(m_aMutex);

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

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Indexes.hxx"

using namespace ::connectivity;
using namespace ::connectivity::firebird;

using namespace ::osl;

using namespace ::com::sun::star;
using namespace ::com::sun::star::sdbc;

Indexes::Indexes(Table* pTable, Mutex& rMutex, const std::vector<OUString>& rVector)
    : OIndexesHelper(pTable, rMutex, rVector)
    , m_pTable(pTable)
{
}

// XDrop
void Indexes::dropObject(sal_Int32 /*nPosition*/, const OUString& sIndexName)
{
    OUString sSql("DROP INDEX \"" + sIndexName + "\"");
    m_pTable->getConnection()->createStatement()->execute(sSql);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

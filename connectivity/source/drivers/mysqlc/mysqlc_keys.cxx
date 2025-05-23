/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mysqlc_keys.hxx"
#include "mysqlc_table.hxx"

connectivity::mysqlc::Keys::Keys(Table* pTable, osl::Mutex& rMutex,
                                 const ::std::vector<OUString>& rNames)
    : OKeysHelper(pTable, rMutex, rNames)
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

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

#include <AutoRetrievingBase.hxx>

#include <osl/diagnose.h>
#include <o3tl/string_view.hxx>

namespace connectivity
{
    OUString OAutoRetrievingBase::getTransformedGeneratedStatement(const OUString& _sInsertStatement) const
    {
        OSL_ENSURE( m_bAutoRetrievingEnabled,"Illegal call here. isAutoRetrievingEnabled is false!");
        OUString sStmt = _sInsertStatement.toAsciiUpperCase();
        if ( sStmt.startsWith("INSERT") )
        {
            static const char sTable[] = "$table";
            const sal_Int32 nColumnIndex {m_sGeneratedValueStatement.indexOf("$column")};
            if ( nColumnIndex>=0 )
            { // we need a column
            }
            const sal_Int32 nTableIndex {m_sGeneratedValueStatement.indexOf(sTable)};
            if ( nTableIndex>=0 )
            { // we need a table name
                sal_Int32 nIntoIndex = sStmt.indexOf("INTO ") + 5;
                while (nIntoIndex<sStmt.getLength() && sStmt[nIntoIndex]==' ') ++nIntoIndex;
                const std::u16string_view sTableName = o3tl::getToken(sStmt, 0, ' ', nIntoIndex);
                return m_sGeneratedValueStatement.replaceAt(nTableIndex, strlen(sTable), sTableName);
            }
            return m_sGeneratedValueStatement;
        }
        return OUString();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

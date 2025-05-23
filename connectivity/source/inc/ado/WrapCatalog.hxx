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
#pragma once

#include <sal/config.h>

#include <string_view>

#include <ado/WrapTypeDefs.hxx>

namespace connectivity::ado
{
    class WpADOCatalog : public WpOLEBase<_ADOCatalog>
    {
    public:
        WpADOCatalog(_ADOCatalog* pInt = nullptr)  :   WpOLEBase<_ADOCatalog>(pInt){}
        WpADOCatalog(const WpADOCatalog& rhs) : WpOLEBase<_ADOCatalog>(rhs) {}

        WpADOCatalog& operator=(const WpADOCatalog& rhs)
            {WpOLEBase<_ADOCatalog>::operator=(rhs); return *this;}

        OUString GetObjectOwner(std::u16string_view _rName, ObjectTypeEnum _eNum);

        void putref_ActiveConnection(IDispatch* pCon);
        WpADOTables     get_Tables();
        WpADOViews      get_Views();
        WpADOGroups     get_Groups();
        WpADOUsers      get_Users();
        ADOProcedures*  get_Procedures();
        void Create();
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

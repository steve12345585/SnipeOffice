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

#include <file/FConnection.hxx>

namespace connectivity::dbase
{
    class ODriver;
    class ODbaseConnection : public file::OConnection
    {
    protected:
        virtual ~ODbaseConnection() override;
    public:
        ODbaseConnection(ODriver*   _pDriver);
        // XServiceInfo
        DECLARE_SERVICE_INFO();

        // XConnection
        virtual css::uno::Reference< css::sdbc::XDatabaseMetaData > SAL_CALL getMetaData(  ) override;
        virtual css::uno::Reference< css::sdbcx::XTablesSupplier > createCatalog() override;
        virtual css::uno::Reference< css::sdbc::XStatement > SAL_CALL createStatement(  ) override;
        virtual css::uno::Reference< css::sdbc::XPreparedStatement > SAL_CALL prepareStatement( const OUString& sql ) override;
        virtual css::uno::Reference< css::sdbc::XPreparedStatement > SAL_CALL prepareCall( const OUString& sql ) override;
    };

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

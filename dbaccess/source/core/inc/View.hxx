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

#include <connectivity/sdbcx/VView.hxx>

#include <com/sun/star/sdbcx/XAlterView.hpp>
#include <com/sun/star/sdb/tools/XViewAccess.hpp>

#include <comphelper/uno3.hxx>
#include <cppuhelper/implbase1.hxx>

namespace dbaccess
{

    // View
    typedef ::connectivity::sdbcx::OView                                View_Base;
    typedef ::cppu::ImplHelper1< css::sdbcx::XAlterView >  View_IBASE;
    class View :public View_Base
                ,public View_IBASE
    {
    public:
        View(
            const css::uno::Reference< css::sdbc::XConnection >& _rxConnection,
            bool _bCaseSensitive,
            const OUString& _rCatalogName,
            const OUString& _rSchemaName,
            const OUString& _rName
        );

        // UNO
        DECLARE_XINTERFACE()
        DECLARE_XTYPEPROVIDER()

        // XAlterView
        virtual void SAL_CALL alterCommand( const OUString& NewCommand ) override;

    protected:
        virtual ~View() override;

    protected:
        // OPropertyContainer
        virtual void SAL_CALL getFastPropertyValue( css::uno::Any& _rValue, sal_Int32 _nHandle ) const override;

    private:
         css::uno::Reference< css::sdb::tools::XViewAccess>     m_xViewAccess;
        sal_Int32                                               m_nCommandHandle;
    private:
        using View_Base::getFastPropertyValue;
    };

} // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

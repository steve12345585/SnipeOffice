/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include "StatementCommonBase.hxx"

#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/sdbc/XBatchExecution.hpp>

namespace connectivity::firebird
{

    typedef ::cppu::ImplHelper1< css::sdbc::XStatement >
        OStatement_Base;

    class OStatement :  public OStatementCommonBase,
                        public OStatement_Base,
                        public css::sdbc::XBatchExecution,
                        public css::lang::XServiceInfo
    {
        XSQLDA* m_pSqlda;
    protected:
        virtual ~OStatement() override {}

    public:
        // a constructor, which is required for returning objects:
        explicit OStatement( Connection* _pConnection)
            : OStatementCommonBase( _pConnection),
              m_pSqlda(nullptr)
        {}

        virtual void disposeResultSet() override;

        DECLARE_SERVICE_INFO();

        virtual void SAL_CALL acquire() noexcept override;
        virtual void SAL_CALL release() noexcept override;

        // XStatement
        virtual css::uno::Reference< css::sdbc::XResultSet > SAL_CALL
            executeQuery(const OUString& sql) override;
        virtual sal_Int32 SAL_CALL executeUpdate(const OUString& sqlIn) override;
        virtual sal_Bool SAL_CALL
            execute(const OUString& sql) override;
        virtual css::uno::Reference< css::sdbc::XConnection > SAL_CALL
            getConnection() override;

        // XBatchExecution - UNSUPPORTED
        virtual void SAL_CALL addBatch( const OUString& sql ) override;
        virtual void SAL_CALL clearBatch(  ) override;
        virtual css::uno::Sequence< sal_Int32 > SAL_CALL executeBatch(  ) override;

        // XInterface
        virtual css::uno::Any SAL_CALL
            queryInterface(const css::uno::Type & rType) override;

        //XTypeProvider
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL
            getTypes() override;
        // OComponentHelper
        virtual void SAL_CALL disposing() override;

    };

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

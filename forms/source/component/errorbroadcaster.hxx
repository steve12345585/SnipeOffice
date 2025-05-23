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

#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/sdb/XSQLErrorBroadcaster.hpp>
#include <cppuhelper/interfacecontainer.hxx>
#include <comphelper/interfacecontainer3.hxx>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdb/SQLErrorEvent.hpp>


namespace frm
{

    typedef ::cppu::ImplHelper1 <   css::sdb::XSQLErrorBroadcaster
                                >   OErrorBroadcaster_BASE;

    class OErrorBroadcaster : public OErrorBroadcaster_BASE
    {
    private:
        ::cppu::OBroadcastHelper&           m_rBHelper;
        ::comphelper::OInterfaceContainerHelper3<css::sdb::XSQLErrorListener>  m_aErrorListeners;

    protected:
        explicit OErrorBroadcaster( ::cppu::OBroadcastHelper& _rBHelper );
        virtual ~OErrorBroadcaster( );

        void disposing();

        void onError( const css::sdbc::SQLException& _rException, const OUString& _rContextDescription );
        void onError( const css::sdb::SQLErrorEvent& _rException );

    protected:
    // XSQLErrorBroadcaster
        virtual void SAL_CALL addSQLErrorListener( const css::uno::Reference< css::sdb::XSQLErrorListener >& _rListener ) override;
        virtual void SAL_CALL removeSQLErrorListener( const css::uno::Reference< css::sdb::XSQLErrorListener >& _rListener ) override;
    };


}   // namespace frm

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

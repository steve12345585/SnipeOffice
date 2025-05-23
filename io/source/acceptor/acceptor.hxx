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

#include <osl/pipe.hxx>
#include <osl/socket.hxx>
#include <mutex>

#include <com/sun/star/uno/Reference.hxx>

namespace com::sun::star::connection { class XConnection; }

namespace io_acceptor {

    class PipeAcceptor
    {
    public:
        PipeAcceptor( OUString sPipeName, OUString sConnectionDescription );

        void init();
        css::uno::Reference < css::connection::XConnection >  accept(  );

        void stopAccepting();

    private:
        std::mutex m_mutex;
        ::osl::Pipe m_pipe;
        OUString m_sPipeName;
        OUString m_sConnectionDescription;
        bool m_bClosed;
    };

    class SocketAcceptor
    {
    public:
        SocketAcceptor( OUString sSocketName ,
                        sal_uInt16 nPort,
                        bool bTcpNoDelay,
                        OUString sConnectionDescription );

        void init();
        css::uno::Reference < css::connection::XConnection > accept();

        void stopAccepting();

    private:
        ::osl::SocketAddr m_addr;
        ::osl::AcceptorSocket m_socket;
        OUString m_sSocketName;
        OUString m_sConnectionDescription;
        sal_uInt16 m_nPort;
        bool m_bTcpNoDelay;
        bool m_bClosed;
    };

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

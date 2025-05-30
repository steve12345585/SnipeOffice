/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IBluetoothSocket.hxx"
#include <osl/socket_decl.hxx>
#include <vector>

#define MAX_LINE_LENGTH 20000

namespace sd
{

    /**
     * [A wrapper for an osl StreamSocket to allow reading lines.]
     *
     * Currently wraps either an osl StreamSocket or a standard c socket,
     * allowing reading and writing for our purposes. Should eventually be
     * returned to being a StreamSocket wrapper if/when Bluetooth is
     * integrated into osl Sockets.
     */
    class BufferedStreamSocket final :
        public IBluetoothSocket,
        private ::osl::StreamSocket
    {
        public:
            /**
             * Create a BufferedStreamSocket on top of an
             * osl::StreamSocket.
             */
            explicit BufferedStreamSocket( const osl::StreamSocket &aSocket );
            /**
             * Create a BufferedStreamSocket on top of a POSIX or WinSock socket.
             */
            explicit BufferedStreamSocket( int aSocket );
            BufferedStreamSocket( const BufferedStreamSocket &aSocket );

            ~BufferedStreamSocket();

            /**
             * Blocks until a line is read.
             * Returns whatever the last call of recv returned, i.e. 0 or less
             * if there was a problem in communications.
             */
            virtual sal_Int32 readLine( OString& aLine ) override;

            virtual sal_Int32 write( const void* pBuffer, sal_uInt32 n ) override;

            virtual void close() override;

            void getPeerAddr(osl::SocketAddr&);
        private:
            sal_Int32 aRead;
            std::vector<char> aBuffer;
            int mSocket;
            bool usingCSocket;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

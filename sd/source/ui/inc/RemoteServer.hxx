/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <osl/socket_decl.hxx>
#include <salhelper/thread.hxx>

#include <sddllapi.h>

namespace osl { class Mutex; }
namespace com::sun::star::presentation { class XSlideShowController; }
namespace com::sun::star::uno { template <class interface_type> class Reference; }

/**
* The port for use for the main communication between LibO and remote control app.
*/
#define PORT 1599

namespace sd
{
    class BufferedStreamSocket;
    class Communicator;

    struct ClientInfo
    {
        OUString mName;

        bool mbIsAlreadyAuthorised;

        ClientInfo( OUString aName,
                    const bool bIsAlreadyAuthorised ) :
            mName(std::move( aName )),
            mbIsAlreadyAuthorised( bIsAlreadyAuthorised ) {}

        virtual ~ClientInfo() {};
    };

    struct ClientInfoInternal;

    class RemoteServer final
    {
        public:
            // For slideshowimpl to inform us.
            static void presentationStarted( const css::uno::Reference<
                css::presentation::XSlideShowController > &rController );
            static void presentationStopped();

            /// ensure that discoverability (eg. for Bluetooth) is enabled
            SD_DLLPUBLIC static void ensureDiscoverable();
            /// restore the state of discoverability from before ensureDiscoverable
            SD_DLLPUBLIC static void restoreDiscoverable();

            // For the communicator
            static void removeCommunicator( Communicator const * pCommunicator );
        //private:
            // these are public because 3 classes and a function need access
            static ::osl::Mutex sDataMutex;
            static ::std::vector<Communicator*> sCommunicators;
    };

    class IPRemoteServer final : public salhelper::Thread
    {
        public:
            // Internal setup
            static void setup();

            // For the control dialog
            SD_DLLPUBLIC static std::vector<std::shared_ptr<ClientInfo>> getClients();
            SD_DLLPUBLIC static bool connectClient(const std::shared_ptr<ClientInfo>& pClient,
                                                   std::u16string_view aPin);
            SD_DLLPUBLIC static void deauthoriseClient(const std::shared_ptr<ClientInfo>& pClient);

            void execute() override;
            void handleAcceptedConnection( BufferedStreamSocket *pSocket ) ;

        private:
            IPRemoteServer();
            virtual ~IPRemoteServer() override;

            osl::AcceptorSocket mSocket;
            ::std::vector<std::shared_ptr<ClientInfoInternal>> mAvailableClients;

            friend class RemoteServer;
            static IPRemoteServer *spServer;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <optional>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/timer.hxx>
#include <unotools/bootstrap.hxx>
#include <com/sun/star/frame/XDesktop2.hpp>
#include <com/sun/star/task/XStatusIndicator.hpp>
#include <com/sun/star/uno/Reference.h>

#include <memory>
#include <string_view>
#include <thread>

namespace com::sun::star::uno { class XComponentContext; }

namespace desktop
{

/*--------------------------------------------------------------------
    Description:    Application-class
 --------------------------------------------------------------------*/
class CommandLineArgs;
class Lockfile;
class Desktop final : public Application
{
    int doShutdown();

    public:
        enum BootstrapError
        {
            BE_OK,
            BE_UNO_SERVICEMANAGER,
            BE_UNO_SERVICE_CONFIG_MISSING,
            BE_PATHINFO_MISSING,
            BE_USERINSTALL_FAILED,
            BE_LANGUAGE_MISSING,
            BE_USERINSTALL_NOTENOUGHDISKSPACE,
            BE_USERINSTALL_NOWRITEACCESS,
            BE_OFFICECONFIG_BROKEN,
            BE_2NDOFFICE_WITHCAT,
        };
        enum BootstrapStatus
        {
            BS_OK,
            BS_TERMINATE
        };

                                Desktop();
                                virtual ~Desktop() override;
        virtual int             Main() override;
        virtual void            Init() override;
        virtual void            InitFinished() override;
        virtual void            DeInit() override;
        virtual bool            QueryExit() override;
        virtual void            Shutdown() override;
        virtual void            Exception(ExceptionCategory nCategory) override;
        virtual void            OverrideSystemSettings( AllSettings& rSettings ) override;
        virtual void            AppEvent( const ApplicationEvent& rAppEvent ) override;

        DECL_LINK( OpenClients_Impl, void*, void );

        static void             OpenClients();
        static void             OpenDefault();
        static void             CheckOpenCLCompute(const css::uno::Reference<css::frame::XDesktop2> &);

        DECL_STATIC_LINK( Desktop, EnableAcceptors_Impl, void*, void);

        static void             HandleAppEvent( const ApplicationEvent& rAppEvent );
        static CommandLineArgs& GetCommandLineArgs();

        static void             HandleBootstrapErrors(
                                    BootstrapError nError, OUString const & aMessage );
        void                    SetBootstrapError(
                                    BootstrapError nError, OUString const & aMessage )
        {
            if ( m_aBootstrapError == BE_OK )
            {
                SAL_INFO("desktop.app", "SetBootstrapError: " << nError << " '" << aMessage << "'");
                m_aBootstrapError = nError;
                m_aBootstrapErrorMessage = aMessage;
            }
        }

        void                    SetBootstrapStatus( BootstrapStatus nStatus )
        {
            m_aBootstrapStatus = nStatus;
        }
        BootstrapStatus          GetBootstrapStatus() const
        {
            return m_aBootstrapStatus;
        }

        // first-start (ever) related methods
        static bool             CheckExtensionDependencies();

        static void             SynchronizeExtensionRepositories(bool bCleanedExtensionCache, Desktop* pDesktop = nullptr);
        void                    SetSplashScreenText( const OUString& rText );
        void                    SetSplashScreenProgress( sal_Int32 );

        // Bootstrap methods
        static void             InitApplicationServiceManager();
            // throws an exception upon failure

    private:
        void                    RegisterServices();
        static void             DeregisterServices();

    public:
        static void             CreateTemporaryDirectory();
        static void             RemoveTemporaryDirectory();

    private:
        static bool             InitializeConfiguration();
        static void             FlushConfiguration();
        static bool             InitializeQuickstartMode( const css::uno::Reference< css::uno::XComponentContext >& rxContext );

        static void             HandleBootstrapPathErrors( ::utl::Bootstrap::Status, std::u16string_view aMsg );

        // Create an error message depending on bootstrap failure code and an optional file url
        static OUString         CreateErrorMsgString( utl::Bootstrap::FailureCode nFailureCode,
                                                      const OUString& aFileURL );

        css::uno::Reference<css::task::XStatusIndicator> m_rSplashScreen;
        void                    OpenSplashScreen();
        void                    CloseSplashScreen();

        DECL_STATIC_LINK( Desktop, ImplInitFilterHdl, ::ConvertData&, bool );
        DECL_STATIC_LINK( Desktop, AsyncInitFirstRun, Timer*, void );
        /** checks if the office is run the first time
            <p>If so, <method>DoFirstRunInitializations</method> is called (asynchronously and delayed) and the
            respective flag in the configuration is reset.</p>
        */
        void                    CheckFirstRun( );

        static void             ShowBackingComponent(Desktop * progress);

        // on-demand acceptors
        static void             createAcceptor(const OUString& aDescription);
        static void             destroyAcceptor(const OUString& aDescription);

        bool                    m_bCleanedExtensionCache;
        bool                    m_bServicesRegistered;
        BootstrapError          m_aBootstrapError;
        OUString                m_aBootstrapErrorMessage;
        BootstrapStatus         m_aBootstrapStatus;

        std::unique_ptr<Lockfile> m_xLockfile;
        Timer                   m_firstRunTimer;
        std::thread             m_aUpdateThread;
};

OUString GetURL_Impl(
    const OUString& rName, std::optional< OUString > const & cwdUrl );

OUString ReplaceStringHookProc(const OUString& rStr);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <config_folders.h>
#include <config_features.h>

#include <rtl/bootstrap.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <cppuhelper/implbase.hxx>

#include <comphelper/diagnose_ex.hxx>
#include <toolkit/helper/vclunohelper.hxx>

#include <comphelper/lok.hxx>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/ucb/CommandFailedException.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/deployment/DeploymentException.hpp>
#include <com/sun/star/deployment/XPackage.hpp>
#include <com/sun/star/deployment/ExtensionManager.hpp>
#include <com/sun/star/deployment/LicenseException.hpp>
#include <com/sun/star/deployment/ui/LicenseDialog.hpp>
#include <com/sun/star/task/OfficeRestartManager.hpp>
#include <com/sun/star/task/XInteractionApprove.hpp>
#include <com/sun/star/task/XInteractionAbort.hpp>
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>

#include <app.hxx>
#include <utility>

#include <dp_misc.h>

using namespace desktop;
using namespace com::sun::star;
using namespace com::sun::star::lang;
using namespace com::sun::star::task;
using namespace com::sun::star::uno;

namespace
{
//For use with XExtensionManager.synchronize
class SilentCommandEnv
    : public ::cppu::WeakImplHelper< ucb::XCommandEnvironment,
                                      task::XInteractionHandler,
                                      ucb::XProgressHandler >
{
    uno::Reference<uno::XComponentContext> mxContext;
    Desktop    *mpDesktop;
    sal_Int32   mnLevel;
    sal_Int32   mnProgress;

public:
    SilentCommandEnv(
        uno::Reference<uno::XComponentContext> xContext,
        Desktop* pDesktop );
    virtual ~SilentCommandEnv() override;

    // XCommandEnvironment
    virtual uno::Reference<task::XInteractionHandler > SAL_CALL
    getInteractionHandler() override;
    virtual uno::Reference<ucb::XProgressHandler >
    SAL_CALL getProgressHandler() override;

    // XInteractionHandler
    virtual void SAL_CALL handle(
        uno::Reference<task::XInteractionRequest > const & xRequest ) override;

    // XProgressHandler
    virtual void SAL_CALL push( uno::Any const & Status ) override;
    virtual void SAL_CALL update( uno::Any const & Status ) override;
    virtual void SAL_CALL pop() override;
};


SilentCommandEnv::SilentCommandEnv(
    uno::Reference<uno::XComponentContext> xContext,
    Desktop* pDesktop ):
    mxContext(std::move( xContext )),
    mpDesktop( pDesktop ),
    mnLevel( 0 ),
    mnProgress( 25 )
{}


SilentCommandEnv::~SilentCommandEnv()
{
    if (mpDesktop)
        mpDesktop->SetSplashScreenText(OUString());
}


Reference<task::XInteractionHandler> SilentCommandEnv::getInteractionHandler()
{
    return this;
}


Reference<ucb::XProgressHandler> SilentCommandEnv::getProgressHandler()
{
    return this;
}


// XInteractionHandler
void SilentCommandEnv::handle( Reference< task::XInteractionRequest> const & xRequest )
{
    deployment::LicenseException licExc;

    uno::Any request( xRequest->getRequest() );
    bool bApprove = true;

    if ( request >>= licExc )
    {
        uno::Reference< ui::dialogs::XExecutableDialog > xDialog(
            deployment::ui::LicenseDialog::create(
            mxContext, VCLUnoHelper::GetInterface( nullptr ),
            licExc.ExtensionName, licExc.Text ) );
        sal_Int16 res = xDialog->execute();
        if ( res == ui::dialogs::ExecutableDialogResults::CANCEL )
            bApprove = false;
        else if ( res == ui::dialogs::ExecutableDialogResults::OK )
            bApprove = true;
        else
        {
            OSL_ASSERT(false);
        }
    }

    // We approve everything here
    uno::Sequence< Reference< task::XInteractionContinuation > > conts( xRequest->getContinuations() );
    Reference< task::XInteractionContinuation > const * pConts = conts.getConstArray();
    sal_Int32 len = conts.getLength();

    for ( sal_Int32 pos = 0; pos < len; ++pos )
    {
        if ( bApprove )
        {
            uno::Reference< task::XInteractionApprove > xInteractionApprove( pConts[ pos ], uno::UNO_QUERY );
            if ( xInteractionApprove.is() )
                xInteractionApprove->select();
        }
        else
        {
            uno::Reference< task::XInteractionAbort > xInteractionAbort( pConts[ pos ], uno::UNO_QUERY );
            if ( xInteractionAbort.is() )
                xInteractionAbort->select();
        }
    }
}


// XProgressHandler
void SilentCommandEnv::push( uno::Any const & rStatus )
{
    OUString sText;
    mnLevel += 1;

    if (mpDesktop && rStatus.hasValue() && (rStatus >>= sText))
    {
        if ( mnLevel <= 3 )
            mpDesktop->SetSplashScreenText( sText );
        else
            mpDesktop->SetSplashScreenProgress( ++mnProgress );
    }
}


void SilentCommandEnv::update( uno::Any const & rStatus )
{
    OUString sText;
    if (mpDesktop && rStatus.hasValue() && (rStatus >>= sText))
    {
        mpDesktop->SetSplashScreenText( sText );
    }
}


void SilentCommandEnv::pop()
{
    mnLevel -= 1;
}

} // end namespace


constexpr OUString aAccessSrvc = u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr;

static sal_Int16 impl_showExtensionDialog( uno::Reference< uno::XComponentContext > const &xContext )
{
    uno::Reference< uno::XInterface > xService;
    sal_Int16 nRet = 0;

    uno::Reference< lang::XMultiComponentFactory > xServiceManager( xContext->getServiceManager() );
    if( !xServiceManager.is() )
        throw uno::RuntimeException(
            u"impl_showExtensionDialog(): unable to obtain service manager from component context"_ustr, uno::Reference< uno::XInterface > () );

    xService = xServiceManager->createInstanceWithContext( u"com.sun.star.deployment.ui.UpdateRequiredDialog"_ustr, xContext );
    uno::Reference< ui::dialogs::XExecutableDialog > xExecutable( xService, uno::UNO_QUERY );
    if ( xExecutable.is() )
        nRet = xExecutable->execute();

    return nRet;
}


// Check dependencies of all packages

static bool impl_checkDependencies( const uno::Reference< uno::XComponentContext > &xContext )
{
    uno::Sequence< uno::Sequence< uno::Reference< deployment::XPackage > > > xAllPackages;
    uno::Reference< deployment::XExtensionManager > xExtensionManager = deployment::ExtensionManager::get( xContext );

    if ( !xExtensionManager.is() )
    {
        SAL_WARN( "desktop.app", "Could not get the Extension Manager!" );
        return true;
    }

    try {
        xAllPackages = xExtensionManager->getAllExtensions( uno::Reference< task::XAbortChannel >(),
                                                            uno::Reference< ucb::XCommandEnvironment >() );
    }
    catch ( const deployment::DeploymentException & ) { return true; }
    catch ( const ucb::CommandFailedException & ) { return true; }
    catch ( const ucb::CommandAbortedException & ) { return true; }
    catch ( const lang::IllegalArgumentException & e ) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw css::lang::WrappedTargetRuntimeException( e.Message,
                        e.Context, anyEx );
    }

#if OSL_DEBUG_LEVEL >= 2
    sal_Int32 const nMax = 3;
#else
    sal_Int32 const nMax = 2;
#endif

    for (uno::Sequence<uno::Reference<deployment::XPackage>> const& xPackageList : xAllPackages)
    {
        for ( sal_Int32 j = 0; (j<nMax) && (j < xPackageList.getLength()); ++j )
        {
            uno::Reference< deployment::XPackage > xPackage = xPackageList[j];
            if ( xPackage.is() )
            {
                bool bRegistered = false;
                try {
                    beans::Optional< beans::Ambiguous< sal_Bool > > option( xPackage->isRegistered( uno::Reference< task::XAbortChannel >(),
                                                                                                    uno::Reference< ucb::XCommandEnvironment >() ) );
                    if ( option.IsPresent )
                    {
                        ::beans::Ambiguous< sal_Bool > const & reg = option.Value;
                        if ( reg.IsAmbiguous )
                            bRegistered = false;
                        else
                            bRegistered = reg.Value;
                    }
                    else
                        bRegistered = false;
                }
                catch ( const uno::RuntimeException & ) { throw; }
                catch (const uno::Exception & ) {
                   TOOLS_WARN_EXCEPTION( "desktop.app", "" );
                }

                if ( bRegistered )
                {
                    bool bDependenciesValid = false;
                    try {
                        bDependenciesValid = xPackage->checkDependencies( uno::Reference< ucb::XCommandEnvironment >() );
                    }
                    catch ( const deployment::DeploymentException & ) {}
                    if ( ! bDependenciesValid )
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}


// resets the 'check needed' flag (needed, if aborted)

static void impl_setNeedsCompatCheck()
{
    try {
        Reference< XMultiServiceFactory > theConfigProvider(
            configuration::theDefaultProvider::get(
                comphelper::getProcessComponentContext() ) );

        beans::NamedValue v( u"nodepath"_ustr,
                      Any( u"org.openoffice.Setup/Office"_ustr ) );
        Sequence< Any > theArgs{ Any(v) };
        Reference< beans::XPropertySet > pset(
            theConfigProvider->createInstanceWithArguments( aAccessSrvc, theArgs ), UNO_QUERY_THROW );

        Any value( u"never"_ustr );

        pset->setPropertyValue(u"LastCompatibilityCheckID"_ustr, value );
        Reference< util::XChangesBatch >( pset, UNO_QUERY_THROW )->commitChanges();
    }
    catch (const Exception&) {}
}


// to check if we need checking the dependencies of the extensions again, we compare
// the build id of the office with the one of the last check

static bool impl_needsCompatCheck()
{
    bool bNeedsCheck = false;
    OUString aLastCheckBuildID;
    OUString aCurrentBuildID( u"${$BRAND_BASE_DIR/" LIBO_ETC_FOLDER "/" SAL_CONFIGFILE("version") ":buildid}"_ustr );
    rtl::Bootstrap::expandMacros( aCurrentBuildID );

    try {
        Reference< XMultiServiceFactory > theConfigProvider(
            configuration::theDefaultProvider::get(
                comphelper::getProcessComponentContext() ) );

        beans::NamedValue v( u"nodepath"_ustr,
                      Any( u"org.openoffice.Setup/Office"_ustr ) );
        Sequence< Any > theArgs{ Any(v) };
        Reference< beans::XPropertySet > pset(
            theConfigProvider->createInstanceWithArguments( aAccessSrvc, theArgs ), UNO_QUERY_THROW );

        Any result = pset->getPropertyValue(u"LastCompatibilityCheckID"_ustr);

        result >>= aLastCheckBuildID;
        if ( aLastCheckBuildID != aCurrentBuildID )
        {
            bNeedsCheck = true;
            result <<= aCurrentBuildID;
            pset->setPropertyValue(u"LastCompatibilityCheckID"_ustr, result );
            Reference< util::XChangesBatch >( pset, UNO_QUERY_THROW )->commitChanges();
        }
#if OSL_DEBUG_LEVEL >= 2
        bNeedsCheck = true;
#endif
    }
    catch (const css::uno::Exception&) {}

    return bNeedsCheck;
}


// Do we need to check the dependencies of the extensions?
// When there are unresolved issues, we can't continue with startup
bool Desktop::CheckExtensionDependencies()
{
    if (!impl_needsCompatCheck())
    {
        return false;
    }

    const uno::Reference< uno::XComponentContext >& xContext(
        comphelper::getProcessComponentContext());

    bool bDependenciesValid = impl_checkDependencies( xContext );

    short nRet = 0;

    if ( !bDependenciesValid )
        nRet = impl_showExtensionDialog( xContext );

    if ( nRet == -1 )
    {
        impl_setNeedsCompatCheck();
        return true;
    }
    else
        return false;
}

void Desktop::SynchronizeExtensionRepositories(bool bCleanedExtensionCache, Desktop* pDesktop)
{
    const uno::Reference< uno::XComponentContext >& context(
        comphelper::getProcessComponentContext());
    uno::Reference< ucb::XCommandEnvironment > silent(
        new SilentCommandEnv(context, pDesktop));
    if (bCleanedExtensionCache) {
        deployment::ExtensionManager::get(context)->reinstallDeployedExtensions(
            true, u"user"_ustr, Reference<task::XAbortChannel>(), silent);
#if !HAVE_FEATURE_MACOSX_SANDBOX
        if (!comphelper::LibreOfficeKit::isActive())
            task::OfficeRestartManager::get(context)->requestRestart(
                silent->getInteractionHandler());
#endif
    } else {
        // reinstallDeployedExtensions above already calls syncRepositories internally

        // Force syncing repositories on startup. There are cases where the extension
        // registration becomes invalid which leads to extensions not starting up, although
        // installed and active. Syncing extension repos on startup fixes that.
        dp_misc::syncRepositories(/*force=*/true, silent);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

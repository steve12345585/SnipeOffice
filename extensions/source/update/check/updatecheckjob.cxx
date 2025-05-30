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

#include <sal/config.h>

#include "updatecheck.hxx"
#include "updatecheckconfig.hxx"
#include "updatehdl.hxx"
#include "updateprotocol.hxx"

#include <memory>
#include <mutex>
#include <utility>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <com/sun/star/task/XJob.hpp>

namespace beans = css::beans ;
namespace frame = css::frame ;
namespace lang = css::lang ;
namespace task = css::task ;
namespace uno = css::uno ;

namespace
{

class InitUpdateCheckJobThread : public osl::Thread
{
public:
    InitUpdateCheckJobThread( const uno::Reference< uno::XComponentContext > &xContext,
                              const uno::Sequence< beans::NamedValue > &xParameters,
                              bool bShowDialog );

    virtual void SAL_CALL run() override;

    void    setTerminating();

private:
    osl::Condition m_aCondition;
    uno::Reference<uno::XComponentContext> m_xContext;
    uno::Sequence<beans::NamedValue> m_xParameters;
    bool m_bShowDialog;
    bool m_bTerminating;

    std::mutex m_mutex;
    rtl::Reference<UpdateCheck> m_controller;
};

class UpdateCheckJob :
    public ::cppu::WeakImplHelper< task::XJob, lang::XServiceInfo, frame::XTerminateListener >
{
    virtual ~UpdateCheckJob() override;

public:

    UpdateCheckJob(
        css::uno::Reference<css::uno::XComponentContext> const & context,
        css::uno::Reference<css::frame::XDesktop2> const & desktop):
        m_xContext(context), m_xDesktop(desktop)
    {}

    // XJob
    virtual uno::Any SAL_CALL execute(const uno::Sequence<beans::NamedValue>&) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(OUString const & serviceName) override;
    virtual uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XEventListener
    virtual void SAL_CALL disposing( css::lang::EventObject const & evt ) override;

    // XTerminateListener
    virtual void SAL_CALL queryTermination( lang::EventObject const & evt ) override;
    virtual void SAL_CALL notifyTermination( lang::EventObject const & evt ) override;

private:
    uno::Reference<uno::XComponentContext>  m_xContext;

    std::mutex m_mutex;
    uno::Reference< frame::XDesktop2 >      m_xDesktop;
    std::unique_ptr< InitUpdateCheckJobThread > m_pInitThread;

    void handleExtensionUpdates( const uno::Sequence< beans::NamedValue > &rListProp );
    void terminateAndJoinThread();
};

InitUpdateCheckJobThread::InitUpdateCheckJobThread(
            const uno::Reference< uno::XComponentContext > &xContext,
            const uno::Sequence< beans::NamedValue > &xParameters,
            bool bShowDialog ) :
    m_xContext( xContext ),
    m_xParameters( xParameters ),
    m_bShowDialog( bShowDialog ),
    m_bTerminating( false )
{
    create();
}


void SAL_CALL InitUpdateCheckJobThread::run()
{
    osl_setThreadName("InitUpdateCheckJobThread");

    if (!m_bShowDialog) {
        TimeValue tv = { 25, 0 };
        m_aCondition.wait( &tv );
        if ( m_bTerminating )
            return;
    }

    try {
        rtl::Reference< UpdateCheck > aController( UpdateCheck::get() );
        // At least for the automatic ("onFirstVisibleTask", i.e., !m_bShowDialog) check, wait for
        // m_controller during setTerminating, to prevent m_controller from still having threads
        // running during exit (ideally, we would make sure that all threads are joined before exit,
        // but the UpdateCheck logic is rather convoluted, so play it safe for now and only address
        // the automatic update check that is known to cause issues during `make check`, not the
        // manually triggered update check scenario):
        if (!m_bShowDialog) {
            std::scoped_lock l(m_mutex);
            m_controller = aController;
        }
        aController->initialize( m_xParameters, m_xContext );

        if ( m_bShowDialog )
            aController->showDialog( true );
    } catch (const uno::Exception &) {
        // fdo#64962 - don't bring the app down on some unexpected exception.
        TOOLS_WARN_EXCEPTION("extensions.update", "Caught init update exception, thread terminated" );
        {
            std::scoped_lock l(m_mutex);
            m_controller.clear();
        }
    }
}

void InitUpdateCheckJobThread::setTerminating() {
    m_bTerminating = true;
    m_aCondition.set();
    rtl::Reference<UpdateCheck> controller;
    {
        std::scoped_lock l(m_mutex);
        std::swap(controller, m_controller);
    }
    if (controller.is()) {
        controller->waitForUpdateCheckFinished();
    }
}

UpdateCheckJob::~UpdateCheckJob()
{
}

uno::Any
UpdateCheckJob::execute(const uno::Sequence<beans::NamedValue>& namedValues)
{
    for ( sal_Int32 n=namedValues.getLength(); n-- > 0; )
    {
        if ( namedValues[ n ].Name == "DynamicData" )
        {
            uno::Sequence<beans::NamedValue> aListProp;
            if ( namedValues[n].Value >>= aListProp )
            {
                for ( sal_Int32 i=aListProp.getLength(); i-- > 0; )
                {
                    if ( aListProp[ i ].Name == "updateList" )
                    {
                        handleExtensionUpdates( aListProp );
                        return uno::Any();
                    }
                }
            }
        }
    }

    uno::Sequence<beans::NamedValue> aConfig =
        getValue< uno::Sequence<beans::NamedValue> > (namedValues, "JobConfig");

    /* Determine the way we got invoked here -
     * see Developers Guide Chapter "4.7.2 Jobs" to understand the magic
     */

    uno::Sequence<beans::NamedValue> aEnvironment =
        getValue< uno::Sequence<beans::NamedValue> > (namedValues, "Environment");

    OUString aEventName = getValue< OUString > (aEnvironment, "EventName");

    auto thread = std::make_unique<InitUpdateCheckJobThread >(
        m_xContext, aConfig,
        aEventName != "onFirstVisibleTask");
    {
        std::scoped_lock l(m_mutex);
        m_pInitThread = std::move(thread);
    }

    return uno::Any();
}


void UpdateCheckJob::handleExtensionUpdates( const uno::Sequence< beans::NamedValue > &rListProp )
{
    try {
        uno::Sequence< uno::Sequence< OUString > > aList =
            getValue< uno::Sequence< uno::Sequence< OUString > > > ( rListProp, "updateList" );
        bool bPrepareOnly = getValue< bool > ( rListProp, "prepareOnly" );

        // we will first store any new found updates and then check, if there are any
        // pending updates.
        storeExtensionUpdateInfos( m_xContext, aList );

        if ( bPrepareOnly )
            return;

        bool bHasUpdates = checkForPendingUpdates( m_xContext );

        rtl::Reference<UpdateCheck> aController( UpdateCheck::get() );
        if ( ! aController.is() )
            return;

        aController->setHasExtensionUpdates( bHasUpdates );

        if ( ! aController->hasOfficeUpdate() )
        {
            if ( bHasUpdates )
                aController->setUIState( UPDATESTATE_EXT_UPD_AVAIL, true );
            else
                aController->setUIState( UPDATESTATE_NO_UPDATE_AVAIL, true );
        }
    }
    catch( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("extensions.update", "Caught exception, thread terminated");
    }
}


OUString SAL_CALL
UpdateCheckJob::getImplementationName()
{
    return u"vnd.sun.UpdateCheck"_ustr;
}


uno::Sequence< OUString > SAL_CALL
UpdateCheckJob::getSupportedServiceNames()
{
    return { u"com.sun.star.setup.UpdateCheck"_ustr };
}

sal_Bool SAL_CALL
UpdateCheckJob::supportsService( OUString const & serviceName )
{
    return cppu::supportsService(this, serviceName);
}


// XEventListener
void SAL_CALL UpdateCheckJob::disposing( lang::EventObject const & rEvt )
{
    css::uno::Reference<css::frame::XDesktop2> desktop;
    {
        std::scoped_lock l(m_mutex);
        if ( rEvt.Source == m_xDesktop ) {
            std::swap(m_xDesktop, desktop);
        }
    }

    if ( desktop.is() )
    {
        terminateAndJoinThread();
        desktop->removeTerminateListener( this );
    }
}


// XTerminateListener
void SAL_CALL UpdateCheckJob::queryTermination( lang::EventObject const & )
{
}

void UpdateCheckJob::terminateAndJoinThread()
{
    std::unique_ptr<InitUpdateCheckJobThread> thread;
    {
        std::scoped_lock l(m_mutex);
        std::swap(m_pInitThread, thread);
    }
    if (thread != nullptr)
    {
        thread->setTerminating();
        thread->join();
    }
}

void SAL_CALL UpdateCheckJob::notifyTermination( lang::EventObject const & )
{
    terminateAndJoinThread();
}

} // anonymous namespace

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
extensions_update_UpdateCheckJob_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    css::uno::Reference<css::frame::XDesktop2> desktop(
        css::frame::Desktop::create(context));
    rtl::Reference<UpdateCheckJob> job(new UpdateCheckJob(context, desktop));
    desktop->addTerminateListener(job);
    return cppu::acquire(job.get());
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

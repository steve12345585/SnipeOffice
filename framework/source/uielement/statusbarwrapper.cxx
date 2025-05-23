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

#include <uielement/statusbarwrapper.hxx>

#include <uielement/statusbar.hxx>

#include <com/sun/star/ui/UIElementType.hpp>

#include <toolkit/helper/vclunohelper.hxx>

#include <tools/solar.h>
#include <utility>
#include <vcl/svapp.hxx>
#include <vcl/wintypes.hxx>

using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;
using namespace com::sun::star::awt;
using namespace ::com::sun::star::ui;

namespace framework
{

StatusBarWrapper::StatusBarWrapper(
    css::uno::Reference< css::uno::XComponentContext > xContext
    )
 :  UIConfigElementWrapperBase( UIElementType::STATUSBAR ),
    m_xContext(std::move( xContext ))
{
}

StatusBarWrapper::~StatusBarWrapper()
{
}

void SAL_CALL StatusBarWrapper::dispose()
{
    Reference< XComponent > xThis(this);

    css::lang::EventObject aEvent( xThis );
    m_aListenerContainer.disposeAndClear( aEvent );

    SolarMutexGuard g;
    if ( m_bDisposed )
        return;

    if ( m_xStatusBarManager.is() )
        m_xStatusBarManager->dispose();
    m_xStatusBarManager.clear();
    m_xConfigSource.clear();
    m_xConfigData.clear();
    m_xContext.clear();

    m_bDisposed = true;

}

// XInitialization
void SAL_CALL StatusBarWrapper::initialize( const Sequence< Any >& aArguments )
{
    SolarMutexGuard g;

    if ( m_bDisposed )
        throw DisposedException();

    if ( m_bInitialized )
        return;

    UIConfigElementWrapperBase::initialize( aArguments );

    Reference< XFrame > xFrame( m_xWeakFrame );
    if ( !(xFrame.is() && m_xConfigSource.is()) )
        return;

    // Create VCL based toolbar which will be filled with settings data
    StatusBar*        pStatusBar( nullptr );
    rtl::Reference<StatusBarManager> pStatusBarManager;
    {
        SolarMutexGuard aSolarMutexGuard;
        VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xFrame->getContainerWindow() );
        if ( pWindow )
        {
            WinBits nStyles = WinBits( WB_LEFT | WB_3DLOOK );

            pStatusBar = VclPtr<FrameworkStatusBar>::Create( pWindow, nStyles );
            pStatusBarManager = new StatusBarManager( m_xContext, xFrame, pStatusBar );
            static_cast<FrameworkStatusBar*>(pStatusBar)->SetStatusBarManager( pStatusBarManager.get() );
            m_xStatusBarManager = pStatusBarManager;
        }
    }

    try
    {
        m_xConfigData = m_xConfigSource->getSettings( m_aResourceURL, false );
        if ( m_xConfigData.is() && pStatusBar && pStatusBarManager )
        {
            // Fill statusbar with container contents
            pStatusBarManager->FillStatusBar( m_xConfigData );
        }
    }
    catch ( const NoSuchElementException& )
    {
    }
}

// XUIElementSettings
void SAL_CALL StatusBarWrapper::updateSettings()
{
    SolarMutexGuard g;

    if ( m_bDisposed )
        throw DisposedException();

    if ( !(m_bPersistent &&
         m_xConfigSource.is() &&
         m_xStatusBarManager.is()) )
        return;

    try
    {
        m_xConfigData = m_xConfigSource->getSettings( m_aResourceURL, false );
        if ( m_xConfigData.is() )
            m_xStatusBarManager->FillStatusBar( m_xConfigData );
    }
    catch ( const NoSuchElementException& )
    {
    }
}

Reference< XInterface > SAL_CALL StatusBarWrapper::getRealInterface()
{
    SolarMutexGuard g;

    if ( m_xStatusBarManager )
    {
        vcl::Window* pWindow = m_xStatusBarManager->GetStatusBar();
        if ( pWindow )
            return Reference< XInterface >( VCLUnoHelper::GetInterface( pWindow ), UNO_QUERY );
    }

    return Reference< XInterface >();
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include "helpinterceptor.hxx"
#include "helpdispatch.hxx"
#include "newhelp.hxx"
#include <tools/urlobj.hxx>
#include <tools/debug.hxx>

using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::lang;

HelpInterceptor_Impl::HelpInterceptor_Impl() :

    m_pWindow  ( nullptr ),
    m_nCurPos   ( 0 )

{
}


HelpInterceptor_Impl::~HelpInterceptor_Impl()
{
}


void HelpInterceptor_Impl::addURL( const OUString& rURL )
{
    size_t nCount = m_vHistoryUrls.size();
    if ( nCount && m_nCurPos < ( nCount - 1 ) )
    {
        m_vHistoryUrls.erase(
            m_vHistoryUrls.begin() + m_nCurPos + 1,
            m_vHistoryUrls.end());
    }
    Reference<XFrame> xFrame(m_xIntercepted, UNO_QUERY);
    Reference<XController> xController;
    if(xFrame.is())
        xController = xFrame->getController();

    m_aCurrentURL = rURL;
    m_vHistoryUrls.emplace_back( rURL );
    m_nCurPos = m_vHistoryUrls.size() - 1;
// TODO ?
    if ( m_xListener.is() )
    {
        css::frame::FeatureStateEvent aEvent;
        aEvent.FeatureURL.Complete = rURL;
        aEvent.Source = static_cast<css::frame::XDispatch*>(this);
        m_xListener->statusChanged( aEvent );
    }

    m_pWindow->UpdateToolbox();
}


void HelpInterceptor_Impl::setInterception( const Reference< XFrame >& xFrame )
{
    m_xIntercepted.set( xFrame, UNO_QUERY );

    if ( m_xIntercepted.is() )
        m_xIntercepted->registerDispatchProviderInterceptor( static_cast<XDispatchProviderInterceptor*>(this) );
}


bool HelpInterceptor_Impl::HasHistoryPred() const
{
    return m_nCurPos > 0;
}

bool HelpInterceptor_Impl::HasHistorySucc() const
{
    return m_nCurPos < ( m_vHistoryUrls.size() - 1 );
}


// XDispatchProvider

Reference< XDispatch > SAL_CALL HelpInterceptor_Impl::queryDispatch(

    const URL& aURL, const OUString& aTargetFrameName, sal_Int32 nSearchFlags )

{
    Reference< XDispatch > xResult;
    if ( m_xSlaveDispatcher.is() )
        xResult = m_xSlaveDispatcher->queryDispatch( aURL, aTargetFrameName, nSearchFlags );

    bool bHelpURL = aURL.Complete.toAsciiLowerCase().match("vnd.sun.star.help",0);

    if ( bHelpURL )
    {
        DBG_ASSERT( xResult.is(), "invalid dispatch" );
        xResult = new HelpDispatch_Impl( *this, xResult );
    }

    return xResult;
}


Sequence < Reference < XDispatch > > SAL_CALL HelpInterceptor_Impl::queryDispatches(

    const Sequence< DispatchDescriptor >& aDescripts )

{
    Sequence< Reference< XDispatch > > aReturn( aDescripts.getLength() );
    std::transform(aDescripts.begin(), aDescripts.end(), aReturn.getArray(),
        [this](const DispatchDescriptor& rDescr) -> Reference<XDispatch> {
            return queryDispatch(rDescr.FeatureURL, rDescr.FrameName, rDescr.SearchFlags); });
    return aReturn;
}


// XDispatchProviderInterceptor

Reference< XDispatchProvider > SAL_CALL HelpInterceptor_Impl::getSlaveDispatchProvider()

{
    return m_xSlaveDispatcher;
}


void SAL_CALL HelpInterceptor_Impl::setSlaveDispatchProvider( const Reference< XDispatchProvider >& xNewSlave )

{
    m_xSlaveDispatcher = xNewSlave;
}


Reference< XDispatchProvider > SAL_CALL HelpInterceptor_Impl::getMasterDispatchProvider()

{
    return m_xMasterDispatcher;
}


void SAL_CALL HelpInterceptor_Impl::setMasterDispatchProvider( const Reference< XDispatchProvider >& xNewMaster )

{
    m_xMasterDispatcher = xNewMaster;
}


// XInterceptorInfo

Sequence< OUString > SAL_CALL HelpInterceptor_Impl::getInterceptedURLs()

{
    Sequence<OUString> aURLList { u"vnd.sun.star.help://*"_ustr };
    return aURLList;
}


// XDispatch

void SAL_CALL HelpInterceptor_Impl::dispatch(
    const URL& aURL, const Sequence< css::beans::PropertyValue >& )
{
    bool bBack = aURL.Complete == ".uno:Backward";
    if ( !bBack && aURL.Complete != ".uno:Forward" )
        return;

    if ( m_vHistoryUrls.empty() )
        return;

    size_t nPos = ( bBack && m_nCurPos > 0 ) ? --m_nCurPos
                                            : ( !bBack && m_nCurPos < m_vHistoryUrls.size() - 1 )
                                            ? ++m_nCurPos
                                            : std::numeric_limits<std::size_t>::max();

    if ( nPos < std::numeric_limits<std::size_t>::max() )
    {
        m_pWindow->loadHelpContent(m_vHistoryUrls[nPos], false); // false => don't add item to history again!
    }

    m_pWindow->UpdateToolbox();
}


void SAL_CALL HelpInterceptor_Impl::addStatusListener(
    const Reference< XStatusListener >& xControl, const URL& )
{
    DBG_ASSERT( !m_xListener.is(), "listener already exists" );
    m_xListener = xControl;
}


void SAL_CALL HelpInterceptor_Impl::removeStatusListener(
    const Reference< XStatusListener >&, const URL&)
{
    m_xListener = nullptr;
}

// HelpListener_Impl -----------------------------------------------------

HelpListener_Impl::HelpListener_Impl( HelpInterceptor_Impl* pInter )
{
    pInterceptor = pInter;
    pInterceptor->addStatusListener( this, css::util::URL() );
}


void SAL_CALL HelpListener_Impl::statusChanged( const css::frame::FeatureStateEvent& Event )
{
    INetURLObject aObj( Event.FeatureURL.Complete );
    aFactory = aObj.GetHost();
    aChangeLink.Call( *this );
}


void SAL_CALL HelpListener_Impl::disposing( const css::lang::EventObject& )
{
    pInterceptor->removeStatusListener( this, css::util::URL() );
    pInterceptor = nullptr;
}

HelpStatusListener_Impl::HelpStatusListener_Impl(
        Reference < XDispatch > const & aDispatch, URL const & rURL)
{
    aDispatch->addStatusListener(this, rURL);
}

HelpStatusListener_Impl::~HelpStatusListener_Impl()
{
    if(xDispatch.is())
        xDispatch->removeStatusListener(this, css::util::URL());
}

void HelpStatusListener_Impl::statusChanged(
    const FeatureStateEvent& rEvent )
{
    aStateEvent = rEvent;
}

void HelpStatusListener_Impl::disposing( const EventObject& )
{
    xDispatch->removeStatusListener(this, css::util::URL());
    xDispatch = nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/linguistic2/LinguServiceManager.hpp>
#include <com/sun/star/linguistic2/XLinguServiceEventBroadcaster.hpp>
#include <com/sun/star/linguistic2/XProofreadingIterator.hpp>
#include <com/sun/star/linguistic2/LinguServiceEventFlags.hpp>

#include <unotools/lingucfg.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <com/sun/star/uno/Reference.h>
#include <comphelper/processfactory.hxx>
#include <vcl/svapp.hxx>
#include <dlelstnr.hxx>
#include <proofreadingiterator.hxx>
#include <swmodule.hxx>
#include <wrtsh.hxx>
#include <view.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::linguistic2;
using namespace ::com::sun::star::linguistic2::LinguServiceEventFlags;

SwLinguServiceEventListener::SwLinguServiceEventListener()
{
    const Reference< XComponentContext >& xContext( comphelper::getProcessComponentContext() );
    try
    {
        m_xDesktop = frame::Desktop::create(xContext);
        m_xDesktop->addTerminateListener( this );

        m_xLngSvcMgr = LinguServiceManager::create(xContext);
        m_xLngSvcMgr->addLinguServiceManagerListener( static_cast<XLinguServiceEventListener *>(this) );

        if (SvtLinguConfig().HasGrammarChecker())
        {
            m_xGCIterator = sw::proofreadingiterator::get(xContext);
            Reference< XLinguServiceEventBroadcaster > xBC( m_xGCIterator, UNO_QUERY );
            if (xBC.is())
                xBC->addLinguServiceEventListener( static_cast<XLinguServiceEventListener *>(this) );
        }
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "sw", "SwLinguServiceEventListener c-tor" );
    }
}

SwLinguServiceEventListener::~SwLinguServiceEventListener()
{
}

void SAL_CALL SwLinguServiceEventListener::processLinguServiceEvent(
            const LinguServiceEvent& rLngSvcEvent )
{
    SolarMutexGuard aGuard;

    bool bIsSpellWrong = 0 != (rLngSvcEvent.nEvent & SPELL_WRONG_WORDS_AGAIN);
    bool bIsSpellAll   = 0 != (rLngSvcEvent.nEvent & SPELL_CORRECT_WORDS_AGAIN);
    if (0 != (rLngSvcEvent.nEvent & PROOFREAD_AGAIN))
        bIsSpellWrong = bIsSpellAll = true;     // have all spelling and grammar checked...
    if (bIsSpellWrong || bIsSpellAll)
    {
        SwModule::CheckSpellChanges( false, bIsSpellWrong, bIsSpellAll, false );
    }
    if (!(rLngSvcEvent.nEvent & HYPHENATE_AGAIN))
        return;

    SwView *pSwView = SwModule::GetFirstView();

    //!! since this function may be called within the ctor of
    //!! SwView (during formatting) where the WrtShell is not yet
    //!! created, we have to check for the WrtShellPtr to see
    //!! if it is already available
    while (pSwView && pSwView->GetWrtShellPtr())
    {
        pSwView->GetWrtShell().ChgHyphenation();
        pSwView = SwModule::GetNextView( pSwView );
    }
}

void SAL_CALL SwLinguServiceEventListener::disposing(
            const EventObject& rEventObj )
{
    SolarMutexGuard aGuard;

    if (m_xLngSvcMgr.is() && rEventObj.Source == m_xLngSvcMgr)
        m_xLngSvcMgr = nullptr;
    if (m_xGCIterator.is() && rEventObj.Source == m_xGCIterator)
        m_xGCIterator = nullptr;
}

void SAL_CALL SwLinguServiceEventListener::queryTermination(
            const EventObject& /*rEventObj*/ )
{
}

void SAL_CALL SwLinguServiceEventListener::notifyTermination(
            const EventObject& rEventObj )
{
    SolarMutexGuard aGuard;

    if (m_xDesktop.is()  &&  rEventObj.Source == m_xDesktop)
    {
        if (m_xLngSvcMgr.is())
            m_xLngSvcMgr = nullptr;
        if (m_xGCIterator.is())
            m_xGCIterator = nullptr;
        m_xDesktop = nullptr;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

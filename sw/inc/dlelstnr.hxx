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

#include <com/sun/star/linguistic2/XLinguServiceEventListener.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <cppuhelper/implbase.hxx>

namespace com::sun::star {
    namespace linguistic2 {
        class XLinguServiceManager2;
        class XProofreadingIterator;
    }
    namespace frame {
        class XDesktop2;
    }
}

/** EventListener that triggers spellchecking and hyphenation.

    Happens when relevant changes (to the dictionaries of the dictionary list, or
    properties) were made.
*/
class SwLinguServiceEventListener final :
    public cppu::WeakImplHelper
    <
        css::linguistic2::XLinguServiceEventListener,
        css::frame::XTerminateListener
    >
{
    css::uno::Reference<css::frame::XDesktop2 >                      m_xDesktop;
    css::uno::Reference<css::linguistic2::XLinguServiceManager2 >    m_xLngSvcMgr;
    css::uno::Reference<css::linguistic2::XProofreadingIterator >    m_xGCIterator;

    SwLinguServiceEventListener(const SwLinguServiceEventListener &) = delete;
    SwLinguServiceEventListener & operator = (const SwLinguServiceEventListener &) = delete;

public:
    SwLinguServiceEventListener();
    virtual ~SwLinguServiceEventListener() override;

    /// XEventListener
    virtual void SAL_CALL disposing( const css::lang::EventObject& rEventObj ) override;

    /// XLinguServiceEventListener
    virtual void SAL_CALL processLinguServiceEvent( const css::linguistic2::LinguServiceEvent& rLngSvcEvent ) override;

    /// XTerminateListener
    virtual void SAL_CALL queryTermination( const css::lang::EventObject& rEventObj ) override;
    virtual void SAL_CALL notifyTermination( const css::lang::EventObject& rEventObj ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

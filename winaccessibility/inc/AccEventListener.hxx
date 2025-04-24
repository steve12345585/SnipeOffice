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

#include <com/sun/star/accessibility/XAccessibleEventListener.hpp>
#include <com/sun/star/accessibility/XAccessible.hpp>

#include <cppuhelper/implbase.hxx>

class AccObjectWinManager;

/**
 * AccEventListener is the general event listener for all controls. It defines the
 * procedure of all the event handling and provides the basic support for some simple
 * methods.
 */
class AccEventListener : public ::cppu::WeakImplHelper<css::accessibility::XAccessibleEventListener>
{
protected:
    //accessible owner's pointer
    css::uno::Reference<css::accessibility::XAccessible> m_xAccessible;
    // reference to object's manager
    AccObjectWinManager& m_rObjManager;

public:
    AccEventListener(css::accessibility::XAccessible* pAcc, AccObjectWinManager& rManager);
    virtual ~AccEventListener() override;

    // XEventListener
    virtual void SAL_CALL disposing(const css::lang::EventObject& Source) override;

    // XAccessibleEventListener
    virtual void SAL_CALL
    notifyEvent(const css::accessibility::AccessibleEventObject& aEvent) override;

    virtual void HandleNameChangedEvent();

    virtual void HandleChildChangedEvent(css::uno::Any oldValue, css::uno::Any newValue);
    virtual void HandleDescriptionChangedEvent();

    //for state changed event
    virtual void HandleStateChangedEvent(css::uno::Any oldValue, css::uno::Any newValue);
    virtual void SetComponentState(sal_Int64 state, bool enable);
    virtual void FireStatePropertyChange(sal_Int64 state, bool set);
    virtual void FireStateFocusedChange(bool enable);

    //for bound rect changed event
    virtual void HandleBoundrectChangedEvent();

    //for visible data changed event
    virtual void HandleVisibleDataChangedEvent();

    // get the accessible role of m_xAccessible
    virtual short GetRole();
    //get the accessible parent's role
    virtual short GetParentRole();

    void RemoveMeFromBroadcaster(bool isNotifyDestroy);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

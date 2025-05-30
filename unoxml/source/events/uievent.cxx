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

#include <uievent.hxx>

using namespace css::uno;
using namespace css::xml::dom::events;
using namespace css::xml::dom::views;

namespace DOM::events
{
    CUIEvent::CUIEvent()
        : m_detail(0)
    {
    }

    Reference< XAbstractView > SAL_CALL
    CUIEvent::getView()
    {
        std::unique_lock const g(m_Mutex);
        return m_view;
    }

    sal_Int32 SAL_CALL CUIEvent::getDetail()
    {
        std::unique_lock const g(m_Mutex);
        return m_detail;
    }

    void SAL_CALL CUIEvent::initUIEvent(const OUString& typeArg,
                     sal_Bool canBubbleArg,
                     sal_Bool cancelableArg,
                     const Reference< XAbstractView >& viewArg,
                     sal_Int32 detailArg)
    {
        CEvent::initEvent(typeArg, canBubbleArg, cancelableArg);
        std::unique_lock const g(m_Mutex);
        m_view = viewArg;
        m_detail = detailArg;
    }


    // delegate to CEvent, since we are inheriting from CEvent and XEvent
    OUString SAL_CALL CUIEvent::getType()
    {
        return CEvent::getType();
    }

    Reference< XEventTarget > SAL_CALL CUIEvent::getTarget()
    {
        return CEvent::getTarget();
    }

    Reference< XEventTarget > SAL_CALL CUIEvent::getCurrentTarget()
    {
        return CEvent::getCurrentTarget();
    }

    PhaseType SAL_CALL CUIEvent::getEventPhase()
    {
        return CEvent::getEventPhase();
    }

    sal_Bool SAL_CALL CUIEvent::getBubbles()
    {
        return CEvent::getBubbles();
    }

    sal_Bool SAL_CALL CUIEvent::getCancelable()
    {
        // mutation events cannot be canceled
        return false;
    }

    css::util::Time SAL_CALL CUIEvent::getTimeStamp()
    {
        return CEvent::getTimeStamp();
    }

    void SAL_CALL CUIEvent::stopPropagation()
    {
        CEvent::stopPropagation();
    }
    void SAL_CALL CUIEvent::preventDefault()
    {
        CEvent::preventDefault();
    }

    void SAL_CALL CUIEvent::initEvent(const OUString& eventTypeArg, sal_Bool canBubbleArg,
        sal_Bool cancelableArg)
    {
        // base initializer
        CEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

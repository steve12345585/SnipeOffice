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
#include <cppuhelper/implbase.hxx>

#include "a11yfocustracker.hxx"
#include "osxvcltypes.h"
#include <set>
#include <com/sun/star/awt/Rectangle.hpp>

class AquaA11yEventListener
    : public ::cppu::WeakImplHelper<css::accessibility::XAccessibleEventListener>
{
public:
    AquaA11yEventListener(id wrapperObject, sal_Int16 role);
    virtual ~AquaA11yEventListener() override;

    // XEventListener
    virtual void SAL_CALL disposing(const css::lang::EventObject& Source) override;

    // XAccessibleEventListener
    virtual void SAL_CALL
    notifyEvent(const css::accessibility::AccessibleEventObject& aEvent) override;

private:
    const id m_wrapperObject;
    const sal_Int16 m_role;
    css::awt::Rectangle m_oldBounds;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

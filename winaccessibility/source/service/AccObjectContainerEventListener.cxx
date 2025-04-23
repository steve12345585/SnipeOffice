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

// AccObjectContainerEventListener.cpp: implementation of the AccContainerEventListener class.

#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>

#include <AccObjectContainerEventListener.hxx>
#include <AccObjectWinManager.hxx>
#include <unomsaaevent.hxx>

using namespace com::sun::star::uno;
using namespace com::sun::star::accessibility;

AccObjectContainerEventListener::AccObjectContainerEventListener(
    css::accessibility::XAccessible* pAcc, AccObjectWinManager& rManager)
    : AccContainerEventListener(pAcc, rManager)
{
}
AccObjectContainerEventListener::~AccObjectContainerEventListener() {}

/**
 *  handle the VISIBLE_DATA_CHANGED event
 *  For SHAPES, the visible_data_changed event should be mapped to LOCATION_CHANGED event
  */
void AccObjectContainerEventListener::HandleVisibleDataChangedEvent()
{
    AccContainerEventListener::HandleBoundrectChangedEvent();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

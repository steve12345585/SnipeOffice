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

#include <TimerTriggeredControllerLock.hxx>
#include <ControllerLockGuard.hxx>
#include <ChartModel.hxx>
#include <utility>

namespace chart
{
using namespace ::com::sun::star;

TimerTriggeredControllerLock::TimerTriggeredControllerLock(
    rtl::Reference<::chart::ChartModel> xModel)
    : m_xModel(std::move(xModel))
    , m_aTimer("chart2 TimerTriggeredControllerLock")
{
    m_aTimer.SetTimeout(4 * EDIT_UPDATEDATA_TIMEOUT);
    m_aTimer.SetInvokeHandler(LINK(this, TimerTriggeredControllerLock, TimerTimeout));
}
TimerTriggeredControllerLock::~TimerTriggeredControllerLock() { m_aTimer.Stop(); }

void TimerTriggeredControllerLock::startTimer()
{
    if (!m_apControllerLockGuard)
        m_apControllerLockGuard.reset(new ControllerLockGuardUNO(m_xModel));
    m_aTimer.Start();
}
IMPL_LINK_NOARG(TimerTriggeredControllerLock, TimerTimeout, Timer*, void)
{
    m_apControllerLockGuard.reset();
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

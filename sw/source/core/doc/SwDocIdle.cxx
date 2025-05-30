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

#include <view.hxx>
#include <wrtsh.hxx>
#include <doc.hxx>
#include <docsh.hxx>
#include <viewopt.hxx>
#include <vcl/scheduler.hxx>

#include <SwDocIdle.hxx>
#include <IDocumentTimerAccess.hxx>

namespace sw
{

sal_uInt64 SwDocIdle::UpdateMinPeriod( sal_uInt64 /* nTimeNow */ ) const
{
    bool bReadyForSchedule = true;

    SwView* pView = m_rDoc.GetDocShell() ? m_rDoc.GetDocShell()->GetView() : nullptr;
    if( pView )
    {
        SwWrtShell& rWrtShell = pView->GetWrtShell();
        bReadyForSchedule = rWrtShell.GetViewOptions()->IsIdle();
    }

    if( bReadyForSchedule && !m_rDoc.getIDocumentTimerAccess().IsDocIdle() )
        bReadyForSchedule = false;

    return bReadyForSchedule
        ? Scheduler::ImmediateTimeoutMs : Scheduler::InfiniteTimeoutMs;
}

SwDocIdle::SwDocIdle( SwDoc &doc, const char * pDebugIdleName )
    : Idle(pDebugIdleName), m_rDoc( doc )
{
}

SwDocIdle::~SwDocIdle()
{
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

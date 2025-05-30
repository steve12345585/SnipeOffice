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

#include <sfx2/sidebar/AsynchronousCall.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/lokhelper.hxx>

namespace sfx2::sidebar {

AsynchronousCall::AsynchronousCall (const SfxViewFrame* pViewFrame, Action aAction)
    : maAction(std::move(aAction)),
      mnCallId(nullptr),
      mpViewFrame(pViewFrame)
{
}

AsynchronousCall::~AsynchronousCall()
{
    CancelRequest();
}

void AsynchronousCall::RequestCall()
{
    if (mnCallId == nullptr)
    {
        Link<void*,void> aLink (LINK(this, AsynchronousCall, HandleUserCall));
        mnCallId = Application::PostUserEvent(aLink);
    }
}

void AsynchronousCall::CancelRequest()
{
    if (mnCallId != nullptr)
    {
        Application::RemoveUserEvent(mnCallId);
        mnCallId = nullptr;
    }
}

void AsynchronousCall::Sync()
{
    if (mnCallId != nullptr) {
        SfxLokLanguageGuard aGuard(mpViewFrame ? mpViewFrame->GetViewShell() : nullptr);
        maAction();
        CancelRequest();
    }
}

IMPL_LINK_NOARG(AsynchronousCall, HandleUserCall, void*, void )
{
    mnCallId = nullptr;
    if (maAction)
    {
        SfxLokLanguageGuard aGuard(mpViewFrame ? mpViewFrame->GetViewShell() : nullptr);
        maAction();
    }
}

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

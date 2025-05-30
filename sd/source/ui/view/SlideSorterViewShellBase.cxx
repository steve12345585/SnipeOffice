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

#include <SlideSorterViewShellBase.hxx>
#include <DrawDocShell.hxx>
#include <framework/FrameworkHelper.hxx>
#include <sfx2/viewfac.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>

namespace sd {

class DrawDocShell;


// We have to expand the SFX_IMPL_VIEWFACTORY macro to call LateInit() after a
// new SlideSorterViewShellBase object has been constructed.

SfxViewFactory* SlideSorterViewShellBase::s_pFactory;
SfxViewShell* SlideSorterViewShellBase::CreateInstance (
    SfxViewFrame& rFrame, SfxViewShell *pOldView)
{
    SlideSorterViewShellBase* pBase = new SlideSorterViewShellBase(rFrame, pOldView);
    pBase->LateInit(framework::FrameworkHelper::msSlideSorterURL);
    return pBase;
}

void SlideSorterViewShellBase::RegisterFactory( SfxInterfaceId nPrio )
{
    s_pFactory = new SfxViewFactory(&CreateInstance,nPrio,"SlideSorter");
    InitFactory();
}

void SlideSorterViewShellBase::InitFactory()
{
    SFX_VIEW_REGISTRATION(DrawDocShell);
}

SlideSorterViewShellBase::SlideSorterViewShellBase (
    SfxViewFrame& _rFrame,
    SfxViewShell* pOldShell)
    : ImpressViewShellBase (_rFrame, pOldShell)
{
}

SlideSorterViewShellBase::~SlideSorterViewShellBase()
{
}

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

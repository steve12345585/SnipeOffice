/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_VCL_LOK_HXX
#define INCLUDED_VCL_LOK_HXX

#include <sal/config.h>
#include <vcl/dllapi.h>
#include <rtl/strbuf.hxx>
#include <LibreOfficeKit/LibreOfficeKitTypes.h>

namespace vcl::lok
{
bool VCL_DLLPUBLIC isUnipoll();
void VCL_DLLPUBLIC registerPollCallbacks(LibreOfficeKitPollCallback pPollCallback,
                                         LibreOfficeKitWakeCallback pWakeCallback, void* pData);
void VCL_DLLPUBLIC unregisterPollCallbacks();

// Called to tell VCL that the number of document views has changed, so that VCL
// can adjust e.g. sizes of bitmap caches to scale well with larger number of users.
void VCL_DLLPUBLIC numberOfViewsChanged(int count);

// Trim memory use by wiping various internal caches
void VCL_DLLPUBLIC trimMemory(int nTarget);

// Dump internal state of VCL windows etc. for debugging
void VCL_DLLPUBLIC dumpState(rtl::OStringBuffer& rState);
}

#endif // INCLUDE_VCL_LOK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

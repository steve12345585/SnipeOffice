/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <string_view>
#include <vector>

#include <sal/types.h>

void callVirtualFunction(std::string_view signature, sal_uInt32 target, sal_uInt64 const* arguments,
                         void* returnValue);

sal_uInt64 vtableCall(sal_Int32 functionIndex, sal_Int32 vtableOffset, unsigned thisPtr,
                      std::vector<sal_uInt64> const& arguments, unsigned indirectRet);

void const* getVtableSlotFunction(std::string_view signature);

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

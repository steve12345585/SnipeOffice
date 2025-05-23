/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

#include <CLucene.h>

#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

#include <rtl/ustring.hxx>
#include <vector>

std::vector<TCHAR> OUStringToTCHARVec(OUString const& rStr);
OUString TCHARArrayToOUString(TCHAR const* str);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

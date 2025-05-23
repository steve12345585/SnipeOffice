/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_TEST_CALLGRIND_HXX
#define INCLUDED_TEST_CALLGRIND_HXX

#include <sal/config.h>
#include <test/testdllapi.hxx>

void OOO_DLLPUBLIC_TEST callgrindStart();
void OOO_DLLPUBLIC_TEST callgrindDump(const char* name);

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

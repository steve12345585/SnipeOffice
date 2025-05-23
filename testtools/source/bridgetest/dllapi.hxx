/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_TESTTOOLS_SOURCE_BRIDGETEST_DLLAPI_HXX
#define INCLUDED_TESTTOOLS_SOURCE_BRIDGETEST_DLLAPI_HXX

#include <sal/config.h>

#include <sal/types.h>

#if defined LO_DLLIMPLEMENTATION_TESTTOOLS
#define LO_DLLPUBLIC_TESTTOOLS SAL_DLLPUBLIC_EXPORT
#else
#define LO_DLLPUBLIC_TESTTOOLS SAL_DLLPUBLIC_IMPORT
#endif

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

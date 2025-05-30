/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define LOK_USE_UNSTABLE_API
#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitInit.h>

// fake usage for loplugin:unreffun plugin
#include "test.h"

// just make sure this stuff compiles from a plain C file
LibreOfficeKit* compile_test(void) { return lok_init("install/path"); }

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#pragma once

#include <sal/config.h>

#include <sal/types.h>

enum SAL_DLLPUBLIC_RTTI DesktopType
{
    DESKTOP_NONE, // headless, i.e. no X connection at all
    DESKTOP_UNKNOWN, // unknown desktop, simple WM, etc.
    DESKTOP_GNOME,
    DESKTOP_UNITY,
    DESKTOP_XFCE,
    DESKTOP_MATE,
    DESKTOP_PLASMA5,
    DESKTOP_PLASMA6,
    DESKTOP_LXQT
}; // keep in sync with desktop_strings[] in salplug.cxx

DesktopType get_desktop_environment();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <comphelper/mediamimetype.hxx>

#include <unotools/resmgr.hxx>

#ifdef _WIN32
#define AVMEDIA_MANAGER_SERVICE_NAME "com.sun.star.comp.avmedia.Manager_DirectX"
#else
#ifdef MACOSX
#define AVMEDIA_MANAGER_SERVICE_NAME "com.sun.star.comp.avmedia.Manager_MacAVF"
#else
#define AVMEDIA_MANAGER_SERVICE_NAME "com.sun.star.comp.avmedia.Manager_GStreamer"
#endif
#endif

inline OUString AvmResId(TranslateId aId)
{
    return Translate::get(aId, Translate::Create("avmedia"));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

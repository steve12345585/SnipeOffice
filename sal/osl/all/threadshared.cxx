/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <sal/config.h>

#include <osl/thread.h>

#include <thread_internal.hxx>

#include <utility>

namespace
{
rtl_TextEncoding& getThreadTextEncodingImpl()
{
    // Use OS-specific initial value
    static thread_local rtl_TextEncoding s_enc = getThreadTextEncodingForInitialization();
    return s_enc;
}
}

rtl_TextEncoding SAL_CALL osl_getThreadTextEncoding() { return getThreadTextEncodingImpl(); }

rtl_TextEncoding SAL_CALL osl_setThreadTextEncoding(rtl_TextEncoding Encoding)
{
    return std::exchange(getThreadTextEncodingImpl(), Encoding);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

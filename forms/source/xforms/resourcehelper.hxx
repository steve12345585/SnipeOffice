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

#include <rtl/ustring.hxx>
#include <unotools/resmgr.hxx>

namespace xforms
{
    /// get a resource string for the current language
    OUString getResource(TranslateId);

    // overloaded: get a resource string, and substitute parameters
    OUString getResource(TranslateId, std::u16string_view);
    OUString getResource(TranslateId, std::u16string_view,
                                           std::u16string_view);
    OUString getResource(TranslateId, std::u16string_view,
                                           std::u16string_view,
                                           std::u16string_view);

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

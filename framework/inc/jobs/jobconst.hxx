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

#include <rtl/ustring.hxx>

namespace framework
{
/**
    @short  defines all constant values used within a job environment.

    @descr  The protocol between job executor/dispatch and jobs is fix and well defined.
            But why every implemented job should write such constant values directly into
            it's code. Typos can occur or code will be changed by new developers ...
            Shared set of constant values can help to improve the maintenance of this code.
 */
class JobConst
{
public:
    static constexpr OUString ANSWER_DEACTIVATE_JOB = u"Deactivate"_ustr;
    static constexpr OUString ANSWER_SAVE_ARGUMENTS = u"SaveArguments"_ustr;
    static constexpr OUString ANSWER_SEND_DISPATCHRESULT = u"SendDispatchResult"_ustr;
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

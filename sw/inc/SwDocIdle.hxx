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

#include <vcl/idle.hxx>

class SwDoc;

namespace sw
{
/**
 * An Idle, which is just ready to be scheduled for idle documents.
 *
 * Currently it's missing the notification, when busy documents become idle
 * again, so it relies on any task being triggered to recheck, which is
 * quite probably not a problem, as busy documents have a high chance to have
 * generated idle tasks.
 */
class SwDocIdle final : public Idle
{
private:
    SwDoc& m_rDoc;

    virtual sal_uInt64 UpdateMinPeriod(sal_uInt64 nTimeNow) const override;

public:
    SwDocIdle(SwDoc& doc, const char* pDebugIdleName);
    virtual ~SwDocIdle() override;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

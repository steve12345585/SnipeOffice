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

#include "portxt.hxx"

class SwToxPortion : public SwTextPortion
{
public:
    SwToxPortion() { SetWhichPor(PortionType::Tox); }
    virtual void Paint(const SwTextPaintInfo& rInf) const override;
};

class SwIsoToxPortion : public SwToxPortion
{
    SwTwips m_nViewWidth;

public:
    SwIsoToxPortion();
    virtual bool Format(SwTextFormatInfo& rInf) override;
    virtual void Paint(const SwTextPaintInfo& rInf) const override;
    virtual SwLinePortion* Compress() override;
    virtual SwTwips GetViewWidth(const SwTextSizeInfo& rInf) const override;

    // Accessibility: pass information about this portion to the PortionHandler
    virtual void HandlePortion(SwPortionHandler& rPH) const override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

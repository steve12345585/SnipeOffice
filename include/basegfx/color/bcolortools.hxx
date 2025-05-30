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

#include <basegfx/basegfxdllapi.h>

namespace basegfx
{
class BColor;
}

namespace basegfx::utils
{
/// Transform from RGB to HSL
BASEGFX_DLLPUBLIC BColor rgb2hsl(const BColor& rRGBColor);
/// Transform from HSL to RGB
BASEGFX_DLLPUBLIC BColor hsl2rgb(const BColor& rHSLColor);

/// Transform from RGB to HSV
BASEGFX_DLLPUBLIC BColor rgb2hsv(const BColor& rRGBColor);
/// Transform from HSV to RGB
BASEGFX_DLLPUBLIC BColor hsv2rgb(const BColor& rHSVColor);

} // end of namespace basegfx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

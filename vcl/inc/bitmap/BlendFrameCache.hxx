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

#include <tools/color.hxx>
#include <tools/gen.hxx>

#include <vcl/bitmapex.hxx>

struct BlendFrameCache
{
    Size m_aLastSize;
    sal_uInt8 m_nLastAlpha;
    Color m_aLastColorTopLeft;
    Color m_aLastColorTopRight;
    Color m_aLastColorBottomRight;
    Color m_aLastColorBottomLeft;
    BitmapEx m_aLastResult;

    BlendFrameCache(Size const& rSize, sal_uInt8 nAlpha, Color const& rColorTopLeft,
                    Color const& rColorTopRight, Color const& rColorBottomRight,
                    Color const& rColorBottomLeft);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

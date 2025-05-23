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

#ifndef INCLUDED_SVX_SOURCE_SVDRAW_GRADTRNS_HXX
#define INCLUDED_SVX_SOURCE_SVDRAW_GRADTRNS_HXX

#include <tools/color.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/utils/bgradient.hxx>

class SdrObject;

class GradTransVector
{
public:
    basegfx::B2DPoint           maPositionA;
    basegfx::B2DPoint           maPositionB;
    Color                       aCol1;
    Color                       aCol2;
};

class GradTransGradient
{
public:
    basegfx::BGradient          aGradient;
};

struct GradTransformer
{
    GradTransformer() = delete;

    static void GradToVec(GradTransGradient const & rG, GradTransVector& rV,
        const SdrObject* pObj);
    static void VecToGrad(GradTransVector const & rV, GradTransGradient& rG,
        GradTransGradient const & rGOld, const SdrObject* pObj, bool bMoveSingle, bool bMoveFirst);
};

#endif // INCLUDED_SVX_SOURCE_SVDRAW_GRADTRNS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

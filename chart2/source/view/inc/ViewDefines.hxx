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

#include <sal/types.h>

namespace chart
{
#define CHART_3DOBJECT_SEGMENTCOUNT (sal_Int32(32))
//There needs to be a little distance between grid lines and walls in 3D, otherwise the lines are partly hidden by the walls
#define GRID_TO_WALL_DISTANCE (1.0)

const double ZDIRECTION = 1.0;
const sal_Int32 AXIS2D_TICKLENGTH = 150; //value like in old chart
const sal_Int32 AXIS2D_TICKLABELSPACING = 100; //value like in old chart

} //end namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

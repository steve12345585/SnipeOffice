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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_TRANSITIONTOOLS_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_TRANSITIONTOOLS_HXX

#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>

namespace slideshow::internal
{
/// Create a unit rect.
::basegfx::B2DPolygon createUnitRect();

/// Flips on X-axis:
::basegfx::B2DPolyPolygon flipOnXAxis(::basegfx::B2DPolyPolygon const& polypoly);

/// Flips on Y-axis:
::basegfx::B2DPolyPolygon flipOnYAxis(::basegfx::B2DPolyPolygon const& polypoly);
}

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_TRANSITIONTOOLS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

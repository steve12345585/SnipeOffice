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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_RANDOMWIPE_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_RANDOMWIPE_HXX

#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <memory>

#include "parametricpolypolygon.hxx"

namespace slideshow::internal
{
class RandomWipe : public ParametricPolyPolygon
{
public:
    RandomWipe(sal_Int32 nElements, bool randomBars /* true: generates a horizontal random bar wipe,
                                                      false: generates a dissolve wipe */);
    virtual ::basegfx::B2DPolyPolygon operator()(double t) override;

private:
    ::std::unique_ptr<::basegfx::B2DPoint[]> m_positions;
    sal_Int32 m_nElements;
    ::basegfx::B2DPolygon m_rect;
};
}

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_RANDOMWIPE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

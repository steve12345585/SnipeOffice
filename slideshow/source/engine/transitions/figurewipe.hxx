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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_FIGUREWIPE_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_FIGUREWIPE_HXX

#include <utility>

#include "parametricpolypolygon.hxx"


namespace slideshow::internal {

class FigureWipe : public ParametricPolyPolygon
{
public:
    static std::shared_ptr<FigureWipe> createTriangleWipe();
    static std::shared_ptr<FigureWipe> createArrowHeadWipe();
    static std::shared_ptr<FigureWipe> createStarWipe( sal_Int32 nPoints );
    static std::shared_ptr<FigureWipe> createPentagonWipe();
    static std::shared_ptr<FigureWipe> createHexagonWipe();

    virtual ::basegfx::B2DPolyPolygon operator () ( double t ) override;
    explicit FigureWipe( ::basegfx::B2DPolygon figure ) : m_figure(std::move(figure)) {}
private:
    const ::basegfx::B2DPolygon m_figure;
};


}

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_TRANSITIONS_FIGUREWIPE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <com/sun/star/drawing/PointSequenceSequence.hpp>
#include "LoggedResources.hxx"
#include <vector>

namespace com::sun::star::text
{
struct GraphicCrop;
}

namespace writerfilter::dmapper
{
/// Handles <wp:wrapPolygon> from DOCX and the pWrapPolygonVertices shape property from RTF.
class WrapPolygon final : public virtual SvRefBase
{
public:
    typedef std::vector<css::awt::Point> Points_t;
    typedef ::tools::SvRef<WrapPolygon> Pointer_t;

private:
    Points_t mPoints;

public:
    WrapPolygon();
    ~WrapPolygon() override;

    void addPoint(const css::awt::Point& rPoint);

    Points_t::const_iterator begin() const;
    Points_t::const_iterator end() const;

    WrapPolygon::Pointer_t move(const css::awt::Point& rMove) const;
    WrapPolygon::Pointer_t scale(double scaleX, double scaleY) const;
    WrapPolygon::Pointer_t correctWordWrapPolygon(const css::awt::Size& rSrcSize) const;
    WrapPolygon::Pointer_t correctWordWrapPolygonPixel(const css::awt::Size& rSrcSize) const;
    WrapPolygon::Pointer_t correctCrop(const css::awt::Size& rGraphicSize,
                                       const css::text::GraphicCrop& rGraphicCrop) const;
    css::drawing::PointSequenceSequence getPointSequenceSequence() const;
};

class WrapPolygonHandler : public LoggedProperties
{
public:
    WrapPolygonHandler();
    virtual ~WrapPolygonHandler() override;

    const WrapPolygon::Pointer_t& getPolygon() const { return mpPolygon; }

private:
    WrapPolygon::Pointer_t mpPolygon;

    sal_Int32 mnX;
    sal_Int32 mnY;

    // Properties
    virtual void lcl_attribute(Id Name, const Value& val) override;
    virtual void lcl_sprm(Sprm& sprm) override;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

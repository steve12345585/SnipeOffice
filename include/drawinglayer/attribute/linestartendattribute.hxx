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

#include <drawinglayer/drawinglayerdllapi.h>
#include <o3tl/cow_wrapper.hxx>

namespace basegfx
{
class B2DPolyPolygon;
}

namespace drawinglayer::attribute
{
class ImpLineStartEndAttribute;
}

namespace drawinglayer::attribute
{
class DRAWINGLAYER_DLLPUBLIC LineStartEndAttribute
{
public:
    typedef o3tl::cow_wrapper<ImpLineStartEndAttribute> ImplType;

private:
    ImplType mpLineStartEndAttribute;

public:
    /// constructors/assignmentoperator/destructor
    LineStartEndAttribute(double fWidth, const basegfx::B2DPolyPolygon& rPolyPolygon,
                          bool bCentered);
    LineStartEndAttribute();
    LineStartEndAttribute(const LineStartEndAttribute&);
    LineStartEndAttribute& operator=(const LineStartEndAttribute&);
    ~LineStartEndAttribute();

    // checks if the incarnation is default constructed
    bool isDefault() const;

    // compare operator
    bool operator==(const LineStartEndAttribute& rCandidate) const;

    // data read access
    double getWidth() const;
    const basegfx::B2DPolyPolygon& getB2DPolyPolygon() const;
    bool isCentered() const;
    bool isActive() const;
};

} // end of namespace drawinglayer::attribute

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

class Graphic;

namespace basegfx
{
class B2DRange;
}

namespace drawinglayer::attribute
{
class ImpFillGraphicAttribute;

class DRAWINGLAYER_DLLPUBLIC FillGraphicAttribute
{
public:
    typedef o3tl::cow_wrapper<ImpFillGraphicAttribute> ImplType;

private:
    ImplType mpFillGraphicAttribute;

public:
    /// constructors/assignmentoperator/destructor
    FillGraphicAttribute();
    FillGraphicAttribute(const Graphic& rGraphic, const basegfx::B2DRange& rGraphicRange,
                         bool bTiling, double fOffsetX = 0.0, double fOffsetY = 0.0);
    FillGraphicAttribute(const FillGraphicAttribute&);
    FillGraphicAttribute& operator=(const FillGraphicAttribute&);
    ~FillGraphicAttribute();

    // checks if the incarnation is default constructed
    bool isDefault() const;

    // compare operator
    bool operator==(const FillGraphicAttribute& rCandidate) const;

    // data read access
    const Graphic& getGraphic() const;
    const basegfx::B2DRange& getGraphicRange() const;
    bool getTiling() const;
    double getOffsetX() const;
    double getOffsetY() const;
};

} // end of namespace drawinglayer::attribute

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

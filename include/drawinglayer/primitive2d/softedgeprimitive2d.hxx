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
#include <drawinglayer/primitive2d/BufferedDecompositionGroupPrimitive2D.hxx>

namespace drawinglayer::primitive2d
{
class DRAWINGLAYER_DLLPUBLIC SoftEdgePrimitive2D final
    : public BufferedDecompositionGroupPrimitive2D
{
private:
    /// Soft edge size, in logical units (100ths of mm)
    double mfRadius;

    /// last used DiscreteSoftRadius and ClippedRange
    double mfLastDiscreteSoftRadius;
    basegfx::B2DRange maLastClippedRange;

    /// helpers
    bool prepareValuesAndcheckValidity(basegfx::B2DRange& rSoftRange,
                                       basegfx::B2DRange& rClippedRange,
                                       basegfx::B2DVector& rDiscreteSoftSize,
                                       double& rfDiscreteSoftRadius,
                                       const geometry::ViewInformation2D& rViewInformation) const;

protected:
    /** method which is to be used to implement the local decomposition of a 2D primitive. */
    virtual void
    create2DDecomposition(Primitive2DContainer& rContainer,
                          const geometry::ViewInformation2D& rViewInformation) const override;

public:
    /// constructor
    SoftEdgePrimitive2D(double fRadius, Primitive2DContainer&& aChildren);

    /// data read access
    double getRadius() const { return mfRadius; }

    /// compare operator
    virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

    /// get range
    virtual basegfx::B2DRange
    getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;

    /// The default implementation will return an empty sequence
    virtual void
    get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor,
                       const geometry::ViewInformation2D& rViewInformation) const override;

    /// provide unique ID
    virtual sal_uInt32 getPrimitive2DID() const override;
};
} // end of namespace drawinglayer::primitive2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <VPolarTransformation.hxx>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Sequence;

namespace chart
{

VPolarTransformation::VPolarTransformation( const PolarPlottingPositionHelper& rPositionHelper )
        : m_aPositionHelper(rPositionHelper)
        , m_aUnitCartesianToScene( rPositionHelper.getUnitCartesianToScene() )
{
}

VPolarTransformation::~VPolarTransformation()
{
}

// ____ XTransformation2 ____
css::drawing::Position3D VPolarTransformation::transform(
                        const Sequence< double >& rSourceValues ) const
{
    double fScaledLogicAngle  = rSourceValues[0];
    double fScaledLogicRadius = rSourceValues[1];

    if( m_aPositionHelper.isSwapXAndY() )
        std::swap(fScaledLogicAngle,fScaledLogicRadius);

    double fAngleDegree = m_aPositionHelper.transformToAngleDegree( fScaledLogicAngle, false );
    double fAnglePi     = basegfx::deg2rad(fAngleDegree);
    double fRadius      = m_aPositionHelper.transformToRadius( fScaledLogicRadius, false);

    double fX=fRadius*cos(fAnglePi);
    double fY=fRadius*sin(fAnglePi);
    double fZ=rSourceValues[2];

    //!! applying matrix to vector does ignore translation, so it is important to use a B3DPoint here instead of B3DVector
    ::basegfx::B3DPoint aPoint(fX,fY,fZ);
    ::basegfx::B3DPoint aRet = m_aUnitCartesianToScene * aPoint;
    return css::drawing::Position3D(aRet.getX(), aRet.getY(), aRet.getZ());
}

css::drawing::Position3D VPolarTransformation::transform(
                        const css::drawing::Position3D& rSourceValues ) const
{
    double fScaledLogicAngle  = rSourceValues.PositionX;
    double fScaledLogicRadius = rSourceValues.PositionY;

    if( m_aPositionHelper.isSwapXAndY() )
        std::swap(fScaledLogicAngle,fScaledLogicRadius);

    double fAngleDegree = m_aPositionHelper.transformToAngleDegree( fScaledLogicAngle, false );
    double fAnglePi     = basegfx::deg2rad(fAngleDegree);
    double fRadius      = m_aPositionHelper.transformToRadius( fScaledLogicRadius, false);

    double fX=fRadius*cos(fAnglePi);
    double fY=fRadius*sin(fAnglePi);
    double fZ=rSourceValues.PositionZ;

    //!! applying matrix to vector does ignore translation, so it is important to use a B3DPoint here instead of B3DVector
    ::basegfx::B3DPoint aPoint(fX,fY,fZ);
    ::basegfx::B3DPoint aRet = m_aUnitCartesianToScene * aPoint;
    return css::drawing::Position3D(aRet.getX(), aRet.getY(), aRet.getZ());
}

}  // namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

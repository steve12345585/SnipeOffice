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

#include <sal/config.h>

#include <o3tl/string_view.hxx>
#include <svx/EnhancedCustomShape2d.hxx>
#include <svx/EnhancedCustomShapeGeometry.hxx>
#include <svx/EnhancedCustomShapeTypeNames.hxx>
#include <svx/svdoashp.hxx>
#include <svx/svdtrans.hxx>
#include <svx/svdogrp.hxx>
#include <svx/svdopath.hxx>
#include <svx/svdorect.hxx>
#include <svx/svdpage.hxx>
#include <svx/xflclit.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xlineit0.hxx>
#include <svx/xlnstit.hxx>
#include <svx/xlnedit.hxx>
#include <svx/xlnstwit.hxx>
#include <svx/xlnedwit.hxx>
#include <svx/xlnstcit.hxx>
#include <svx/xlnedcit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/xhatch.hxx>
#include <svx/sdshitm.hxx>
#include <comphelper/configuration.hxx>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeParameterType.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeSegmentCommand.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeAdjustmentValue.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeSegment.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeTextFrame.hpp>
#include <basegfx/numeric/ftools.hxx>
#include <basegfx/color/bcolortools.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <unordered_set>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::drawing::EnhancedCustomShapeSegmentCommand;

void EnhancedCustomShape2d::SetEnhancedCustomShapeParameter( EnhancedCustomShapeParameter& rParameter, const sal_Int32 nValue )
{
    sal_uInt32 nDat = static_cast<sal_uInt32>(nValue);
    sal_Int32  nNewValue = nValue;

    // check if this is a special point
    if ( ( nDat >> 16 ) == 0x8000 )
    {
        nNewValue = static_cast<sal_uInt16>(nDat);
        rParameter.Type = EnhancedCustomShapeParameterType::EQUATION;
    }
    else
        rParameter.Type = EnhancedCustomShapeParameterType::NORMAL;
    rParameter.Value <<= nNewValue;
}

OUString EnhancedCustomShape2d::GetEquation( const sal_uInt16 nFlags, sal_Int32 nP1, sal_Int32 nP2, sal_Int32 nP3 )
{
    OUString aEquation;
    bool b1Special = ( nFlags & 0x2000 ) != 0;
    bool b2Special = ( nFlags & 0x4000 ) != 0;
    bool b3Special = ( nFlags & 0x8000 ) != 0;
    switch( nFlags & 0xff )
    {
        case 0 :
        case 14 :
        {
            sal_Int32 nOptimize = 0;
            if ( nP1 )
                nOptimize |= 1;
            if ( nP2 )
                nOptimize |= 2;
            if ( b1Special )
                nOptimize |= 4;
            if ( b2Special )
                nOptimize |= 8;
            switch( nOptimize )
            {
                case 0 :
                break;
                case 1 :
                case 4 :
                case 5 :
                    EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
                break;
                case 2 :
                case 8 :
                case 10:
                    EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
                break;
                default :
                {
                    EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
                    aEquation += "+";
                    EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
                }
                break;
            }
            if ( b3Special || nP3 )
            {
                aEquation += "-";
                EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            }
        }
        break;
        case 1 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            if ( b2Special || ( nP2 != 1 ) )
            {
                aEquation += "*";
                EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            }
            if ( b3Special || ( ( nP3 != 1 ) && ( nP3 != 0 ) ) )
            {
                aEquation += "/";
                EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            }
        }
        break;
        case 2 :
        {
            aEquation += "(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "+";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ")/2";
        }
        break;
        case 3 :
        {
            aEquation += "abs(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ")";
        }
        break;
        case 4 :
        {
            aEquation += "min(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ")";
        }
        break;
        case 5 :
        {
            aEquation += "max(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ")";
        }
        break;
        case 6 :
        {
            aEquation += "if(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += ")";
        }
        break;
        case 7 :
        {
            aEquation += "sqrt(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "+";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "*";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "+";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += ")";
        }
        break;
        case 8 :
        {
            aEquation += "atan2(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ")/(pi/180)";
        }
        break;
        case 9 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*sin(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "*(pi/180))";
        }
        break;
        case 10 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*cos(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "*(pi/180))";
        }
        break;
        case 11 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*cos(atan2(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "))";
        }
        break;
        case 12 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*sin(atan2(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += ",";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "))";
        }
        break;
        case 13 :
        {
            aEquation += "sqrt(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ")";
        }
        break;
        case 15 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*sqrt(1-(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "/";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ")"
                "*(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "/";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "))";
        }
        break;
        case 16 :
        {
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*tan(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += ")";
        }
        break;
        case 0x80 :
        {
            aEquation += "sqrt(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "-";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "*";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += ")";
        }
        break;
        case 0x81 :
        {
            aEquation += "(cos(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*(pi/180))*(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "-10800)+sin(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*(pi/180))*(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "-10800))+10800";
        }
        break;
        case 0x82 :
        {
            aEquation += "-(sin(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*(pi/180))*(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP1, b1Special );
            aEquation += "-10800)-cos(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP3, b3Special );
            aEquation += "*(pi/180))*(";
            EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( aEquation, nP2, b2Special );
            aEquation += "-10800))+10800";
        }
        break;
    }
    return aEquation;
}

void EnhancedCustomShape2d::AppendEnhancedCustomShapeEquationParameter( OUString& rParameter, const sal_Int32 nPara, const bool bIsSpecialValue )
{
    if ( bIsSpecialValue )
    {
        if ( nPara & 0x400 )
        {
            rParameter += "?";
            rParameter += OUString::number( nPara & 0xff );
            rParameter += " ";
        }
        else
        {
            switch( nPara )
            {
                case DFF_Prop_adjustValue :
                case DFF_Prop_adjust2Value :
                case DFF_Prop_adjust3Value :
                case DFF_Prop_adjust4Value :
                case DFF_Prop_adjust5Value :
                case DFF_Prop_adjust6Value :
                case DFF_Prop_adjust7Value :
                case DFF_Prop_adjust8Value :
                case DFF_Prop_adjust9Value :
                case DFF_Prop_adjust10Value :
                {
                    rParameter += "$";
                    rParameter += OUString::number( nPara - DFF_Prop_adjustValue );
                    rParameter += " ";
                }
                break;
                case DFF_Prop_geoLeft :
                {
                    rParameter += "left";
                }
                break;
                case DFF_Prop_geoTop :
                {
                    rParameter += "top";
                }
                break;
                case DFF_Prop_geoRight :
                {
                    rParameter += "right";
                }
                break;
                case DFF_Prop_geoBottom :
                {
                    rParameter += "bottom";
                }
                break;
            }
        }
    }
    else
    {
        rParameter += OUString::number( nPara );
    }
}

void EnhancedCustomShape2d::SetEnhancedCustomShapeHandleParameter( EnhancedCustomShapeParameter& rParameter, const sal_Int32 nPara, const bool bIsSpecialValue, bool bHorz )
{
    sal_Int32 nValue = 0;
    if ( bIsSpecialValue )
    {
        if ( ( nPara >= 0x100 ) && ( nPara <= 0x107 ) )
        {
            nValue = nPara & 0xff;
            rParameter.Type = EnhancedCustomShapeParameterType::ADJUSTMENT;
        }
        else if ( ( nPara >= 3 ) && ( nPara <= 0x82 ) )
        {
            nValue = nPara - 3;
            rParameter.Type = EnhancedCustomShapeParameterType::EQUATION;
        }
        else if ( nPara == 0 )
        {
            nValue = 0;
            if ( bHorz )
                rParameter.Type = EnhancedCustomShapeParameterType::LEFT;
            else
                rParameter.Type = EnhancedCustomShapeParameterType::TOP;
        }
        else if ( nPara == 1 )
        {
            nValue = 0;
            if ( bHorz )
                rParameter.Type = EnhancedCustomShapeParameterType::RIGHT;
            else
                rParameter.Type = EnhancedCustomShapeParameterType::BOTTOM;
        }
        else if ( nPara == 2 )  // means to be centered, but should not be
        {                       // used in our implementation
            nValue = 5600;
            rParameter.Type = EnhancedCustomShapeParameterType::NORMAL;
        }
        else
        {
            nValue = nPara;
            rParameter.Type = EnhancedCustomShapeParameterType::NORMAL;
        }
    }
    else
    {
        nValue = nPara;
        rParameter.Type = EnhancedCustomShapeParameterType::NORMAL;
    }
    rParameter.Value <<= nValue;
}

bool EnhancedCustomShape2d::ConvertSequenceToEnhancedCustomShape2dHandle(
    const css::beans::PropertyValues& rHandleProperties,
        EnhancedCustomShape2d::Handle& rDestinationHandle )
{
    bool bRetValue = false;
    if ( rHandleProperties.hasElements() )
    {
        rDestinationHandle.nFlags = HandleFlags::NONE;
        for ( const css::beans::PropertyValue& rPropVal : rHandleProperties )
        {
            if ( rPropVal.Name == "Position" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aPosition )
                    bRetValue = true;
            }
            else if ( rPropVal.Name == "MirroredX" )
            {
                bool bMirroredX;
                if ( rPropVal.Value >>= bMirroredX )
                {
                    if ( bMirroredX )
                        rDestinationHandle.nFlags |= HandleFlags::MIRRORED_X;
                }
            }
            else if ( rPropVal.Name == "MirroredY" )
            {
                bool bMirroredY;
                if ( rPropVal.Value >>= bMirroredY )
                {
                    if ( bMirroredY )
                        rDestinationHandle.nFlags |= HandleFlags::MIRRORED_Y;
                }
            }
            else if ( rPropVal.Name == "Switched" )
            {
                bool bSwitched;
                if ( rPropVal.Value >>= bSwitched )
                {
                    if ( bSwitched )
                        rDestinationHandle.nFlags |= HandleFlags::SWITCHED;
                }
            }
            else if ( rPropVal.Name == "Polar" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aPolar )
                    rDestinationHandle.nFlags |= HandleFlags::POLAR;
            }
            else if ( rPropVal.Name == "RefX" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.nRefX )
                    rDestinationHandle.nFlags |= HandleFlags::REFX;
            }
            else if ( rPropVal.Name == "RefY" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.nRefY )
                    rDestinationHandle.nFlags |= HandleFlags::REFY;
            }
            else if ( rPropVal.Name == "RefAngle" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.nRefAngle )
                    rDestinationHandle.nFlags |= HandleFlags::REFANGLE;
            }
            else if ( rPropVal.Name == "RefR" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.nRefR )
                    rDestinationHandle.nFlags |= HandleFlags::REFR;
            }
            else if ( rPropVal.Name == "RadiusRangeMinimum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aRadiusRangeMinimum )
                    rDestinationHandle.nFlags |= HandleFlags::RADIUS_RANGE_MINIMUM;
            }
            else if ( rPropVal.Name == "RadiusRangeMaximum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aRadiusRangeMaximum )
                    rDestinationHandle.nFlags |= HandleFlags::RADIUS_RANGE_MAXIMUM;
            }
            else if ( rPropVal.Name == "RangeXMinimum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aXRangeMinimum )
                    rDestinationHandle.nFlags |= HandleFlags::RANGE_X_MINIMUM;
            }
            else if ( rPropVal.Name == "RangeXMaximum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aXRangeMaximum )
                    rDestinationHandle.nFlags |= HandleFlags::RANGE_X_MAXIMUM;
            }
            else if ( rPropVal.Name == "RangeYMinimum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aYRangeMinimum )
                    rDestinationHandle.nFlags |= HandleFlags::RANGE_Y_MINIMUM;
            }
            else if ( rPropVal.Name == "RangeYMaximum" )
            {
                if ( rPropVal.Value >>= rDestinationHandle.aYRangeMaximum )
                    rDestinationHandle.nFlags |= HandleFlags::RANGE_Y_MAXIMUM;
            }
        }
    }
    return bRetValue;
}

void EnhancedCustomShape2d::ApplyShapeAttributes( const SdrCustomShapeGeometryItem& rGeometryItem )
{
    // AdjustmentValues
    static constexpr OUStringLiteral sAdjustmentValues( u"AdjustmentValues" );
    const Any* pAny = rGeometryItem.GetPropertyValueByName( sAdjustmentValues );
    if ( pAny )
        *pAny >>= m_seqAdjustmentValues;


    // Coordsize
    static constexpr OUStringLiteral sViewBox( u"ViewBox" );
    const Any* pViewBox = rGeometryItem.GetPropertyValueByName( sViewBox );
    css::awt::Rectangle aViewBox;
    if ( pViewBox && (*pViewBox >>= aViewBox ) )
    {
        m_nCoordLeft    = aViewBox.X;
        m_nCoordTop     = aViewBox.Y;
        m_nCoordWidthG  = aViewBox.Width;
        if (m_nCoordWidthG < 0)
            m_nCoordWidthG = o3tl::saturating_toggle_sign(m_nCoordWidthG);
        m_nCoordHeightG = aViewBox.Height;
        if (m_nCoordHeightG < 0)
            m_nCoordHeightG = o3tl::saturating_toggle_sign(m_nCoordHeightG);
    }
    static constexpr OUString sPath( u"Path"_ustr );
    static constexpr OUStringLiteral sCoordinates( u"Coordinates" );
    static constexpr OUStringLiteral sGluePoints( u"GluePoints" );
    static constexpr OUStringLiteral sGluePointLeavingDirections( u"GluePointLeavingDirections" );
    static constexpr OUStringLiteral sSegments( u"Segments" );
    static constexpr OUStringLiteral sSubViewSize( u"SubViewSize" );
    static constexpr OUStringLiteral sStretchX( u"StretchX" );
    static constexpr OUStringLiteral sStretchY( u"StretchY" );
    static constexpr OUStringLiteral sTextFrames( u"TextFrames" );
    static constexpr OUStringLiteral sEquations( u"Equations" );
    static constexpr OUStringLiteral sHandles( u"Handles" );


    // Path/Coordinates
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sCoordinates );
    if ( pAny )
        *pAny >>= m_seqCoordinates;


    // Path/GluePoints
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sGluePoints );
    if ( pAny )
        *pAny >>= m_seqGluePoints;

    // Path/GluePointLeavingDirections
    pAny = rGeometryItem.GetPropertyValueByName(sPath, sGluePointLeavingDirections);
    if (pAny)
        *pAny >>= m_seqGluePointLeavingDirections;

    // Path/Segments
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sSegments );
    if ( pAny )
        *pAny >>= m_seqSegments;


    // Path/SubViewSize
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sSubViewSize );
    if ( pAny )
        *pAny >>= m_seqSubViewSize;


    // Path/StretchX
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sStretchX );
    if ( pAny )
    {
        sal_Int32 nStretchX = 0;
        if ( *pAny >>= nStretchX )
            m_nXRef = nStretchX;
    }


    // Path/StretchY
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sStretchY );
    if ( pAny )
    {
        sal_Int32 nStretchY = 0;
        if ( *pAny >>= nStretchY )
            m_nYRef = nStretchY;
    }


    // Path/TextFrames
    pAny = rGeometryItem.GetPropertyValueByName( sPath, sTextFrames );
    if ( pAny )
        *pAny >>= m_seqTextFrames;


    // Equations
    pAny = rGeometryItem.GetPropertyValueByName( sEquations );
    if ( pAny )
        *pAny >>= m_seqEquations;


    // Handles
    pAny = rGeometryItem.GetPropertyValueByName( sHandles );
    if ( pAny )
        *pAny >>= m_seqHandles;
}

EnhancedCustomShape2d::~EnhancedCustomShape2d()
{
}

void EnhancedCustomShape2d::SetPathSize( sal_Int32 nIndex )
{
    sal_Int32 nWidth = 0;
    sal_Int32 nHeight = 0;

    if ( m_seqSubViewSize.hasElements() && nIndex < m_seqSubViewSize.getLength() ) {
        nWidth = m_seqSubViewSize[ nIndex ].Width;
        nHeight = m_seqSubViewSize[ nIndex ].Height;
        SAL_INFO(
            "svx",
            "set subpath " << nIndex << " size: " << nWidth << " x "
                << nHeight);
    }

    if ( nWidth && nHeight ) {
        m_nCoordWidth = nWidth;
        m_nCoordHeight = nHeight;
    } else {
        m_nCoordWidth = m_nCoordWidthG;
        m_nCoordHeight = m_nCoordHeightG;
    }

    m_fXScale = m_nCoordWidth == 0 ? 0.0 : static_cast<double>(m_aLogicRect.GetWidth()) / static_cast<double>(m_nCoordWidth);
    m_fYScale = m_nCoordHeight == 0 ? 0.0 : static_cast<double>(m_aLogicRect.GetHeight()) / static_cast<double>(m_nCoordHeight);
    if ( m_bOOXMLShape )
    {
        SAL_INFO(
            "svx",
            "ooxml shape, path width: " << m_nCoordWidth << " height: "
                << m_nCoordHeight);

        // Try to set up scale separately, if given only width or height
        // This is possible case in OOXML when only width or height is non-zero
        if ( m_nCoordWidth == 0 )
        {
            if ( nWidth )
                m_fXScale = static_cast<double>(m_aLogicRect.GetWidth()) / static_cast<double>(nWidth);
            else
                m_fXScale = 1.0;
        }
        if ( m_nCoordHeight == 0 )
        {
            if ( nHeight )
                m_fYScale = static_cast<double>(m_aLogicRect.GetHeight()) / static_cast<double>(nHeight);
            else
                m_fYScale = 1.0;
        }
    }
    if ( static_cast<sal_uInt32>(m_nXRef) != 0x80000000 && m_aLogicRect.GetHeight() )
    {
        m_fXRatio = static_cast<double>(m_aLogicRect.GetWidth()) / static_cast<double>(m_aLogicRect.GetHeight());
        if ( m_fXRatio > 1 )
            m_fXScale /= m_fXRatio;
        else
            m_fXRatio = 1.0;
    }
    else
        m_fXRatio = 1.0;
    if ( static_cast<sal_uInt32>(m_nYRef) != 0x80000000 && m_aLogicRect.GetWidth() )
    {
        m_fYRatio = static_cast<double>(m_aLogicRect.GetHeight()) / static_cast<double>(m_aLogicRect.GetWidth());
        if ( m_fYRatio > 1 )
            m_fYScale /= m_fYRatio;
        else
            m_fYRatio = 1.0;
    }
    else
        m_fYRatio = 1.0;

    if (comphelper::IsFuzzing())
    {
        if (fabs(m_fXScale) > 100000)
        {
            SAL_WARN("svx", "unreasonable X Scale of: " << m_fXScale);
            m_fXScale = 1.0;
        }
        if (fabs(m_fYScale) > 100000)
        {
            SAL_WARN("svx", "unreasonable Y Scale of: " << m_fYScale);
            m_fYScale = 1.0;
        }
    }
}

EnhancedCustomShape2d::EnhancedCustomShape2d(SdrObjCustomShape& rSdrObjCustomShape)
:   SfxItemSet          ( rSdrObjCustomShape.GetMergedItemSet() ),
    mrSdrObjCustomShape ( rSdrObjCustomShape ),
    m_eSpType             ( mso_sptNil ),
    m_nCoordLeft          ( 0 ),
    m_nCoordTop           ( 0 ),
    m_nCoordWidthG        ( 21600 ),
    m_nCoordHeightG       ( 21600 ),
    m_bOOXMLShape         ( false ),
    m_nXRef               ( 0x80000000 ),
    m_nYRef               ( 0x80000000 ),
    m_nColorData          ( 0 ),
    m_bFilled             ( rSdrObjCustomShape.GetMergedItem( XATTR_FILLSTYLE ).GetValue() != drawing::FillStyle_NONE ),
    m_bStroked            ( rSdrObjCustomShape.GetMergedItem( XATTR_LINESTYLE ).GetValue() != drawing::LineStyle_NONE ),
    m_bFlipH              ( false ),
    m_bFlipV              ( false )
{
    // bTextFlow needs to be set before clearing the TextDirection Item

    ClearItem( SDRATTR_TEXTDIRECTION ); //SJ: vertical writing is not required, by removing this item no outliner is created

    // #i105323# For 2D AutoShapes, the shadow attribute does not need to be applied to any
    // of the constructed helper SdrObjects. This would lead to problems since the shadow
    // of one helper object would fall on one helper object behind it (e.g. with the
    // eyes of the smiley shape). This is not wanted; instead a single shadow 'behind'
    // the AutoShape visualisation is wanted. This is done with primitive functionality
    // now in SdrCustomShapePrimitive2D::create2DDecomposition, but only for 2D objects
    // (see there and in EnhancedCustomShape3d::Create3DObject to read more).
    // This exception may be removed later when AutoShapes will create primitives directly.
    // So, currently remove the ShadowAttribute from the ItemSet to not apply it to any
    // 2D helper shape.
    ClearItem(SDRATTR_SHADOW);

    Point aP( mrSdrObjCustomShape.GetSnapRect().Center() );
    Size aS( mrSdrObjCustomShape.GetLogicRect().GetSize() );
    aP.AdjustX( -(aS.Width() / 2) );
    aP.AdjustY( -(aS.Height() / 2) );
    m_aLogicRect = tools::Rectangle( aP, aS );

    OUString sShapeType;
    const SdrCustomShapeGeometryItem& rGeometryItem(mrSdrObjCustomShape.GetMergedItem( SDRATTR_CUSTOMSHAPE_GEOMETRY ));
    static constexpr OUStringLiteral sType = u"Type";
    const Any* pAny = rGeometryItem.GetPropertyValueByName( sType );
    if ( pAny ) {
        *pAny >>= sShapeType;
        m_bOOXMLShape = sShapeType.startsWith("ooxml-");
        SAL_INFO("svx", "shape type: " << sShapeType << " " << m_bOOXMLShape);
    }
    m_eSpType = EnhancedCustomShapeTypeNames::Get( sShapeType );

    static constexpr OUStringLiteral sMirroredX = u"MirroredX";
    static constexpr OUStringLiteral sMirroredY = u"MirroredY";
    pAny = rGeometryItem.GetPropertyValueByName( sMirroredX );
    if ( pAny )
        *pAny >>= m_bFlipH;
    pAny = rGeometryItem.GetPropertyValueByName( sMirroredY );
    if ( pAny )
        *pAny >>= m_bFlipV;

    m_nRotateAngle = Degree100(static_cast<sal_Int32>(mrSdrObjCustomShape.GetObjectRotation() * 100.0));

    /*const sal_Int32* pDefData =*/ ApplyShapeAttributes( rGeometryItem );
    SetPathSize();

    switch( m_eSpType )
    {
        case mso_sptCan :                       m_nColorData = 0x20400000; break;
        case mso_sptCube :                      m_nColorData = 0x302e0000; break;
        case mso_sptActionButtonBlank :         m_nColorData = 0x502ce400; break;
        case mso_sptActionButtonHome :          m_nColorData = 0x702ce4ce; break;
        case mso_sptActionButtonHelp :          m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonInformation :   m_nColorData = 0x702ce4c5; break;
        case mso_sptActionButtonBackPrevious :  m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonForwardNext :   m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonBeginning :     m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonEnd :           m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonReturn :        m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonDocument :      m_nColorData = 0x702ce4ec; break;
        case mso_sptActionButtonSound :         m_nColorData = 0x602ce4c0; break;
        case mso_sptActionButtonMovie :         m_nColorData = 0x602ce4c0; break;
        case mso_sptBevel :                     m_nColorData = 0x502ce400; break;
        case mso_sptFoldedCorner :              m_nColorData = 0x20e00000; break;
        case mso_sptSmileyFace :                m_nColorData = 0x20e00000; break;
        case mso_sptNil :
        {
            // Because calculation method has changed in #i102797 original color encoding for
            // Octagon Bevel and Diamond Bevel can no longer be used. We keep the color coding
            // only for self-created shapes, as authors may have already considered the change.
            // We use ColorData compatible to OOXML.
            if (sShapeType == "col-60da8460") // Octagon Bevel
            {
                m_nColorData = 0x60ecc240;
            }
            else if (sShapeType == "col-502ad400") // Diamond Bevel
            {
                m_nColorData = 0x502ce400;
            }
            else if (sShapeType.getLength() > 4 && sShapeType.match( "col-" ))
            {
                m_nColorData = o3tl::toUInt32(sShapeType.subView( 4 ), 16);
            }
        }
        break;
        case mso_sptCurvedLeftArrow :
        case mso_sptCurvedRightArrow :
        case mso_sptCurvedUpArrow :
        case mso_sptCurvedDownArrow :           m_nColorData = 0x20d00000; break;
        case mso_sptRibbon2 :                   m_nColorData = 0x30ee0000; break;
        case mso_sptRibbon :                    m_nColorData = 0x30ee0000; break;

        case mso_sptEllipseRibbon2 :            m_nColorData = 0x30ee0000; break;
        case mso_sptEllipseRibbon :             m_nColorData = 0x30ee0000; break;

        case mso_sptVerticalScroll :            m_nColorData = 0x30ee0000; break;
        case mso_sptHorizontalScroll :          m_nColorData = 0x30ee0000; break;
        default:
            break;
    }

    sal_Int32 nLength = m_seqEquations.getLength();

    if ( !nLength )
        return;

    m_vEquationResults.resize( nLength );
}

using EnhancedCustomShape::ExpressionFunct;

double EnhancedCustomShape2d::GetEnumFunc( const ExpressionFunct eFunc ) const
{
    double fRet = 0.0;
    switch( eFunc )
    {
        case ExpressionFunct::EnumPi :         fRet = M_PI; break;
        case ExpressionFunct::EnumLeft :       fRet = static_cast<double>(m_nCoordLeft); break;
        case ExpressionFunct::EnumTop :        fRet = static_cast<double>(m_nCoordTop); break;
        case ExpressionFunct::EnumRight :      fRet = (static_cast<double>(m_nCoordLeft) + static_cast<double>(m_nCoordWidth)) * m_fXRatio; break;
        case ExpressionFunct::EnumBottom :     fRet = (static_cast<double>(m_nCoordTop) + static_cast<double>(m_nCoordHeight)) * m_fYRatio; break;
        case ExpressionFunct::EnumXStretch :   fRet = m_nXRef; break;
        case ExpressionFunct::EnumYStretch :   fRet = m_nYRef; break;
        case ExpressionFunct::EnumHasStroke :  fRet = m_bStroked ? 1.0 : 0.0; break;
        case ExpressionFunct::EnumHasFill :    fRet = m_bFilled ? 1.0 : 0.0; break;
        case ExpressionFunct::EnumWidth :      fRet = m_nCoordWidth; break;
        case ExpressionFunct::EnumHeight :     fRet = m_nCoordHeight; break;
        case ExpressionFunct::EnumLogWidth :   fRet = m_aLogicRect.GetWidth(); break;
        case ExpressionFunct::EnumLogHeight :  fRet = m_aLogicRect.GetHeight(); break;
        default: break;
    }
    return fRet;
}
double EnhancedCustomShape2d::GetAdjustValueAsDouble( const sal_Int32 nIndex ) const
{
    double fNumber = 0.0;
    if ( nIndex < m_seqAdjustmentValues.getLength() )
    {
        if ( m_seqAdjustmentValues[ nIndex ].Value.getValueTypeClass() == TypeClass_DOUBLE )
            m_seqAdjustmentValues[ nIndex ].Value >>= fNumber;
        else
        {
            sal_Int32 nNumber = 0;
            m_seqAdjustmentValues[ nIndex ].Value >>= nNumber;
            fNumber = static_cast<double>(nNumber);
        }
    }
    return fNumber;
}
double EnhancedCustomShape2d::GetEquationValueAsDouble( const sal_Int32 nIndex ) const
{
    double fNumber = 0.0;
    static sal_uInt32 nLevel = 0;
    if ( nIndex >= static_cast<sal_Int32>(m_vEquationResults.size()) )
        return fNumber;

    if (!m_vEquationResults[nIndex].bParsed)
    {
        m_vEquationResults[nIndex].bParsed = true;
        try
        {
            m_vEquationResults[nIndex].xNode = EnhancedCustomShape::FunctionParser::parseFunction( m_seqEquations[ nIndex ], *this );
        }
        catch ( EnhancedCustomShape::ParseError& )
        {
            SAL_INFO(
                "svx",
                "error: equation number: " << nIndex << ", parser failed ("
                    << m_seqEquations[nIndex] << ")");
        }
    }
    if ( m_vEquationResults[ nIndex ].xNode )
    {
        nLevel ++;
        try
        {
            if ( m_vEquationResults[ nIndex ].bReady )
                fNumber = m_vEquationResults[ nIndex ].fValue;
            else {
                // cast to non const, so that we can optimize by caching
                // equation results, without changing all the const in the stack
                struct EquationResult &aResult = const_cast<EnhancedCustomShape2d*>(this)->m_vEquationResults[ nIndex ];

                fNumber = aResult.fValue = (*m_vEquationResults[ nIndex ].xNode)();
                aResult.bReady = true;

                SAL_INFO("svx", "equation " << nLevel << " (level: " << m_seqEquations[nIndex] << "): "
                         << fNumber << " --> " << 180.0*fNumber/10800000.0);
            }
            if ( !std::isfinite( fNumber ) )
                fNumber = 0.0;
        }
        catch ( ... )
        {
            SAL_WARN("svx", "EnhancedCustomShape2d::GetEquationValueAsDouble failed");
        }
        nLevel --;
    }
    SAL_INFO(
        "svx",
        "?" << nIndex << " --> " << fNumber << " (angle: "
            << 180.0*fNumber/10800000.0 << ")");

    return fNumber;
}

bool EnhancedCustomShape2d::SetAdjustValueAsDouble( const double& rValue, const sal_Int32 nIndex )
{
    bool bRetValue = false;
    if ( nIndex < m_seqAdjustmentValues.getLength() )
    {
        // updating our local adjustment sequence
        auto pseqAdjustmentValues = m_seqAdjustmentValues.getArray();
        pseqAdjustmentValues[ nIndex ].Value <<= rValue;
        pseqAdjustmentValues[ nIndex ].State = css::beans::PropertyState_DIRECT_VALUE;
        bRetValue = true;
    }
    return bRetValue;
}

basegfx::B2DPoint EnhancedCustomShape2d::GetPointAsB2DPoint( const css::drawing::EnhancedCustomShapeParameterPair& rPair,
                                        const bool bScale, const bool bReplaceGeoSize ) const
{
    double fValX, fValY;
    // width
    GetParameter(fValX, rPair.First, bReplaceGeoSize, false);
    fValX -= m_nCoordLeft;
    if (bScale)
    {
        fValX *= m_fXScale;
    }
    // height
    GetParameter(fValY, rPair.Second, false, bReplaceGeoSize);
    fValY -= m_nCoordTop;
    if (bScale)
    {
        fValY *= m_fYScale;
    }
    return basegfx::B2DPoint(fValX,fValY);
}

Point EnhancedCustomShape2d::GetPoint( const css::drawing::EnhancedCustomShapeParameterPair& rPair,
                                        const bool bScale, const bool bReplaceGeoSize ) const
{
    basegfx::B2DPoint aPoint(GetPointAsB2DPoint(rPair, bScale, bReplaceGeoSize));
    return Point(static_cast<tools::Long>(aPoint.getX()), static_cast<tools::Long>(aPoint.getY()));
}

void EnhancedCustomShape2d::GetParameter( double& rRetValue, const EnhancedCustomShapeParameter& rParameter,
                                              const bool bReplaceGeoWidth, const bool bReplaceGeoHeight ) const
{
    rRetValue = 0.0;
    switch ( rParameter.Type )
    {
        case EnhancedCustomShapeParameterType::ADJUSTMENT :
        {
            sal_Int32 nAdjustmentIndex = 0;
            if ( rParameter.Value >>= nAdjustmentIndex )
            {
                rRetValue = GetAdjustValueAsDouble( nAdjustmentIndex );
            }
        }
        break;
        case EnhancedCustomShapeParameterType::EQUATION :
        {
            sal_Int32 nEquationIndex = 0;
            if ( rParameter.Value >>= nEquationIndex )
            {
                rRetValue = GetEquationValueAsDouble( nEquationIndex );
            }
        }
        break;
        case EnhancedCustomShapeParameterType::NORMAL :
        {
            if ( rParameter.Value.getValueTypeClass() == TypeClass_DOUBLE )
            {
                double fValue(0.0);
                if ( rParameter.Value >>= fValue )
                {
                    rRetValue = fValue;
                }
            }
            else
            {
                sal_Int32 nValue = 0;
                if ( rParameter.Value >>= nValue )
                {
                    rRetValue = nValue;
                    if ( bReplaceGeoWidth && ( nValue == m_nCoordWidth ) )
                        rRetValue *= m_fXRatio;
                    else if ( bReplaceGeoHeight && ( nValue == m_nCoordHeight ) )
                        rRetValue *= m_fYRatio;
                }
            }
        }
        break;
        case EnhancedCustomShapeParameterType::LEFT :
        case EnhancedCustomShapeParameterType::TOP :
        {
            rRetValue  = 0.0;
        }
        break;
        case EnhancedCustomShapeParameterType::RIGHT :
        {
            rRetValue = m_nCoordWidth;
        }
        break;
        case EnhancedCustomShapeParameterType::BOTTOM :
        {
            rRetValue = m_nCoordHeight;
        }
        break;
    }
}

// nLumDat 28-31 = number of luminance entries in nLumDat
// nLumDat 27-24 = nLumDatEntry 0
// nLumDat 23-20 = nLumDatEntry 1 ...
// each 4bit entry is to be interpreted as a 10 percent signed luminance changing
sal_Int32 EnhancedCustomShape2d::GetLuminanceChange( sal_uInt32 nIndex ) const
{
    const sal_uInt32 nCount = m_nColorData >> 28;
    if ( !nCount )
        return 0;

    if ( nIndex >= nCount )
        nIndex = nCount - 1;

    const sal_Int32 nLumDat = m_nColorData << ( ( 1 + nIndex ) << 2 );
    return ( nLumDat >> 28 ) * 10;
}

Color EnhancedCustomShape2d::GetColorData( const Color& rFillColor, sal_uInt32 nIndex, double dBrightness ) const
{
    if ( m_bOOXMLShape || ( mso_sptMin == m_eSpType /* ODF "non-primitive" */ ) )
    { //do LibreOffice way, using dBrightness
        if ( dBrightness == 0.0)
        {
            return rFillColor;
        }
        else
        {
            if (dBrightness >=0.0)
            { //lighten, blending with white
                return Color( static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetRed() * (1.0-dBrightness) + dBrightness * 255.0, 0.0, 255.0)  )),
                              static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetGreen() * (1.0-dBrightness) + dBrightness * 255.0, 0.0, 255.0) )),
                              static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetBlue() * (1.0-dBrightness) + dBrightness * 255.0, 0.0, 255.0) ))  );
            }
            else
            { //darken (indicated by negative sign), blending with black
                return Color( static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetRed() * (1.0+dBrightness), 0.0, 255.0)  )),
                              static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetGreen() * (1.0+dBrightness), 0.0, 255.0) )),
                              static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(rFillColor.GetBlue() * (1.0+dBrightness), 0.0, 255.0) ))  );
            }
        }
    }
    else
    { //do OpenOffice way, using nColorData
        const sal_Int32 nLuminance = GetLuminanceChange(nIndex);
        if( !nLuminance )
            return rFillColor;

        basegfx::BColor aHSVColor=
                basegfx::utils::rgb2hsv(
                    basegfx::BColor(rFillColor.GetRed()/255.0,
                                    rFillColor.GetGreen()/255.0,
                                    rFillColor.GetBlue()/255.0));

        if( nLuminance > 0 )
        {
            aHSVColor.setGreen(
                aHSVColor.getGreen() * (1.0-nLuminance/100.0));
            aHSVColor.setBlue(
                nLuminance/100.0 +
                (1.0-nLuminance/100.0)*aHSVColor.getBlue());
        }
        else if( nLuminance < 0 )
        {
            aHSVColor.setBlue(
                (1.0+nLuminance/100.0)*aHSVColor.getBlue());
        }

        aHSVColor = basegfx::utils::hsv2rgb(aHSVColor);
        return Color( static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(aHSVColor.getRed(),0.0,1.0) * 255.0 + 0.5 )),
                    static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(aHSVColor.getGreen(),0.0,1.0) * 255.0 + 0.5 )),
                    static_cast<sal_uInt8>(static_cast< sal_Int32 >( std::clamp(aHSVColor.getBlue(),0.0,1.0) * 255.0 + 0.5 )) );
    }
}

tools::Rectangle EnhancedCustomShape2d::GetTextRect() const
{
    if ( !m_seqTextFrames.hasElements() )
        return m_aLogicRect;
    sal_Int32 nIndex = 0;
    Point aTopLeft( GetPoint( m_seqTextFrames[ nIndex ].TopLeft, !m_bOOXMLShape, true ) );
    Point aBottomRight( GetPoint( m_seqTextFrames[ nIndex ].BottomRight, !m_bOOXMLShape, true ) );
    tools::Rectangle aRect( aTopLeft, aBottomRight );
    if ( m_bFlipH )
    {
        aRect.SetLeft(m_aLogicRect.GetWidth() - 1 - aBottomRight.X());
        aRect.SetRight( m_aLogicRect.GetWidth() - 1 - aTopLeft.X());
    }
    if ( m_bFlipV )
    {
        aRect.SetTop(m_aLogicRect.GetHeight() - 1 - aBottomRight.Y());
        aRect.SetBottom(m_aLogicRect.GetHeight() - 1 - aTopLeft.Y());
    }
    SAL_INFO("svx", aRect.GetWidth() << " x " << aRect.GetHeight());
    if( aRect.GetWidth() <= 1 || aRect.GetHeight() <= 1 )
        return m_aLogicRect;
    aRect.Move( m_aLogicRect.Left(), m_aLogicRect.Top() );
    aRect.Normalize();
    return aRect;
}

sal_uInt32 EnhancedCustomShape2d::GetHdlCount() const
{
    return m_seqHandles.getLength();
}

bool EnhancedCustomShape2d::GetHandlePosition( const sal_uInt32 nIndex, Point& rReturnPosition ) const
{
    bool bRetValue = false;
    if ( nIndex < GetHdlCount() )
    {
        Handle aHandle;
        if ( ConvertSequenceToEnhancedCustomShape2dHandle( m_seqHandles[ nIndex ], aHandle ) )
        {
            if ( aHandle.nFlags & HandleFlags::POLAR )
            {
                Point aReferencePoint( GetPoint( aHandle.aPolar ) );

                double      fAngle;
                double      fRadius;
                GetParameter( fRadius, aHandle.aPosition.First, false, false );
                GetParameter( fAngle,  aHandle.aPosition.Second, false, false );

                double a = basegfx::deg2rad(360.0 - fAngle);
                double dx = fRadius * m_fXScale;
                double fX = dx * cos( a );
                double fY =-dx * sin( a );
                rReturnPosition =
                    Point(
                        basegfx::fround<tools::Long>( fX + aReferencePoint.X() ),
                        basegfx::fTools::equalZero(m_fXScale) ? aReferencePoint.Y() :
                        basegfx::fround<tools::Long>( ( fY * m_fYScale ) / m_fXScale + aReferencePoint.Y() ) );
            }
            else
            {
                if ( aHandle.nFlags & HandleFlags::SWITCHED )
                {
                    if ( m_aLogicRect.GetHeight() > m_aLogicRect.GetWidth() )
                    {
                        std::swap(aHandle.aPosition.First, aHandle.aPosition.Second);
                    }
                }
                if (m_bOOXMLShape)
                    rReturnPosition = GetPoint(aHandle.aPosition, false /*bScale*/);
                else
                    rReturnPosition = GetPoint(aHandle.aPosition, true /*bScale*/);
            }
            const GeoStat aGeoStat(mrSdrObjCustomShape.GetGeoStat());
            if ( aGeoStat.m_nShearAngle )
            {
                double nTan = aGeoStat.mfTanShearAngle;
                if (m_bFlipV != m_bFlipH)
                    nTan = -nTan;
                ShearPoint( rReturnPosition, Point( m_aLogicRect.GetWidth() / 2, m_aLogicRect.GetHeight() / 2 ), nTan );
            }
            if ( m_nRotateAngle )
            {
                double a = toRadians(m_nRotateAngle);
                RotatePoint( rReturnPosition, Point( m_aLogicRect.GetWidth() / 2, m_aLogicRect.GetHeight() / 2 ), sin( a ), cos( a ) );
            }
            if ( m_bFlipH )
                rReturnPosition.setX( m_aLogicRect.GetWidth() - rReturnPosition.X() );
            if ( m_bFlipV )
                rReturnPosition.setY( m_aLogicRect.GetHeight() - rReturnPosition.Y() );
            rReturnPosition.Move( m_aLogicRect.Left(), m_aLogicRect.Top() );
            bRetValue = true;
        }
    }
    return bRetValue;
}

static double lcl_getXAdjustmentValue(std::u16string_view rShapeType, const sal_uInt32 nHandleIndex,
                                      const double fX, const double fW, const double fH)
{
    // degenerated shapes are not worth to calculate special case for each shape type
    if (fW <= 0.0 || fH <= 0.0)
        return 50000;

    // pattern (w - x) / ss * 100000 or (r - x) / ss * 100000
    if ((rShapeType == u"ooxml-bentArrow" && nHandleIndex == 2) || (rShapeType == u"ooxml-chevron")
        || (rShapeType == u"ooxml-curvedRightArrow") || (rShapeType == u"ooxml-foldedCorner")
        || (rShapeType == u"ooxml-homePlate") || (rShapeType == u"ooxml-notchedRightArrow")
        || (rShapeType == u"ooxml-nonIsoscelesTrapezoid" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-rightArrow")
        || (rShapeType == u"ooxml-rightArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-round1Rect")
        || (rShapeType == u"ooxml-round2DiagRect" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-round2SameRect" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-snip1Rect")
        || (rShapeType == u"ooxml-snip2DiagRect" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-snip2SameRect" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-snipRoundRect" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-swooshArrow") || (rShapeType == u"ooxml-stripedRightArrow"))
        return (fW - fX) / std::min(fW, fH) * 100000.0;

    // pattern  x / ss * 100000 or (x - l) / ss * 100000
    if ((rShapeType == u"ooxml-bentArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-bentArrow" && nHandleIndex == 3)
        || (rShapeType == u"ooxml-corner")
        || (rShapeType == u"ooxml-curvedDownArrow") || (rShapeType == u"ooxml-curvedLeftArrow")
        || (rShapeType == u"ooxml-curvedUpArrow") || (rShapeType == u"ooxml-leftArrow")
        || (rShapeType == u"ooxml-leftArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-leftRightArrow")
        || (rShapeType == u"ooxml-leftRightArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-leftRightRibbon")
        || (rShapeType == u"ooxml-nonIsoscelesTrapezoid" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-parallelogram")
        || (rShapeType == u"ooxml-round2DiagRect" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-round2SameRect" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-roundRect")
        || (rShapeType == u"ooxml-snip2DiagRect" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-snip2SameRect" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-snipRoundRect" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-uturnArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-uturnArrow" && nHandleIndex == 3))
        return fX / std::min(fW, fH) * 100000.0;

    // pattern (hc - x) / ss * 200000
    if ((rShapeType == u"ooxml-downArrowCallout" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-leftRightUpArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-quadArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-quadArrowCallout" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-upArrowCallout" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-upDownArrowCallout" && nHandleIndex == 0))
        return (fW / 2.0 - fX) / std::min(fW, fH) * 200000.0;

    // pattern (hc - x) / ss * 100000
    if ((rShapeType == u"ooxml-downArrowCallout" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-leftRightUpArrow" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-quadArrow" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-quadArrowCallout" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-upArrowCallout" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-upDownArrowCallout" && nHandleIndex == 1))
        return (fW / 2.0 - fX) / std::min(fW, fH) * 100000.0;

    // pattern (w - x) / ss * 50000 or (r - x) / ss * 50000
    if ((rShapeType == u"ooxml-bentUpArrow") || (rShapeType == u"ooxml-leftUpArrow")
        || (rShapeType == u"ooxml-uturnArrow" && nHandleIndex == 1))
        return (fW - fX) / std::min(fW, fH) * 50000.0;

    // pattern x / ss * 200000
    if (rShapeType == u"ooxml-nonIsoscelesTrapezoid" && nHandleIndex == 0)
        return fX / std::min(fW, fH) * 200000.0;

    // pattern (hc - x) / w * 200000
    if ((rShapeType == u"ooxml-downArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-ellipseRibbon") || (rShapeType == u"ooxml-ellipseRibbon2")
        || (rShapeType == u"ooxml-leftRightArrowCallout" && nHandleIndex == 3)
        || (rShapeType == u"ooxml-ribbon") || (rShapeType == u"ooxml-ribbon2")
        || (rShapeType == u"ooxml-upArrow" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-upDownArrow" && nHandleIndex == 0))
        return (fW / 2.0 - fX) / fW * 200000.0;

    // pattern (x - hc) / w * 100000
    if ((rShapeType == u"ooxml-cloudCallout") || (rShapeType == u"ooxml-doubleWave")
        || (rShapeType == u"ooxml-wave") || (rShapeType == u"ooxml-wedgeEllipseCallout")
        || (rShapeType == u"ooxml-wedgeRectCallout")
        || (rShapeType == u"ooxml-wedgeRoundRectCallout"))
        return (fX - fW / 2.0) / fW * 100000.0;

    // pattern (x - hc) / w * 200000
    if (rShapeType == u"ooxml-teardrop")
        return (fX - fW / 2.0) / fW * 200000.0;

    // pattern (w - x) / w * 100000 or (r - x) / w * 100000
    if (rShapeType == u"ooxml-leftArrowCallout" && nHandleIndex == 3)
        return (fW - fX) / fW * 100000.0;

    // pattern (hc - x) / h * 100000
    if (rShapeType == u"ooxml-mathDivide")
        return (fW / 2.0 - fX) / fH * 100000.0;

    // pattern x / w * 100000, simple scaling
    if (o3tl::starts_with(rShapeType, u"ooxml-"))
        return fX / fW * 100000.0;

    return fX; // method is unknown
}

static double lcl_getYAdjustmentValue(std::u16string_view rShapeType, const sal_uInt32 nHandleIndex,
                                      const double fY, const double fW, const double fH)
{
    // degenerated shapes are not worth to calculate a special case for each shape type
    if (fW <= 0.0 || fH <= 0.0)
        return 50000;

    // pattern (vc - y) / ss * 100000
    if ((rShapeType == u"ooxml-leftArrowCallout" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-leftRightArrowCallout" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-rightArrowCallout" && nHandleIndex == 1))
        return (fH / 2.0 - fY) / std::min(fW, fH) * 100000.0;

    // pattern (vc - y) / ss * 200000
    if ((rShapeType == u"ooxml-curvedLeftArrow") || (rShapeType == u"ooxml-curvedRightArrow")
        || (rShapeType == u"ooxml-leftArrowCallout" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-leftRightArrowCallout" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-mathPlus")
        || (rShapeType == u"ooxml-rightArrowCallout" && nHandleIndex == 0))
        return (fH / 2.0 - fY) / std::min(fW, fH) * 200000.0;

    // pattern (h - y) / ss * 100000 or (b - y) / ss * 100000
    if ((rShapeType == u"ooxml-bentUpArrow" && nHandleIndex == 0) || (rShapeType == u"ooxml-corner")
        || (rShapeType == u"ooxml-curvedDownArrow") || (rShapeType == u"ooxml-downArrow")
        || (rShapeType == u"ooxml-downArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-uturnArrow" && nHandleIndex == 2))
        return (fH - fY) / std::min(fW, fH) * 100000.0;

    // pattern (h - y) / ss * 200000 or (b - y) / ss * 200000
    if (rShapeType == u"ooxml-leftUpArrow" && nHandleIndex == 0) // - adj2 * 2 outside
        return (fH - fY) / std::min(fW, fH) * 200000.0;

    // pattern  y / ss * 100000 or (y - t) / ss * 100000
    if ((rShapeType == u"ooxml-bentUpArrow" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-bracePair") || (rShapeType == u"ooxml-bracketPair")
        || (rShapeType == u"ooxml-can") || (rShapeType == u"ooxml-cube")
        || (rShapeType == u"ooxml-curvedUpArrow") || (rShapeType == u"ooxml-halfFrame")
        || (rShapeType == u"ooxml-leftBrace" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-leftBracket") || (rShapeType == u"ooxml-leftRightUpArrow")
        || (rShapeType == u"ooxml-leftUpArrow" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-mathMultiply") || (rShapeType == u"ooxml-quadArrow")
        || (rShapeType == u"ooxml-quadArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-rightBrace" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-rightBracket") || (rShapeType == u"ooxml-upArrow")
        || (rShapeType == u"ooxml-upArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-upDownArrow")
        || (rShapeType == u"ooxml-upDownArrowCallout" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-verticalScroll"))
        return fY / std::min(fW, fH) * 100000.0;

    // pattern y / ss * 50000
    if (rShapeType == u"ooxml-bentArrow")
        return fY / std::min(fW, fH) * 50000.0;

    // pattern (vc - y) / h * 100000
    if ((rShapeType == u"ooxml-mathDivide" && nHandleIndex == 1) // -adj1 / 2 - adj3 outside
        || (rShapeType == u"ooxml-mathEqual" && nHandleIndex == 0) // -adj2 / 2 outside
        || (rShapeType == u"ooxml-mathNotEqual" && nHandleIndex == 0) // -adj3 / 2 outside
        || (rShapeType == u"ooxml-star4") || (rShapeType == u"ooxml-star6")
        || (rShapeType == u"ooxml-star8") || (rShapeType == u"ooxml-star10")
        || (rShapeType == u"ooxml-star12") || (rShapeType == u"ooxml-star16")
        || (rShapeType == u"ooxml-star24") || (rShapeType == u"ooxml-star32"))
        return (fH / 2.0 - fY) / fH * 100000.0;

    // pattern (vc - y) / h * 200000
    if ((rShapeType == u"ooxml-leftArrow") || (rShapeType == u"ooxml-leftRightArrow")
        || (rShapeType == u"ooxml-mathDivide" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-mathEqual" && nHandleIndex == 1)
        || (rShapeType == u"ooxml-mathMinus") || (rShapeType == u"ooxml-notchedRightArrow")
        || (rShapeType == u"ooxml-mathNotEqual" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-quadArrowCallout" && nHandleIndex == 3)
        || (rShapeType == u"ooxml-rightArrow") || (rShapeType == u"ooxml-stripedRightArrow")
        || (rShapeType == u"ooxml-upDownArrowCallout" && nHandleIndex == 3))
        return (fH / 2.0 - fY) / fH * 200000.0;

    // pattern (y - vc) / h * 100000
    if ((rShapeType == u"ooxml-cloudCallout") || (rShapeType == u"ooxml-wedgeEllipseCallout")
        || (rShapeType == u"ooxml-wedgeRectCallout")
        || (rShapeType == u"ooxml-wedgeRoundRectCallout"))
        return (fY - fH / 2.0) / fH * 100000.0;

    // pattern (h - y) / h * 100000 or (b - y) / h * 100000
    if ((rShapeType == u"ooxml-ellipseRibbon" && nHandleIndex == 2)
        || (rShapeType == u"ooxml-ellipseRibbon2" && nHandleIndex == 0)
        || (rShapeType == u"ooxml-ribbon2")
        || (rShapeType == u"ooxml-upArrowCallout" && nHandleIndex == 3))
        return (fH - fY) / fH * 100000.0;

    // special pattern smiley
    if (rShapeType == u"ooxml-smileyFace")
        return (fY - fH * 16515.0 / 21600.0) / fH * 100000.0;

    // special pattern for star with odd number of tips, because center of star not center of shape
    if (rShapeType == u"ooxml-star5")
        return (fH / 2.0 - fY * 100000.0 / 110557.0) / fH * 100000.0;
    if (rShapeType == u"ooxml-star7")
        return (fH / 2.0 - fY * 100000.0 / 105210.0) / fH * 100000.0;

    // special pattern swooshArrow
    if (rShapeType == u"ooxml-swooshArrow")
        return (fY - std::min(fW, fH) / 8.0) / fH * 100000.0;

    // special pattern leftRightRibbon
    if (rShapeType == u"ooxml-leftRightRibbon")
        return fY / fH * 200000 - 100000;

    // pattern y / h * 100000, simple scaling
    if (o3tl::starts_with(rShapeType, u"ooxml-"))
        return fY / fH * 100000.0;

    return fY; // method is unknown
}

static double lcl_getAngleInOOXMLUnit(double fDY, double fDX)
{
    if (fDX != 0.0 || fDY != 0.0)
    {
        double fAngleRad(atan2(fDY, fDX));
        double fAngle = basegfx::rad2deg(fAngleRad);
        // atan2 returns angle in ]-pi; pi], OOXML preset shapes use [0;360[.
        if (fAngle < 0.0)
            fAngle += 360.0;
        // OOXML uses angle unit 1/60000 degree.
        fAngle *= 60000.0;
        return fAngle;
    }
    return 0.0; // no angle defined for origin in polar coordinate system
}

static double lcl_getRadiusDistance(double fWR, double fHR, double fX, double fY)
{
    // Get D so, that point (fX|fY) is on the ellipse, that has width fWR-D and
    // height fHR-D and center in origin.
    // Get solution of ellipse equation (fX/(fWR-D))^2 + (fY/(fHR-D)^2 = 1 by solving
    // fX^2*(fHR-D)^2 + fY^2*(fWR-D)^2 - (fWR-D)^2 * (fHR-D)^2 = 0 with Newton-method.
    if (fX == 0.0)
        return std::min(fHR - fY, fWR);
    else if (fY == 0.0)
        return std::min(fWR - fX, fHR);

    double fD = std::min(fWR, fHR) - std::hypot(fX, fY); // iteration start value
    sal_uInt8 nIter(0);
    bool bFound(false);
    do
    {
        ++nIter;
        const double fOldD(fD);
        const double fWRmD(fWR - fD);
        const double fHRmD(fHR - fD);
        double fNumerator
            = fX * fX * fHRmD * fHRmD + fY * fY * fWRmD * fWRmD - fWRmD * fWRmD * fHRmD * fHRmD;
        double fDenominator
            = 2.0 * (fHRmD * (fWRmD * fWRmD - fX * fX) + fWRmD * (fHRmD * fHRmD - fY * fY));
        if (fDenominator != 0.0)
        {
            fD = fD - fNumerator / fDenominator;
            bFound = fabs(fOldD - fD) < 1.0E-12;
        }
        else
            fD = fD * 0.9; // new start value
    } while (nIter < 50 && !bFound);
    return fD;
}

bool EnhancedCustomShape2d::SetHandleControllerPosition( const sal_uInt32 nIndex, const css::awt::Point& rPosition )
{
    // The method name is misleading. Essentially it calculates the adjustment values from a given
    // handle position.

    // For ooxml-foo shapes, the way to calculate the adjustment value from the handle position depends on
    // the type of the shape, therefore need 'Type'.
    OUString sShapeType(u"non-primitive"_ustr); // default for ODF
    const SdrCustomShapeGeometryItem& rGeometryItem(mrSdrObjCustomShape.GetMergedItem( SDRATTR_CUSTOMSHAPE_GEOMETRY ));
    const Any* pAny = rGeometryItem.GetPropertyValueByName(u"Type"_ustr);
    if (pAny)
        *pAny >>= sShapeType;

    bool bRetValue = false;
    if ( nIndex < GetHdlCount() )
    {
        Handle aHandle;
        if ( ConvertSequenceToEnhancedCustomShape2dHandle( m_seqHandles[ nIndex ], aHandle ) )
        {
            Point aP( rPosition.X, rPosition.Y );
            // apply the negative object rotation to the controller position

            aP.Move( -m_aLogicRect.Left(), -m_aLogicRect.Top() );
            if ( m_bFlipH )
                aP.setX( m_aLogicRect.GetWidth() - aP.X() );
            if ( m_bFlipV )
                aP.setY( m_aLogicRect.GetHeight() - aP.Y() );
            if ( m_nRotateAngle )
            {
                double a = -toRadians(m_nRotateAngle);
                RotatePoint( aP, Point( m_aLogicRect.GetWidth() / 2, m_aLogicRect.GetHeight() / 2 ), sin( a ), cos( a ) );
            }
            const GeoStat aGeoStat(mrSdrObjCustomShape.GetGeoStat());
            if ( aGeoStat.m_nShearAngle )
            {
                double nTan = -aGeoStat.mfTanShearAngle;
                if (m_bFlipV != m_bFlipH)
                    nTan = -nTan;
                ShearPoint( aP, Point( m_aLogicRect.GetWidth() / 2, m_aLogicRect.GetHeight() / 2 ), nTan );
            }

            double fPos1 = aP.X();  //( bFlipH ) ? aLogicRect.GetWidth() - aP.X() : aP.X();
            double fPos2 = aP.Y();  //( bFlipV ) ? aLogicRect.GetHeight() -aP.Y() : aP.Y();
            fPos1 = !basegfx::fTools::equalZero(m_fXScale) ? (fPos1 / m_fXScale) : SAL_MAX_INT32;
            fPos2 = !basegfx::fTools::equalZero(m_fYScale) ? (fPos2 / m_fYScale) : SAL_MAX_INT32;
            // revert -nCoordLeft and -nCoordTop aus GetPoint()
            fPos1 += m_nCoordLeft;
            fPos2 += m_nCoordTop;

            // Used for scaling the adjustment values based on handle positions
            double fWidth;
            double fHeight;

            if ( m_nCoordWidth || m_nCoordHeight )
            {
                fWidth = m_nCoordWidth;
                fHeight = m_nCoordHeight;
            }
            else
            {
                fWidth = m_aLogicRect.GetWidth();
                fHeight = m_aLogicRect.GetHeight();
            }

            if ( aHandle.nFlags & HandleFlags::SWITCHED )
            {
                if ( m_aLogicRect.GetHeight() > m_aLogicRect.GetWidth() )
                {
                    double fX = fPos1;
                    double fY = fPos2;
                    double fTmp = fWidth;
                    fPos1 = fY;
                    fPos2 = fX;
                    fHeight = fWidth;
                    fWidth = fTmp;
                }
            }

            sal_Int32 nFirstAdjustmentValue = -1, nSecondAdjustmentValue = -1;

            // ODF shapes are expected to use a direct binding between position and adjustment
            // values. OOXML preset shapes use known formulas. These are calculated backward to
            // get the adjustment values. So far we do not have a general method to calculate
            // the adjustment values for any shape from the handle position.
            if ( aHandle.aPosition.First.Type == EnhancedCustomShapeParameterType::ADJUSTMENT )
                aHandle.aPosition.First.Value >>= nFirstAdjustmentValue;
            if ( aHandle.aPosition.Second.Type == EnhancedCustomShapeParameterType::ADJUSTMENT )
                aHandle.aPosition.Second.Value>>= nSecondAdjustmentValue;

            if ( aHandle.nFlags & ( HandleFlags::POLAR | HandleFlags::REFR | HandleFlags::REFANGLE))
            { // Polar-Handle

                if (aHandle.nFlags & HandleFlags::REFR)
                    nFirstAdjustmentValue = aHandle.nRefR;
                if (aHandle.nFlags & HandleFlags::REFANGLE)
                    nSecondAdjustmentValue = aHandle.nRefAngle;

                double fAngle(0.0);
                double fRadius(0.0);
                // 'then' treats only shapes of type "ooxml-foo", fontwork shapes have been mapped
                // to MS binary import and will be treated in 'else'.
                if (m_bOOXMLShape)
                {
                    // DrawingML polar handles set REFR or REFANGLE instead of POLAR
                    // use the shape center instead.
                    double fDX = fPos1 - fWidth / 2.0;
                    double fDY = fPos2 - fHeight / 2.0;

                    // There exists no common pattern. 'radius' or 'angle' might have special meaning.
                    if (sShapeType == "ooxml-blockArc" && nIndex == 1)
                    {
                        // usual angle, special radius
                        fAngle = lcl_getAngleInOOXMLUnit(fDY, fDX);
                        // The value connected to REFR is the _difference_ between the outer
                        // ellipse given by shape width and height and the inner ellipse through
                        // the handle position.
                        double fRadiusDifference
                            = lcl_getRadiusDistance(fWidth / 2.0, fHeight / 2.0, fDX, fDY);
                        double fss(std::min(fWidth, fHeight));
                        if (fss != 0)
                            fRadius = fRadiusDifference * 100000.0 / fss;
                    }
                    else if (sShapeType == "ooxml-donut" || sShapeType == "ooxml-noSmoking")
                    {
                        // no angle adjustment, radius bound to x-coordinate of handle
                        double fss(std::min(fWidth, fHeight));
                        if (fss != 0.0)
                            fRadius = fPos1 * 100000.0 / fss;
                    }
                    else if ((sShapeType == "ooxml-circularArrow"
                              || sShapeType == "ooxml-leftRightCircularArrow"
                              || sShapeType == "ooxml-leftCircularArrow")
                             && nIndex == 0)
                    {
                        // The value adj2 is the increase compared to the angle in adj3
                        double fHandleAngle = lcl_getAngleInOOXMLUnit(fDY, fDX);
                        if (sShapeType == "ooxml-leftCircularArrow")
                            fAngle = GetAdjustValueAsDouble(2) - fHandleAngle;
                        else
                            fAngle = fHandleAngle - GetAdjustValueAsDouble(2);
                        if (fAngle < 0.0) // 0deg to 360deg cut
                            fAngle += 21600000.0;
                        // no REFR
                    }
                    else if ((sShapeType == "ooxml-circularArrow"
                              || sShapeType == "ooxml-leftCircularArrow"
                              || sShapeType == "ooxml-leftRightCircularArrow")
                             && nIndex == 2)
                    {
                        // The value adj1 connected to REFR is the thickness of the arc. The adjustvalue adj5
                        // has the _difference_ between the outer ellipse given by shape width and height
                        // and the middle ellipse of the arc. The handle is on the outer side of the
                        // arc. So we calculate the difference between the ellipse through the handle
                        // and the outer ellipse and subtract then.
                        double fRadiusDifferenceHandle
                            = lcl_getRadiusDistance(fWidth / 2.0, fHeight / 2.0, fDX, fDY);
                        double fadj5(GetAdjustValueAsDouble(4));
                        double fss(std::min(fWidth, fHeight));
                        if (fss != 0.0)
                        {
                            fadj5 = fadj5 * fss / 100000.0;
                            fRadius = 2.0 * (fadj5 - fRadiusDifferenceHandle);
                            fRadius = fRadius * 100000.0 / fss;
                        }
                        // ToDo: Get angle adj3 exact. Use approximation for now
                        fAngle = lcl_getAngleInOOXMLUnit(fDY, fDX);
                    }
                    else if ((sShapeType == "ooxml-circularArrow"
                              || sShapeType == "ooxml-leftCircularArrow"
                              || sShapeType == "ooxml-leftRightCircularArrow")
                             && nIndex == 3)
                    {
                        // ToDo: Getting handle position from adjustment value adj5 is complex.
                        // Analytical or numerical solution for backward calculation is missing.
                        // Approximation for now, using a line from center through handle position.
                        double fAngleRad(0.0);
                        if (fDX != 0.0 || fDY != 0.0)
                            fAngleRad = atan2(fDY, fDX);
                        double fHelpX = cos(fAngleRad) * fHeight / 2.0;
                        double fHelpY = sin(fAngleRad) * fWidth / 2.0;
                        if (fHelpX != 0.0 || fHelpY != 0.0)
                        {
                            double fHelpAngle = atan2(fHelpY, fHelpX);
                            double fOuterX = fWidth / 2.0 * cos(fHelpAngle);
                            double fOuterY = fHeight / 2.0 * sin(fHelpAngle);
                            double fOuterRadius = std::hypot(fOuterX, fOuterY);
                            double fHandleRadius = std::hypot(fDX, fDY);
                            fRadius = (fOuterRadius - fHandleRadius) / 2.0;
                            double fss(std::min(fWidth, fHeight));
                            if (fss != 0.0)
                                fRadius = fRadius * 100000.0 / fss;
                        }
                        // no REFANGLE
                    }
                    else if (sShapeType == "ooxml-mathNotEqual" && nIndex == 1)
                    {
                        double fadj1(GetAdjustValueAsDouble(0));
                        double fadj3(GetAdjustValueAsDouble(2));
                        fadj1 = fadj1 * fHeight / 100000.0;
                        fadj3 = fadj3 * fHeight / 100000.0;
                        double fDYRefHorizBar = fDY + fadj1 + fadj3;
                        if (fDX != 0.0 || fDYRefHorizBar != 0.0)
                        {
                            double fRawAngleDeg = basegfx::rad2deg(atan2(fDYRefHorizBar, fDX));
                            fAngle = (fRawAngleDeg + 180.0) * 60000.0;
                        }
                        // no REFR
                    }
                    else
                    {
                        // no special meaning of radius or angle, suitable for "ooxml-arc",
                        // "ooxml-chord", "ooxml-pie" and circular arrows value adj4.
                        fAngle = lcl_getAngleInOOXMLUnit(fDY, fDX);
                        fRadius = std::hypot(fDX, fDY);
                        double fss(std::min(fWidth, fHeight));
                        if (fss != 0.0)
                            fRadius = fRadius * 100000.0 / fss;
                    }
                }
                else // e.g. shapes from ODF, MS binary import or shape type "fontwork-foo"
                {
                    double fXRef, fYRef;
                    if (aHandle.nFlags & HandleFlags::POLAR)
                    {
                        GetParameter(fXRef, aHandle.aPolar.First, false, false);
                        GetParameter(fYRef, aHandle.aPolar.Second, false, false);
                    }
                    else
                    {
                        fXRef = fWidth / 2.0;
                        fYRef = fHeight / 2.0;
                    }
                    const double fDX = fPos1 - fXRef;
                    const double fDY = fPos2 - fYRef;
                    // ToDo: MS binary uses fixed-point number for the angle. Make sure conversion
                    // to double is done in import and export.
                    // ToDo: Angle unit is degree, but range ]-180;180] or [0;360[? Assume ]-180;180].
                    if (fDX != 0.0 || fDY != 0.0)
                    {
                        fRadius = std::hypot(fDX, fDY);
                        fAngle = basegfx::rad2deg(atan2(fDY, fDX));
                    }
                }

                // All formats can restrict the radius to a range
                if ( aHandle.nFlags & HandleFlags::RADIUS_RANGE_MINIMUM )
                {
                    double fMin;
                    GetParameter( fMin,  aHandle.aRadiusRangeMinimum, false, false );
                    if ( fRadius < fMin )
                        fRadius = fMin;
                }
                if ( aHandle.nFlags & HandleFlags::RADIUS_RANGE_MAXIMUM )
                {
                    double fMax;
                    GetParameter( fMax, aHandle.aRadiusRangeMaximum, false, false );
                    if ( fRadius > fMax )
                        fRadius = fMax;
                }

                if ( nFirstAdjustmentValue >= 0 )
                    SetAdjustValueAsDouble( fRadius, nFirstAdjustmentValue );
                if ( nSecondAdjustmentValue >= 0 )
                    SetAdjustValueAsDouble( fAngle,  nSecondAdjustmentValue );
            }
            else // XY-Handle
            {
                // Calculating the adjustment values follows in most cases some patterns, which only
                // need width and height of the shape and handle position. These patterns are calculated
                // in the static, local methods. More complex calculations or additional steps are
                // done here.
                // Values for corner cases like 'root(negative)' or 'div zero' are meaningless dummies.
                // Identifiers often refer to guide names in OOXML shape definitions.
                double fAdjustX = fPos1;
                double fAdjustY = fPos2;
                if (aHandle.nFlags & HandleFlags::REFX)
                {
                    nFirstAdjustmentValue = aHandle.nRefX;
                    if ((sShapeType == "ooxml-gear6") || (sShapeType == "ooxml-gear9"))
                    {
                        // special, needs angle calculations
                        double fss(std::min(fWidth, fHeight));
                        double fadj1(GetAdjustValueAsDouble(0)); // from point D6 or D9
                        double fth(fadj1 * fss / 100000.0); // radius difference
                        double frw(fWidth / 2.0 - fth); // inner ellipse
                        double frh(fHeight / 2.0 - fth);
                        double fDX(fPos1 - fWidth / 2.0);
                        double fDY(fPos2 - fHeight / 2.0);
                        double fbA(-1.7); // effective angle for point A6 or A9, dummy value
                        if (fDX != 0.0 || fDY != 0.0)
                            fbA = atan2(fDY, fDX);
                        double faA(fbA); // corresponding circle angle, dummy value
                        double ftmpX(frh * cos(fbA));
                        double ftmpY(frw * sin(fbA));
                        if (ftmpX != 0.0 || ftmpY != 0.0)
                            faA = atan2(ftmpY, ftmpX); // range ]-pi..pi], here -pi < faA < -pi/2
                        // screen 270 deg = mathematic coordinate system -pi/2
                        double fha(-M_PI_2 - faA); // positive circle angle difference to 270 deg
                        if (abs(fha) == M_PI_2) // should not happen, but ensure no tan(90deg)
                            fha = 0.12; // dummy value
                        double flFD(2 * std::min(frw, frh) * tan(fha) - fth);
                        if (fss != 0.0)
                            fAdjustX = flFD / fss * 100000.0;
                    }
                    else
                    {
                        fAdjustX
                            = lcl_getXAdjustmentValue(sShapeType, nIndex, fPos1, fWidth, fHeight);
                        if ((sShapeType == "ooxml-curvedDownArrow")
                            || (sShapeType == "ooxml-curvedUpArrow"))
                        {
                            double fss(std::min(fWidth, fHeight));
                            if (fss != 0.0)
                            {
                                double fadj3(GetAdjustValueAsDouble(2));
                                double fHScaled(100000.0 * fHeight / fss);
                                double fRadicand(fHScaled * fHScaled - fadj3 * fadj3);
                                double fSqrt = fRadicand >= 0.0 ? sqrt(fRadicand) : 0.0;
                                double fPart(200000.0 * fWidth / fss * (fSqrt + fHScaled));
                                fAdjustX = fPart - 4.0 * fHScaled * fAdjustX;
                                if (nIndex == 0)
                                {
                                    // calculate adj1
                                    double fadj2(GetAdjustValueAsDouble(1));
                                    fAdjustX = fAdjustX - fadj2 * (fSqrt + fHScaled);
                                    double fDenominator(fSqrt - 3.0 * fHScaled);
                                    fAdjustX /= fDenominator != 0.0 ? fDenominator : 1.0;
                                }
                                else
                                {
                                    // nIndex == 1, calculate adj2
                                    double fadj1(GetAdjustValueAsDouble(0));
                                    fAdjustX = fAdjustX - fadj1 * (fSqrt - fHScaled);
                                    double fDenominator(fSqrt + 3.0 * fHScaled);
                                    fAdjustX /= fDenominator != 0.0 ? fDenominator : 1.0;
                                }
                            }
                        }
                    }
                }

                if (aHandle.nFlags & HandleFlags::REFY)
                {
                    nSecondAdjustmentValue = aHandle.nRefY;
                    if ((sShapeType == "ooxml-gear6") || (sShapeType == "ooxml-gear9"))
                    {
                        // special, acts more like a polar handle radius
                        double fDX = fPos1 - fWidth / 2.0;
                        double fDY = fPos2 - fHeight / 2.0;
                        double fRadiusDifference
                            = lcl_getRadiusDistance(fWidth / 2.0, fHeight / 2.0, fDX, fDY);
                        double fss(std::min(fWidth, fHeight));
                        if (fss != 0)
                            fAdjustY = fRadiusDifference / fss * 100000.0;
                    }
                    else
                    {
                        fAdjustY
                            = lcl_getYAdjustmentValue(sShapeType, nIndex, fPos2, fWidth, fHeight);
                        if (sShapeType == "ooxml-mathDivide" && nIndex == 1)
                            fAdjustY = fAdjustY - GetAdjustValueAsDouble(0) / 2.0
                                       - GetAdjustValueAsDouble(2);
                        else if (sShapeType == "ooxml-mathEqual" && nIndex == 0)
                            fAdjustY -= GetAdjustValueAsDouble(1) / 2.0;
                        else if (sShapeType == "ooxml-mathNotEqual" && nIndex == 0)
                            fAdjustY -= GetAdjustValueAsDouble(2) / 2.0;
                        else if (sShapeType == "ooxml-leftUpArrow" && nIndex == 0)
                            fAdjustY -= GetAdjustValueAsDouble(1) * 2.0;
                        else if ((sShapeType == "ooxml-curvedRightArrow")
                                 || (sShapeType == "ooxml-curvedLeftArrow"))
                        {
                            double fss(std::min(fWidth, fHeight));
                            if (fss != 0.0)
                            {
                                double fadj3(GetAdjustValueAsDouble(2));
                                double fWScaled(100000.0 * fWidth / fss);
                                double fRadicand(fWScaled * fWScaled - fadj3 * fadj3);
                                double fSqrt = fRadicand >= 0.0 ? sqrt(fRadicand) : 0.0;
                                if (nIndex == 0)
                                {
                                    // calculate adj1
                                    double fadj2(GetAdjustValueAsDouble(1));
                                    fAdjustY = fWScaled * (2.0 * fAdjustY - fadj2);
                                    fAdjustY += (200000.0 / fss * fHeight - fadj2) * fSqrt;
                                    double fDenominator(fSqrt + fWScaled);
                                    fAdjustY /= fDenominator != 0.0 ? fDenominator : 1.0;
                                }
                                else
                                {
                                    // nIndex == 1, calculate adj2
                                    double fadj1(GetAdjustValueAsDouble(0));
                                    fAdjustY = fWScaled * (2.0 * fAdjustY + fadj1);
                                    fAdjustY += (200000.0 / fss * fHeight - fadj1) * fSqrt;
                                    double fDenominator(fSqrt + 3.0 * fWScaled);
                                    fAdjustY /= fDenominator != 0.0 ? fDenominator : 1.0;
                                }
                            }
                        }
                        else if (sShapeType == "ooxml-uturnArrow" && nIndex == 2)
                        {
                            double fss(std::min(fWidth, fHeight));
                            if (fss != 0.0)
                            {
                                double fadj5(GetAdjustValueAsDouble(4));
                                fAdjustY += fHeight / fss * (fadj5 - 100000.0);
                            }
                        }
                        else if (sShapeType == "ooxml-leftRightRibbon")
                        {
                            if (nIndex == 0)
                                fAdjustY = GetAdjustValueAsDouble(2) - fAdjustY;
                            else // nIndex == 2
                                fAdjustY = GetAdjustValueAsDouble(0) + fAdjustY;
                        }
                    }
                }

                if ( nFirstAdjustmentValue >= 0 )
                {
                    if ( aHandle.nFlags & HandleFlags::RANGE_X_MINIMUM )        // check if horizontal handle needs to be within a range
                    {
                        double fXMin;
                        GetParameter( fXMin, aHandle.aXRangeMinimum, false, false );
                        if (fAdjustX < fXMin)
                            fAdjustX = fXMin;
                    }
                    if ( aHandle.nFlags & HandleFlags::RANGE_X_MAXIMUM )        // check if horizontal handle needs to be within a range
                    {
                        double fXMax;
                        GetParameter( fXMax, aHandle.aXRangeMaximum, false, false );
                        if (fAdjustX > fXMax)
                            fAdjustX = fXMax;
                    }
                    SetAdjustValueAsDouble(fAdjustX, nFirstAdjustmentValue);
                }
                if ( nSecondAdjustmentValue >= 0 )
                {
                    if ( aHandle.nFlags & HandleFlags::RANGE_Y_MINIMUM )        // check if vertical handle needs to be within a range
                    {
                        double fYMin;
                        GetParameter( fYMin, aHandle.aYRangeMinimum, false, false );
                        if (fAdjustY < fYMin)
                            fAdjustY = fYMin;
                    }
                    if ( aHandle.nFlags & HandleFlags::RANGE_Y_MAXIMUM )        // check if vertical handle needs to be within a range
                    {
                        double fYMax;
                        GetParameter( fYMax, aHandle.aYRangeMaximum, false, false );
                        if (fAdjustY > fYMax)
                            fAdjustY = fYMax;
                    }
                    SetAdjustValueAsDouble(fAdjustY, nSecondAdjustmentValue);
                }
            }
            // and writing them back into the GeometryItem
            SdrCustomShapeGeometryItem aGeometryItem(mrSdrObjCustomShape.GetMergedItem( SDRATTR_CUSTOMSHAPE_GEOMETRY ));
            css::beans::PropertyValue aPropVal;
            aPropVal.Name = "AdjustmentValues";
            aPropVal.Value <<= m_seqAdjustmentValues;
            aGeometryItem.SetPropertyValue( aPropVal );
            mrSdrObjCustomShape.SetMergedItem( aGeometryItem );
            bRetValue = true;
        }
    }
    return bRetValue;
}

void EnhancedCustomShape2d::SwapStartAndEndArrow( SdrObject* pObj ) //#108274
{
    XLineStartItem       aLineStart;
    aLineStart.SetLineStartValue(pObj->GetMergedItem( XATTR_LINEEND ).GetLineEndValue());
    XLineStartWidthItem  aLineStartWidth(pObj->GetMergedItem( XATTR_LINEENDWIDTH ).GetValue());
    XLineStartCenterItem aLineStartCenter(pObj->GetMergedItem( XATTR_LINEENDCENTER ).GetValue());

    XLineEndItem         aLineEnd;
    aLineEnd.SetLineEndValue(pObj->GetMergedItem( XATTR_LINESTART ).GetLineStartValue());
    XLineEndWidthItem    aLineEndWidth(pObj->GetMergedItem( XATTR_LINESTARTWIDTH ).GetValue());
    XLineEndCenterItem   aLineEndCenter(pObj->GetMergedItem( XATTR_LINESTARTCENTER ).GetValue());

    pObj->SetMergedItem( aLineStart );
    pObj->SetMergedItem( aLineStartWidth );
    pObj->SetMergedItem( aLineStartCenter );
    pObj->SetMergedItem( aLineEnd );
    pObj->SetMergedItem( aLineEndWidth );
    pObj->SetMergedItem( aLineEndCenter );
}

static basegfx::B2DPolygon CreateArc( const tools::Rectangle& rRect, const Point& rStart, const Point& rEnd, const bool bClockwise )
{
    tools::Rectangle aRect( rRect );
    Point aStart( rStart );
    Point aEnd( rEnd );

    sal_Int32 bSwapStartEndAngle = 0;

    if ( aRect.Left() > aRect.Right() )
        bSwapStartEndAngle ^= 0x01;
    if ( aRect.Top() > aRect.Bottom() )
        bSwapStartEndAngle ^= 0x11;
    if ( bSwapStartEndAngle )
    {
        aRect.Normalize();
        if ( bSwapStartEndAngle & 1 )
        {
            Point aTmp( aStart );
            aStart = aEnd;
            aEnd = aTmp;
        }
    }

    tools::Polygon aTempPoly( aRect, aStart, aEnd, PolyStyle::Arc );
    basegfx::B2DPolygon aRetval;

    if ( bClockwise )
    {
        for ( sal_uInt16 j = aTempPoly.GetSize(); j--; )
        {
            aRetval.append(basegfx::B2DPoint(aTempPoly[ j ].X(), aTempPoly[ j ].Y()));
        }
    }
    else
    {
        for ( sal_uInt16 j = 0; j < aTempPoly.GetSize(); j++ )
        {
            aRetval.append(basegfx::B2DPoint(aTempPoly[ j ].X(), aTempPoly[ j ].Y()));
        }
    }

    return aRetval;
}

static double lcl_getNormalizedCircleAngleRad(const double fWR, const double fHR, const double fEllipseAngleDeg)
{
    double fRet(0.0);
    double fEAngleDeg(fmod(fEllipseAngleDeg, 360.0));
    if (fEAngleDeg < 0.0)
        fEAngleDeg += 360.0;
    if (fEAngleDeg == 0.0 || fEAngleDeg == 90.0 || fEAngleDeg == 180.0 || fEAngleDeg == 270.0)
        return basegfx::deg2rad(fEAngleDeg);
    const double fX(fHR * cos(basegfx::deg2rad(fEAngleDeg)));
    const double fY(fWR * sin(basegfx::deg2rad(fEAngleDeg)));
    if (fX != 0.0 || fY != 0.0)
    {
        fRet = atan2(fY, fX);
        if (fRet < 0.0)
            fRet += 2 * M_PI;
    }
    return fRet;
}

static double lcl_getNormalizedAngleRad(const double fCircleAngleDeg)
{
    double fRet(fmod(fCircleAngleDeg, 360.0));
    if (fRet < 0.0)
        fRet += 360.0;
    return basegfx::deg2rad(fRet);
}

void EnhancedCustomShape2d::CreateSubPath(
    sal_Int32& rSrcPt,
    sal_Int32& rSegmentInd,
    std::vector< std::pair< rtl::Reference<SdrPathObj>, double> >& rObjectList,
    const bool bLineGeometryNeededOnly,
    const bool bSortFilledObjectsToBack,
    sal_Int32 nIndex)
{
    bool bNoFill = false;
    bool bNoStroke = false;
    double dBrightness = 0.0; //no blending

    basegfx::B2DPolyPolygon aNewB2DPolyPolygon;
    basegfx::B2DPolygon aNewB2DPolygon;

    SetPathSize( nIndex );

    sal_Int32 nSegInfoSize = m_seqSegments.getLength();
    if ( !nSegInfoSize )
    {
        for (const EnhancedCustomShapeParameterPair& rCoordinate : m_seqCoordinates)
        {
            const Point aTempPoint(GetPoint( rCoordinate, true, true ));
            aNewB2DPolygon.append(basegfx::B2DPoint(aTempPoint.X(), aTempPoint.Y()));
        }

        aNewB2DPolygon.setClosed(true);
    }
    else
    {
        sal_Int32 nCoordSize = m_seqCoordinates.getLength();
        for ( ;rSegmentInd < nSegInfoSize; )
        {
            sal_Int16 nCommand = m_seqSegments[ rSegmentInd ].Command;
            sal_Int16 nPntCount= m_seqSegments[ rSegmentInd++ ].Count;

            switch ( nCommand )
            {
                case NOFILL :
                    bNoFill = true;
                break;
                case NOSTROKE :
                    bNoStroke = true;
                break;
                case DARKEN :
                    dBrightness = -0.4; //use sign to distinguish DARKEN from LIGHTEN
                    break;
                case DARKENLESS :
                    dBrightness = -0.2;
                    break;
                case LIGHTEN :
                    dBrightness = 0.4;
                    break;
                case LIGHTENLESS :
                    dBrightness = 0.2;
                    break;
                case MOVETO :
                {
                    if(aNewB2DPolygon.count() > 1)
                    {
                        // #i76201# Add conversion to closed polygon when first and last points are equal
                        basegfx::utils::checkClosed(aNewB2DPolygon);
                        aNewB2DPolyPolygon.append(aNewB2DPolygon);
                    }

                    aNewB2DPolygon.clear();

                    if ( rSrcPt < nCoordSize )
                    {
                        const Point aTempPoint(GetPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                        SAL_INFO(
                            "svx",
                            "moveTo: " << aTempPoint.X() << ","
                                << aTempPoint.Y());
                        aNewB2DPolygon.append(basegfx::B2DPoint(aTempPoint.X(), aTempPoint.Y()));
                    }
                }
                break;
                case ENDSUBPATH :
                break;
                case CLOSESUBPATH :
                {
                    if(aNewB2DPolygon.count())
                    {
                        if(aNewB2DPolygon.count() > 1)
                        {
                            aNewB2DPolygon.setClosed(true);
                            aNewB2DPolyPolygon.append(aNewB2DPolygon);
                        }

                        aNewB2DPolygon.clear();
                    }
                }
                break;
                case CURVETO :
                {
                    for ( sal_uInt16 i = 0; ( i < nPntCount ) && ( ( rSrcPt + 2 ) < nCoordSize ); i++ )
                    {
                        const Point aControlA(GetPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                        const Point aControlB(GetPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                        const Point aEnd(GetPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));

                        DBG_ASSERT(aNewB2DPolygon.count(), "EnhancedCustomShape2d::CreateSubPath: Error in adding control point (!)");
                        aNewB2DPolygon.appendBezierSegment(
                            basegfx::B2DPoint(aControlA.X(), aControlA.Y()),
                            basegfx::B2DPoint(aControlB.X(), aControlB.Y()),
                            basegfx::B2DPoint(aEnd.X(), aEnd.Y()));
                    }
                }
                break;

                case ANGLEELLIPSE: // command U
                case ANGLEELLIPSETO: // command T
                {
                    // Some shapes will need special handling, decide on property 'Type'.
                    OUString sShpType;
                    const SdrCustomShapeGeometryItem& rGeometryItem = mrSdrObjCustomShape.GetMergedItem(SDRATTR_CUSTOMSHAPE_GEOMETRY);
                    const Any* pAny = rGeometryItem.GetPropertyValueByName(u"Type"_ustr);
                    if (pAny)
                        *pAny >>= sShpType;
                    // User defined shapes in MS binary format, which contain command U or T after import
                    // in LibreOffice, starts with "mso".
                    const bool bIsFromBinaryImport(sShpType.startsWith("mso"));
                    // The only own or imported preset shapes with U command are those listed below.
                    // Command T is not used in preset shapes.
                    const std::unordered_set<OUString> aPresetShapesWithU =
                        { u"ellipse"_ustr,  u"ring"_ustr, u"smiley"_ustr, u"sun"_ustr, u"forbidden"_ustr, u"flowchart-connector"_ustr,
                          u"flowchart-summing-junction"_ustr, u"flowchart-or"_ustr, u"cloud-callout"_ustr};
                    std::unordered_set<OUString>::const_iterator aIter = aPresetShapesWithU.find(sShpType);
                    const bool bIsPresetShapeWithU(aIter != aPresetShapesWithU.end());

                    for (sal_uInt16 i = 0; (i < nPntCount) && ((rSrcPt + 2) < nCoordSize); i++)
                    {
                        // ANGLEELLIPSE is the same as ANGLEELLIPSETO, only that it
                        // makes an implicit MOVETO. That ends the previous subpath.
                        if (ANGLEELLIPSE == nCommand)
                        {
                            if (aNewB2DPolygon.count() > 1)
                            {
                                // #i76201# Add conversion to closed polygon when first and last points are equal
                                basegfx::utils::checkClosed(aNewB2DPolygon);
                                aNewB2DPolyPolygon.append(aNewB2DPolygon);
                            }
                            aNewB2DPolygon.clear();
                        }

                        // Read all parameters, but do not finally handle them.
                        basegfx::B2DPoint aCenter(GetPointAsB2DPoint(m_seqCoordinates[ rSrcPt ], true, true));
                        double fWR; // horizontal ellipse radius
                        double fHR; // vertical ellipse radius
                        GetParameter(fWR, m_seqCoordinates[rSrcPt + 1].First, true, false);
                        GetParameter(fHR, m_seqCoordinates[rSrcPt + 1].Second, false, true);
                        double fStartAngle;
                        GetParameter(fStartAngle, m_seqCoordinates[rSrcPt + 2].First, false, false);
                        double fEndAngle;
                        GetParameter(fEndAngle, m_seqCoordinates[rSrcPt + 2].Second, false, false);
                        // Increasing here allows flat case differentiation tree by using 'continue'.
                        rSrcPt += 3;

                        double fScaledWR(fWR * m_fXScale);
                        double fScaledHR(fHR * m_fYScale);
                        if (fScaledWR == 0.0 && fScaledHR == 0.0)
                        {
                            // degenerated ellipse, add center point
                            aNewB2DPolygon.append(aCenter);
                            continue;
                        }

                        if (bIsFromBinaryImport)
                        {
                            // If a shape comes from MS binary ('escher') import, the angles are in degrees*2^16
                            // and the second angle is not an end angle, but a swing angle.
                            // MS Word shows this behavior: 0deg right, 90deg top, 180deg left and 270deg
                            // bottom. Third and forth parameter are horizontal and vertical radius, not width
                            // and height as noted in VML spec. A positive swing angle goes counter-clock
                            // wise (in user view). The swing angle might go several times around in case
                            // abs(swing angle) >= 360deg. Stroke accumulates, so that e.g. dash-dot might fill the
                            // gaps of previous turn. Fill does not accumulate but uses even-odd rule, semi-transparent
                            // fill does not become darker. The start and end points of the arc are calculated by
                            // using the angles on a circle and then scaling the circle to the ellipse. Caution, that
                            // is different from angle handling in ARCANGLETO and ODF.
                            // The following implementation generates such rendering. It is only for rendering legacy
                            // MS shapes and independent of the meaning of commands U and T in ODF specification.

                            // The WordArt shape 'RingOutside' has already angles in degree, all other need
                            // conversion from fixed-point number.
                            double fSwingAngle = fEndAngle;
                            if (sShpType != "mso-spt143")
                            {
                                fStartAngle /= 65536.0;
                                fSwingAngle = fEndAngle / 65536.0;
                            }
                            // Convert orientation
                            fStartAngle = -fStartAngle;
                            fSwingAngle = -fSwingAngle;

                            fEndAngle = fStartAngle + fSwingAngle;
                            if (fSwingAngle < 0.0)
                                std::swap(fStartAngle, fEndAngle);
                            double fFrom(fStartAngle);
                            double fTo(fFrom + 180.0);
                            basegfx::B2DPolygon aTempB2DPolygon;
                            double fS; // fFrom in radians in [0..2Pi[
                            double fE; // fTo or fEndAngle in radians in [0..2PI[
                            while (fTo < fEndAngle)
                            {
                                fS = lcl_getNormalizedAngleRad(fFrom);
                                fE = lcl_getNormalizedAngleRad(fTo);
                                aTempB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fScaledWR, fScaledHR, fS,fE));
                                fFrom = fTo;
                                fTo += 180.0;
                            }
                            fS = lcl_getNormalizedAngleRad(fFrom);
                            fE = lcl_getNormalizedAngleRad(fEndAngle);
                            aTempB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fScaledWR, fScaledHR,fS, fE));
                            if (fSwingAngle < 0)
                                aTempB2DPolygon.flip();
                            aNewB2DPolygon.append(aTempB2DPolygon);
                            continue;
                        }

                        // The not yet handled shapes are own preset shapes, or preset shapes from MS binary import, or user
                        // defined shapes, or foreign shapes. Shapes from OOXML import do not use ANGLEELLIPSE or
                        // ANGLEELLIPSETO, but use ARCANGLETO.
                        if (bIsPresetShapeWithU)
                        {
                            // Besides "cloud-callout" all preset shapes have angle values '0 360'.
                            // The imported "cloud-callout" has angle values '0 360' too, only our own "cloud-callout"
                            // has values '0 23592960'. But that is fixedfloat and means 360*2^16. Thus all these shapes
                            // have a full ellipse with start at 0deg.
                            aNewB2DPolygon.append(basegfx::utils::createPolygonFromEllipse(aCenter, fScaledWR, fScaledHR));
                            continue;
                        }

                        // In all other cases, full ODF conform handling is necessary. ODF rules:
                        // Third and forth parameter are horizontal and vertical radius.
                        // An angle determines the start or end point of the segment by intersection of the second angle
                        // leg with the ellipse. The first angle leg is always the positive x-axis. For the position
                        // of the intersection points the angle is used modulo 360deg in range [0deg..360deg[.
                        // The position of range [0deg..360deg[ is the same as in command ARCANGLETO, with 0deg right,
                        // 90deg bottom, 180deg left and 270deg top. Only if abs(end angle - start angle) == 360 deg,
                        // a full ellipse is drawn. The segment is always drawn clock wise (in user view) from start
                        // point to end point. The end point of the segment becomes the new "current" point.

                        if (fabs(fabs(fEndAngle - fStartAngle) - 360.0) < 1.0E-15)
                        {
                            // draw full ellipse
                            // Because createPolygonFromEllipseSegment cannot create full ellipse and
                            // createPolygonFromEllipse has no varying starts, we use two half ellipses.
                            const double fS(lcl_getNormalizedCircleAngleRad(fWR, fHR, fStartAngle));
                            const double fH(lcl_getNormalizedCircleAngleRad(fWR, fHR, fStartAngle + 180.0));
                            const double fE(lcl_getNormalizedCircleAngleRad(fWR, fHR, fEndAngle));
                            aNewB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fScaledWR, fScaledHR, fS, fH));
                            aNewB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fScaledWR, fScaledHR, fH, fE));
                            continue;
                        }

                        // remaining cases with central segment angle < 360
                        double fS(lcl_getNormalizedCircleAngleRad(fWR, fHR, fStartAngle));
                        double fE(lcl_getNormalizedCircleAngleRad(fWR, fHR, fEndAngle));
                        aNewB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fScaledWR, fScaledHR, fS, fE));
                    } // end for
                } // end case
                break;

                case QUADRATICCURVETO :
                {
                    for ( sal_Int32 i(0); ( i < nPntCount ) && ( rSrcPt + 1 < nCoordSize ); i++ )
                    {
                        DBG_ASSERT(aNewB2DPolygon.count(), "EnhancedCustomShape2d::CreateSubPath: Error no previous point for Q (!)");
                        if (aNewB2DPolygon.count() > 0)
                        {
                            const basegfx::B2DPoint aPreviousEndPoint(aNewB2DPolygon.getB2DPoint(aNewB2DPolygon.count()-1));
                            const basegfx::B2DPoint aControlQ(GetPointAsB2DPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                            const basegfx::B2DPoint aEnd(GetPointAsB2DPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                            const basegfx::B2DPoint aControlA((aPreviousEndPoint + (aControlQ * 2)) / 3);
                            const basegfx::B2DPoint aControlB(((aControlQ * 2) + aEnd) / 3);
                            aNewB2DPolygon.appendBezierSegment(aControlA, aControlB, aEnd);
                        }
                        else // no previous point; ill structured path, but try to draw as much as possible
                        {
                            rSrcPt++; // skip control point
                            const basegfx::B2DPoint aEnd(GetPointAsB2DPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                            aNewB2DPolygon.append(aEnd);
                        }
                    }
                }
                break;

                case LINETO :
                {
                    for ( sal_Int32 i(0); ( i < nPntCount ) && ( rSrcPt < nCoordSize ); i++ )
                    {
                        const Point aTempPoint(GetPoint( m_seqCoordinates[ rSrcPt++ ], true, true ));
                        SAL_INFO(
                            "svx",
                            "lineTo: " << aTempPoint.X() << ","
                                << aTempPoint.Y());
                        aNewB2DPolygon.append(basegfx::B2DPoint(aTempPoint.X(), aTempPoint.Y()));
                    }
                }
                break;

                case ARC :
                case CLOCKWISEARC :
                case ARCTO :
                case CLOCKWISEARCTO :
                {
                    bool bClockwise = ( nCommand == CLOCKWISEARC ) || ( nCommand == CLOCKWISEARCTO );
                    bool bImplicitMoveTo = (nCommand == ARC) || (nCommand == CLOCKWISEARC);
                    sal_uInt32 nXor = bClockwise ? 3 : 2;
                    for ( sal_uInt16 i = 0; ( i < nPntCount ) && ( ( rSrcPt + 3 ) < nCoordSize ); i++ )
                    {
                        if (bImplicitMoveTo)
                        {
                            if (aNewB2DPolygon.count() > 1)
                            {
                                // #i76201# Add conversion to closed polygon when first and last
                                // points are equal
                                basegfx::utils::checkClosed(aNewB2DPolygon);
                                aNewB2DPolyPolygon.append(aNewB2DPolygon);
                            }
                            aNewB2DPolygon.clear();
                        }
                        tools::Rectangle aRect = tools::Rectangle::Normalize( GetPoint( m_seqCoordinates[ rSrcPt ], true, true ), GetPoint( m_seqCoordinates[ rSrcPt + 1 ], true, true ) );
                        if ( aRect.GetWidth() && aRect.GetHeight() )
                        {
                            Point aStart( GetPoint( m_seqCoordinates[ static_cast<sal_uInt16>( rSrcPt + nXor ) ], true, true ) );
                            Point aEnd( GetPoint( m_seqCoordinates[ static_cast<sal_uInt16>( rSrcPt + ( nXor ^ 1 ) ) ], true, true ) );
                            aNewB2DPolygon.append(CreateArc( aRect, aStart, aEnd, bClockwise));
                        }
                        rSrcPt += 4;
                    }
                }
                break;

                case ARCANGLETO :
                {
                    double fWR, fHR; // in Shape coordinate system
                    double fStartAngle, fSwingAngle; // in deg

                    for ( sal_uInt16 i = 0; ( i < nPntCount ) && ( rSrcPt + 1 < nCoordSize ); i++ )
                    {
                        basegfx::B2DPoint aTempPair;
                        aTempPair = GetPointAsB2DPoint(m_seqCoordinates[static_cast<sal_uInt16>(rSrcPt)], false /*bScale*/, false /*bReplaceGeoSize*/);
                        fWR = aTempPair.getX();
                        fHR = aTempPair.getY();
                        aTempPair = GetPointAsB2DPoint(m_seqCoordinates[static_cast<sal_uInt16>(rSrcPt + 1)], false /*bScale*/, false /*bReplaceGeoSize*/);
                        fStartAngle = aTempPair.getX();
                        fSwingAngle = aTempPair.getY();

                        // tdf#122323 MS Office clamps the swing angle to [-360,360]. Such restriction
                        // is neither in OOXML nor in ODF. Nevertheless, to be compatible we do it for
                        // "ooxml-foo" shapes. Those shapes have their origin in MS Office.
                        if (m_bOOXMLShape)
                        {
                            fSwingAngle = std::clamp(fSwingAngle, -360.0, 360.0);
                        }

                        SAL_INFO("svx", "ARCANGLETO scale: " << fWR << "x" << fHR << " angles: " << fStartAngle << "," << fSwingAngle);

                        if (aNewB2DPolygon.count() > 0) // otherwise no "current point"
                        {
                            // use similar methods as in command U
                            basegfx::B2DPolygon aTempB2DPolygon;

                            if (fWR == 0.0 && fHR == 0.0)
                            {
                                // degenerated ellipse, add this one point
                                aTempB2DPolygon.append(basegfx::B2DPoint(0.0, 0.0));
                            }
                            else
                            {
                                double fEndAngle = fStartAngle + fSwingAngle;
                                // Generate arc with ellipse left|top = 0|0.
                                basegfx::B2DPoint aCenter(fWR, fHR);
                                if (fSwingAngle < 0.0)
                                    std::swap(fStartAngle, fEndAngle);
                                double fS; // fFrom in radians in [0..2Pi[
                                double fE; // fTo or fEndAngle in radians in [0..2PI[
                                double fFrom(fStartAngle);
                                // createPolygonFromEllipseSegment expects angles in [0..2PI[.
                                if (fSwingAngle >= 360.0 || fSwingAngle <= -360.0)
                                {
                                    double fTo(fFrom + 180.0);
                                    while (fTo < fEndAngle)
                                    {
                                        fS = lcl_getNormalizedCircleAngleRad(fWR, fHR, fFrom);
                                        fE = lcl_getNormalizedCircleAngleRad(fWR, fHR, fTo);
                                        aTempB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fWR, fHR, fS,fE));
                                        fFrom = fTo;
                                        fTo += 180.0;
                                    }
                                }
                                fS = lcl_getNormalizedCircleAngleRad(fWR, fHR, fFrom);
                                fE = lcl_getNormalizedCircleAngleRad(fWR, fHR, fEndAngle);
                                aTempB2DPolygon.append(basegfx::utils::createPolygonFromEllipseSegment(aCenter, fWR, fHR,fS, fE));
                                if (fSwingAngle < 0)
                                    aTempB2DPolygon.flip();
                                aTempB2DPolygon.removeDoublePoints();
                            }
                            // Scale arc to 1/100mm
                            basegfx::B2DHomMatrix aMatrix = basegfx::utils::createScaleB2DHomMatrix(m_fXScale, m_fYScale);
                            aTempB2DPolygon.transform(aMatrix);

                            // Now that we have the arc, move it to the "current point".
                            basegfx::B2DPoint aCurrentPointB2D( aNewB2DPolygon.getB2DPoint(aNewB2DPolygon.count() - 1 ) );
                            const double fDx(aCurrentPointB2D.getX() - aTempB2DPolygon.getB2DPoint(0).getX());
                            const double fDy(aCurrentPointB2D.getY() - aTempB2DPolygon.getB2DPoint(0).getY());
                            aTempB2DPolygon.translate(fDx, fDy);
                            aNewB2DPolygon.append(aTempB2DPolygon);
                        }

                        rSrcPt += 2;
                    }
                }
                break;

                case ELLIPTICALQUADRANTX :
                case ELLIPTICALQUADRANTY :
                {
                    if (nPntCount && (rSrcPt < nCoordSize))
                    {
                        // The arc starts at the previous point and ends at the point given in the parameter.
                        basegfx::B2DPoint aStart;
                        basegfx::B2DPoint aEnd;
                        sal_uInt16 i = 0;
                        if (rSrcPt)
                        {
                            aStart = GetPointAsB2DPoint(m_seqCoordinates[rSrcPt - 1], true, true);
                        }
                        else
                        {   // no previous point, path is ill-structured. But we want to show as much as possible.
                            // Thus make a moveTo to the point given as parameter and continue from there.
                            aStart = GetPointAsB2DPoint(m_seqCoordinates[static_cast<sal_uInt16>(rSrcPt)], true, true);
                            aNewB2DPolygon.append(aStart);
                            rSrcPt++;
                            i++;
                        }
                        // If there are several points, then the direction changes with every point.
                        bool bIsXDirection(nCommand == ELLIPTICALQUADRANTX);
                        basegfx::B2DPolygon aArc;
                        for ( ; ( i < nPntCount ) && ( rSrcPt < nCoordSize ); i++ )
                        {
                            aEnd = GetPointAsB2DPoint(m_seqCoordinates[rSrcPt], true, true);
                            basegfx::B2DPoint aCenter;
                            double fRadiusX = fabs(aEnd.getX() - aStart.getX());
                            double fRadiusY = fabs(aEnd.getY() - aStart.getY());
                            if (bIsXDirection)
                            {
                                aCenter = basegfx::B2DPoint(aStart.getX(),aEnd.getY());
                                if (aEnd.getX()<aStart.getX())
                                {
                                    if (aEnd.getY()<aStart.getY()) // left, up
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, M_PI_2, M_PI);
                                    }
                                    else // left, down
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, M_PI, 1.5*M_PI);
                                        aArc.flip();
                                    }
                                }
                                else // aEnd.getX()>=aStart.getX()
                                {
                                    if (aEnd.getY()<aStart.getY()) // right, up
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, 0.0, M_PI_2);
                                        aArc.flip();
                                    }
                                    else // right, down
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, 1.5*M_PI, 2*M_PI);
                                    }
                                }
                            }
                            else // y-direction
                            {
                                aCenter = basegfx::B2DPoint(aEnd.getX(),aStart.getY());
                                if (aEnd.getX()<aStart.getX())
                                {
                                    if (aEnd.getY()<aStart.getY()) // up, left
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, 1.5*M_PI, 2*M_PI);
                                        aArc.flip();
                                    }
                                    else // down, left
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, 0.0, M_PI_2);
                                    }
                                }
                                else // aEnd.getX()>=aStart.getX()
                                {
                                    if (aEnd.getY()<aStart.getY()) // up, right
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, M_PI, 1.5*M_PI);
                                    }
                                    else // down, right
                                    {
                                        aArc = basegfx::utils::createPolygonFromEllipseSegment(aCenter, fRadiusX, fRadiusY, M_PI_2, M_PI);
                                        aArc.flip();
                                    }
                                }
                            }
                            aNewB2DPolygon.append(aArc);
                            rSrcPt++;
                            bIsXDirection = !bIsXDirection;
                            aStart = aEnd;
                        }
                    }
                    // else error in path syntax, do nothing
                }
                break;

#ifdef DBG_CUSTOMSHAPE
                case UNKNOWN :
                default :
                {
                    SAL_WARN( "svx", "CustomShapes::unknown PolyFlagValue :" << nCommand );
                }
                break;
#endif
            }
            if ( nCommand == ENDSUBPATH )
                break;
        }
    }
    if ( rSegmentInd == nSegInfoSize )
        rSegmentInd++;

    if(aNewB2DPolygon.count() > 1)
    {
        // #i76201# Add conversion to closed polygon when first and last points are equal
        basegfx::utils::checkClosed(aNewB2DPolygon);
        aNewB2DPolyPolygon.append(aNewB2DPolygon);
    }

    if(!aNewB2DPolyPolygon.count())
        return;

    // #i37011#
    bool bForceCreateTwoObjects(false);

    if(!bSortFilledObjectsToBack && !aNewB2DPolyPolygon.isClosed() && !bNoStroke)
    {
        bForceCreateTwoObjects = true;
    }

    if(bLineGeometryNeededOnly)
    {
        bForceCreateTwoObjects = true;
        bNoFill = true;
        bNoStroke = false;
    }

    if(bForceCreateTwoObjects || bSortFilledObjectsToBack)
    {
        if(m_bFilled && !bNoFill)
        {
            basegfx::B2DPolyPolygon aClosedPolyPolygon(aNewB2DPolyPolygon);
            aClosedPolyPolygon.setClosed(true);
            rtl::Reference<SdrPathObj> pFill(new SdrPathObj(
                mrSdrObjCustomShape.getSdrModelFromSdrObject(),
                SdrObjKind::Polygon,
                std::move(aClosedPolyPolygon)));
            SfxItemSet aTempSet(*this);
            aTempSet.Put(makeSdrShadowItem(false));
            aTempSet.Put(XLineStyleItem(drawing::LineStyle_NONE));
            pFill->SetMergedItemSet(aTempSet);
            rObjectList.push_back(std::pair< rtl::Reference<SdrPathObj>, double >(std::move(pFill), dBrightness));
        }

        if(!bNoStroke)
        {
            // there is no reason to use OBJ_PLIN here when the polygon is actually closed,
            // the non-fill is defined by XFILL_NONE. Since SdrPathObj::ImpForceKind() needs
            // to correct the polygon (here: open it) using the type, the last edge may get lost.
            // Thus, use a type that fits the polygon
            rtl::Reference<SdrPathObj> pStroke(new SdrPathObj(
                mrSdrObjCustomShape.getSdrModelFromSdrObject(),
                aNewB2DPolyPolygon.isClosed() ? SdrObjKind::Polygon : SdrObjKind::PolyLine,
                aNewB2DPolyPolygon));
            SfxItemSet aTempSet(*this);
            aTempSet.Put(makeSdrShadowItem(false));
            aTempSet.Put(XFillStyleItem(drawing::FillStyle_NONE));
            pStroke->SetMergedItemSet(aTempSet);
            rObjectList.push_back(std::pair< rtl::Reference<SdrPathObj>, double >(std::move(pStroke), dBrightness));
        }
    }
    else
    {
        rtl::Reference<SdrPathObj> pObj;
        SfxItemSet aTempSet(*this);
        aTempSet.Put(makeSdrShadowItem(false));

        if(bNoFill)
        {
            // see comment above about OBJ_PLIN
            pObj = new SdrPathObj(
                mrSdrObjCustomShape.getSdrModelFromSdrObject(),
                aNewB2DPolyPolygon.isClosed() ? SdrObjKind::Polygon : SdrObjKind::PolyLine,
                aNewB2DPolyPolygon);
            aTempSet.Put(XFillStyleItem(drawing::FillStyle_NONE));
        }
        else
        {
            aNewB2DPolyPolygon.setClosed(true);
            pObj = new SdrPathObj(
                mrSdrObjCustomShape.getSdrModelFromSdrObject(),
                SdrObjKind::Polygon,
                std::move(aNewB2DPolyPolygon));
        }

        if(bNoStroke)
        {
            aTempSet.Put(XLineStyleItem(drawing::LineStyle_NONE));
        }

        pObj->SetMergedItemSet(aTempSet);
        rObjectList.push_back(std::pair< rtl::Reference<SdrPathObj>, double >(std::move(pObj), dBrightness));
    }
}

static void CorrectCalloutArrows(
    MSO_SPT eSpType,
    sal_uInt32 nLineObjectCount,
    std::vector< std::pair< rtl::Reference<SdrPathObj>, double> >& vObjectList )
{
    bool bAccent = false;
    switch( eSpType )
    {
        case mso_sptCallout1 :
        case mso_sptBorderCallout1 :
        case mso_sptCallout90 :
        case mso_sptBorderCallout90 :
        default:
        break;

        case mso_sptAccentCallout1 :
        case mso_sptAccentBorderCallout1 :
        case mso_sptAccentCallout90 :
        case mso_sptAccentBorderCallout90 :
        {
            sal_uInt32 nLine = 0;

            for ( const std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
            {
                SdrPathObj* pObj(rCandidate.first.get());

                if(pObj->IsLine())
                {
                    nLine++;
                    if ( nLine == nLineObjectCount )
                    {
                        pObj->ClearMergedItem( XATTR_LINESTART );
                        pObj->ClearMergedItem( XATTR_LINEEND );
                    }
                }
            }
        }
        break;

        // switch start & end
        case mso_sptAccentCallout2 :
        case mso_sptAccentBorderCallout2 :
            bAccent = true;
            [[fallthrough]];
        case mso_sptCallout2 :
        case mso_sptBorderCallout2 :
        {
            sal_uInt32 nLine = 0;

            for ( const std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
            {
                SdrPathObj* pObj(rCandidate.first.get());

                if(pObj->IsLine())
                {
                    nLine++;
                    if ( nLine == 1 )
                        pObj->ClearMergedItem( XATTR_LINEEND );
                    else if ( ( bAccent && ( nLine == nLineObjectCount - 1 ) ) || ( !bAccent && ( nLine == nLineObjectCount ) ) )
                        pObj->ClearMergedItem( XATTR_LINESTART );
                    else
                    {
                        pObj->ClearMergedItem( XATTR_LINESTART );
                        pObj->ClearMergedItem( XATTR_LINEEND );
                    }
                }
            }
        }
        break;

        case mso_sptAccentCallout3 :
        case mso_sptAccentBorderCallout3 :
        case mso_sptCallout3 :
        case mso_sptBorderCallout3 :
        {
            sal_uInt32 nLine = 0;

            for ( const std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
            {
                SdrPathObj* pObj(rCandidate.first.get());

                if(pObj->IsLine())
                {
                    if ( nLine )
                    {
                        pObj->ClearMergedItem( XATTR_LINESTART );
                        pObj->ClearMergedItem( XATTR_LINEEND );
                    }
                    else
                        EnhancedCustomShape2d::SwapStartAndEndArrow( pObj );

                    nLine++;
                }
            }
        }
        break;
    }
}

void EnhancedCustomShape2d::AdaptObjColor(
    SdrPathObj& rObj,
    double dBrightness,
    const SfxItemSet& rCustomShapeSet,
    sal_uInt32& nColorIndex,
    sal_uInt32 nColorCount)
{
    if ( rObj.IsLine() )
        return;

    const drawing::FillStyle eFillStyle = rObj.GetMergedItem(XATTR_FILLSTYLE).GetValue();
    if (eFillStyle == drawing::FillStyle_NONE)
        return;

    switch( eFillStyle )
    {
        default:
        case drawing::FillStyle_SOLID:
        {
            if ( nColorCount || 0.0 != dBrightness )
            {
                Color aFillColor = GetColorData(
                    rCustomShapeSet.Get( XATTR_FILLCOLOR ).GetColorValue(),
                    nColorCount ? std::min(nColorIndex, nColorCount-1) : nColorIndex,
                    dBrightness );
                rObj.SetMergedItem( XFillColorItem( u""_ustr, aFillColor ) );
            }
            break;
        }
        case drawing::FillStyle_GRADIENT:
        {
            basegfx::BGradient aBGradient(rObj.GetMergedItem(XATTR_FILLGRADIENT).GetGradientValue());

            if ( nColorCount || 0.0 != dBrightness )
            {
                basegfx::BColorStops aColorStops(aBGradient.GetColorStops());
                for (auto& candidate : aColorStops)
                {
                    candidate = basegfx::BColorStop(
                        candidate.getStopOffset(),
                        GetColorData(
                            Color(candidate.getStopColor()),
                            nColorCount ? std::min(nColorIndex, nColorCount-1) : nColorIndex,
                            dBrightness ).getBColor());
                }
                aBGradient.SetColorStops(aColorStops);
            }

            rObj.SetMergedItem( XFillGradientItem( u""_ustr, aBGradient ) );
            break;
        }
        case drawing::FillStyle_HATCH:
        {
            XHatch aXHatch(rObj.GetMergedItem(XATTR_FILLHATCH).GetHatchValue());

            if ( nColorCount || 0.0 != dBrightness )
            {
                aXHatch.SetColor(
                    GetColorData(
                        aXHatch.GetColor(),
                        nColorCount ? std::min(nColorIndex, nColorCount-1) : nColorIndex,
                        dBrightness ));
            }

            rObj.SetMergedItem( XFillHatchItem( u""_ustr, aXHatch ) );
            break;
        }
        case drawing::FillStyle_BITMAP:
        {
            if ( nColorCount || 0.0 != dBrightness )
            {
                BitmapEx aBitmap(rObj.GetMergedItem(XATTR_FILLBITMAP).GetGraphicObject().GetGraphic().GetBitmapEx());

                short nLuminancePercent = static_cast< short > ( GetLuminanceChange(
                        nColorCount ? std::min(nColorIndex, nColorCount-1) : nColorIndex));
                aBitmap.Adjust( nLuminancePercent, 0, 0, 0, 0 );

                rObj.SetMergedItem(XFillBitmapItem(OUString(), Graphic(aBitmap)));
            }

            break;
        }
    }

    if ( nColorIndex < nColorCount )
        nColorIndex++;
}

rtl::Reference<SdrObject> EnhancedCustomShape2d::CreatePathObj( bool bLineGeometryNeededOnly )
{
    if ( !m_seqCoordinates.hasElements() )
    {
        return nullptr;
    }

    std::vector< std::pair< rtl::Reference<SdrPathObj>, double > > vObjectList;
    const bool bSortFilledObjectsToBack(SortFilledObjectsToBackByDefault(m_eSpType));
    sal_Int32 nSubPathIndex(0);
    sal_Int32 nSrcPt(0);
    sal_Int32 nSegmentInd(0);
    rtl::Reference<SdrObject> pRet;

    while( nSegmentInd <= m_seqSegments.getLength() )
    {
        CreateSubPath(
            nSrcPt,
            nSegmentInd,
            vObjectList,
            bLineGeometryNeededOnly,
            bSortFilledObjectsToBack,
            nSubPathIndex);
        nSubPathIndex++;
    }

    if ( !vObjectList.empty() )
    {
        const SfxItemSet& rCustomShapeSet(mrSdrObjCustomShape.GetMergedItemSet());
        const sal_uInt32 nColorCount(m_nColorData >> 28);
        sal_uInt32 nColorIndex(0);

        // #i37011# remove invisible objects
        std::vector< std::pair< rtl::Reference<SdrPathObj>, double> > vNewList;

        for ( std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
        {
            SdrPathObj* pObj(rCandidate.first.get());
            const drawing::LineStyle eLineStyle(pObj->GetMergedItem(XATTR_LINESTYLE).GetValue());
            const drawing::FillStyle eFillStyle(pObj->GetMergedItem(XATTR_FILLSTYLE).GetValue());
            const auto pText = pObj->getActiveText();

            // #i40600# if bLineGeometryNeededOnly is set, linestyle does not matter
            if(pText || bLineGeometryNeededOnly || (drawing::LineStyle_NONE != eLineStyle) || (drawing::FillStyle_NONE != eFillStyle))
                vNewList.push_back(std::move(rCandidate));
        }

        vObjectList = std::move(vNewList);

        if(1 == vObjectList.size())
        {
            // a single object, correct some values
            AdaptObjColor(
                *vObjectList.begin()->first,
                vObjectList.begin()->second,
                rCustomShapeSet,
                nColorIndex,
                nColorCount);
        }
        else
        {
            sal_Int32 nLineObjectCount(0);

            // correct some values and collect content data
            for ( const std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
            {
                SdrPathObj* pObj(rCandidate.first.get());

                if(pObj->IsLine())
                {
                    nLineObjectCount++;
                }
                else
                {
                    AdaptObjColor(
                        *pObj,
                        rCandidate.second,
                        rCustomShapeSet,
                        nColorIndex,
                        nColorCount);

                    // OperationSmiley: when we have access to the SdrObjCustomShape and the
                    // CustomShape is built with more than a single filled Geometry, use it
                    // to define that all helper geometries defined here (SdrObjects currently)
                    // will use the same FillGeometryDefinition (from the referenced SdrObjCustomShape).
                    // This will all same-filled objects look like filled smoothly with the same style.
                    pObj->setFillGeometryDefiningShape(&mrSdrObjCustomShape);
                }
            }

            // #i88870# correct line arrows for callouts
            if ( nLineObjectCount )
            {
                CorrectCalloutArrows(
                    m_eSpType,
                    nLineObjectCount,
                    vObjectList);
            }

            // sort objects so that filled ones are in front. Necessary
            // for some strange objects
            if(bSortFilledObjectsToBack)
            {
                std::vector< std::pair< rtl::Reference<SdrPathObj>, double> > vTempList;
                vTempList.reserve(vObjectList.size());

                for ( std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
                {
                    SdrPathObj* pObj(rCandidate.first.get());
                    if ( !pObj->IsLine() )
                        vTempList.push_back(std::move(rCandidate));
                }

                for ( std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
                {
                    if ( rCandidate.first )
                        vTempList.push_back(std::move(rCandidate));
                }

                vObjectList = std::move(vTempList);
            }
        }
    }

    // #i37011#
    if(!vObjectList.empty())
    {
        // copy remaining objects to pRet
        if(vObjectList.size() > 1)
        {
            pRet = new SdrObjGroup(mrSdrObjCustomShape.getSdrModelFromSdrObject());

            for ( std::pair< rtl::Reference<SdrPathObj>, double >& rCandidate : vObjectList )
            {
                pRet->GetSubList()->NbcInsertObject(rCandidate.first.get());
            }
        }
        else if(1 == vObjectList.size())
        {
            pRet = vObjectList.begin()->first;
        }

        if(pRet)
        {
            // move to target position
            tools::Rectangle aCurRect(pRet->GetSnapRect());
            aCurRect.Move(m_aLogicRect.Left(), m_aLogicRect.Top());
            pRet->NbcSetSnapRect(aCurRect);
        }
    }

    return pRet;
}

rtl::Reference<SdrObject> EnhancedCustomShape2d::CreateObject( bool bLineGeometryNeededOnly )
{
    rtl::Reference<SdrObject> pRet;

    if ( m_eSpType == mso_sptRectangle )
    {
        pRet = new SdrRectObj(mrSdrObjCustomShape.getSdrModelFromSdrObject(), m_aLogicRect);
        pRet->SetMergedItemSet( *this );
    }
    if ( !pRet )
        pRet = CreatePathObj( bLineGeometryNeededOnly );

    return pRet;
}

/** Set the stylesheet immediately after creation, to avoid unnecessary stylesheet adding and removing. */
rtl::Reference<SdrObject> EnhancedCustomShape2d::CreateObject( bool bLineGeometryNeededOnly, SfxStyleSheet* pNewStyleSheet )
{
    rtl::Reference<SdrObject> pRet;

    if ( m_eSpType == mso_sptRectangle )
    {
        pRet = new SdrRectObj(mrSdrObjCustomShape.getSdrModelFromSdrObject(), m_aLogicRect);
        pRet->NbcSetStyleSheet(pNewStyleSheet, true);
        pRet->SetMergedItemSet( *this );
    }
    else
    {
        pRet = CreatePathObj( bLineGeometryNeededOnly );
        if (pRet)
            pRet->NbcSetStyleSheet(pNewStyleSheet, true);
    }

    return pRet;
}

static SdrEscapeDirection lcl_GetEscapeDirection(sal_Int32 nDirection)
{
    switch (nDirection)
    {
        case 1:  return SdrEscapeDirection::LEFT;
        case 2:  return SdrEscapeDirection::RIGHT;
        case 3:  return SdrEscapeDirection::TOP;
        case 4:  return SdrEscapeDirection::BOTTOM;
        default: return SdrEscapeDirection::SMART;
    }
}

void EnhancedCustomShape2d::ApplyGluePoints(SdrObject* pObj)
{
    if ( !pObj )
        return;

    SdrEscapeDirection aDirection = SdrEscapeDirection::SMART;
    for (size_t i = 0; i < m_seqGluePoints.size(); i++)
    {
        EnhancedCustomShapeParameterPair aGluePointPair = m_seqGluePoints[i];
        if (m_seqGluePointLeavingDirections.hasElements())
        {
            sal_Int32 aGluePointLeavingDirection = m_seqGluePointLeavingDirections[i];
            aDirection = lcl_GetEscapeDirection(aGluePointLeavingDirection);
        }

        SdrGluePoint aGluePoint;
        aGluePoint.SetPos( GetPoint( aGluePointPair, !m_bOOXMLShape, true ) );
        aGluePoint.SetPercent( false );
        aGluePoint.SetAlign( SdrAlign::VERT_TOP | SdrAlign::HORZ_LEFT );
        aGluePoint.SetEscDir( aDirection );
        SdrGluePointList* pList = pObj->ForceGluePointList();
        if( pList )
            /* sal_uInt16 nId = */ pList->Insert( aGluePoint );
    }
}

rtl::Reference<SdrObject> EnhancedCustomShape2d::CreateLineGeometry()
{
    return CreateObject( true );
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

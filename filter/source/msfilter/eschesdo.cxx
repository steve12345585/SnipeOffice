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

#include <memory>
#include "eschesdo.hxx"
#include <o3tl/any.hxx>
#include <svx/svdobj.hxx>
#include <tools/poly.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <svx/unopage.hxx>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/TextContentAnchorType.hpp>
#include <com/sun/star/drawing/CircleKind.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <comphelper/extract.hxx>
#include <com/sun/star/drawing/HomogenMatrix3.hpp>
#include <basegfx/matrix/b2dhommatrix.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::text;

constexpr o3tl::Length geUnitsSrc(o3tl::Length::mm100);
// PowerPoint: 576 dpi, WinWord: 1440 dpi, Excel: 1440 dpi
constexpr o3tl::Length geUnitsDest(o3tl::Length::twip);

ImplEESdrWriter::ImplEESdrWriter( EscherEx& rEx )
    : mpEscherEx(&rEx)
    , mpPicStrm(nullptr)
    , mpHostAppData(nullptr)
    , mpSdrPage( nullptr )
{
}



Point ImplEESdrWriter::ImplMapPoint( const Point& rPoint )
{
    return o3tl::convert( rPoint, geUnitsSrc, geUnitsDest );
}

Size ImplEESdrWriter::ImplMapSize( const Size& rSize )
{
    Size aRetSize( o3tl::convert( rSize, geUnitsSrc, geUnitsDest ) );

    if ( !aRetSize.Width() )
        aRetSize.AdjustWidth( 1 );
    if ( !aRetSize.Height() )
        aRetSize.AdjustHeight( 1 );
    return aRetSize;
}

void ImplEESdrWriter::ImplFlipBoundingBox( ImplEESdrObject& rObj, EscherPropertyContainer& rPropOpt )
{
    sal_Int32 nAngle = rObj.GetAngle();
    tools::Rectangle aRect( rObj.GetRect() );

    // for position calculations, we normalize the angle between 0 and 90 degrees
    if ( nAngle < 0 )
        nAngle = ( 36000 + nAngle ) % 36000;
    if ( nAngle % 18000 == 0 )
        nAngle = 0;
    while ( nAngle > 9000 )
        nAngle = ( 18000 - ( nAngle % 18000 ) );

    double fVal = basegfx::deg2rad<100>(nAngle);
    double  fCos = cos( fVal );
    double  fSin = sin( fVal );

    double  nWidthHalf = static_cast<double>(aRect.GetWidth()) / 2;
    double  nHeightHalf = static_cast<double>(aRect.GetHeight()) / 2;

    // fdo#70838:
    // when you rotate an object, the top-left corner of its bounding box is moved
    // nXDiff and nYDiff pixels. To get their values we use these equations:
    //
    //   fSin * nHeightHalf + fCos * nWidthHalf  == nXDiff + nWidthHalf
    //   fSin * nWidthHalf  + fCos * nHeightHalf == nYDiff + nHeightHalf

    double nXDiff = fSin * nHeightHalf + fCos * nWidthHalf  - nWidthHalf;
    double nYDiff = fSin * nWidthHalf  + fCos * nHeightHalf - nHeightHalf;

    aRect.Move( static_cast<sal_Int32>(nXDiff), static_cast<sal_Int32>(nYDiff) );

    // calculate the proper angle value to be saved
    nAngle = rObj.GetAngle();
    if ( nAngle < 0 )
        nAngle = ( 36000 + nAngle ) % 36000;
    else
        nAngle = ( 36000 - ( nAngle % 36000 ) );

    nAngle *= 655;
    nAngle += 0x8000;
    nAngle &=~0xffff;                                   // nAngle round to full degrees
    rPropOpt.AddOpt( ESCHER_Prop_Rotation, nAngle );

    rObj.SetAngle( nAngle );
    rObj.SetRect( aRect );
}


sal_uInt32 ImplEESdrWriter::ImplWriteShape( ImplEESdrObject& rObj,
                                EscherSolverContainer& rSolverContainer,
                                const bool bOOxmlExport )
{
    sal_uInt32 nShapeID = 0;
    sal_uInt16 nShapeType = 0;
    bool bDontWriteText = false;        // if a metafile is written as shape replacement, then the text is already part of the metafile
    bool bAdditionalText = false;
    sal_uInt32 nGrpShapeID = 0;
    auto addShape = [this, &rObj, &rSolverContainer, &nShapeID, &nShapeType](sal_uInt16 nType, ShapeFlag nFlags)
    {
        nShapeType = nType;
        nShapeID = mpEscherEx->GenerateShapeId();
        rObj.SetShapeId( nShapeID );
        mpEscherEx->AddShape( nType, nFlags, nShapeID );
        rSolverContainer.AddShape( rObj.GetShapeRef(), nShapeID );
    };

    do {
        mpHostAppData = mpEscherEx->StartShape( rObj.GetShapeRef(), (mpEscherEx->GetGroupLevel() > 1) ? &rObj.GetRect() : nullptr );
        if ( mpHostAppData && mpHostAppData->DontWriteShape() )
            break;

        // #i51348# get shape name
        OUString aShapeName;
        if( const SdrObject* pSdrObj = rObj.GetSdrObject() )
            if (!pSdrObj->GetName().isEmpty())
                aShapeName = pSdrObj->GetName();
        uno::Reference< drawing::XShape> xShape = rObj.GetShapeRef();
        if (xShape.is())
        {
            uno::Reference<beans::XPropertySet> xPropertySet(xShape, uno::UNO_QUERY);
            if (xPropertySet.is())
            {
                uno::Sequence<beans::PropertyValue> aGrabBag;
                uno::Reference< XPropertySetInfo > xPropInfo = xPropertySet->getPropertySetInfo();
                if ( xPropInfo.is() && xPropInfo->hasPropertyByName( u"InteropGrabBag"_ustr ) )
                {
                    xPropertySet->getPropertyValue( u"InteropGrabBag"_ustr ) >>= aGrabBag;
                    for (const beans::PropertyValue& rProp : aGrabBag)
                    {
                        if (rProp.Name == "mso-edit-as")
                        {
                            OUString rEditAs;
                            rProp.Value >>= rEditAs;
                            mpEscherEx->SetEditAs(rEditAs);
                            break;
                        }
                    }
                }
            }
        }

        if( rObj.GetType() == "drawing.Group" )
        {
            Reference< XIndexAccess > xXIndexAccess( rObj.GetShapeRef(), UNO_QUERY );

            if( xXIndexAccess.is() && 0 != xXIndexAccess->getCount() )
            {
                nShapeID = mpEscherEx->EnterGroup( aShapeName, &rObj.GetRect() );
                nShapeType = ESCHER_ShpInst_Min;

                for( sal_uInt32 n = 0, nCnt = xXIndexAccess->getCount();
                        n < nCnt; ++n )
                {
                    ImplEESdrObject aObj( *o3tl::doAccess<Reference<XShape>>(
                                    xXIndexAccess->getByIndex( n )) );
                    if( aObj.IsValid() )
                    {
                        aObj.SetOOXML(bOOxmlExport);
                        ImplWriteShape( aObj, rSolverContainer, bOOxmlExport );
                    }
                }
                mpEscherEx->LeaveGroup();
            }
            break;
        }
        rObj.SetAngle( rObj.ImplGetInt32PropertyValue( u"RotateAngle"_ustr ));

        if( ( rObj.ImplGetPropertyValue( u"IsFontwork"_ustr ) &&
            ::cppu::any2bool( rObj.GetUsrAny() ) ) ||
            rObj.GetType() == "drawing.Measure" )
        {
            rObj.SetType(u"drawing.dontknow"_ustr);
        }

        const css::awt::Size   aSize100thmm( rObj.GetShapeRef()->getSize() );
        const css::awt::Point  aPoint100thmm( rObj.GetShapeRef()->getPosition() );
        tools::Rectangle   aRect100thmm( Point( aPoint100thmm.X, aPoint100thmm.Y ), Size( aSize100thmm.Width, aSize100thmm.Height ) );
        if ( !mpPicStrm )
            mpPicStrm = mpEscherEx->QueryPictureStream();
        EscherPropertyContainer aPropOpt( mpEscherEx->GetGraphicProvider(), mpPicStrm, aRect100thmm );

        // #i51348# shape name
        if (!aShapeName.isEmpty())
            aPropOpt.AddOpt( ESCHER_Prop_wzName, aShapeName );
        if ( InteractionInfo* pInteraction = mpHostAppData ? mpHostAppData->GetInteractionInfo():nullptr )
        {
            const std::unique_ptr< SvMemoryStream >& pMemStrm = pInteraction->getHyperlinkRecord();
            if (pMemStrm)
            {
                aPropOpt.AddOpt(ESCHER_Prop_pihlShape, false, 0, *pMemStrm);
            }
            aPropOpt.AddOpt( ESCHER_Prop_fPrint, 0x00080008 );
        }

        if ( rObj.GetType() == "drawing.Custom" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            ShapeFlag nMirrorFlags;

            OUString sCustomShapeType;
            MSO_SPT eShapeType = EscherPropertyContainer::GetCustomShapeType( rObj.GetShapeRef(), nMirrorFlags, sCustomShapeType, rObj.GetOOXML() );
            if ( sCustomShapeType == "col-502ad400" || sCustomShapeType == "col-60da8460" )
            {
                addShape( ESCHER_ShpInst_PictureFrame, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );

                if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"MetaFile"_ustr, false ) )
                {
                    aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
                    aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x100000 );        // no fill
                    aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x90000 );        // no linestyle
                    SdrObject* pObj = SdrObject::getSdrObjectFromXShape(rObj.GetShapeRef());
                    if ( pObj )
                    {
                        tools::Rectangle aBound = pObj->GetCurrentBoundRect();
                        Point aPosition( ImplMapPoint( aBound.TopLeft() ) );
                        Size aSize( ImplMapSize( aBound.GetSize() ) );
                        rObj.SetRect( tools::Rectangle( aPosition, aSize ) );
                        rObj.SetAngle( 0 );
                        bDontWriteText = true;
                    }
                }
            }
            else
            {
                const Reference< XPropertySet > xPropSet = rObj.mXPropSet;
                drawing::FillStyle eFS = drawing::FillStyle_NONE;
                if(xPropSet.is())
                {
                    uno::Reference< XPropertySetInfo > xPropInfo = xPropSet->getPropertySetInfo();
                    if ( xPropInfo.is() && xPropInfo->hasPropertyByName(u"FillStyle"_ustr))
                        xPropSet->getPropertyValue(u"FillStyle"_ustr) >>= eFS;
                }

                if (eFS == drawing::FillStyle_BITMAP && eShapeType == mso_sptMax)
                {
                    // We can't map this custom shape to a DOC preset and it has a bitmap fill.
                    // Make sure that at least the bitmap fill is not lost.
                    addShape( ESCHER_ShpInst_PictureFrame, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                    if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"Bitmap"_ustr, false, true, true, bOOxmlExport ) )
                        aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
                }
                else
                {
                    addShape(sal::static_int_cast< sal_uInt16 >(eShapeType),
                             nMirrorFlags | ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor);
                    aPropOpt.CreateCustomShapeProperties( eShapeType, rObj.GetShapeRef() );
                    aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
                    if ( rObj.ImplGetText() )
                    {
                        if ( !aPropOpt.IsFontWork() )
                            aPropOpt.CreateTextProperties( rObj.mXPropSet, mpEscherEx->QueryTextID(
                                rObj.GetShapeRef(), rObj.GetShapeId() ), true, false );
                    }
                }
            }
        }
        else if ( rObj.GetType() == "drawing.Rectangle" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            sal_Int32 nRadius = rObj.ImplGetInt32PropertyValue(u"CornerRadius"_ustr);
            if( nRadius )
            {
                nRadius = ImplMapSize( Size( nRadius, 0 )).Width();
                addShape( ESCHER_ShpInst_RoundRectangle, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                sal_Int32 nLength = rObj.GetRect().GetWidth();
                if ( nLength > rObj.GetRect().GetHeight() )
                    nLength = rObj.GetRect().GetHeight();
                nLength >>= 1;
                if ( nRadius >= nLength || nLength == 0 )
                    nRadius = 0x2a30;                           // 0x2a30 is PPTs maximum radius
                else
                    nRadius = ( 0x2a30 * nRadius ) / nLength;
                aPropOpt.AddOpt( ESCHER_Prop_adjustValue, nRadius );
            }
            else
            {
                addShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            }
            aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
            if( rObj.ImplGetText() )
                aPropOpt.CreateTextProperties( rObj.mXPropSet,
                    mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                        rObj.GetShapeId() ), false, false );
        }
        else if ( rObj.GetType() == "drawing.Ellipse" )
        {
            CircleKind  eCircleKind = CircleKind_FULL;
            PolyStyle   ePolyKind = PolyStyle();
            if ( rObj.ImplGetPropertyValue( u"CircleKind"_ustr ) )
            {
                eCircleKind = *o3tl::doAccess<CircleKind>(rObj.GetUsrAny());
                switch ( eCircleKind )
                {
                    case CircleKind_SECTION :
                    {
                        ePolyKind = PolyStyle::Pie;
                    }
                    break;
                    case CircleKind_ARC :
                    {
                        ePolyKind = PolyStyle::Arc;
                    }
                    break;

                    case CircleKind_CUT :
                    {
                        ePolyKind = PolyStyle::Chord;
                    }
                    break;

                    default:
                        eCircleKind = CircleKind_FULL;
                }
            }
            if ( eCircleKind == CircleKind_FULL )
            {
                mpEscherEx->OpenContainer( ESCHER_SpContainer );
                addShape( ESCHER_ShpInst_Ellipse, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
            }
            else
            {
                sal_Int32 nStartAngle, nEndAngle;
                if ( !rObj.ImplGetPropertyValue( u"CircleStartAngle"_ustr ) )
                    break;
                nStartAngle = *o3tl::doAccess<sal_Int32>(rObj.GetUsrAny());
                if( !rObj.ImplGetPropertyValue( u"CircleEndAngle"_ustr ) )
                    break;
                nEndAngle = *o3tl::doAccess<sal_Int32>(rObj.GetUsrAny());

                Point aStart, aEnd, aCenter;
                aStart.setX( static_cast<sal_Int32>( cos( basegfx::deg2rad<100>(nStartAngle) ) * 100.0 ) );
                aStart.setY( - static_cast<sal_Int32>( sin( basegfx::deg2rad<100>(nStartAngle) ) * 100.0 ) );
                aEnd.setX( static_cast<sal_Int32>( cos( basegfx::deg2rad<100>(nEndAngle) ) * 100.0 ) );
                aEnd.setY( - static_cast<sal_Int32>( sin( basegfx::deg2rad<100>(nEndAngle) ) * 100.0 ) );
                const tools::Rectangle& rRect = aRect100thmm;
                aCenter.setX( rRect.Left() + ( rRect.GetWidth() / 2 ) );
                aCenter.setY( rRect.Top() + ( rRect.GetHeight() / 2 ) );
                aStart.AdjustX(aCenter.X() );
                aStart.AdjustY(aCenter.Y() );
                aEnd.AdjustX(aCenter.X() );
                aEnd.AdjustY(aCenter.Y() );
                tools::Polygon aPolygon( rRect, aStart, aEnd, ePolyKind );
                if( rObj.GetAngle() )
                {
                    aPolygon.Rotate( rRect.TopLeft(), Degree10(static_cast<sal_Int16>( rObj.GetAngle() / 10 )) );
                    rObj.SetAngle( 0 );
                }
                mpEscherEx->OpenContainer( ESCHER_SpContainer );
                addShape( ESCHER_ShpInst_NotPrimitive, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                css::awt::Rectangle aNewRect;
                switch ( ePolyKind )
                {
                    case PolyStyle::Pie :
                    case PolyStyle::Chord :
                    {
                        aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYPOLYGON, false, aNewRect, &aPolygon );
                        aPropOpt.CreateFillProperties( rObj.mXPropSet, true  );
                    }
                    break;

                    case PolyStyle::Arc :
                    {
                        aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYLINE, false, aNewRect, &aPolygon );
                        aPropOpt.CreateLineProperties( rObj.mXPropSet, false );
                    }
                    break;
                }
                rObj.SetRect( tools::Rectangle( ImplMapPoint( Point( aNewRect.X, aNewRect.Y ) ),
                                            ImplMapSize( Size( aNewRect.Width, aNewRect.Height ) ) ) );
            }
            if ( rObj.ImplGetText() )
                aPropOpt.CreateTextProperties( rObj.mXPropSet,
                    mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                        rObj.GetShapeId() ), false, false );

        }
        else if ( rObj.GetType() == "drawing.Control" )
        {
            const Reference< XPropertySet > xPropSet = rObj.mXPropSet;
            const Reference<XPropertySetInfo> xPropInfo = xPropSet.is() ? xPropSet->getPropertySetInfo() : Reference<XPropertySetInfo>();
            // This code is expected to be called only for DOCX/XLSX formats.
            if (xPropInfo.is() && bOOxmlExport)
            {
                bool bInline = false;
                if (xPropInfo->hasPropertyByName(u"AnchorType"_ustr))
                {
                    text::TextContentAnchorType eAnchorType;
                    xPropSet->getPropertyValue(u"AnchorType"_ustr) >>= eAnchorType;
                    bInline = eAnchorType == text::TextContentAnchorType_AS_CHARACTER;
                }

                mpEscherEx->OpenContainer( ESCHER_SpContainer );
                nShapeType = bInline ? ESCHER_ShpInst_PictureFrame : ESCHER_ShpInst_HostControl;
                const ShapeFlag nFlags = ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor;
                nShapeID = rObj.GetShapeId();
                if (nShapeID)
                {
                    mpEscherEx->AddShape(nShapeType, nFlags, nShapeID );
                    rSolverContainer.AddShape(rObj.GetShapeRef(), nShapeID);
                }
                else
                {
                    addShape(nShapeType, nFlags);
                }
            }
            else
                break;
        }
        else if ( rObj.GetType() == "drawing.Connector" )
        {
            sal_uInt16 nSpType;
            ShapeFlag nSpFlags;
            css::awt::Rectangle aNewRect;
            if ( ! aPropOpt.CreateConnectorProperties( rObj.GetShapeRef(),
                            rSolverContainer, aNewRect, nSpType, nSpFlags ) )
                break;
            rObj.SetRect( tools::Rectangle( ImplMapPoint( Point( aNewRect.X, aNewRect.Y ) ),
                                        ImplMapSize( Size( aNewRect.Width, aNewRect.Height ) ) ) );

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( nSpType, nSpFlags );
        }
        else if ( rObj.GetType() == "drawing.Measure" )
        {
            break;
        }
        else if ( rObj.GetType() == "drawing.Line" )
        {
            css::awt::Rectangle aNewRect;
            aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_LINE, false, aNewRect );
            //i27942: Poly/Lines/Bezier do not support text.

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            ShapeFlag nFlags = ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor;
            if( aNewRect.Height < 0 )
                nFlags |= ShapeFlag::FlipV;
            if( aNewRect.Width < 0 )
                nFlags |= ShapeFlag::FlipH;

            addShape( ESCHER_ShpInst_Line, nFlags );
            aPropOpt.AddOpt( ESCHER_Prop_shapePath, ESCHER_ShapeComplex );
            aPropOpt.CreateLineProperties( rObj.mXPropSet, false );
            rObj.SetAngle( 0 );
        }
        else if ( rObj.GetType() == "drawing.PolyPolygon" )
        {
            if( rObj.ImplHasText() )
            {
                nGrpShapeID = ImplEnterAdditionalTextGroup( rObj.GetShapeRef(), &rObj.GetRect() );
                bAdditionalText = true;
            }
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_NotPrimitive, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            css::awt::Rectangle aNewRect;
            aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYPOLYGON, false, aNewRect );
            aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
            rObj.SetAngle( 0 );
        }
        else if ( rObj.GetType() == "drawing.PolyLine" )
        {
            //i27942: Poly/Lines/Bezier do not support text.

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_NotPrimitive, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            css::awt::Rectangle aNewRect;
            aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYLINE, false, aNewRect );
            aPropOpt.CreateLineProperties( rObj.mXPropSet, false );
            rObj.SetAngle( 0 );
        }
        else if ( rObj.GetType() == "drawing.OpenBezier" )
        {
            //i27942: Poly/Lines/Bezier do not support text.

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_NotPrimitive, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            css::awt::Rectangle aNewRect;
            aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYLINE, true, aNewRect );
            aPropOpt.CreateLineProperties( rObj.mXPropSet, false );
            rObj.SetAngle( 0 );
        }
        else if ( rObj.GetType() == "drawing.ClosedBezier" )
        {
            if ( rObj.ImplHasText() )
            {
                nGrpShapeID = ImplEnterAdditionalTextGroup( rObj.GetShapeRef(), &rObj.GetRect() );
                bAdditionalText = true;
            }
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_NotPrimitive, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            css::awt::Rectangle aNewRect;
            aPropOpt.CreatePolygonProperties( rObj.mXPropSet, ESCHER_CREATEPOLYGON_POLYPOLYGON, true, aNewRect );
            aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
            rObj.SetAngle( 0 );
        }
        else if ( rObj.GetType() == "drawing.GraphicObject" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );

            // a GraphicObject can also be a ClickMe element
            if( rObj.IsEmptyPresObj() )
            {
                addShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveMaster | ShapeFlag::HaveAnchor );
                sal_uInt32 nTxtBxId = mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                                                        rObj.GetShapeId() );
                aPropOpt.AddOpt( ESCHER_Prop_lTxid, nTxtBxId );
                aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x10001 );
                aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x10001 );
                aPropOpt.AddOpt( ESCHER_Prop_hspMaster, 0 );
            }
            else
            {
                if( rObj.ImplGetText() )
                {
                    /* SJ #i34951#: because M. documents are not allowing GraphicObjects containing text, we
                       have to create a simple Rectangle with fill bitmap instead (while not allowing BitmapMode_Repeat).
                    */
                    addShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                    if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"Graphic"_ustr, true, true, false ) )
                    {
                        aPropOpt.AddOpt( ESCHER_Prop_WrapText, ESCHER_WrapNone );
                        aPropOpt.AddOpt( ESCHER_Prop_AnchorText, ESCHER_AnchorMiddle );
                        aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x140014 );
                        aPropOpt.AddOpt( ESCHER_Prop_fillBackColor, 0x8000000 );
                        aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x80000 );
                        if ( rObj.ImplGetText() )
                            aPropOpt.CreateTextProperties( rObj.mXPropSet,
                                mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                                    rObj.GetShapeId() ), false, false );
                    }
                }
                else
                {
                    addShape( ESCHER_ShpInst_PictureFrame, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
                    if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"Graphic"_ustr, false, true, true, bOOxmlExport ) )
                        aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
                }
            }
        }
        else if ( rObj.GetType() == "drawing.Text" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_TextBox, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            aPropOpt.CreateFillProperties( rObj.mXPropSet, true );
            if( rObj.ImplGetText() )
                aPropOpt.CreateTextProperties( rObj.mXPropSet,
                    mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                        rObj.GetShapeId() ) );
        }
        else if ( rObj.GetType() == "drawing.Page" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x40004 );
            aPropOpt.AddOpt( ESCHER_Prop_fFillOK, 0x100001 );
            aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x110011 );
            aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x90008 );
            aPropOpt.AddOpt( ESCHER_Prop_fshadowObscured, 0x10001 );
        }
        else if ( rObj.GetType() == "drawing.Frame" )
        {
            break;
        }
        else if ( rObj.GetType() == "drawing.OLE2" )
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            if( rObj.IsEmptyPresObj() )
            {
                addShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveMaster | ShapeFlag::HaveAnchor );
                sal_uInt32 nTxtBxId = mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                                                        rObj.GetShapeId() );
                aPropOpt.AddOpt( ESCHER_Prop_lTxid, nTxtBxId );
                aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x10001 );
                aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x10001 );
                aPropOpt.AddOpt( ESCHER_Prop_hspMaster, 0 );
            }
            else
            {
                //2do: could be made an option in HostAppData whether OLE object should be written or not
                const bool bAppOLE = true;
                addShape( ESCHER_ShpInst_PictureFrame,
                    ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor | (bAppOLE ? ShapeFlag::OLEShape : ShapeFlag::NONE) );
                if ( aPropOpt.CreateOLEGraphicProperties( rObj.GetShapeRef() ) )
                {
                    if ( bAppOLE )
                    {   // snooped from Xcl hex dump, nobody knows the trouble I have seen
                        aPropOpt.AddOpt( ESCHER_Prop_FitTextToShape,    0x00080008 );
                        aPropOpt.AddOpt( ESCHER_Prop_pictureId,     0x00000001 );
                        aPropOpt.AddOpt( ESCHER_Prop_fillColor,     0x08000041 );
                        aPropOpt.AddOpt( ESCHER_Prop_fillBackColor, 0x08000041 );
                        aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest,    0x00110010 );
                        aPropOpt.AddOpt( ESCHER_Prop_lineColor,     0x08000040 );
                        aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash,0x00080008 );
                        aPropOpt.AddOpt( ESCHER_Prop_fPrint,            0x00080000 );
                    }
                    aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
                }
            }
        }
        else if( '3' == rObj.GetType()[8] &&
                 'D' == rObj.GetType()[9] )   // drawing.3D
        {
            // SceneObject, CubeObject, SphereObject, LatheObject, ExtrudeObject, PolygonObject
            if ( !rObj.ImplGetPropertyValue( u"Bitmap"_ustr ) )
                break;

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_PictureFrame, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );

            if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"Bitmap"_ustr, false ) )
                aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
        }
        else if ( rObj.GetType() == "drawing.Caption" )
        {
            rObj.SetAngle( 0 );
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_TextBox, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"MetaFile"_ustr, false ) )
                aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
        }
        else if ( rObj.GetType() == "drawing.dontknow" )
        {
            rObj.SetAngle( 0 );
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            addShape( ESCHER_ShpInst_PictureFrame, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            if ( aPropOpt.CreateGraphicProperties( rObj.mXPropSet, u"MetaFile"_ustr, false ) )
                aPropOpt.AddOpt( ESCHER_Prop_LockAgainstGrouping, 0x800080 );
        }
        else
        {
            break;
        }
        aPropOpt.CreateShadowProperties( rObj.mXPropSet );

        if( SDRLAYER_NOTFOUND != mpEscherEx->GetHellLayerId() &&
            rObj.ImplGetPropertyValue( u"LayerID"_ustr ) &&
            *o3tl::doAccess<sal_Int16>(rObj.GetUsrAny()) == mpEscherEx->GetHellLayerId().get() )
        {
            aPropOpt.AddOpt( ESCHER_Prop_fPrint, 0x200020 );
        }

        {
            tools::Rectangle aRect( rObj.GetRect() );
            aRect.Normalize();
            rObj.SetRect( aRect );
        }

        if( rObj.GetAngle() )
            ImplFlipBoundingBox( rObj, aPropOpt );

        aPropOpt.CreateShapeProperties( rObj.GetShapeRef() );
        const SdrObject* sdrObj = rObj.GetSdrObject();
        mpEscherEx->AddSdrObjectVMLObject(*sdrObj );
        mpEscherEx->Commit( aPropOpt, rObj.GetRect());
        if( mpEscherEx->GetGroupLevel() > 1 )
            mpEscherEx->AddChildAnchor( rObj.GetRect() );

        if ( mpHostAppData )
        {   //! with AdditionalText the App has to control whether these are written or not
            mpHostAppData->WriteClientAnchor( *mpEscherEx, rObj.GetRect() );
            mpHostAppData->WriteClientData( *mpEscherEx );
            if ( !bDontWriteText )
                mpHostAppData->WriteClientTextbox( *mpEscherEx );
        }
        mpEscherEx->CloseContainer();       // ESCHER_SpContainer

        if( bAdditionalText )
        {
            mpEscherEx->EndShape( nShapeType, nShapeID );
            ImplWriteAdditionalText( rObj );
        }

    } while ( false );

    if ( bAdditionalText )
        mpEscherEx->EndShape( ESCHER_ShpInst_Min, nGrpShapeID );
    else
        mpEscherEx->EndShape( nShapeType, nShapeID );
    return nShapeID;
}

void ImplEESdrWriter::ImplWriteAdditionalText( ImplEESdrObject& rObj )
{
    sal_uInt32 nShapeID = 0;
    sal_uInt16 nShapeType = 0;
    do
    {
        mpHostAppData = mpEscherEx->StartShape( rObj.GetShapeRef(), (mpEscherEx->GetGroupLevel() > 1) ? &rObj.GetRect() : nullptr );
        if ( mpHostAppData && mpHostAppData->DontWriteShape() )
            break;

        const css::awt::Size   aSize100thmm( rObj.GetShapeRef()->getSize() );
        const css::awt::Point  aPoint100thmm( rObj.GetShapeRef()->getPosition() );
        tools::Rectangle   aRect100thmm( Point( aPoint100thmm.X, aPoint100thmm.Y ), Size( aSize100thmm.Width, aSize100thmm.Height ) );
        if ( !mpPicStrm )
            mpPicStrm = mpEscherEx->QueryPictureStream();
        EscherPropertyContainer aPropOpt( mpEscherEx->GetGraphicProvider(), mpPicStrm, aRect100thmm );
        rObj.SetAngle( rObj.ImplGetInt32PropertyValue( u"RotateAngle"_ustr ));
        sal_Int32 nAngle = rObj.GetAngle();
        if( rObj.GetType() == "drawing.Line" )
        {
//2do: this does not work right
            double fDist = hypot( rObj.GetRect().GetWidth(),
                                    rObj.GetRect().GetHeight() );
            rObj.SetRect( tools::Rectangle( Point(),
                            Point( static_cast<sal_Int32>( fDist ), -1 ) ) );

            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            mpEscherEx->AddShape( ESCHER_ShpInst_TextBox, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor );
            if ( rObj.ImplGetText() )
                aPropOpt.CreateTextProperties( rObj.mXPropSet,
                    mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                        rObj.GetShapeId() ) );

            aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x90000 );
            aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x100000 );
            aPropOpt.AddOpt( ESCHER_Prop_FitTextToShape, 0x60006 );     // Size Shape To Fit Text
            if ( nAngle < 0 )
                nAngle = ( 36000 + nAngle ) % 36000;
            if ( nAngle )
                ImplFlipBoundingBox( rObj, aPropOpt );
        }
        else
        {
            mpEscherEx->OpenContainer( ESCHER_SpContainer );
            nShapeID = mpEscherEx->GenerateShapeId();
            nShapeType = ESCHER_ShpInst_TextBox;
            mpEscherEx->AddShape( nShapeType, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor, nShapeID );
            if ( rObj.ImplGetText() )
                aPropOpt.CreateTextProperties( rObj.mXPropSet,
                    mpEscherEx->QueryTextID( rObj.GetShapeRef(),
                        rObj.GetShapeId() ) );
            aPropOpt.AddOpt( ESCHER_Prop_fNoLineDrawDash, 0x90000 );
            aPropOpt.AddOpt( ESCHER_Prop_fNoFillHitTest, 0x100000 );

            if( nAngle < 0 )
                nAngle = ( 36000 + nAngle ) % 36000;
            else
                nAngle = ( 36000 - ( nAngle % 36000 ) );

            nAngle *= 655;
            nAngle += 0x8000;
            nAngle &=~0xffff;   // nAngle round to full degrees
            aPropOpt.AddOpt( ESCHER_Prop_Rotation, nAngle );
            mpEscherEx->SetGroupSnapRect( mpEscherEx->GetGroupLevel(),
                                            rObj.GetRect() );
            mpEscherEx->SetGroupLogicRect( mpEscherEx->GetGroupLevel(),
                                            rObj.GetRect() );
        }
        rObj.SetAngle( nAngle );
        aPropOpt.CreateShapeProperties( rObj.GetShapeRef() );
        const SdrObject* sdrObj = rObj.GetSdrObject();
        mpEscherEx->AddSdrObjectVMLObject(*sdrObj );
        mpEscherEx->Commit( aPropOpt, rObj.GetRect());

        // write the childanchor
        mpEscherEx->AddChildAnchor( rObj.GetRect() );

#if defined EES_WRITE_EPP
        // ClientAnchor
        mpEscherEx->AddClientAnchor( maRect );
        // ClientTextbox
        mpEscherEx->OpenContainer( ESCHER_ClientTextbox );
        mpEscherEx->AddAtom( 4, EPP_TextHeaderAtom );
        *mpStrm << (sal_uInt32)EPP_TEXTTYPE_Other;                              // Text in a Shape
        ImplWriteTextStyleAtom();
        mpEscherEx->CloseContainer();   // ESCHER_ClientTextBox
#else // !EES_WRITE_EPP
        if ( mpHostAppData )
        {   //! the App has to control whether these are written or not
            mpHostAppData->WriteClientAnchor( *mpEscherEx, rObj.GetRect() );
            mpHostAppData->WriteClientData( *mpEscherEx );
            mpHostAppData->WriteClientTextbox( *mpEscherEx );
        }
#endif // EES_WRITE_EPP
        mpEscherEx->CloseContainer();   // ESCHER_SpContainer
    } while ( false );
    mpEscherEx->LeaveGroup();
    mpEscherEx->EndShape( nShapeType, nShapeID );
}


sal_uInt32 ImplEESdrWriter::ImplEnterAdditionalTextGroup( const Reference< XShape >& rShape,
            const tools::Rectangle* pBoundRect )
{
    mpHostAppData = mpEscherEx->EnterAdditionalTextGroup();
    sal_uInt32 nGrpId = mpEscherEx->EnterGroup( pBoundRect );
    mpHostAppData = mpEscherEx->StartShape( rShape, pBoundRect );
    return nGrpId;
}


void ImplEESdrWriter::ImplWritePage(
            EscherSolverContainer& rSolverContainer, bool ooxmlExport )
{
    const sal_uInt32 nShapes = mXShapes->getCount();
    for( sal_uInt32 n = 0; n < nShapes; ++n )
    {
        ImplEESdrObject aObj( *o3tl::doAccess<Reference<XShape>>(
                                    mXShapes->getByIndex( n )) );
        if( aObj.IsValid() )
        {
            ImplWriteShape( aObj, rSolverContainer, ooxmlExport );
        }
    }
}

ImplEESdrWriter::~ImplEESdrWriter()
{
    DBG_ASSERT( !mpSolverContainer, "ImplEESdrWriter::~ImplEESdrWriter: unwritten SolverContainer" );
    if (mXDrawPage.is())
        mXDrawPage->dispose();
}


bool ImplEESdrWriter::ImplInitPage( const SdrPage& rPage )
{
    rtl::Reference<SvxDrawPage> pSvxDrawPage;
    if ( mpSdrPage != &rPage || !mXDrawPage.is() )
    {
        // eventually write SolverContainer of current page, deletes the Solver
        ImplFlushSolverContainer();

        mpSdrPage = nullptr;
        if (mXDrawPage.is())
            mXDrawPage->dispose();
        pSvxDrawPage = new SvxDrawPage( const_cast<SdrPage*>(&rPage) );
        mXDrawPage = pSvxDrawPage;
        mXShapes = mXDrawPage;
        if ( !mXShapes.is() )
            return false;
        mpSdrPage = &rPage;

        mpSolverContainer.reset( new EscherSolverContainer );
    }
    else
        pSvxDrawPage = mXDrawPage;

    return pSvxDrawPage != nullptr;
}

bool ImplEESdrWriter::ImplInitUnoShapes( const Reference< XShapes >& rxShapes )
{
    // eventually write SolverContainer of current page, deletes the Solver
    ImplFlushSolverContainer();

    if( !rxShapes.is() )
        return false;

    mpSdrPage = nullptr;
    mXDrawPage.clear();
    mXShapes = rxShapes;

    mpSolverContainer.reset( new EscherSolverContainer );
    return true;
}

void ImplEESdrWriter::ImplExitPage()
{
    // close all groups before the solver container is written
    while( mpEscherEx->GetGroupLevel() )
        mpEscherEx->LeaveGroup();

    ImplFlushSolverContainer();
    mpSdrPage = nullptr;   // reset page for next init
}


void ImplEESdrWriter::ImplFlushSolverContainer()
{
    if ( mpSolverContainer )
    {
        mpSolverContainer->WriteSolver( mpEscherEx->GetStream() );
        mpSolverContainer.reset();
    }
}

void ImplEESdrWriter::ImplWriteCurrentPage(bool ooxmlExport)
{
    assert(mpSolverContainer && "ImplEESdrWriter::ImplWriteCurrentPage: no SolverContainer");
    ImplWritePage( *mpSolverContainer, ooxmlExport );
    ImplExitPage();
}

sal_uInt32 ImplEESdrWriter::ImplWriteTheShape( ImplEESdrObject& rObj , bool ooxmlExport )
{
    assert(mpSolverContainer && "ImplEESdrWriter::ImplWriteShape: no SolverContainer");
    return ImplWriteShape( rObj, *mpSolverContainer, ooxmlExport );
}

void EscherEx::AddSdrPage( const SdrPage& rPage, bool ooxmlExport )
{
    if ( mpImplEESdrWriter->ImplInitPage( rPage ) )
        mpImplEESdrWriter->ImplWriteCurrentPage(ooxmlExport);
}

void EscherEx::AddUnoShapes( const Reference< XShapes >& rxShapes, bool ooxmlExport )
{
    if ( mpImplEESdrWriter->ImplInitUnoShapes( rxShapes ) )
        mpImplEESdrWriter->ImplWriteCurrentPage(ooxmlExport);
}

sal_uInt32 EscherEx::AddSdrObject(const SdrObject& rObj, bool ooxmlExport, sal_uInt32 nId)
{
    ImplEESdrObject aObj(*mpImplEESdrWriter, rObj, mbOOXML , nId);
    if( aObj.IsValid() )
        return mpImplEESdrWriter->ImplWriteTheShape( aObj, ooxmlExport );
    return 0;
}


void EscherEx::EndSdrObjectPage()
{
    mpImplEESdrWriter->ImplExitPage();
}

EscherExHostAppData* EscherEx::StartShape( const Reference< XShape >& /* rShape */, const tools::Rectangle* /*pChildAnchor*/ )
{
    return nullptr;
}

void EscherEx::EndShape( sal_uInt16 /* nShapeType */, sal_uInt32 /* nShapeID */ )
{
}

sal_uInt32 EscherEx::QueryTextID( const Reference< XShape >&, sal_uInt32 )
{
    return 0;
}

// add a dummy rectangle shape into the escher stream
sal_uInt32 EscherEx::AddDummyShape()
{
    OpenContainer( ESCHER_SpContainer );
    sal_uInt32 nShapeID = GenerateShapeId();
    AddShape( ESCHER_ShpInst_Rectangle, ShapeFlag::HaveShapeProperty | ShapeFlag::HaveAnchor, nShapeID );
    CloseContainer();

    return nShapeID;
}

// static
const SdrObject* EscherEx::GetSdrObject( const Reference< XShape >& rShape )
{
    const SdrObject* pRet  = SdrObject::getSdrObjectFromXShape( rShape );
    DBG_ASSERT( pRet, "EscherEx::GetSdrObject: no SdrObj" );
    return pRet;
}


ImplEESdrObject::ImplEESdrObject( ImplEESdrWriter& rEx,
                                  const SdrObject& rObj, bool bOOXML, sal_uInt32 nId) :
    mnShapeId(nId),
    mnTextSize( 0 ),
    mnAngle( 0 ),
    mbValid( false ),
    mbPresObj( false ),
    mbEmptyPresObj( false ),
    mbOOXML(bOOXML)
{
    SdrPage* pPage = rObj.getSdrPageFromSdrObject();
    DBG_ASSERT( pPage, "ImplEESdrObject::ImplEESdrObject: no SdrPage" );
    if( pPage && rEx.ImplInitPage( *pPage ) )
    {
        // why not declare a const parameter if the object will
        // not be modified?
        mXShape.set( const_cast<SdrObject*>(&rObj)->getUnoShape(), UNO_QUERY );
        Init();
    }
}

ImplEESdrObject::ImplEESdrObject( const Reference< XShape >& rShape ) :
    mXShape( rShape ),
    mnShapeId( 0 ),
    mnTextSize( 0 ),
    mnAngle( 0 ),
    mbValid( false ),
    mbPresObj( false ),
    mbEmptyPresObj( false ),
    mbOOXML(false)
{
    Init();
}


ImplEESdrObject::~ImplEESdrObject()
{
}

static basegfx::B2DRange getUnrotatedGroupBoundRange(const Reference< XShape >& rxShape)
{
    basegfx::B2DRange aRetval;

    try
    {
        if(rxShape.is())
        {
            if(rxShape->getShapeType() == "com.sun.star.drawing.GroupShape")
            {
                // it's a group shape, iterate over children
                const Reference< XIndexAccess > xXIndexAccess(rxShape, UNO_QUERY);

                if(xXIndexAccess.is())
                {
                    for(sal_uInt32 n(0), nCnt = xXIndexAccess->getCount(); n < nCnt; ++n)
                    {
                        const Reference< XShape > axShape(xXIndexAccess->getByIndex(n), UNO_QUERY);

                        if(axShape.is())
                        {
                            // we are calculating the bound for a group, correct rotation for sub-objects
                            // to get the unrotated bounds for the group
                            const basegfx::B2DRange aExtend(getUnrotatedGroupBoundRange(axShape));

                            aRetval.expand(aExtend);
                        }
                    }
                }
            }
            else
            {
                // iT#s a xShape, get its transformation
                const Reference< XPropertySet > xPropSet(rxShape, UNO_QUERY);

                if(xPropSet.is())
                {
                    const Any aAny = xPropSet->getPropertyValue(u"Transformation"_ustr);

                    if(aAny.hasValue())
                    {
                        HomogenMatrix3 aMatrix;

                        if(aAny >>= aMatrix)
                        {
                            basegfx::B2DHomMatrix aHomogenMatrix;

                            aHomogenMatrix.set(0, 0, aMatrix.Line1.Column1);
                            aHomogenMatrix.set(0, 1, aMatrix.Line1.Column2);
                            aHomogenMatrix.set(0, 2, aMatrix.Line1.Column3);
                            aHomogenMatrix.set(1, 0, aMatrix.Line2.Column1);
                            aHomogenMatrix.set(1, 1, aMatrix.Line2.Column2);
                            aHomogenMatrix.set(1, 2, aMatrix.Line2.Column3);
                            // For this to be a valid 2D transform matrix, the last row must be [0,0,1]
                            assert( aMatrix.Line3.Column1 == 0 );
                            assert( aMatrix.Line3.Column2 == 0 );
                            assert( aMatrix.Line3.Column3 == 1 );

                            basegfx::B2DVector aScale, aTranslate;
                            double fRotate, fShearX;

                            // decompose transformation
                            aHomogenMatrix.decompose(aScale, aTranslate, fRotate, fShearX);

                            // check if rotation needs to be corrected
                            if(!basegfx::fTools::equalZero(fRotate))
                            {
                                // to correct, keep in mind that ppt graphics are rotated around their center
                                const basegfx::B2DPoint aCenter(aHomogenMatrix * basegfx::B2DPoint(0.5, 0.5));

                                aHomogenMatrix.translate(-aCenter.getX(), -aCenter.getY());
                                aHomogenMatrix.rotate(-fRotate);
                                aHomogenMatrix.translate(aCenter.getX(), aCenter.getY());
                            }


                            // check if shear needs to be corrected (always correct shear,
                            // ppt does not know about it)
                            if(!basegfx::fTools::equalZero(fShearX))
                            {
                                const basegfx::B2DPoint aMinimum(aHomogenMatrix * basegfx::B2DPoint(0.0, 0.0));

                                aHomogenMatrix.translate(-aMinimum.getX(), -aMinimum.getY());
                                aHomogenMatrix.shearX(-fShearX);
                                aHomogenMatrix.translate(aMinimum.getX(), aMinimum.getY());
                            }

                            // create range. It's no longer rotated (or sheared), so use
                            // minimum and maximum values
                            aRetval.expand(aHomogenMatrix * basegfx::B2DPoint(0.0, 0.0));
                            aRetval.expand(aHomogenMatrix * basegfx::B2DPoint(1.0, 1.0));
                        }
                    }
                }
            }
        }
    }
    catch(css::uno::Exception&)
    {
    }

    return aRetval;
}

void ImplEESdrObject::Init()
{
    mXPropSet.set( mXShape, UNO_QUERY );
    if( !mXPropSet.is() )
        return;

    // detect name first to make below test (is group) work
    mType = mXShape->getShapeType();
    (void)mType.startsWith( "com.sun.star.", &mType );  // strip "com.sun.star."
    (void)mType.endsWith( "Shape", &mType );  // strip "Shape"

    if(GetType() == "drawing.Group")
    {
        // if it's a group, the unrotated range is needed for that group
        const basegfx::B2DRange aUnrotatedRange(getUnrotatedGroupBoundRange(mXShape));
        if (aUnrotatedRange.isEmpty())
        {
            SetRect(tools::Rectangle());
        }
        else
        {
            const Point aNewP(basegfx::fround<tools::Long>(aUnrotatedRange.getMinX()),
                              basegfx::fround<tools::Long>(aUnrotatedRange.getMinY()));
            const Size aNewS(basegfx::fround<tools::Long>(aUnrotatedRange.getWidth()),
                             basegfx::fround<tools::Long>(aUnrotatedRange.getHeight()));

            SetRect(ImplEESdrWriter::ImplMapPoint(aNewP), ImplEESdrWriter::ImplMapSize(aNewS));
        }
    }
    else
    {
        // if it's no group, use position and size directly, rotated/sheared or not
        const Point aOldP(mXShape->getPosition().X, mXShape->getPosition().Y);
        const Size aOldS(mXShape->getSize().Width, mXShape->getSize().Height);

        SetRect(ImplEESdrWriter::ImplMapPoint(aOldP), ImplEESdrWriter::ImplMapSize(aOldS));
    }

    if( ImplGetPropertyValue( u"IsPresentationObject"_ustr ) )
        mbPresObj = ::cppu::any2bool( mAny );

    if( mbPresObj && ImplGetPropertyValue( u"IsEmptyPresentationObject"_ustr ) )
        mbEmptyPresObj = ::cppu::any2bool( mAny );

    mbValid = true;
}

bool ImplEESdrObject::ImplGetPropertyValue( const OUString& rString )
{
    bool bRetValue = false;
    if( mbValid )
    {
        try
        {
            mAny = mXPropSet->getPropertyValue( rString );
            if( mAny.hasValue() )
                bRetValue = true;
        }
        catch( const css::uno::Exception& )
        {
            bRetValue = false;
        }
    }
    return bRetValue;
}

void ImplEESdrObject::SetRect( const Point& rPos, const Size& rSz )
{
    maRect = tools::Rectangle( rPos, rSz );
}

const SdrObject* ImplEESdrObject::GetSdrObject() const
{
    return EscherEx::GetSdrObject( mXShape );
}

// loads and converts text from shape, result is saved in mnTextSize
sal_uInt32 ImplEESdrObject::ImplGetText()
{
    Reference< XText > xXText( mXShape, UNO_QUERY );
    mnTextSize = 0;
    if (xXText.is())
    {
        try
        {
            mnTextSize = xXText->getString().getLength();
        }
        catch (const uno::RuntimeException&)
        {
            TOOLS_WARN_EXCEPTION("filter.ms", "ImplGetText");
        }
    }
    return mnTextSize;
}

bool ImplEESdrObject::ImplHasText() const
{
    Reference< XText > xXText( mXShape, UNO_QUERY );
    return xXText.is() && !xXText->getString().isEmpty();
}


void ImplEESdrObject::SetOOXML(bool bOOXML)
{
    mbOOXML = bOOXML;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

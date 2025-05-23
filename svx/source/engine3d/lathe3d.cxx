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


#include <svx/deflt3d.hxx>
#include <svx/strings.hrc>
#include <svx/dialmgr.hxx>
#include <svx/svdobjkind.hxx>
#include <svx/lathe3d.hxx>
#include <svx/svdopath.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svx3ditems.hxx>
#include <svx/xlineit0.hxx>
#include <sdr/properties/e3dlatheproperties.hxx>
#include <sdr/contact/viewcontactofe3dlathe.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <utility>


// DrawContact section
std::unique_ptr<sdr::contact::ViewContact> E3dLatheObj::CreateObjectSpecificViewContact()
{
    return std::make_unique<sdr::contact::ViewContactOfE3dLathe>(*this);
}

std::unique_ptr<sdr::properties::BaseProperties> E3dLatheObj::CreateObjectSpecificProperties()
{
    return std::make_unique<sdr::properties::E3dLatheProperties>(*this);
}

// Constructor from 3D polygon, scale is the conversion factor for the coordinates
E3dLatheObj::E3dLatheObj(
    SdrModel& rSdrModel,
    const E3dDefaultAttributes& rDefault,
    basegfx::B2DPolyPolygon aPoly2D)
:   E3dCompoundObject(rSdrModel),
    maPolyPoly2D(std::move(aPoly2D))
{
    // since the old class PolyPolygon3D did mirror the given PolyPolygons in Y, do the same here
    basegfx::B2DHomMatrix aMirrorY;
    aMirrorY.scale(1.0, -1.0);
    maPolyPoly2D.transform(aMirrorY);

    // Set Defaults
    SetDefaultAttributes(rDefault);

    // Superfluous items removed, in particular to prevent duplicate
    // start and end points
    maPolyPoly2D.removeDoublePoints();

    if(maPolyPoly2D.count())
    {
        const basegfx::B2DPolygon rPoly(maPolyPoly2D.getB2DPolygon(0));
        sal_uInt32 nSegCnt(rPoly.count());

        if(nSegCnt && !rPoly.isClosed())
        {
            nSegCnt -= 1;
        }

        GetProperties().SetObjectItemDirect(makeSvx3DVerticalSegmentsItem(nSegCnt));
    }
}

E3dLatheObj::E3dLatheObj(SdrModel& rSdrModel)
:    E3dCompoundObject(rSdrModel)
{
    // Set Defaults
    const E3dDefaultAttributes aDefault;

    SetDefaultAttributes(aDefault);
}

E3dLatheObj::E3dLatheObj(SdrModel& rSdrModel, E3dLatheObj const & rSource)
:    E3dCompoundObject(rSdrModel, rSource)
{
    // Set Defaults
    const E3dDefaultAttributes aDefault;

    SetDefaultAttributes(aDefault);

    maPolyPoly2D = rSource.maPolyPoly2D;
}

E3dLatheObj::~E3dLatheObj()
{
}

void E3dLatheObj::SetDefaultAttributes(const E3dDefaultAttributes& rDefault)
{
    GetProperties().SetObjectItemDirect(Svx3DSmoothNormalsItem(rDefault.GetDefaultLatheSmoothed()));
    GetProperties().SetObjectItemDirect(Svx3DSmoothLidsItem(rDefault.GetDefaultLatheSmoothFrontBack()));
    GetProperties().SetObjectItemDirect(Svx3DCharacterModeItem(rDefault.GetDefaultLatheCharacterMode()));
    GetProperties().SetObjectItemDirect(Svx3DCloseFrontItem(rDefault.GetDefaultLatheCloseFront()));
    GetProperties().SetObjectItemDirect(Svx3DCloseBackItem(rDefault.GetDefaultLatheCloseBack()));
}

SdrObjKind E3dLatheObj::GetObjIdentifier() const
{
    return SdrObjKind::E3D_Lathe;
}

rtl::Reference<SdrObject> E3dLatheObj::CloneSdrObject(SdrModel& rTargetModel) const
{
    return new E3dLatheObj(rTargetModel, *this);
}

// Convert the object to group object consisting of n polygons

rtl::Reference<SdrObject> E3dLatheObj::DoConvertToPolyObj(bool /*bBezier*/, bool /*bAddText*/) const
{
    return nullptr;
}

// Set Local parameters set to re-create geometry

void E3dLatheObj::SetPolyPoly2D(const basegfx::B2DPolyPolygon& rNew)
{
    if(maPolyPoly2D == rNew)
        return;

    maPolyPoly2D = rNew;
    maPolyPoly2D.removeDoublePoints();

    if(maPolyPoly2D.count())
    {
        const basegfx::B2DPolygon rPoly(maPolyPoly2D.getB2DPolygon(0));
        sal_uInt32 nSegCnt(rPoly.count());

        if(nSegCnt && !rPoly.isClosed())
        {
            nSegCnt -= 1;
        }

        GetProperties().SetObjectItemDirect(makeSvx3DVerticalSegmentsItem(nSegCnt));
    }

    ActionChanged();
}

// Get the name of the object (singular)

OUString E3dLatheObj::TakeObjNameSingul() const
{
    OUString sName(SvxResId(STR_ObjNameSingulLathe3d));

    OUString aName(GetName());
    if (!aName.isEmpty())
    {
        sName += " '" + aName + "'";
    }
    return sName;
}

// Get the name of the object (plural)

OUString E3dLatheObj::TakeObjNamePlural() const
{
    return SvxResId(STR_ObjNamePluralLathe3d);
}

bool E3dLatheObj::IsBreakObjPossible()
{
    return true;
}

rtl::Reference<SdrAttrObj> E3dLatheObj::GetBreakObj()
{
    // create PathObj
    basegfx::B3DPolyPolygon aLathePoly3D(basegfx::utils::createB3DPolyPolygonFromB2DPolyPolygon(maPolyPoly2D));
    basegfx::B2DPolyPolygon aTransPoly(TransformToScreenCoor(aLathePoly3D));
    rtl::Reference<SdrPathObj> pPathObj(new SdrPathObj(getSdrModelFromSdrObject(), SdrObjKind::PolyLine, std::move(aTransPoly)));

    // Set Attribute
    SfxItemSet aSet(GetObjectItemSet());

    // Enable lines to guarantee that the object becomes visible
    aSet.Put(XLineStyleItem(css::drawing::LineStyle_SOLID));

    pPathObj->SetMergedItemSet(aSet);

    return pPathObj;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

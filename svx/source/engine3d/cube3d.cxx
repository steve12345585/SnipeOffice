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


#include <svx/strings.hrc>
#include <svx/deflt3d.hxx>
#include <svx/dialmgr.hxx>
#include <svx/cube3d.hxx>
#include <svx/svdobjkind.hxx>
#include <basegfx/point/b3dpoint.hxx>
#include <sdr/contact/viewcontactofe3dcube.hxx>


// DrawContact section

std::unique_ptr<sdr::contact::ViewContact> E3dCubeObj::CreateObjectSpecificViewContact()
{
    return std::make_unique<sdr::contact::ViewContactOfE3dCube>(*this);
}


E3dCubeObj::E3dCubeObj(
    SdrModel& rSdrModel,
    const E3dDefaultAttributes& rDefault,
    const basegfx::B3DPoint& aPos,
    const basegfx::B3DVector& r3DSize)
:   E3dCompoundObject(rSdrModel)
{
    // Set Defaults
    SetDefaultAttributes(rDefault);

    // position centre or left, bottom, back (dependent on bPosIsCenter)
    m_aCubePos = aPos;
    m_aCubeSize = r3DSize;
}

E3dCubeObj::E3dCubeObj(SdrModel& rSdrModel)
:   E3dCompoundObject(rSdrModel)
{
    // Set Defaults
    const E3dDefaultAttributes aDefault;

    SetDefaultAttributes(aDefault);
}

E3dCubeObj::E3dCubeObj(SdrModel& rSdrModel, E3dCubeObj const & rSource)
:   E3dCompoundObject(rSdrModel, rSource)
{
    // Set Defaults
    const E3dDefaultAttributes aDefault;

    SetDefaultAttributes(aDefault);

    m_aCubePos = rSource.m_aCubePos;
    m_aCubeSize = rSource.m_aCubeSize;
    m_bPosIsCenter = rSource.m_bPosIsCenter;
}

E3dCubeObj::~E3dCubeObj()
{
}

void E3dCubeObj::SetDefaultAttributes(const E3dDefaultAttributes& rDefault)
{
    m_aCubePos = rDefault.GetDefaultCubePos();
    m_aCubeSize = rDefault.GetDefaultCubeSize();
    m_bPosIsCenter = rDefault.GetDefaultCubePosIsCenter();
}

SdrObjKind E3dCubeObj::GetObjIdentifier() const
{
    return SdrObjKind::E3D_Cube;
}

// Convert the object into a group object consisting of 6 polygons

rtl::Reference<SdrObject> E3dCubeObj::DoConvertToPolyObj(bool /*bBezier*/, bool /*bAddText*/) const
{
    return nullptr;
}

rtl::Reference<SdrObject> E3dCubeObj::CloneSdrObject(SdrModel& rTargetModel) const
{
    return new E3dCubeObj(rTargetModel, *this);
}

// Set local parameters with geometry re-creating

void E3dCubeObj::SetCubePos(const basegfx::B3DPoint& rNew)
{
    if(m_aCubePos != rNew)
    {
        m_aCubePos = rNew;
        ActionChanged();
    }
}

void E3dCubeObj::SetCubeSize(const basegfx::B3DVector& rNew)
{
    if(m_aCubeSize != rNew)
    {
        m_aCubeSize = rNew;
        ActionChanged();
    }
}

void E3dCubeObj::SetPosIsCenter(bool bNew)
{
    if(m_bPosIsCenter != bNew)
    {
        m_bPosIsCenter = bNew;
        ActionChanged();
    }
}

// Get the name of the object (singular)

OUString E3dCubeObj::TakeObjNameSingul() const
{
    OUString sName = SvxResId(STR_ObjNameSingulCube3d);

    OUString aName(GetName());
    if (!aName.isEmpty())
    {
        sName += " \'" + aName + "'";
    }
    return sName;
}

// Get the name of the object (plural)

OUString E3dCubeObj::TakeObjNamePlural() const
{
    return SvxResId(STR_ObjNamePluralCube3d);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

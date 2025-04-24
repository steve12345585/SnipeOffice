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

#ifndef INCLUDED_SVX_SPHERE3D_HXX
#define INCLUDED_SVX_SPHERE3D_HXX

#include <svl/intitem.hxx>
#include <svl/itemset.hxx>
#include <svx/obj3d.hxx>
#include <svx/svxdllapi.h>
#include <svx/svddef.hxx>

class E3dDefaultAttributes;

/**
 * SphereObject with diameter r3DSize.
 * The count of planes depends on the horizontal and vertical segment count.
 */
class E3dSphereObj final : public E3dCompoundObject
{
private:
    basegfx::B3DPoint               m_aCenter;
    basegfx::B3DVector              m_aSize;

    virtual std::unique_ptr<sdr::contact::ViewContact> CreateObjectSpecificViewContact() override;
    virtual std::unique_ptr<sdr::properties::BaseProperties> CreateObjectSpecificProperties() override;
    void SetDefaultAttributes(const E3dDefaultAttributes& rDefault);

private:
    // protected destructor - due to final, make private
    virtual ~E3dSphereObj() override;

public:
    SVXCORE_DLLPUBLIC E3dSphereObj(
        SdrModel& rSdrModel,
        const E3dDefaultAttributes& rDefault,
        const basegfx::B3DPoint& rCenter,
        const basegfx::B3DVector& r3DSize);

    // FG: This constructor is only called from MakeObject from the 3d-Objectfactory
    //     when a document with a sphere is loaded.  This constructor does not call
    //     CreateSphere, or create any spheres.
    E3dSphereObj(SdrModel& rSdrModel);
    E3dSphereObj(SdrModel& rSdrModel, E3dSphereObj const & rSource);

    // horizontal segments:
    sal_uInt32 GetHorizontalSegments() const
        { return GetObjectItemSet().Get(SDRATTR_3DOBJ_HORZ_SEGS).GetValue(); }

    // VerticalSegments:
    sal_uInt32 GetVerticalSegments() const
        { return GetObjectItemSet().Get(SDRATTR_3DOBJ_VERT_SEGS).GetValue(); }

    virtual SdrObjKind GetObjIdentifier() const override;
    virtual rtl::Reference<SdrObject> DoConvertToPolyObj(bool bBezier, bool bAddText) const override;

    virtual rtl::Reference<SdrObject> CloneSdrObject(SdrModel& rTargetModel) const override;

    const basegfx::B3DPoint& Center() const { return m_aCenter; }
    const basegfx::B3DVector& Size() const { return m_aSize; }

    // set local parameters when the geometry is recreated
    void SetCenter(const basegfx::B3DPoint& rNew);
    void SetSize(const basegfx::B3DVector& rNew);

    // TakeObjName...() is for displaying in the UI, eg "3 selected frames."
    virtual OUString TakeObjNameSingul() const override;
    virtual OUString TakeObjNamePlural() const override;
};

#endif // INCLUDED_SVX_SPHERE3D_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

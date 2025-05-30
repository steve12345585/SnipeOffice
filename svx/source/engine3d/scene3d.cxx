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

#include <cstdlib>

#include <svx/strings.hrc>
#include <svx/dialmgr.hxx>
#include <svx/svditer.hxx>

#include <svx/svdobjkind.hxx>
#include <svx/svdpage.hxx>
#include <svx/scene3d.hxx>
#include <svx/svdtrans.hxx>
#include <sdr/properties/e3dsceneproperties.hxx>
#include <svx/sdr/contact/viewcontactofe3dscene.hxx>
#include <svx/svddrag.hxx>
#include "helperminimaldepth3d.hxx"
#include <algorithm>
#include <drawinglayer/geometry/viewinformation3d.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <svx/e3dsceneupdater.hxx>
#include <svx/svdmodel.hxx>
#include <osl/diagnose.h>

namespace {

class ImpRemap3DDepth
{
    sal_uInt32                  mnOrdNum;
    double                      mfMinimalDepth;

    // bit field
    bool                        mbIsScene : 1;

public:
    ImpRemap3DDepth(sal_uInt32 nOrdNum, double fMinimalDepth);
    explicit ImpRemap3DDepth(sal_uInt32 nOrdNum);

    // for ::std::sort
    bool operator<(const ImpRemap3DDepth& rComp) const;

    sal_uInt32 GetOrdNum() const { return mnOrdNum; }
    bool IsScene() const { return mbIsScene; }
};

}

ImpRemap3DDepth::ImpRemap3DDepth(sal_uInt32 nOrdNum, double fMinimalDepth)
:   mnOrdNum(nOrdNum),
    mfMinimalDepth(fMinimalDepth),
    mbIsScene(false)
{
}

ImpRemap3DDepth::ImpRemap3DDepth(sal_uInt32 nOrdNum)
:   mnOrdNum(nOrdNum),
    mfMinimalDepth(0.0),
    mbIsScene(true)
{
}

bool ImpRemap3DDepth::operator<(const ImpRemap3DDepth& rComp) const
{
    if(IsScene())
    {
        return false;
    }
    else
    {
        if(rComp.IsScene())
        {
            return true;
        }
        else
        {
            return mfMinimalDepth < rComp.mfMinimalDepth;
        }
    }
}

class Imp3DDepthRemapper
{
    std::vector< ImpRemap3DDepth > maVector;

public:
    explicit Imp3DDepthRemapper(E3dScene const & rScene);

    sal_uInt32 RemapOrdNum(sal_uInt32 nOrdNum) const;
};

Imp3DDepthRemapper::Imp3DDepthRemapper(E3dScene const & rScene)
{
    // only called when rScene.GetSubList() and nObjCount > 1
    SdrObjList* pList = rScene.GetSubList();
    const size_t nObjCount(pList->GetObjCount());

    for(size_t a = 0; a < nObjCount; ++a)
    {
        SdrObject* pCandidate = pList->GetObj(a);

        if(pCandidate)
        {
            if(auto pCompoundObj = dynamic_cast< const E3dCompoundObject*>(pCandidate))
            {
                // single 3d object, calc depth
                const double fMinimalDepth(getMinimalDepthInViewCoordinates(*pCompoundObj));
                ImpRemap3DDepth aEntry(a, fMinimalDepth);
                maVector.push_back(aEntry);
            }
            else
            {
                // scene, use standard entry for scene
                ImpRemap3DDepth aEntry(a);
                maVector.push_back(aEntry);
            }
        }
    }

    // now, we need to sort the maVector by its members minimal depth. The
    // smaller, the nearer to the viewer.
    ::std::sort(maVector.begin(), maVector.end());
}

sal_uInt32 Imp3DDepthRemapper::RemapOrdNum(sal_uInt32 nOrdNum) const
{
    if(nOrdNum < maVector.size())
    {
        nOrdNum = maVector[(maVector.size() - 1) - nOrdNum].GetOrdNum();
    }

    return nOrdNum;
}


// BaseProperties section

std::unique_ptr<sdr::properties::BaseProperties> E3dScene::CreateObjectSpecificProperties()
{
    return std::make_unique<sdr::properties::E3dSceneProperties>(*this);
}


// DrawContact section

std::unique_ptr<sdr::contact::ViewContact> E3dScene::CreateObjectSpecificViewContact()
{
    return std::make_unique<sdr::contact::ViewContactOfE3dScene>(*this);
}


E3dScene::E3dScene(SdrModel& rSdrModel)
:   E3dObject(rSdrModel),
    m_aCamera(basegfx::B3DPoint(0.0, 0.0, 4.0), basegfx::B3DPoint()),
    m_bDrawOnlySelected(false),
    mbSkipSettingDirty(false)
{
    // Set defaults
    SetDefaultAttributes();
}

E3dScene::E3dScene(SdrModel& rSdrModel, E3dScene const & rSource)
:   E3dObject(rSdrModel, rSource),
    m_aCamera(basegfx::B3DPoint(0.0, 0.0, 4.0), basegfx::B3DPoint()),
    m_bDrawOnlySelected(false),
    mbSkipSettingDirty(false)
{
    // Set defaults
    SetDefaultAttributes();

    // copy child SdrObjects
    if (rSource.GetSubList())
    {
        CopyObjects(*rSource.GetSubList());

        // tdf#116979: needed here, we need bSnapRectDirty to be true
        // which it is after using SdrObject::operator= (see above),
        // but set to false again using CopyObjects
        SetBoundAndSnapRectsDirty();
    }

    // copy local data
    m_aCamera = rSource.m_aCamera;
    m_aCameraSet = rSource.m_aCameraSet;
    static_cast<sdr::properties::E3dSceneProperties&>(GetProperties()).SetSceneItemsFromCamera();
    InvalidateBoundVolume();
    RebuildLists();
    ImpCleanup3DDepthMapper();
    GetViewContact().ActionChanged();
}

void E3dScene::SetDefaultAttributes()
{
    // Set defaults
    m_aCamera.SetViewWindow(-2, -2, 4, 4);
    m_aCameraSet.SetDeviceRectangle(-2, 2, -2, 2);
    m_aCamera.SetDeviceWindow(tools::Rectangle(0, 0, 10, 10));
    tools::Rectangle aRect(0, 0, 10, 10);
    m_aCameraSet.SetViewportRectangle(aRect);

    // set defaults for Camera from ItemPool
    m_aCamera.SetProjection(GetPerspective());
    basegfx::B3DPoint aActualPosition(m_aCamera.GetPosition());
    double fNew = GetDistance();

    if(fabs(fNew - aActualPosition.getZ()) > 1.0)
    {
        m_aCamera.SetPosition( basegfx::B3DPoint( aActualPosition.getX(), aActualPosition.getY(), fNew) );
    }

    fNew = GetFocalLength() / 100.0;
    m_aCamera.SetFocalLength(fNew);
}

E3dScene::~E3dScene()
{
    ImpCleanup3DDepthMapper();
}

SdrPage* E3dScene::getSdrPageFromSdrObjList() const
{
    return getSdrPageFromSdrObject();
}

SdrObject* E3dScene::getSdrObjectFromSdrObjList() const
{
    return const_cast< E3dScene* >(this);
}

SdrObjList* E3dScene::getChildrenOfSdrObject() const
{
    return const_cast< E3dScene* >(this);
}

basegfx::B2DPolyPolygon E3dScene::TakeXorPoly() const
{
    const sdr::contact::ViewContactOfE3dScene& rVCScene = static_cast< sdr::contact::ViewContactOfE3dScene& >(GetViewContact());
    const drawinglayer::geometry::ViewInformation3D& aViewInfo3D(rVCScene.getViewInformation3D());
    const basegfx::B3DPolyPolygon aCubePolyPolygon(CreateWireframe());

    basegfx::B2DPolyPolygon aRetval(basegfx::utils::createB2DPolyPolygonFromB3DPolyPolygon(aCubePolyPolygon,
        aViewInfo3D.getObjectToView()));
    aRetval.transform(rVCScene.getObjectTransformation());

    return aRetval;
}

void E3dScene::ImpCleanup3DDepthMapper()
{
    mp3DDepthRemapper.reset();
}

sal_uInt32 E3dScene::RemapOrdNum(sal_uInt32 nNewOrdNum) const
{
    if(!mp3DDepthRemapper)
    {
        const size_t nObjCount(GetSubList() ? GetSubList()->GetObjCount() : 0);

        if(nObjCount > 1)
        {
            mp3DDepthRemapper.reset(new Imp3DDepthRemapper(*this));
        }
    }

    if(mp3DDepthRemapper)
    {
        return mp3DDepthRemapper->RemapOrdNum(nNewOrdNum);
    }

    return nNewOrdNum;
}

SdrObjKind E3dScene::GetObjIdentifier() const
{
    return SdrObjKind::E3D_Scene;
}

void E3dScene::SetBoundRectDirty()
{
    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(pScene == this)
    {
        // avoid resetting aOutRect which in case of a 3D scene used as 2d object
        // is model data,not re-creatable view data
    }
    else
    {
        // if not the outmost scene it is used as group in 3d, call parent
        E3dObject::SetBoundRectDirty();
    }
}

void E3dScene::NbcSetSnapRect(const tools::Rectangle& rRect)
{
    SetBoundAndSnapRectsDirty();
    E3dObject::NbcSetSnapRect(rRect);
    m_aCamera.SetDeviceWindow(rRect);
    m_aCameraSet.SetViewportRectangle(rRect);

    ImpCleanup3DDepthMapper();
}

void E3dScene::NbcMove(const Size& rSize)
{
    tools::Rectangle aNewSnapRect = GetSnapRect();
    aNewSnapRect.Move(rSize);
    NbcSetSnapRect(aNewSnapRect);
}

void E3dScene::NbcResize(const Point& rRef, const Fraction& rXFact,
                                            const Fraction& rYFact)
{
    tools::Rectangle aNewSnapRect = GetSnapRect();
    ResizeRect(aNewSnapRect, rRef, rXFact, rYFact);
    NbcSetSnapRect(aNewSnapRect);
}

// Set new camera, and thus mark the scene and if possible the bound volume
// as changed

void E3dScene::SetCamera(const Camera3D& rNewCamera)
{
    m_aCamera = rNewCamera;
    static_cast<sdr::properties::E3dSceneProperties&>(GetProperties()).SetSceneItemsFromCamera();

    SetBoundAndSnapRectsDirty();

    // Turn off ratio
    GetCameraSet().SetRatio(0.0);

    // Set Imaging geometry
    basegfx::B3DPoint aVRP(m_aCamera.GetViewPoint());
    basegfx::B3DVector aVPN(aVRP - m_aCamera.GetVRP());
    basegfx::B3DVector aVUV(m_aCamera.GetVUV());

    // use SetViewportValues() to set VRP, VPN and VUV as vectors, too.
    // Else these values would not be exported/imported correctly.
    GetCameraSet().SetViewportValues(aVRP, aVPN, aVUV);

    // Set perspective
    GetCameraSet().SetPerspective(m_aCamera.GetProjection() == ProjectionType::Perspective);
    GetCameraSet().SetViewportRectangle(m_aCamera.GetDeviceWindow());

    ImpCleanup3DDepthMapper();
}

// Inform parent of changes of a child

void E3dScene::StructureChanged()
{
    E3dObject::StructureChanged();

    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(nullptr != pScene && !pScene->mbSkipSettingDirty)
    {
        SetBoundAndSnapRectsDirty();
    }

    ImpCleanup3DDepthMapper();
}

// Determine the overall scene object

E3dScene* E3dScene::getRootE3dSceneFromE3dObject() const
{
    E3dScene* pParent(getParentE3dSceneFromE3dObject());

    if(nullptr != pParent)
    {
        return pParent->getRootE3dSceneFromE3dObject();
    }

    return const_cast< E3dScene* >(this);
}

void E3dScene::removeAllNonSelectedObjects()
{
    E3DModifySceneSnapRectUpdater aUpdater(this);

    size_t a = 0;
    while (a < GetObjCount())
    {
        SdrObject* pObj = GetObj(a);

        if(pObj)
        {
            bool bRemoveObject(false);

            if(E3dScene* pScene = DynCastE3dScene(pObj))
            {
                // iterate over this sub-scene
                pScene->removeAllNonSelectedObjects();

                // check object count. Empty scenes can be deleted
                const size_t nObjCount(pScene->GetSubList() ? pScene->GetSubList()->GetObjCount() : 0);

                if(!nObjCount)
                {
                    // all objects removed, scene can be removed, too
                    bRemoveObject = true;
                }
            }
            else if(auto pCompound = dynamic_cast<E3dCompoundObject*>(pObj))
            {
                if(!pCompound->GetSelected())
                {
                    bRemoveObject = true;
                }
            }

            if(bRemoveObject)
            {
                NbcRemoveObject(pObj->GetOrdNum());
                continue;
            }
        }
        ++a;
    }
}

rtl::Reference<SdrObject> E3dScene::CloneSdrObject(SdrModel& rTargetModel) const
{
    return new E3dScene(rTargetModel, *this);
}

void E3dScene::SuspendReportingDirtyRects()
{
    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(nullptr != pScene)
    {
        pScene->mbSkipSettingDirty = true;
    }
}

void E3dScene::ResumeReportingDirtyRects()
{
    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(nullptr != pScene)
    {
        pScene->mbSkipSettingDirty = false;
    }
}

void E3dScene::SetAllSceneRectsDirty()
{
    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(nullptr != pScene)
    {
        pScene->SetBoundAndSnapRectsDirty();
    }
}

// Rebuild Light- and label- object lists rebuild (after loading, allocation)

void E3dScene::RebuildLists()
{
    // first delete
    const SdrLayerID nCurrLayerID(GetLayer());
    SdrObjListIter a3DIterator(GetSubList(), SdrIterMode::Flat);

    // then examine all the objects in the scene
    while(a3DIterator.IsMore())
    {
        E3dObject* p3DObj(static_cast< E3dObject* >(a3DIterator.Next()));
        p3DObj->NbcSetLayer(nCurrLayerID);
    }

    ImpCleanup3DDepthMapper();
}

std::unique_ptr<SdrObjGeoData> E3dScene::NewGeoData() const
{
    return std::make_unique<E3DSceneGeoData>();
}

void E3dScene::SaveGeoData(SdrObjGeoData& rGeo) const
{
    E3dObject::SaveGeoData (rGeo);

    static_cast<E3DSceneGeoData &>(rGeo).aCamera = m_aCamera;
}

void E3dScene::RestoreGeoData(const SdrObjGeoData& rGeo)
{
    // #i94832# removed E3DModifySceneSnapRectUpdater here.
    // It should not be needed, is already part of E3dObject::RestoreGeoData
    E3dObject::RestoreGeoData (rGeo);
    SetCamera (static_cast<const E3DSceneGeoData &>(rGeo).aCamera);
}

// Something was changed in the style sheet, so change scene

void E3dScene::Notify(SfxBroadcaster &rBC, const SfxHint  &rHint)
{
    SetBoundAndSnapRectsDirty();
    E3dObject::Notify(rBC, rHint);
}

void E3dScene::RotateScene (const Point& rRef, double sn, double cs)
{
    Point NewCenter;

    auto const& rRectangle = getOutRectangle();
    Point Center = rRectangle.Center();

        // Only the center is moved. The corners are moved by NbcMove. For the
        // rotation a cartesian coordinate system is used in which the pivot
        // point is the origin, and the y-axis increases upward, the X-axis to
        // the right. This must be especially noted for the Y-values.
        // (When considering a flat piece of paper the Y-axis pointing downwards
    Center.setX(Center.X() - rRef.X());
    Center.setY(rRef.Y() - Center.Y());
                  // A few special cases has to be dealt with first (n * 90 degrees n integer)
    if (sn==1.0 && cs==0.0) { // 90deg
        NewCenter.setX( -Center.Y() );
        NewCenter.setY( -Center.X() );
    } else if (sn==0.0 && cs==-1.0) { // 180deg
        NewCenter.setX( -Center.X() );
        NewCenter.setY( -Center.Y() );
    } else if (sn==-1.0 && cs==0.0) { // 270deg
        NewCenter.setX(  Center.Y() );
        NewCenter.setY( -Center.X() );
    }
    else          // Here it is rotated to any angle in the mathematically
                  // positive direction!
    {             // xnew = x * cos(alpha) - y * sin(alpha)
                  // ynew = x * sin(alpha) + y * cos(alpha)
                  // Bottom Right is not rotated: the pages of aOutRect must
                  // remain parallel to the coordinate axes.
        NewCenter.setX( static_cast<tools::Long>(Center.X() * cs - Center.Y() * sn) );
        NewCenter.setY( static_cast<tools::Long>(Center.X() * sn + Center.Y() * cs) );
    }

    Size Differenz;
    Point DiffPoint = NewCenter - Center;
    Differenz.setWidth( DiffPoint.X() );
    Differenz.setHeight( -DiffPoint.Y() );  // Note that the Y-axis is counted ad positive downward.
    NbcMove (Differenz);  // Actually executes the coordinate transformation.
}

OUString E3dScene::TakeObjNameSingul() const
{
    OUString sName(SvxResId(STR_ObjNameSingulScene3d));

    OUString aName(GetName());
    if (!aName.isEmpty())
        sName += " '" + aName + "'";
    return sName;
}

OUString E3dScene::TakeObjNamePlural() const
{
    return SvxResId(STR_ObjNamePluralScene3d);
}

// The NbcRotate routine overrides the one of the SdrObject. The idea is
// to be able to rotate the scene relative to the position of the scene
// and then the objects in the scene

void E3dScene::NbcSetTransform(const basegfx::B3DHomMatrix& rMatrix)
{
    if(maTransformation != rMatrix)
    {
        // call parent
        E3dObject::NbcSetTransform(rMatrix);
    }
}

void E3dScene::SetTransform(const basegfx::B3DHomMatrix& rMatrix)
{
    if(rMatrix != maTransformation)
    {
        // call parent
        E3dObject::SetTransform(rMatrix);
    }
}

void E3dScene::NbcRotate(const Point& rRef, Degree100 nAngle, double sn, double cs)
{
    // So currently the gluepoints are defined relative to the scene aOutRect.
    // Before turning the gluepoints are defined relative to the page. They
    // take no part in the rotation of the scene. To ensure this, there is the
    // SetGlueReallyAbsolute(sal_True);

    // So that was the scene, now the objects used in the scene
    // 3D objects, if there is only one it can still have multiple surfaces but
    // the surfaces do not have to be connected. This allows you to access child
    // objects. So going through the entire list and rotate around the Z axis
    // through the enter of aOutRect's (Steiner's theorem), so RotateZ

    RotateScene (rRef, sn, cs);  // Rotates the scene
    double fAngleInRad = toRadians(nAngle);

    basegfx::B3DHomMatrix aRotation;
    aRotation.rotate(0.0, 0.0, fAngleInRad);
    NbcSetTransform(aRotation * GetTransform());

    SetBoundAndSnapRectsDirty();    // This forces a recalculation of all BoundRects
    NbcRotateGluePoints(rRef,nAngle,sn,cs);  // Rotate the gluepoints (who still
                                            // have coordinates relative to the
                                            // original page)
    SetGlueReallyAbsolute(false);  // from now they are again relative to BoundRect (that is defined as aOutRect)
    SetBoundAndSnapRectsDirty();
}

void E3dScene::RecalcSnapRect()
{
    E3dScene* pScene(getRootE3dSceneFromE3dObject());

    if(pScene == this)
    {
        // The Scene is used as a 2D-Object, take the SnapRect from the
        // 2D Display settings
        maSnapRect = pScene->m_aCamera.GetDeviceWindow();
    }
    else
    {
        // The Scene itself is a member of another scene, get the SnapRect
        // as a composite object
        // call parent
        E3dObject::RecalcSnapRect();

        for (const rtl::Reference<SdrObject>& pObj : *this)
        {
            E3dObject* pCandidate(DynCastE3dObject(pObj.get()));

            if(pCandidate)
            {
                maSnapRect.Union(pCandidate->GetSnapRect());
            }
        }
    }
}

bool E3dScene::IsBreakObjPossible()
{
    // Break scene, if all members are able to break
    SdrObjListIter a3DIterator(GetSubList(), SdrIterMode::DeepWithGroups);

    while ( a3DIterator.IsMore() )
    {
        E3dObject* pObj = static_cast<E3dObject*>(a3DIterator.Next());
        if(!pObj->IsBreakObjPossible())
            return false;
    }

    return true;
}

basegfx::B2DPolyPolygon E3dScene::TakeCreatePoly(const SdrDragStat& /*rDrag*/) const
{
    return TakeXorPoly();
}

bool E3dScene::BegCreate(SdrDragStat& rStat)
{
    rStat.SetOrtho4Possible();
    tools::Rectangle aRect1(rStat.GetStart(), rStat.GetNow());
    aRect1.Normalize();
    rStat.SetActionRect(aRect1);
    NbcSetSnapRect(aRect1);
    return true;
}

bool E3dScene::MovCreate(SdrDragStat& rStat)
{
    tools::Rectangle aRect1;
    rStat.TakeCreateRect(aRect1);
    aRect1.Normalize();
    rStat.SetActionRect(aRect1);
    NbcSetSnapRect(aRect1);
    SetBoundRectDirty();
    m_bSnapRectDirty=true;
    return true;
}

bool E3dScene::EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd)
{
    tools::Rectangle aRect1;
    rStat.TakeCreateRect(aRect1);
    aRect1.Normalize();
    NbcSetSnapRect(aRect1);
    SetBoundAndSnapRectsDirty();
    return (eCmd==SdrCreateCmd::ForceEnd || rStat.GetPointCount()>=2);
}

bool E3dScene::BckCreate(SdrDragStat& /*rStat*/)
{
    return false;
}

void E3dScene::BrkCreate(SdrDragStat& /*rStat*/)
{
}

void E3dScene::SetSelected(bool bNew)
{
    // call parent
    E3dObject::SetSelected(bNew);

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        E3dObject* pCandidate(DynCastE3dObject(pObj.get()));

        if(pCandidate)
        {
            pCandidate->SetSelected(bNew);
        }
    }
}

void E3dScene::NbcInsertObject(SdrObject* pObj, size_t nPos)
{
    // Is it even a 3D object?
    if(DynCastE3dObject(pObj))
    {
        // Normal 3D object, insert means call parent
        SdrObjList::NbcInsertObject(pObj, nPos);

        // local needed stuff
        InvalidateBoundVolume();
        StructureChanged();
    }
    else
    {
        // No 3D object, inserted a page in place in a scene ...
        if (SdrPage* pPage = getSdrObjectFromSdrObjList()->getSdrPageFromSdrObject())
            pPage->InsertObject(pObj, nPos);
    }
}

void E3dScene::InsertObject(SdrObject* pObj, size_t nPos)
{
    // Is it even a 3D object?
    if(DynCastE3dObject(pObj))
    {
        // call parent
        SdrObjList::InsertObject(pObj, nPos);

        // local needed stuff
        InvalidateBoundVolume();
        StructureChanged();
    }
    else
    {
        // No 3D object, inserted a page in place in a scene ...
        if (SdrPage* pPage = getSdrObjectFromSdrObjList()->getSdrPageFromSdrObject())
            pPage->InsertObject(pObj, nPos);
    }
}

rtl::Reference<SdrObject> E3dScene::NbcRemoveObject(size_t nObjNum)
{
    // call parent
    rtl::Reference<SdrObject> pRetval = SdrObjList::NbcRemoveObject(nObjNum);

    InvalidateBoundVolume();
    StructureChanged();

    return pRetval;
}

rtl::Reference<SdrObject> E3dScene::RemoveObject(size_t nObjNum)
{
    // call parent
    rtl::Reference<SdrObject> pRetval(SdrObjList::RemoveObject(nObjNum));

    InvalidateBoundVolume();
    StructureChanged();

    return pRetval;
}

void E3dScene::SetBoundAndSnapRectsDirty(bool bNotMyself, bool bRecursive)
{
    // call parent
    E3dObject::SetBoundAndSnapRectsDirty(bNotMyself, bRecursive);

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        E3dObject* pCandidate = DynCastE3dObject(pObj.get());

        if(pCandidate)
        {
            pCandidate->SetBoundAndSnapRectsDirty(bNotMyself, false);
        }
    }
}

void E3dScene::NbcSetLayer(SdrLayerID nLayer)
{
    // call parent
    E3dObject::NbcSetLayer(nLayer);

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        E3dObject* pCandidate = DynCastE3dObject(pObj.get());

        if(pCandidate)
        {
            pCandidate->NbcSetLayer(nLayer);
        }
    }
}

void E3dScene::handlePageChange(SdrPage* pOldPage, SdrPage* pNewPage)
{
    if(pOldPage == pNewPage)
        return;

    // call parent
    E3dObject::handlePageChange(pOldPage, pNewPage);

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        E3dObject* pCandidate = DynCastE3dObject(pObj.get());

        if(pCandidate)
        {
            pCandidate->handlePageChange(pOldPage, pNewPage);
        }
        else
        {
            OSL_ENSURE(false, "E3dScene::handlePageChange invalid object list (!)");
        }
    }
}

SdrObjList* E3dScene::GetSubList() const
{
    return const_cast< E3dScene* >(this);
}

basegfx::B3DRange E3dScene::RecalcBoundVolume() const
{
    basegfx::B3DRange aRetval;

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        const E3dObject* p3DObject = DynCastE3dObject(pObj.get());

        if(p3DObject)
        {
            basegfx::B3DRange aLocalRange(p3DObject->GetBoundVolume());
            aLocalRange.transform(p3DObject->GetTransform());
            aRetval.expand(aLocalRange);
        }
    }

    return aRetval;
}

void E3dScene::SetTransformChanged()
{
    // call parent
    E3dObject::SetTransformChanged();

    for (const rtl::Reference<SdrObject>& pObj : *this)
    {
        E3dObject* pCandidate = DynCastE3dObject(pObj.get());

        if(pCandidate)
        {
            pCandidate->SetTransformChanged();
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

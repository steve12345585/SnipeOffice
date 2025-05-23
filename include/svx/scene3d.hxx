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

#ifndef INCLUDED_SVX_SCENE3D_HXX
#define INCLUDED_SVX_SCENE3D_HXX

#include <svx/camera3d.hxx>
#include <tools/b3dtrans.hxx>
#include <svx/svdpage.hxx>
#include <svx/svxdllapi.h>
#include <svx/obj3d.hxx>
#include <svx/svx3ditems.hxx>
#include <memory>

/*************************************************************************
|*
|* GeoData relevant for undo actions
|*
\************************************************************************/

class E3DSceneGeoData final : public E3DObjGeoData
{
public:
    Camera3D                    aCamera;

    E3DSceneGeoData() {}
};

class Imp3DDepthRemapper;

/*************************************************************************
|*
|* base class for 3D scenes
|*
\************************************************************************/

class SVXCORE_DLLPUBLIC E3dScene final : public E3dObject, public SdrObjList
{
    virtual std::unique_ptr<sdr::properties::BaseProperties> CreateObjectSpecificProperties() override;
    virtual std::unique_ptr<sdr::contact::ViewContact> CreateObjectSpecificViewContact() override;

    // transformations
    B3dCamera                   m_aCameraSet;
    Camera3D                    m_aCamera;

    mutable std::unique_ptr<Imp3DDepthRemapper> mp3DDepthRemapper;

    // Flag to determine if only selected objects should be drawn
    bool                        m_bDrawOnlySelected       : 1;

    bool mbSkipSettingDirty : 1;

    void RebuildLists();

    virtual void Notify(SfxBroadcaster &rBC, const SfxHint  &rHint) override;

    void SetDefaultAttributes();
    void ImpCleanup3DDepthMapper();

public:
    E3dScene(SdrModel& rSdrModel);
    E3dScene(SdrModel& rSdrModel, E3dScene const &);
    virtual ~E3dScene() override;

    virtual void StructureChanged() override;

    // derived from SdrObjList
    virtual SdrPage* getSdrPageFromSdrObjList() const override;
    virtual SdrObject* getSdrObjectFromSdrObjList() const override;

    // derived from SdrObject
    virtual SdrObjList* getChildrenOfSdrObject() const override;

    virtual void SetBoundRectDirty() override;

    virtual basegfx::B2DPolyPolygon TakeXorPoly() const override;

    sal_uInt32 RemapOrdNum(sal_uInt32 nOrdNum) const;

    // Perspective: enum ProjectionType { ProjectionType::Parallel, ProjectionType::Perspective }
    ProjectionType GetPerspective() const
        { return static_cast<ProjectionType>(GetObjectItemSet().Get(SDRATTR_3DSCENE_PERSPECTIVE).GetValue()); }

    // Distance:
    double GetDistance() const
        { return static_cast<double>(GetObjectItemSet().Get(SDRATTR_3DSCENE_DISTANCE).GetValue()); }

    // Focal length: before cm, now 1/10th mm (*100)
    double GetFocalLength() const
        { return GetObjectItemSet().Get(SDRATTR_3DSCENE_FOCAL_LENGTH).GetValue(); }

    // set flag to draw only selected
    void SetDrawOnlySelected(bool bNew) { m_bDrawOnlySelected = bNew; }
    bool GetDrawOnlySelected() const { return m_bDrawOnlySelected; }
    virtual SdrObjKind GetObjIdentifier() const override;

    virtual void    NbcSetSnapRect(const tools::Rectangle& rRect) override;
    virtual void    NbcMove(const Size& rSize) override;
    virtual void    NbcResize(const Point& rRef, const Fraction& rXFact,
                                                 const Fraction& rYFact) override;
    virtual void    RecalcSnapRect() override;

    virtual E3dScene* getRootE3dSceneFromE3dObject() const override;
    void SetCamera(const Camera3D& rNewCamera);
    const Camera3D& GetCamera() const { return m_aCamera; }
    void removeAllNonSelectedObjects();

    virtual rtl::Reference<SdrObject> CloneSdrObject(SdrModel& rTargetModel) const override;

    virtual std::unique_ptr<SdrObjGeoData> NewGeoData() const override;
    virtual void          SaveGeoData(SdrObjGeoData& rGeo) const override;
    virtual void          RestoreGeoData(const SdrObjGeoData& rGeo) override;

    virtual void NbcSetTransform(const basegfx::B3DHomMatrix& rMatrix) override;
    virtual void SetTransform(const basegfx::B3DHomMatrix& rMatrix) override;

    virtual void NbcRotate(const Point& rRef, Degree100 nAngle, double sn, double cs) override;
    void RotateScene(const Point& rRef, double sn, double cs);

    // TakeObjName...() is for the display in the UI, for example "3 frames selected".
    virtual OUString TakeObjNameSingul() const override;
    virtual OUString TakeObjNamePlural() const override;

    // get transformations
    B3dCamera& GetCameraSet() { return m_aCameraSet; }
    const B3dCamera& GetCameraSet() const { return m_aCameraSet; }

    // break up
    virtual bool IsBreakObjPossible() override;

    // polygon which is built during creation
    virtual basegfx::B2DPolyPolygon TakeCreatePoly(const SdrDragStat& rDrag) const override;

    // create moves
    virtual bool BegCreate(SdrDragStat& rStat) override;
    virtual bool MovCreate(SdrDragStat& rStat) override; // true=Xor must be repainted
    virtual bool EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd) override;
    virtual bool BckCreate(SdrDragStat& rStat) override;
    virtual void BrkCreate(SdrDragStat& rStat) override;

    void SuspendReportingDirtyRects();
    void ResumeReportingDirtyRects();
    void SetAllSceneRectsDirty();

    // set selection from E3dObject (temporary flag for 3D actions)
    virtual void SetSelected(bool bNew) override;

    // derived from SdrObjList
    virtual void NbcInsertObject(SdrObject* pObj, size_t nPos=SAL_MAX_SIZE) override;
    virtual void InsertObject(SdrObject* pObj, size_t nPos=SAL_MAX_SIZE) override;
    virtual rtl::Reference<SdrObject> NbcRemoveObject(size_t nObjNum) override;
    virtual rtl::Reference<SdrObject> RemoveObject(size_t nObjNum) override;

    // needed for group functionality
    virtual void SetBoundAndSnapRectsDirty(bool bNotMyself = false, bool bRecursive = true) override;
    virtual void NbcSetLayer(SdrLayerID nLayer) override;

    // react on model/page change
    virtual void handlePageChange(SdrPage* pOldPage, SdrPage* pNewPage) override;

    virtual SdrObjList* GetSubList() const override;
    virtual void SetTransformChanged() override;

private:
    virtual basegfx::B3DRange RecalcBoundVolume() const override;
};

#endif // INCLUDED_SVX_SCENE3D_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

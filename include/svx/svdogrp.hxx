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

#include <memory>
#include <svx/svdobj.hxx>
#include <svx/svxdllapi.h>
#include <svx/svdpage.hxx>

//   SdrObjGroup
class SVXCORE_DLLPUBLIC SdrObjGroup final : public SdrObject, public SdrObjList
{
public:
    // Basic DiagramHelper support
    virtual const std::shared_ptr< svx::diagram::IDiagramHelper >& getDiagramHelper() const override;

private:
    virtual std::unique_ptr<sdr::contact::ViewContact> CreateObjectSpecificViewContact() override;
    virtual std::unique_ptr<sdr::properties::BaseProperties>
    CreateObjectSpecificProperties() override;

    Point maRefPoint; // Reference point inside the object group

    // Allow *only* DiagramHelper itself to set this internal reference to
    // tightly control usage
    friend class svx::diagram::IDiagramHelper;
    std::shared_ptr< svx::diagram::IDiagramHelper > mp_DiagramHelper;

public:
    SdrObjGroup(SdrModel& rSdrModel);
    // Copy constructor
    SdrObjGroup(SdrModel& rSdrModel, SdrObjGroup const& rSource);
    virtual ~SdrObjGroup() override;

    // derived from SdrObjList
    virtual SdrPage* getSdrPageFromSdrObjList() const override;
    virtual SdrObject* getSdrObjectFromSdrObjList() const override;

    // derived from SdrObject
    virtual SdrObjList* getChildrenOfSdrObject() const override;

    virtual void SetBoundRectDirty() override;
    virtual SdrObjKind GetObjIdentifier() const override;
    virtual void TakeObjInfo(SdrObjTransformInfoRec& rInfo) const override;
    virtual SdrLayerID GetLayer() const override;
    virtual void NbcSetLayer(SdrLayerID nLayer) override;

    // react on model/page change
    virtual void handlePageChange(SdrPage* pOldPage, SdrPage* pNewPage) override;

    virtual SdrObjList* GetSubList() const override;
    virtual void SetGrabBagItem(const css::uno::Any& rVal) override;

    virtual const tools::Rectangle& GetCurrentBoundRect() const override;
    virtual const tools::Rectangle& GetSnapRect() const override;

    virtual rtl::Reference<SdrObject> CloneSdrObject(SdrModel& rTargetModel) const override;

    virtual OUString TakeObjNameSingul() const override;
    virtual OUString TakeObjNamePlural() const override;

    virtual void RecalcSnapRect() override;
    virtual basegfx::B2DPolyPolygon TakeXorPoly() const override;

    // special drag methods
    virtual bool beginSpecialDrag(SdrDragStat& rDrag) const override;

    virtual bool BegCreate(SdrDragStat& rStat) override;

    virtual Degree100 GetRotateAngle() const override;
    virtual Degree100 GetShearAngle(bool bVertical = false) const override;

    virtual void Move(const Size& rSiz) override;
    virtual void Resize(const Point& rRef, const Fraction& xFact, const Fraction& yFact,
                        bool bUnsetRelative = true) override;
    virtual void Rotate(const Point& rRef, Degree100 nAngle, double sn, double cs) override;
    virtual void Mirror(const Point& rRef1, const Point& rRef2) override;
    virtual void Shear(const Point& rRef, Degree100 nAngle, double tn, bool bVShear) override;
    virtual void SetAnchorPos(const Point& rPnt) override;
    virtual void SetRelativePos(const Point& rPnt) override;
    virtual void SetSnapRect(const tools::Rectangle& rRect) override;
    virtual void SetLogicRect(const tools::Rectangle& rRect) override;

    virtual void NbcMove(const Size& rSiz) override;
    virtual void NbcResize(const Point& rRef, const Fraction& xFact,
                           const Fraction& yFact) override;
    virtual void NbcRotate(const Point& rRef, Degree100 nAngle, double sn, double cs) override;
    virtual void NbcMirror(const Point& rRef1, const Point& rRef2) override;
    virtual void NbcShear(const Point& rRef, Degree100 nAngle, double tn, bool bVShear) override;
    virtual void NbcSetAnchorPos(const Point& rPnt) override;
    virtual void NbcSetRelativePos(const Point& rPnt) override;
    virtual void NbcSetSnapRect(const tools::Rectangle& rRect) override;
    virtual void NbcSetLogicRect(const tools::Rectangle& rRect, bool bAdaptTextMinSize = true) override;

    virtual void NbcReformatText() override;

    virtual rtl::Reference<SdrObject> DoConvertToPolyObj(bool bBezier,
                                                         bool bAddText) const override;

    virtual void dumpAsXml(xmlTextWriterPtr pWriter) const override;
    virtual void AddToHdlList(SdrHdlList& rHdlList) const override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

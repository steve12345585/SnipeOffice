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
#include <optional>
#include <svx/svdotext.hxx>
#include <svx/svdglue.hxx>
#include <svx/svxdllapi.h>
#include <svx/xpoly.hxx>


class SdrDragMethod;
class SdrPageView;

namespace sdr::properties {
    class ConnectorProperties;
}


/// Utility class SdrObjConnection
class SdrObjConnection final
{
    friend class                SdrEdgeObj;
    friend class                ImpEdgeHdl;
    friend class                SdrCreateView;

    Point                       m_aObjOfs;       // set during dragging of a node
    SdrObject*                  m_pSdrObj;          // referenced object
    sal_uInt16                  m_nConId;        // connector number

    bool                        m_bBestConn : 1;   // true -> the best-matching connector is searched for
    bool                        m_bBestVertex : 1; // true -> the best-matching vertex to connect is searched for
    bool                        m_bAutoVertex : 1; // autoConnector at apex nCon
    bool                        m_bAutoCorner : 1; // autoConnector at corner nCon

public:
    SdrObjConnection() { ResetVars(); }

    void ResetVars();
    bool TakeGluePoint(SdrGluePoint& rGP) const;

    void SetBestConnection( bool rB ) { m_bBestConn = rB; };
    void SetBestVertex( bool rB ) { m_bBestVertex = rB; };
    void SetAutoVertex( bool rB ) { m_bAutoVertex = rB; };
    void SetConnectorId( sal_uInt16 nId ) { m_nConId = nId; };

    bool IsBestConnection() const { return m_bBestConn; };
    bool IsAutoVertex() const { return m_bAutoVertex; };
    sal_uInt16 GetConnectorId() const { return m_nConId; };
    SdrObject* GetSdrObject() const { return m_pSdrObj; }
};


enum class SdrEdgeLineCode { Obj1Line2, Obj1Line3, Obj2Line2, Obj2Line3, MiddleLine };

/// Utility class SdrEdgeInfoRec
class SdrEdgeInfoRec
{
public:
    // The 5 distances are set on dragging or via SetAttr and are
    // evaluated by ImpCalcEdgeTrack. Only 0-3 longs are transported
    // via Get/SetAttr/Get/SetStyleSh though.
    Point                       m_aObj1Line2;
    Point                       m_aObj1Line3;
    Point                       m_aObj2Line2;
    Point                       m_aObj2Line3;
    Point                       m_aMiddleLine;

    // Following values are set by ImpCalcEdgeTrack
    tools::Long                        m_nAngle1;           // exit angle at Obj1
    tools::Long                        m_nAngle2;           // exit angle at Obj2
    sal_uInt16                  m_nObj1Lines;        // 1..3
    sal_uInt16                  m_nObj2Lines;        // 1..3
    sal_uInt16                  m_nMiddleLine;       // 0xFFFF=none, otherwise point number of the beginning of the line

    // The value determines how curved connectors are routed. With value 'true' it is routed
    // compatible to OOXML, with value 'false' LO routing is used.
    // The value is set/get via property SDRATTR_EDGEOOXMLCURVE.
    bool m_bUseOOXMLCurve;

public:
    SdrEdgeInfoRec()
    :   m_nAngle1(0),
        m_nAngle2(0),
        m_nObj1Lines(0),
        m_nObj2Lines(0),
        m_nMiddleLine(0xFFFF),
        m_bUseOOXMLCurve(false)
    {}

    Point& ImpGetLineOffsetPoint(SdrEdgeLineCode eLineCode);
    sal_uInt16 ImpGetPolyIdx(SdrEdgeLineCode eLineCode, const XPolygon& rXP) const;
    bool ImpIsHorzLine(SdrEdgeLineCode eLineCode, const XPolygon& rXP) const;
    void ImpSetLineOffset(SdrEdgeLineCode eLineCode, const XPolygon& rXP, tools::Long nVal);
    tools::Long ImpGetLineOffset(SdrEdgeLineCode eLineCode, const XPolygon& rXP) const;
};


/// Utility class SdrEdgeObjGeoData
class SdrEdgeObjGeoData final : public SdrTextObjGeoData
{
public:
    SdrObjConnection            m_aCon1;  // connection status of the beginning of the line
    SdrObjConnection            m_aCon2;  // connection status of the end of the line
    std::optional<XPolygon>     m_pEdgeTrack;
    bool                        m_bEdgeTrackDirty; // true -> connector track needs to be recalculated
    bool                        m_bEdgeTrackUserDefined;
    SdrEdgeInfoRec              m_aEdgeInfo;

public:
    SdrEdgeObjGeoData();
    virtual ~SdrEdgeObjGeoData() override;
};


/// Utility class SdrEdgeObj
class SVXCORE_DLLPUBLIC SdrEdgeObj final : public SdrTextObj
{
private:
    // to allow sdr::properties::ConnectorProperties access to ImpSetAttrToEdgeInfo()
    friend class sdr::properties::ConnectorProperties;

    friend class                SdrCreateView;
    friend class                ImpEdgeHdl;

    SAL_DLLPRIVATE virtual std::unique_ptr<sdr::contact::ViewContact> CreateObjectSpecificViewContact() override;
    SAL_DLLPRIVATE virtual std::unique_ptr<sdr::properties::BaseProperties> CreateObjectSpecificProperties() override;

    SdrObjConnection            m_aCon1;  // Connection status of the beginning of the line
    SdrObjConnection            m_aCon2;  // Connection status of the end of the line

    std::optional<XPolygon>     m_pEdgeTrack;
    sal_uInt16                  m_nNotifyingCount; // Locking
    SdrEdgeInfoRec              m_aEdgeInfo;

    bool                        m_bEdgeTrackDirty : 1; // true -> Connection track needs to be recalculated
    bool                        m_bEdgeTrackUserDefined : 1;

    // Bool to allow suppression of default connects at object
    // inside test (HitTest) and object center test (see ImpFindConnector())
    bool                        mbSuppressDefaultConnect : 1;

    // Flag value for avoiding infinite loops when calculating
    // BoundRects from ring-connected connectors. A coloring algorithm
    // is used here. When the GetCurrentBoundRect() calculation of a
    // SdrEdgeObj is running, the flag is set, else it is always
    // false.
    bool                        mbBoundRectCalculationRunning : 1;

    // #i123048# need to remember if layouting was suppressed before to get
    // to a correct state for first real layouting
    bool                        mbSuppressed : 1;

public:
    // Interface to default connect suppression
    void SetSuppressDefaultConnect(bool bNew) { mbSuppressDefaultConnect = bNew; }
    bool GetSuppressDefaultConnect() const { return mbSuppressDefaultConnect; }

private:
    SAL_DLLPRIVATE virtual void Notify(SfxBroadcaster& rBC, const SfxHint& rHint) override;

    SAL_DLLPRIVATE static XPolygon ImpCalcObjToCenter(const Point& rStPt, tools::Long nEscAngle, const tools::Rectangle& rRect, const Point& rCenter);
    SAL_DLLPRIVATE void ImpRecalcEdgeTrack();   // recalculation of the connection track
    SAL_DLLPRIVATE XPolygon ImpCalcEdgeTrack(const XPolygon& rTrack0, SdrObjConnection& rCon1, SdrObjConnection& rCon2, SdrEdgeInfoRec* pInfo) const;
    SAL_DLLPRIVATE XPolygon ImpCalcEdgeTrack(const Point& rPt1, tools::Long nAngle1, const tools::Rectangle& rBoundRect1, const tools::Rectangle& rBewareRect1,
        const Point& rPt2, tools::Long nAngle2, const tools::Rectangle& rBoundRect2, const tools::Rectangle& rBewareRect2,
        sal_uIntPtr* pnQuality, SdrEdgeInfoRec* pInfo) const;
    SAL_DLLPRIVATE static bool ImpFindConnector(const Point& rPt, const SdrPageView& rPV, SdrObjConnection& rCon, const SdrEdgeObj* pThis, OutputDevice* pOut=nullptr, SdrDragStat* pDragStat = nullptr);
    SAL_DLLPRIVATE static SdrEscapeDirection ImpCalcEscAngle(SdrObject const * pObj, const Point& aPt2);
    SAL_DLLPRIVATE void ImpSetTailPoint(bool bTail1, const Point& rPt);
    SAL_DLLPRIVATE void ImpUndirtyEdgeTrack();  // potential recalculation of the connection track
    SAL_DLLPRIVATE void ImpDirtyEdgeTrack();    // invalidate connector path, so it will be recalculated next time
    SAL_DLLPRIVATE void ImpSetAttrToEdgeInfo(); // copying values from the pool to aEdgeInfo
    SAL_DLLPRIVATE void ImpSetEdgeInfoToAttr(); // copying values from the aEdgeInfo to the pool

    // protected destructor
    SAL_DLLPRIVATE virtual ~SdrEdgeObj() override;

public:
    SdrEdgeObj(SdrModel& rSdrModel);
    // Copy constructor
    SAL_DLLPRIVATE SdrEdgeObj(SdrModel& rSdrModel, SdrEdgeObj const & rSource);

    // react on model/page change
    SAL_DLLPRIVATE virtual void handlePageChange(SdrPage* pOldPage, SdrPage* pNewPage) override;

    SdrObjConnection& GetConnection(bool bTail1) { return *(bTail1 ? &m_aCon1 : &m_aCon2); }
    SAL_DLLPRIVATE virtual void TakeObjInfo(SdrObjTransformInfoRec& rInfo) const override;
    SAL_DLLPRIVATE virtual SdrObjKind GetObjIdentifier() const override;
    SAL_DLLPRIVATE virtual const tools::Rectangle& GetCurrentBoundRect() const override;
    SAL_DLLPRIVATE virtual const tools::Rectangle& GetSnapRect() const override;
    SAL_DLLPRIVATE virtual SdrGluePoint GetVertexGluePoint(sal_uInt16 nNum) const override;
    SAL_DLLPRIVATE virtual SdrGluePoint GetCornerGluePoint(sal_uInt16 nNum) const override;
    SAL_DLLPRIVATE virtual const SdrGluePointList* GetGluePointList() const override;
    SAL_DLLPRIVATE virtual SdrGluePointList* ForceGluePointList() override;

    // * for all of the below: bTail1=true: beginning of the line,
    //   otherwise end of the line
    // * pObj=NULL: disconnect connector
    void SetEdgeTrackDirty() { m_bEdgeTrackDirty=true; }
    void ConnectToNode(bool bTail1, SdrObject* pObj) override;
    SAL_DLLPRIVATE void DisconnectFromNode(bool bTail1) override;
    SdrObject* GetConnectedNode(bool bTail1) const override;
    const SdrObjConnection& GetConnection(bool bTail1) const { return *(bTail1 ? &m_aCon1 : &m_aCon2); }
    SAL_DLLPRIVATE bool CheckNodeConnection(bool bTail1) const;

    SAL_DLLPRIVATE virtual void RecalcSnapRect() override;
    SAL_DLLPRIVATE virtual void TakeUnrotatedSnapRect(tools::Rectangle& rRect) const override;
    virtual rtl::Reference<SdrObject> CloneSdrObject(SdrModel& rTargetModel) const override;
    SAL_DLLPRIVATE virtual OUString TakeObjNameSingul() const override;
    SAL_DLLPRIVATE virtual OUString TakeObjNamePlural() const override;

    void    SetEdgeTrackPath( const basegfx::B2DPolyPolygon& rPoly );
    basegfx::B2DPolyPolygon GetEdgeTrackPath() const;

    SAL_DLLPRIVATE virtual basegfx::B2DPolyPolygon TakeXorPoly() const override;
    SAL_DLLPRIVATE virtual sal_uInt32 GetHdlCount() const override;
    SAL_DLLPRIVATE virtual void AddToHdlList(SdrHdlList& rHdlList) const override;

    // special drag methods
    SAL_DLLPRIVATE virtual bool hasSpecialDrag() const override;
    SAL_DLLPRIVATE virtual bool beginSpecialDrag(SdrDragStat& rDrag) const override;
    SAL_DLLPRIVATE virtual bool applySpecialDrag(SdrDragStat& rDrag) override;
    SAL_DLLPRIVATE virtual OUString getSpecialDragComment(const SdrDragStat& rDrag) const override;

    // FullDrag support
    SAL_DLLPRIVATE virtual rtl::Reference<SdrObject> getFullDragClone() const override;

    SAL_DLLPRIVATE virtual void NbcSetSnapRect(const tools::Rectangle& rRect) override;
    SAL_DLLPRIVATE virtual void NbcMove(const Size& aSize) override;
    SAL_DLLPRIVATE virtual void NbcResize(const Point& rRefPnt, const Fraction& aXFact, const Fraction& aYFact) override;

    // #i54102# added rotate, mirror and shear support
    SAL_DLLPRIVATE virtual void NbcRotate(const Point& rRef, Degree100 nAngle, double sn, double cs) override;
    SAL_DLLPRIVATE virtual void NbcMirror(const Point& rRef1, const Point& rRef2) override;
    SAL_DLLPRIVATE virtual void NbcShear(const Point& rRef, Degree100 nAngle, double tn, bool bVShear) override;

    // #102344# Added missing implementation
    SAL_DLLPRIVATE virtual void NbcSetAnchorPos(const Point& rPnt) override;

    SAL_DLLPRIVATE virtual bool BegCreate(SdrDragStat& rStat) override;
    SAL_DLLPRIVATE virtual bool MovCreate(SdrDragStat& rStat) override;
    SAL_DLLPRIVATE virtual bool EndCreate(SdrDragStat& rStat, SdrCreateCmd eCmd) override;
    SAL_DLLPRIVATE virtual bool BckCreate(SdrDragStat& rStat) override;
    SAL_DLLPRIVATE virtual void BrkCreate(SdrDragStat& rStat) override;
    SAL_DLLPRIVATE virtual basegfx::B2DPolyPolygon TakeCreatePoly(const SdrDragStat& rDrag) const override;
    SAL_DLLPRIVATE virtual PointerStyle GetCreatePointer() const override;
    SAL_DLLPRIVATE virtual rtl::Reference<SdrObject> DoConvertToPolyObj(bool bBezier, bool bAddText) const override;

    SAL_DLLPRIVATE virtual sal_uInt32 GetSnapPointCount() const override;
    SAL_DLLPRIVATE virtual Point GetSnapPoint(sal_uInt32 i) const override;
    SAL_DLLPRIVATE virtual bool IsPolyObj() const override;
    SAL_DLLPRIVATE virtual sal_uInt32 GetPointCount() const override;
    SAL_DLLPRIVATE virtual Point GetPoint(sal_uInt32 i) const override;
    SAL_DLLPRIVATE virtual void NbcSetPoint(const Point& rPnt, sal_uInt32 i) override;

    SAL_DLLPRIVATE virtual std::unique_ptr<SdrObjGeoData> NewGeoData() const override;
    SAL_DLLPRIVATE virtual void SaveGeoData(SdrObjGeoData& rGeo) const override;
    SAL_DLLPRIVATE virtual void RestoreGeoData(const SdrObjGeoData& rGeo) override;

    /** updates edges that are connected to the edges of this object
        as if the connected objects send a repaint broadcast
        #103122#
    */
    SAL_DLLPRIVATE void Reformat();

    // helper methods for the StarOffice api
    SAL_DLLPRIVATE Point GetTailPoint( bool bTail ) const;
    void SetTailPoint( bool bTail, const Point& rPt );
    SAL_DLLPRIVATE void setGluePointIndex( bool bTail, sal_Int32 nId = -1 );
    SAL_DLLPRIVATE sal_Int32 getGluePointIndex( bool bTail );

    SAL_DLLPRIVATE virtual bool TRGetBaseGeometry(basegfx::B2DHomMatrix& rMatrix, basegfx::B2DPolyPolygon& rPolyPolygon) const override;
    SAL_DLLPRIVATE virtual void TRSetBaseGeometry(const basegfx::B2DHomMatrix& rMatrix, const basegfx::B2DPolyPolygon& rPolyPolygon) override;

    // for geometry access
    SAL_DLLPRIVATE ::basegfx::B2DPolygon getEdgeTrack() const;

    // helper method for SdrDragMethod::AddConnectorOverlays. Adds an overlay polygon for
    // this connector to rResult.
    SAL_DLLPRIVATE basegfx::B2DPolygon ImplAddConnectorOverlay(const SdrDragMethod& rDragMethod, bool bTail1, bool bTail2, bool bDetail) const;
};

 // The following item parameters of the SdrItemPool are used to
 // determine the actual connector line routing:
 //
 //  sal_uInt16 EdgeFlowAngle       default 9000 (= 90.00 deg), min 0, max 9000
 //      Clearance angle.
 //      The angle at which the connecting line may run.
 //
 //  sal_uInt16 EdgeEscAngle        default 9000 (= 90.00 Deg), min 0, max 9000
 //      Object exit angle.
 //      The angle at which the connection line may exit from the object.
 //
 //  bool EdgeEscAsRay              default false
 //      true -> the connecting line emerges from the object radially.
 //      Thus, angle specification by the line ObjCenter / connector.
 //
 //  bool EdgeEscUseObjAngle        default false
 //      Object rotation angle is considered
 //      true -> when determining the connector exit angle, angle for
 //      object rotation is taken as an offset.
 //
 //  sal_uIntPtr EdgeFlowDefDist    default 0, min 0, max ?
 //      This is the default minimum distance on calculation of the
 //      connection Line to the docked objects is in logical units.
 //      This distance is overridden within the object, as soon as the
 //      user drags on the lines. When docking onto a new object,
 //      however, this default is used again.
 //
 //
 // General Information About Connectors:
 //
 // There are nodes and edge objects. Two nodes can be joined by an
 // edge. If a connector is connected to a node only at one end, the
 // other end is fixed to an absolute position in the document. It is
 // of course also possible for a connector to be "free" at both ends,
 // i.e. not connected to a node object on each side.
 //
 // A connector object can also theoretically be a node object at the
 // same time. In the first version, however, this will not yet be
 // realized.
 //
 // A connection between node and connector edge can be established by:
 // - Interactive creation of a new edge object at the SdrView where
 //   the beginning or end point of the edge is placed on a connector
 //   (glueing point) of an already existing node object.
 // - Interactive dragging of the beginning or end point of an
 //   existing connector edge object on the SdrView to a connector
 //   (glueing point) of an already existing node object.
 // - Undo/Redo
 //   Moving node objects does not make any connections. Also not the
 //   direct shifting of edge endpoints on the SdrModel... Connections
 //   can also be established, if the connectors are not configured to
 //   be visible in the view.
 //
 // An existing connection between node and edge is retained for:
 // - Dragging (Move/Resize/Rotate/...) of the node object
 // - Moving a connector position in the node object
 // - Simultaneous dragging (Move/Resize/Rotate/...) of the node and the
 //   edge
 //
 // A connection between node and edge can be removed by:
 // - Deleting one of the objects
 // - Dragging the edge object without simultaneously dragging the node
 // - Deleting the connector at the node object
 // - Undo/Redo/Repeat
 // When dragging, the request to remove the connection must be
 // requested from outside of the model (for example, from the
 // SdrView). SdrEdgeObj::Move() itself does not remove the
 // connection.
 //
 // Each node object can have connectors, so-called gluepoints. These
 // are the geometric points at which the connecting edge object ends
 // when the connection is established. By default, each object has no
 // connectors.  Nevertheless, one can dock an edge in certain view
 // settings since then, e.g., connectors can be automatically
 // generated at the 4 vertices of the node object when needed. Each
 // object provides 2x4 so-called default connector positions, 4 at
 // the vertices and 4 at the corner positions. In the normal case,
 // these are located at the 8 handle positions; exceptions here are
 // ellipses, parallelograms, ... .  In addition, user-specific
 // connectors can be set for each node object.
 //
 // Then there is also the possibility to dock an edge on an object
 // with the attribute "bUseBestConnector". The best-matching
 // connector position for the routing of the connection line is then
 // used from the offering of connectors of the object or/and of the
 // vertices. The user assigns this attribute by docking the node in
 // its center (see, e.g., Visio).
 // 09-06-1996: bUseBestConnector uses vertex gluepoints only.
 //
 // And here is some terminology:
 //   Connector : The connector object (edge object)
 //   Node      : Any object to which a connector can be glued to, e.g., a rectangle,
 //               etc.
 //   Gluepoint: The point at which the connector is glued to the node object.
 //               There are:
 //                 Vertex gluepoints: Each node object presents these glue
 //                     points inherently. Perhaps there is already the option
 //                     "automatically glue to object vertex" in Draw (default is
 //                     on).
 //                 Corner gluepoints: These gluepoints, too, are already
 //                     auto-enabled on objects. Similar to the ones above,
 //                     there may already be an option for them in Draw (default is
 //                     off).
 //                 In contrast to Visio, vertex gluepoints and corner glue
 //                     points are not displayed in the UI; they are simply there (if
 //                     the option is activated).
 //                 Custom gluepoints: Any number of them are present on each
 //                     node object. They can be made visible using the option
 //                     (always visible when editing). At the moment, however, they
 //                     are not yet fully implemented.
 //                 Automatic gluepoint selection: If the connector is docked
 //                     to the node object so that the black frame encompasses the
 //                     entire object, then the connector tries to find the most
 //                     convenient of the 4 vertex gluepoints (and only of those).

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

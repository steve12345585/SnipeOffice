/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
* This file is Part of the SnipeOffice project.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef INCLUDED_SVX_SVDOBJPLUSDATA_HXX
#define INCLUDED_SVX_SVDOBJPLUSDATA_HXX

#include <rtl/ustring.hxx>
#include <memory>

class SdrObject;
class SfxBroadcaster;
class SdrObjUserDataList;
class SdrGluePointList;

// Bitsack for DrawObjects
class SdrObjPlusData final
{
    friend class                SdrObject;

    std::unique_ptr<SfxBroadcaster>      pBroadcast;    // broadcaster, if this object is referenced (bVirtObj=true). Also for connectors etc.
    std::unique_ptr<SdrObjUserDataList>  pUserDataList; // application specific data
    std::unique_ptr<SdrGluePointList>    pGluePoints;   // gluepoints for glueing object connectors

    // #i68101#
    // object name, title and description
    OUString aObjName;
    OUString aObjTitle;
    OUString aObjDescription;
    bool isDecorative = false;

public:
    SdrObjPlusData();
    ~SdrObjPlusData();
    SdrObjPlusData* Clone(SdrObject* pObj1) const;

    void SetGluePoints(const SdrGluePointList& rPts);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

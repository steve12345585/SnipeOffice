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

#ifndef INCLUDED_SVX_SVDLAYER_HXX
#define INCLUDED_SVX_SVDLAYER_HXX

#include <svx/svdsob.hxx>
#include <svx/svdtypes.hxx>
#include <svx/svxdllapi.h>
#include <memory>
#include <vector>

/**
 * Note on the layer mix with symbolic/ID-based interface:
 * You create a new layer with
 *    pLayerAdmin->NewLayer("A new layer");
 * This layer is automatically appended to the end of the list.
 *
 * The same holds true for layer sets.
 *
 * The interface for SdrLayerSet is based on LayerIDs. The app must get
 * an ID for it at the SdrLayerAdmin, like so:
 *   SdrLayerID nLayerID=pLayerAdmin->GetLayerID("A new layer");
 *
 * If the layer cannot be found, SDRLAYER_NOTFOUND is returned.
 * The methods with the ID interface usually handle that error in a
 * meaningful way.
 * If you not only got a name, but even a SdrLayer*, you can get the ID
 * much faster via the layer directly.
 *
 * @param bInherited:
 * TRUE If the layer/layer set cannot be found, we examine the parent layer admin,
 *      whether there's a corresponding definition
 * FALSE We only search this layer admin
 *
 * Every page's layer admin has a parent layer admin (the model's). The model
 * itself does not have a parent.
 */

class SdrModel;

class SVXCORE_DLLPUBLIC SdrLayer
{
    friend class SdrLayerAdmin;

    OUString maName;
    OUString maTitle;
    OUString maDescription;
    SdrModel*  m_pModel; // For broadcasting
    bool mbVisibleODF; // corresponds to ODF draw:display
    bool mbPrintableODF; // corresponds to ODF draw:display
    bool mbLockedODF; // corresponds to ODF draw:protected
    SdrLayerID m_nID;

    SdrLayer(SdrLayerID nNewID, OUString aNewName);

public:
    bool operator==(const SdrLayer& rCmpLayer) const;

    void SetName(const OUString& rNewName);
    const OUString& GetName() const { return maName; }

    void SetTitle(const OUString& rTitle) { maTitle = rTitle; }
    const OUString& GetTitle() const { return maTitle; }

    void SetDescription(const OUString& rDesc) { maDescription = rDesc; }
    const OUString& GetDescription() const { return maDescription; }

    void SetVisibleODF(bool bVisibleODF) { mbVisibleODF = bVisibleODF; }
    bool IsVisibleODF() const { return mbVisibleODF; }

    void SetPrintableODF(bool bPrintableODF) { mbPrintableODF = bPrintableODF; }
    bool IsPrintableODF() const { return mbPrintableODF; }

    void SetLockedODF(bool bLockedODF) { mbLockedODF = bLockedODF; }
    bool IsLockedODF() const { return mbLockedODF; }

    SdrLayerID    GetID() const                               { return m_nID; }
    void          SetModel(SdrModel* pNewModel)               { m_pModel=pNewModel; }
};

#define SDRLAYER_MAXCOUNT 255
#define SDRLAYERPOS_NOTFOUND 0xffff

// When Changing the layer data you currently have to set the Modify flag manually
class SVXCORE_DLLPUBLIC SdrLayerAdmin {
friend class SdrView;
friend class SdrModel;
friend class SdrPage;

    std::vector<std::unique_ptr<SdrLayer>> maLayers;
    SdrLayerAdmin* m_pParent; // The page's admin knows the doc's admin
    SdrModel* m_pModel; // For broadcasting
    OUString maControlLayerName;
    // Find a LayerID which is not in use yet. If all have been used up,
    // we return 0.
    // If you want to play safe, check GetLayerCount()<SDRLAYER_MAXCOUNT
    // first, else all are given away already.
    SAL_DLLPRIVATE SdrLayerID         GetUniqueLayerID() const;
    SAL_DLLPRIVATE void               Broadcast() const;
public:
    SAL_DLLPRIVATE explicit SdrLayerAdmin(SdrLayerAdmin* pNewParent=nullptr);
    SdrLayerAdmin(const SdrLayerAdmin& rSrcLayerAdmin);
    ~SdrLayerAdmin();
    SAL_DLLPRIVATE SdrLayerAdmin& operator=(const SdrLayerAdmin& rSrcLayerAdmin);

    SAL_DLLPRIVATE void               SetModel(SdrModel* pNewModel);

    SAL_DLLPRIVATE void               InsertLayer(std::unique_ptr<SdrLayer> pLayer, sal_uInt16 nPos);
    SAL_DLLPRIVATE std::unique_ptr<SdrLayer> RemoveLayer(sal_uInt16 nPos);

    // Delete all layers
    SAL_DLLPRIVATE void               ClearLayers();

    // New layer is created and inserted
    SdrLayer*          NewLayer(const OUString& rName, sal_uInt16 nPos=0xFFFF);

    // Iterate over all layers
    sal_uInt16         GetLayerCount() const                                         { return sal_uInt16(maLayers.size()); }

    SdrLayer*          GetLayer(sal_uInt16 i)                                        { return maLayers[i].get(); }
    const SdrLayer*    GetLayer(sal_uInt16 i) const                                  { return maLayers[i].get(); }

    SAL_DLLPRIVATE sal_uInt16         GetLayerPos(const SdrLayer* pLayer) const;

    SdrLayer*          GetLayer(const OUString& rName);
    const SdrLayer*    GetLayer(const OUString& rName) const;
    SdrLayerID         GetLayerID(const OUString& rName) const;
    SdrLayer*          GetLayerPerID(SdrLayerID nID) { return const_cast<SdrLayer*>(const_cast<const SdrLayerAdmin*>(this)->GetLayerPerID(nID)); }
    const SdrLayer*    GetLayerPerID(SdrLayerID nID) const;

    void               SetControlLayerName(const OUString& rNewName);
    const OUString&    GetControlLayerName() const { return maControlLayerName; }

    // Removes all elements in rOutSet and then adds all IDs of layers from member aLayer
    // that fulfill the criterion visible, printable, or locked respectively.
    void               getVisibleLayersODF( SdrLayerIDSet& rOutSet) const;
    void               getPrintableLayersODF( SdrLayerIDSet& rOutSet) const;
    void               getLockedLayersODF( SdrLayerIDSet& rOutSet) const;

    // Generates a bitfield for settings.xml from the SdrLayerIDSet.
    // Output is a UNO sequence of BYTE (which is 'short' in API).
    void               QueryValue(const SdrLayerIDSet& rViewLayerSet, css::uno::Any& rAny);
};

#endif // INCLUDED_SVX_SVDLAYER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

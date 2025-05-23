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


// #i13033#
// New mechanism to hold a list of all original and cloned objects for later
// re-creating the connections for contained connectors
#include <clonelist.hxx>
#include <svx/svdoedge.hxx>
#include <svx/svdpage.hxx>

void CloneList::AddPair(const SdrObject* pOriginal, SdrObject* pClone)
{
    maOriginalList.push_back(pOriginal);
    maCloneList.push_back(pClone);

    // look for subobjects, too.
    bool bOriginalIsGroup(pOriginal->IsGroupObject());
    bool bCloneIsGroup(pClone->IsGroupObject());

    if(bOriginalIsGroup && DynCastE3dObject(pOriginal) != nullptr && DynCastE3dScene(pOriginal) == nullptr )
        bOriginalIsGroup = false;

    if(bCloneIsGroup && DynCastE3dObject(pClone) != nullptr && DynCastE3dScene(pClone) == nullptr)
        bCloneIsGroup = false;

    if(!(bOriginalIsGroup && bCloneIsGroup))
        return;

    const SdrObjList* pOriginalList = pOriginal->GetSubList();
    SdrObjList* pCloneList = pClone->GetSubList();

    if(pOriginalList && pCloneList
        && pOriginalList->GetObjCount() == pCloneList->GetObjCount())
    {
        for(size_t a = 0; a < pOriginalList->GetObjCount(); ++a)
        {
            // recursive call
            AddPair(pOriginalList->GetObj(a), pCloneList->GetObj(a));
        }
    }
}

const SdrObject* CloneList::GetOriginal(sal_uInt32 nIndex) const
{
    return maOriginalList[nIndex];
}

SdrObject* CloneList::GetClone(sal_uInt32 nIndex) const
{
    return maCloneList[nIndex];
}

void CloneList::CopyConnections() const
{
    sal_uInt32 cloneCount = maCloneList.size();

    for(size_t a = 0; a < maOriginalList.size(); a++)
    {
        const SdrEdgeObj* pOriginalEdge = dynamic_cast<const SdrEdgeObj*>( GetOriginal(a) );
        SdrEdgeObj* pCloneEdge = dynamic_cast<SdrEdgeObj*>( GetClone(a) );

        if(pOriginalEdge && pCloneEdge)
        {
            for (bool bTail1 : { true, false })
            {
                SdrObject* pOriginalNode = pOriginalEdge->GetConnectedNode(bTail1);
                if (pOriginalNode)
                {
                    std::vector<const SdrObject*>::const_iterator it = std::find(maOriginalList.begin(),
                                                                     maOriginalList.end(),
                                                                     pOriginalNode);

                    if(it != maOriginalList.end())
                    {
                        sal_uInt32 nPos = it - maOriginalList.begin();
                        SdrObject *cObj = nullptr;

                        if (nPos < cloneCount)
                            cObj = GetClone(nPos);

                        if(pOriginalNode != cObj)
                            pCloneEdge->ConnectToNode(bTail1, cObj);
                    }
                }
            }
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

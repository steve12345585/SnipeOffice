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

#include <noteurl.hxx>

#include <vcl/imap.hxx>
#include <vcl/imaprect.hxx>
#include <vcl/mapmod.hxx>
#include <vcl/outdev.hxx>

// Global variable
thread_local SwNoteURL* pNoteURL = nullptr;

void SwNoteURL::InsertURLNote(const OUString& rURL, const OUString& rTarget, const SwRect& rRect)
{
    const size_t nCount = m_List.size();
    for (size_t i = 0; i < nCount; ++i)
        if (rRect == m_List[i].GetRect())
            return;

    m_List.emplace_back(rURL, rTarget, rRect);
}

void SwNoteURL::FillImageMap(ImageMap* pMap, const Point& rPos, const MapMode& rMap)
{
    assert(pMap && "FillImageMap: No ImageMap, no cookies!");
    const size_t nCount = m_List.size();
    if (nCount)
    {
        MapMode aMap(MapUnit::Map100thMM);
        for (size_t i = 0; i < nCount; ++i)
        {
            const SwURLNote& rNote = m_List[i];
            SwRect aSwRect(rNote.GetRect());
            aSwRect -= rPos;
            tools::Rectangle aRect(OutputDevice::LogicToLogic(aSwRect.SVRect(), rMap, aMap));
            IMapRectangleObject aObj(aRect, rNote.GetURL(), OUString(), OUString(),
                                     rNote.GetTarget(), OUString(), true, false);
            pMap->InsertIMapObject(aObj);
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

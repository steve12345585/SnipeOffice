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

#include <vcl/dllapi.h>
#include <boost/container/small_vector.hpp>

// used for managing runs e.g. for BiDi, glyph and script fallback
class VCL_DLLPUBLIC ImplLayoutRuns
{
public:
    struct Run
    {
        int m_nMinRunPos;
        int m_nEndRunPos;
        bool m_bRTL;

        Run(int nMinRunPos, int nEndRunPos, bool bRTL)
            : m_nMinRunPos(nMinRunPos)
            , m_nEndRunPos(nEndRunPos)
            , m_bRTL(bRTL)
        {
        }

        inline bool Contains(int nCharPos) const
        {
            return (m_nMinRunPos <= nCharPos) && (nCharPos < m_nEndRunPos);
        }

        bool operator==(const Run&) const = default;
    };

private:
    int mnRunIndex;
    boost::container::small_vector<Run, 8> maRuns;

public:
    ImplLayoutRuns() { mnRunIndex = 0; }

    void Clear() { maRuns.clear(); }
    void AddPos(int nCharPos, bool bRTL);
    void AddRun(int nMinRunPos, int nEndRunPos, bool bRTL);

    void Normalize();
    void ReverseTail(size_t nTailIndex);

    bool IsEmpty() const { return maRuns.empty(); }
    void ResetPos() { mnRunIndex = 0; }
    void NextRun() { ++mnRunIndex; }
    bool GetRun(int* nMinRunPos, int* nEndRunPos, bool* bRTL) const;
    bool GetNextPos(int* nCharPos, bool* bRTL);
    bool PosIsInRun(int nCharPos) const;
    bool PosIsInAnyRun(int nCharPos) const;

    inline auto begin() const { return maRuns.begin(); }
    inline auto end() const { return maRuns.end(); }
    inline const auto& at(size_t nIndex) const { return maRuns.at(nIndex); }
    inline auto size() const { return maRuns.size(); }

    static void PrepareFallbackRuns(ImplLayoutRuns* paRuns, ImplLayoutRuns* paFallbackRuns);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <algorithm>

#include <hintids.hxx>
#include <editeng/lrspitem.hxx>
#include <osl/diagnose.h>

#include <colmgr.hxx>
#include <fmtfsize.hxx>
#include <swtypes.hxx>

// private methods

// set column width to current width
void FitToActualSize(SwFormatCol& rCol, sal_uInt16 nWidth)
{
    const sal_uInt16 nCount = rCol.GetColumns().size();
    for (sal_uInt16 i = 0; i < nCount; ++i)
    {
        const sal_uInt16 nTmp = rCol.CalcColWidth(i, nWidth);
        auto& col = rCol.GetColumns()[i];
        col.SetWishWidth(nTmp);
        // If necessary, shrink borders (as equally as possible) to keep up the invariant that
        // GetWishWidth() >= GetLeft() + GetRight():
        sal_uInt32 const borders = col.GetLeft() + col.GetRight();
        if (borders > nTmp)
        {
            auto const shrink = borders - nTmp;
            auto const half = shrink / 2; // rounds down
            if (col.GetLeft() < col.GetRight())
            {
                auto const shrinkLeft = std::min(sal_uInt32(col.GetLeft()), half);
                col.SetLeft(col.GetLeft() - shrinkLeft);
                col.SetRight(col.GetRight() - (shrink - shrinkLeft));
            }
            else
            {
                auto const shrinkRight = std::min(sal_uInt32(col.GetRight()), half);
                col.SetLeft(col.GetLeft() - (shrink - shrinkRight));
                col.SetRight(col.GetRight() - shrinkRight);
            }
        }
    }
    rCol.SetWishWidth(nWidth);
}

// public methods

// set column quantity and Gutterwidth
void SwColMgr::SetCount(sal_uInt16 nCount, sal_uInt16 nGutterWidth)
{
    m_aFormatCol.Init(nCount, nGutterWidth, m_nWidth);
    m_aFormatCol.SetWishWidth(m_nWidth);
    m_aFormatCol.SetGutterWidth(nGutterWidth, m_nWidth);
}

sal_uInt16 SwColMgr::GetGutterWidth(sal_uInt16 nPos) const
{
    sal_uInt16 nRet;
    if (nPos == USHRT_MAX)
        nRet = GetCount() > 1 ? m_aFormatCol.GetGutterWidth() : DEF_GUTTER_WIDTH;
    else
    {
        OSL_ENSURE(nPos < GetCount() - 1, "column overindexed");
        const SwColumns& rCols = m_aFormatCol.GetColumns();
        nRet = rCols[nPos].GetRight() + rCols[nPos + 1].GetLeft();
    }
    return nRet;
}

void SwColMgr::SetGutterWidth(sal_uInt16 nGutterWidth, sal_uInt16 nPos)
{
    if (nPos == USHRT_MAX)
        m_aFormatCol.SetGutterWidth(nGutterWidth, m_nWidth);
    else
    {
        OSL_ENSURE(nPos < GetCount() - 1, "column overindexed");
        SwColumns& rCols = m_aFormatCol.GetColumns();
        sal_uInt16 nGutterWidth2 = nGutterWidth / 2;
        rCols[nPos].SetRight(nGutterWidth2);
        rCols[nPos + 1].SetLeft(nGutterWidth2);
    }
}

// height separation line
short SwColMgr::GetLineHeightPercent() const
{
    return static_cast<short>(m_aFormatCol.GetLineHeight());
}
void SwColMgr::SetLineHeightPercent(short nPercent)
{
    OSL_ENSURE(nPercent <= 100, "line height may be at most 100%");
    m_aFormatCol.SetLineHeight(static_cast<sal_uInt8>(nPercent));
}

// column width
sal_uInt16 SwColMgr::GetColWidth(sal_uInt16 nIdx) const
{
    OSL_ENSURE(nIdx < GetCount(), "Column array overindexed.");
    return m_aFormatCol.CalcPrtColWidth(nIdx, m_nWidth);
}

void SwColMgr::SetColWidth(sal_uInt16 nIdx, sal_uInt16 nWd)
{
    OSL_ENSURE(nIdx < GetCount(), "Column array overindexed.");
    m_aFormatCol.GetColumns()[nIdx].SetWishWidth(nWd);
}

// newly set size
void SwColMgr::SetActualWidth(sal_uInt16 nW)
{
    m_nWidth = nW;
    ::FitToActualSize(m_aFormatCol, nW);
}

// ctor
SwColMgr::SwColMgr(const SfxItemSet& rSet)
    : m_aFormatCol(rSet.Get(RES_COL))
{
    m_nWidth = o3tl::narrowing<sal_uInt16>(rSet.Get(RES_FRM_SIZE).GetWidth());
    if (m_nWidth < MINLAY)
        m_nWidth = USHRT_MAX;
    const SvxLRSpaceItem& rLR = rSet.Get(RES_LR_SPACE);
    m_nWidth = m_nWidth - o3tl::narrowing<sal_uInt16>(rLR.ResolveLeft({}));
    m_nWidth = m_nWidth - o3tl::narrowing<sal_uInt16>(rLR.ResolveRight({}));
    ::FitToActualSize(m_aFormatCol, m_nWidth);
}

SwColMgr::~SwColMgr() {}

void SwColMgr::SetLineWidthAndColor(SvxBorderLineStyle eStyle, sal_uLong nLWidth, const Color& rCol)
{
    m_aFormatCol.SetLineStyle(eStyle);
    m_aFormatCol.SetLineWidth(nLWidth);
    m_aFormatCol.SetLineColor(rCol);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

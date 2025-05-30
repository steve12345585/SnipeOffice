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

#ifndef INCLUDED_SW_SOURCE_CORE_INC_HFFRM_HXX
#define INCLUDED_SW_SOURCE_CORE_INC_HFFRM_HXX

#include "layfrm.hxx"

class SwViewShell;

class SwHeadFootFrame : public SwLayoutFrame
{
private:
    std::vector<basegfx::B2DPolygon> GetSubsidiaryLinesPolygons(const SwViewShell& rViewShell) const;

protected:
    void FormatSize(SwTwips nUL, const SwBorderAttrs * pAttrs);
    void FormatPrt(SwTwips & nUL, const SwBorderAttrs * pAttrs);
    inline bool GetEatSpacing() const; // in hffrm.cxx

public:
    SwHeadFootFrame(SwFrameFormat * pFrame, SwFrame*, SwFrameType aType);
    virtual void Format( vcl::RenderContext* pRenderContext, const SwBorderAttrs *pAttrs = nullptr ) override;
    virtual SwTwips GrowFrame( SwTwips,
                               SwResizeLimitReason&, bool bTst, bool bInfo ) override;
    virtual SwTwips ShrinkFrame( SwTwips,
                               bool bTst = false, bool bInfo = false ) override;
    virtual void PaintSubsidiaryLines( const SwPageFrame*, const SwRect& ) const override;
    void AddSubsidiaryLinesBounds(const SwViewShell& rViewShell, RectangleVector& rRects) const;
};

/// Header in the document layout, inside a page.
class SwHeaderFrame final : public SwHeadFootFrame
{
public:
    SwHeaderFrame( SwFrameFormat* pFrame, SwFrame* pSib ) : SwHeadFootFrame(pFrame, pSib, SwFrameType::Header) {};

    void dumpAsXml(xmlTextWriterPtr writer = nullptr) const override;
};

/// Footer in the document layout, inside a page.
class SwFooterFrame final : public SwHeadFootFrame
{
public:
    SwFooterFrame( SwFrameFormat* pFrame, SwFrame* pSib ) : SwHeadFootFrame(pFrame, pSib, SwFrameType::Footer) {};

    void dumpAsXml(xmlTextWriterPtr writer = nullptr) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

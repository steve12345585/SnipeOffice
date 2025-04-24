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

#include <o3tl/sorted_vector.hxx>
#include <o3tl/typed_flags_set.hxx>
#include <sal/types.h>
#include <ndindex.hxx>
#include <memory>

class SdrObject;
class SwFrameFormat;
class SwPosFlyFrame;

// ATTENTION: The values of this enum are used directly in the output table!!!
enum SwHTMLFrameType
{
    HTML_FRMTYPE_TABLE,
    HTML_FRMTYPE_TABLE_CAP,
    HTML_FRMTYPE_MULTICOL,
    HTML_FRMTYPE_EMPTY,
    HTML_FRMTYPE_TEXT,
    HTML_FRMTYPE_GRF,
    HTML_FRMTYPE_PLUGIN,
    HTML_FRMTYPE_APPLET,
    HTML_FRMTYPE_IFRAME,
    HTML_FRMTYPE_OLE,
    HTML_FRMTYPE_MARQUEE,
    HTML_FRMTYPE_CONTROL,
    HTML_FRMTYPE_DRAW,
    HTML_FRMTYPE_END
};

enum class HtmlOut {
    TableNode,
    GraphicNode,
    OleNode,
    Div,
    MultiCol,
    Spacer,
    Control,
    AMarquee,
    Marquee,
    GraphicFrame,
    OleGraphic,
    Span,
    InlineHeading
};

enum class HtmlPosition {
    Prefix,
    Before,
    Inside,
    Any
};

enum class HtmlContainerFlags {
    NONE     = 0x00,
    Span     = 0x01,
    Div      = 0x02,
};
namespace o3tl {
    template<> struct typed_flags<HtmlContainerFlags> : is_typed_flags<HtmlContainerFlags, 0x03> {};
}

struct AllHtmlFlags {
    HtmlOut            nOut;
    HtmlPosition       nPosition;
    HtmlContainerFlags nContainer;
};

const AllHtmlFlags & getHTMLOutFramePageFlyTable(SwHTMLFrameType eFrameType, sal_uInt16 nExportMode);
const AllHtmlFlags & getHTMLOutFrameParaFrameTable(SwHTMLFrameType eFrameType, sal_uInt16 nExportMode);
const AllHtmlFlags & getHTMLOutFrameParaPrtAreaTable(SwHTMLFrameType eFrameType, sal_uInt16 nExportMode);
const AllHtmlFlags & getHTMLOutFrameParaOtherTable(SwHTMLFrameType eFrameType, sal_uInt16 nExportMode);
const AllHtmlFlags & getHTMLOutFrameAsCharTable(SwHTMLFrameType eFrameType, sal_uInt16 nExportMode);

class SwHTMLPosFlyFrame
{
    const SwFrameFormat    *m_pFrameFormat;  // the frame
    const SdrObject        *m_pSdrObject;    // maybe Sdr-Object
    SwNodeIndex             m_aNodeIndex;    // Node-Index
    sal_uInt32              m_nOrdNum;       // from SwPosFlyFrame
    sal_Int32               m_nContentIndex;   // its position in content
    AllHtmlFlags            m_nAllFlags;

    SwHTMLPosFlyFrame(const SwHTMLPosFlyFrame&) = delete;
    SwHTMLPosFlyFrame& operator=(const SwHTMLPosFlyFrame&) = delete;

public:

    SwHTMLPosFlyFrame( const SwPosFlyFrame& rPosFly,
                     const SdrObject *pSdrObj, AllHtmlFlags nAllFlags );

    bool operator<( const SwHTMLPosFlyFrame& ) const;

    const SwFrameFormat& GetFormat() const       { return *m_pFrameFormat; }
    const SdrObject*     GetSdrObject() const    { return m_pSdrObject; }
    const SwNodeIndex&   GetNdIndex() const      { return m_aNodeIndex; }
    sal_Int32            GetContentIndex() const { return m_nContentIndex; }
    AllHtmlFlags const & GetOutMode() const      { return m_nAllFlags; }
    HtmlOut              GetOutFn() const        { return m_nAllFlags.nOut; }
    HtmlPosition         GetOutPos() const       { return m_nAllFlags.nPosition; }
};

class SwHTMLPosFlyFrames
    : public o3tl::sorted_vector<std::unique_ptr<SwHTMLPosFlyFrame>,
                o3tl::less_ptr_to,
                o3tl::find_partialorder_ptrequals>
{};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

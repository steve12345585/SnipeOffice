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

#include "TransformerActionInit.hxx"
#include "AttrTransformerAction.hxx"

enum XMLPropOASISTransformerAction
{
    XML_OPTACTION_LINE_MODE = XML_ATACTION_USER_DEFINED,
    XML_OPTACTION_UNDERLINE_TYPE,
    XML_OPTACTION_UNDERLINE_STYLE,
    XML_OPTACTION_UNDERLINE_WIDTH,
    XML_OPTACTION_LINETHROUGH_TYPE,
    XML_OPTACTION_LINETHROUGH_STYLE,
    XML_OPTACTION_LINETHROUGH_WIDTH,
    XML_OPTACTION_LINETHROUGH_TEXT,
    XML_OPTACTION_KEEP_WITH_NEXT,
    XML_OPTACTION_INTERPOLATION,
    XML_OPTACTION_INTERVAL_MAJOR,
    XML_OPTACTION_INTERVAL_MINOR_DIVISOR,
    XML_OPTACTION_SYMBOL_TYPE,
    XML_OPTACTION_SYMBOL_NAME,
    XML_OPTACTION_OPACITY,
    XML_OPTACTION_IMAGE_OPACITY,
    XML_OPTACTION_KEEP_TOGETHER,
    XML_OPTACTION_CONTROL_TEXT_ALIGN,
    XML_ATACTION_CAPTION_ESCAPE_OASIS,
    XML_ATACTION_DECODE_PROTECT
};

extern XMLTransformerActionInit const aGraphicPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aDrawingPagePropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aPageLayoutPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aHeaderFooterPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aTextPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aParagraphPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aSectionPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aTablePropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aTableColumnPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aTableRowPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aTableCellPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aListLevelPropertyOASISAttrActionTable[];
extern XMLTransformerActionInit const aChartPropertyOASISAttrActionTable[];

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

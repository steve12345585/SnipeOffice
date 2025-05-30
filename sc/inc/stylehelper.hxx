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

#include <rtl/ustring.hxx>
#include "scdllapi.h"

//  conversion programmatic <-> display (visible) name
//  currently, the core always has the visible names
//  the api is required to use programmatic names for default styles
//  these programmatic names must never change!

inline constexpr OUString SC_STYLE_PROG_STANDARD = u"Default"_ustr;
inline constexpr OUString SC_STYLE_PROG_RESULT = u"Result"_ustr;
inline constexpr OUString SC_STYLE_PROG_RESULT1 = u"Result2"_ustr;
inline constexpr OUString SC_STYLE_PROG_HEADING = u"Heading"_ustr;
inline constexpr OUString SC_STYLE_PROG_HEADING1 = u"Heading1"_ustr;
inline constexpr OUString SC_STYLE_PROG_REPORT = u"Report"_ustr;

inline constexpr OUString SC_PIVOT_STYLE_PROG_INNER = u"Pivot Table Value"_ustr;
inline constexpr OUString SC_PIVOT_STYLE_PROG_RESULT = u"Pivot Table Result"_ustr;
inline constexpr OUString SC_PIVOT_STYLE_PROG_CATEGORY = u"Pivot Table Category"_ustr;
inline constexpr OUString SC_PIVOT_STYLE_PROG_TITLE = u"Pivot Table Title"_ustr;
inline constexpr OUString SC_PIVOT_STYLE_PROG_FIELDNAME = u"Pivot Table Field"_ustr;
inline constexpr OUString SC_PIVOT_STYLE_PROG_TOP = u"Pivot Table Corner"_ustr;

enum class SfxStyleFamily;

class ScStyleNameConversion
{
public:
    static OUString DisplayToProgrammaticName(const OUString& rDispName, SfxStyleFamily nType);
    static SC_DLLPUBLIC OUString ProgrammaticToDisplayName(const OUString& rProgName,
                                                           SfxStyleFamily nType);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

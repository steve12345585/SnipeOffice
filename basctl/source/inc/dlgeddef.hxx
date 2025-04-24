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

namespace basctl
{

// control properties
#define DLGED_PROP_BACKGROUNDCOLOR   "BackgroundColor"
inline constexpr OUString DLGED_PROP_DROPDOWN = u"Dropdown"_ustr;
inline constexpr OUString DLGED_PROP_FORMATSSUPPLIER = u"FormatsSupplier"_ustr;
inline constexpr OUString DLGED_PROP_HEIGHT = u"Height"_ustr;
inline constexpr OUString DLGED_PROP_LABEL = u"Label"_ustr;
inline constexpr OUString DLGED_PROP_NAME = u"Name"_ustr;
inline constexpr OUString DLGED_PROP_ORIENTATION = u"Orientation"_ustr;
inline constexpr OUString DLGED_PROP_POSITIONX = u"PositionX"_ustr;
inline constexpr OUString DLGED_PROP_POSITIONY = u"PositionY"_ustr;
inline constexpr OUString DLGED_PROP_STEP = u"Step"_ustr;
inline constexpr OUString DLGED_PROP_TABINDEX = u"TabIndex"_ustr;
#define DLGED_PROP_TEXTCOLOR         "TextColor"
#define DLGED_PROP_TEXTLINECOLOR     "TextLineColor"
inline constexpr OUString DLGED_PROP_WIDTH = u"Width"_ustr;
inline constexpr OUString DLGED_PROP_DECORATION = u"Decoration"_ustr;


} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

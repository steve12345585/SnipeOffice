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

#ifndef INCLUDED_VCL_INPUTTYPES_HXX
#define INCLUDED_VCL_INPUTTYPES_HXX

#include <o3tl/typed_flags_set.hxx>

enum class VclInputFlags {
    NONE                  = 0x0000,
    MOUSE                 = 0x0001,
    KEYBOARD              = 0x0002,
    PAINT                 = 0x0004,
    TIMER                 = 0x0008,
    OTHER                 = 0x0010,
    APPEVENT              = 0x0020,
};
namespace o3tl
{
    template<> struct typed_flags<VclInputFlags> : is_typed_flags<VclInputFlags, 0x003f> {};
}

#define VCL_INPUT_ANY                 (VclInputFlags::MOUSE | VclInputFlags::KEYBOARD | VclInputFlags::PAINT | VclInputFlags::TIMER | VclInputFlags::OTHER | VclInputFlags::APPEVENT)

#endif // INCLUDED_VCL_INPUTTYPES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

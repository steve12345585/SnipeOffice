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

#ifndef INCLUDED_SVX_FRAMEBORDERTYPE_HXX
#define INCLUDED_SVX_FRAMEBORDERTYPE_HXX

#include <svx/svxdllapi.h>

namespace svx {


/** Enumerates all borders a frame selection control can contain. */
enum class FrameBorderType
{
    NONE,       /// No border (special state).
    Left,       /// Left frame border.
    Right,      /// Right frame border.
    Top,        /// Top frame border.
    Bottom,     /// Bottom frame border.
    Horizontal, /// Inner horizontal frame border.
    Vertical,   /// Inner vertical frame border.
    TLBR,       /// Top-left to bottom-right frame border.
    BLTR        /// Bottom-left to top-right frame border.
};

/** The number of valid frame border types (excluding FrameBorderType::NONE). */
const int FRAMEBORDERTYPE_COUNT = 8;

/** Returns the frame border type from a 0-based integer index. */
SVX_DLLPUBLIC FrameBorderType GetFrameBorderTypeFromIndex( size_t nIndex );

/** Returns the zero-based index of a valid frame border type. */
size_t GetIndexFromFrameBorderType( FrameBorderType eBorder );


}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

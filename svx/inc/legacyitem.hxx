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
#ifndef INCLUDED_SVX_LEGACYITEM_HXX
#define INCLUDED_SVX_LEGACYITEM_HXX

#include <sal/types.h>

//////////////////////////////////////////////////////////////////////////////
// // svx
//     SvxOrientationItem aOrientation( aRotateAngle.GetValue(), aStacked.GetValue(), 0 );
//     SvxMarginItem               aMargin;
//     SvxRotateModeItem           aRotateMode;
//////////////////////////////////////////////////////////////////////////////

class SvStream;
class SvxOrientationItem;
class SvxMarginItem;
class SvxRotateModeItem;

namespace legacy
{
    namespace SvxOrientation
    {
        sal_uInt16 GetVersion(sal_uInt16 nFileFormatVersion);
        void Create(SvxOrientationItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
        SvStream& Store(const SvxOrientationItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
    }
    namespace SvxMargin
    {
        sal_uInt16 GetVersion(sal_uInt16 nFileFormatVersion);
        void Create(SvxMarginItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
        SvStream& Store(const SvxMarginItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
    }
    namespace SvxRotateMode
    {
        sal_uInt16 GetVersion(sal_uInt16 nFileFormatVersion);
        void Create(SvxRotateModeItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
        SvStream& Store(const SvxRotateModeItem& rItem, SvStream& rStrm, sal_uInt16 nItemVersion);
    }
}

#endif // INCLUDED_SVX_LEGACYITEM_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

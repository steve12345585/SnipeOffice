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

#ifndef INCLUDED_SVX_XLNTRIT_HXX
#define INCLUDED_SVX_XLNTRIT_HXX

#include <svl/intitem.hxx>
#include <svx/svxdllapi.h>

/*************************************************************************
|*
|* Transparency item for lines
|*
\************************************************************************/

class SVXCORE_DLLPUBLIC XLineTransparenceItem final : public SfxUInt16Item
{
public:
                            DECLARE_ITEM_TYPE_FUNCTION(XLineTransparenceItem)
                            XLineTransparenceItem(sal_uInt16 nLineTransparence = 0);
    virtual XLineTransparenceItem* Clone(SfxItemPool* pPool = nullptr) const override;
    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText, const IntlWrapper& ) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

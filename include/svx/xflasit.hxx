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

#ifndef INCLUDED_SVX_XFLASIT_HXX
#define INCLUDED_SVX_XFLASIT_HXX

#include <sal/config.h>

#include <svl/setitem.hxx>
#include <svx/svxdllapi.h>

// SetItem with all ItemSets of fill-attributes
class SVXCORE_DLLPUBLIC XFillAttrSetItem final : public SfxSetItem
{
public:
                            DECLARE_ITEM_TYPE_FUNCTION(XFillAttrSetItem)
                            XFillAttrSetItem(SfxItemSet&& pItemSet );
                            XFillAttrSetItem(SfxItemPool* pItemPool);
                            XFillAttrSetItem(const XFillAttrSetItem& rAttr);
                            XFillAttrSetItem(const XFillAttrSetItem& rAttr,
                                             SfxItemPool* pItemPool);
    virtual XFillAttrSetItem* Clone( SfxItemPool* pToPool = nullptr ) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

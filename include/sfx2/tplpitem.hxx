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
#ifndef INCLUDED_SFX2_TPLPITEM_HXX
#define INCLUDED_SFX2_TPLPITEM_HXX

#include <rtl/ustring.hxx>
#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <svl/flagitem.hxx>
#include <svl/style.hxx>

class SFX2_DLLPUBLIC SfxTemplateItem final : public SfxFlagItem
{
    OUString aStyle;
    OUString aStyleIdentifier;
public:
    static SfxPoolItem* CreateDefault();
    DECLARE_ITEM_TYPE_FUNCTION(SfxTemplateItem)
    SfxTemplateItem();
    SfxTemplateItem( sal_uInt16 nWhich,
                     OUString aStyle,
                     OUString aStyleIdentifier = u""_ustr );

    const OUString&         GetStyleName() const { return aStyle; }
    const OUString&         GetStyleIdentifier() const { return aStyleIdentifier; }

    virtual SfxTemplateItem* Clone( SfxItemPool *pPool = nullptr ) const override;
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual sal_uInt8       GetFlagCount() const override;
    virtual bool            QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool            PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;
    SfxStyleSearchBits      GetValue() const { return static_cast<SfxStyleSearchBits>(SfxFlagItem::GetValue()); }
    void                    SetValue(SfxStyleSearchBits n) { SfxFlagItem::SetValue(static_cast<sal_uInt16>(n)); }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

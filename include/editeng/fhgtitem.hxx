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
#ifndef INCLUDED_EDITENG_FHGTITEM_HXX
#define INCLUDED_EDITENG_FHGTITEM_HXX

#include <svl/poolitem.hxx>
#include <tools/debug.hxx>
#include <editeng/editengdllapi.h>

// class SvxFontHeightItem -----------------------------------------------

// Warning: twips values

/*  [Description]

    This item describes the font height
*/

constexpr sal_uInt16 FONTHEIGHT_16_VERSION = 0x0001;
constexpr sal_uInt16 FONTHEIGHT_UNIT_VERSION = 0x0002;

class EDITENG_DLLPUBLIC SvxFontHeightItem final : public SfxPoolItem
{
    sal_uInt32  nHeight;
    sal_uInt16  nProp;       // default 100%
    MapUnit ePropUnit;       // Percent, Twip, ...

    friend void Create_legacy_direct_set(SvxFontHeightItem& rItem, sal_uInt32 nH, sal_uInt16 nP, MapUnit eP);
    void legacy_direct_set(sal_uInt32 nH, sal_uInt16 nP, MapUnit eP) { nHeight = nH; nProp = nP; ePropUnit = eP; }

protected:
    virtual ItemInstanceManager* getItemInstanceManager() const override;

public:
    static SfxPoolItem* CreateDefault();

    DECLARE_ITEM_TYPE_FUNCTION(SvxFontHeightItem)
    SvxFontHeightItem( const sal_uInt32 nSz /*= 240*/, const sal_uInt16 nPropHeight /*= 100*/,
                       const sal_uInt16 nId  );

    // "pure virtual Methods" from SfxPoolItem
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual size_t          hashCode() const override;
    virtual bool            QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool            PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    virtual bool GetPresentation( SfxItemPresentation ePres,
                                    MapUnit eCoreMetric,
                                    MapUnit ePresMetric,
                                    OUString &rText, const IntlWrapper& ) const override;

    virtual SvxFontHeightItem*   Clone( SfxItemPool *pPool = nullptr ) const override;
    virtual void                 ScaleMetrics( tools::Long nMult, tools::Long nDiv ) override;
    virtual bool                 HasMetrics() const override;

    void SetHeight( sal_uInt32 nNewHeight, const sal_uInt16 nNewProp = 100,
                     MapUnit eUnit = MapUnit::MapRelative );

    void SetHeight( sal_uInt32 nNewHeight, sal_uInt16 nNewProp,
                     MapUnit eUnit, MapUnit eCoreUnit );

    sal_uInt32 GetHeight() const { return nHeight; }

    void SetProp( sal_uInt16 nNewProp, MapUnit eUnit )
        {
            ASSERT_CHANGE_REFCOUNTED_ITEM;
            nProp = nNewProp;
            ePropUnit = eUnit;
        }

    sal_uInt16 GetProp() const { return nProp; }

    MapUnit GetPropUnit() const { return ePropUnit;  }   // Percent, Twip, ...

    void dumpAsXml(xmlTextWriterPtr pWriter) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

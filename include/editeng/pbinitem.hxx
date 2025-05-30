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
#ifndef INCLUDED_EDITENG_PBINITEM_HXX
#define INCLUDED_EDITENG_PBINITEM_HXX

#include <svl/intitem.hxx>
#include <editeng/editengdllapi.h>

// define ----------------------------------------------------------------

#define PAPERBIN_PRINTER_SETTINGS   (sal_uInt8(0xFF))

// class SvxPaperBinItem -------------------------------------------------

/*  [Description]

    This item describes selecting a paper tray of the printer.
*/

class EDITENG_DLLPUBLIC SvxPaperBinItem final : public SfxByteItem
{
public:
    static SfxPoolItem* CreateDefault();
    DECLARE_ITEM_TYPE_FUNCTION(SvxPaperBinItem)
    explicit inline SvxPaperBinItem( const sal_uInt16 nId ,
                            const sal_uInt8 nTray = PAPERBIN_PRINTER_SETTINGS );

    // "pure virtual Methods" from SfxPoolItem
    virtual SvxPaperBinItem* Clone( SfxItemPool *pPool = nullptr ) const override;
    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText, const IntlWrapper& ) const override;
};

inline SvxPaperBinItem::SvxPaperBinItem( const sal_uInt16 nId, const sal_uInt8 nT )
    : SfxByteItem( nId, nT )
{}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

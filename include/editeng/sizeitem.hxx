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
#ifndef INCLUDED_EDITENG_SIZEITEM_HXX
#define INCLUDED_EDITENG_SIZEITEM_HXX

#include <tools/gen.hxx>
#include <svl/poolitem.hxx>
#include <editeng/editengdllapi.h>

// class SvxSizeItem -----------------------------------------------------

/*  [Description]

    This item describes a two-dimensional size.
*/

class EDITENG_DLLPUBLIC SvxSizeItem : public SfxPoolItem
{

    Size m_aSize;

public:
    static SfxPoolItem* CreateDefault();

    DECLARE_ITEM_TYPE_FUNCTION(SvxSizeItem)
    explicit SvxSizeItem( const sal_uInt16 nId );
    SvxSizeItem( const sal_uInt16 nId, const Size& rSize);

    // "pure virtual Methods" from SfxPoolItem
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual bool            QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool            PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText, const IntlWrapper& ) const override;

    virtual SvxSizeItem*     Clone( SfxItemPool *pPool = nullptr ) const override;
    virtual void             ScaleMetrics( tools::Long nMult, tools::Long nDiv ) override;
    virtual bool             HasMetrics() const override;

    const Size& GetSize() const { return m_aSize; }
    void        SetSize(const Size& rSize)
    { ASSERT_CHANGE_REFCOUNTED_ITEM; m_aSize = rSize; }

    tools::Long GetWidth() const { return m_aSize.getWidth();  }
    tools::Long GetHeight() const { return m_aSize.getHeight(); }
    void SetWidth(tools::Long n)
    { ASSERT_CHANGE_REFCOUNTED_ITEM; m_aSize.setWidth(n); }
    void SetHeight(tools::Long n)
    { ASSERT_CHANGE_REFCOUNTED_ITEM; m_aSize.setHeight(n); }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#ifndef INCLUDED_SVL_VOIDITEM_HXX
#define INCLUDED_SVL_VOIDITEM_HXX

#include <svl/poolitem.hxx>

class SVL_DLLPUBLIC SfxVoidItem final : public SfxPoolItem
{
public:
    static SfxPoolItem* CreateDefault();

    DECLARE_ITEM_TYPE_FUNCTION(SfxVoidItem)
    explicit SfxVoidItem(sal_uInt16 nWhich);
    SfxVoidItem(const SfxVoidItem& rCopy);
    SfxVoidItem(SfxVoidItem&& rOrig);
    virtual ~SfxVoidItem() override;

    SfxVoidItem& operator=(SfxVoidItem const&) = delete; // due to SfxPoolItem
    SfxVoidItem& operator=(SfxVoidItem&&) = delete; // due to SfxPoolItem

    virtual bool operator==(const SfxPoolItem&) const override;

    virtual bool GetPresentation(SfxItemPresentation ePres, MapUnit eCoreMetric,
                                 MapUnit ePresMetric, OUString& rText,
                                 const IntlWrapper&) const override;
    virtual void dumpAsXml(xmlTextWriterPtr pWriter) const override;

    // create a copy of itself
    virtual SfxVoidItem* Clone(SfxItemPool* pPool = nullptr) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

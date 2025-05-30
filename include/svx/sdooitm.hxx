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
#ifndef INCLUDED_SVX_SDOOITM_HXX
#define INCLUDED_SVX_SDOOITM_HXX

#include <svl/eitem.hxx>
#include <svx/svxdllapi.h>


// class SdrOnOffItem
// here GetValueTextByVal() returns "on" or "off" instead
// of "TRUE" or "FALSE"

class SVXCORE_DLLPUBLIC SdrOnOffItem: public SfxBoolItem {
public:
    DECLARE_ITEM_TYPE_FUNCTION(SdrOnOffItem)
    SdrOnOffItem(TypedWhichId<SdrOnOffItem> nId, bool bOn)
        : SfxBoolItem(nId, bOn) {}
    virtual SdrOnOffItem* Clone(SfxItemPool* pPool=nullptr) const override;

    virtual OUString GetValueTextByVal(bool bVal) const override;

    virtual bool GetPresentation(SfxItemPresentation ePres, MapUnit eCoreMetric, MapUnit ePresMetric, OUString& rText, const IntlWrapper&) const override;

    virtual void dumpAsXml(xmlTextWriterPtr pWriter) const override;
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

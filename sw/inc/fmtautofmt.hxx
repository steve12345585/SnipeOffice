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
#ifndef INCLUDED_SW_INC_FMTAUTOFMT_HXX
#define INCLUDED_SW_INC_FMTAUTOFMT_HXX

#include "hintids.hxx"
#include <svl/poolitem.hxx>
#include <memory>

class SfxItemSet;

/// Has a shared reference to an "auto-style", i.e. a not named collection of character formats. It
/// is owned by an SwTextAttrEnd, which is then stored in the SwpHints of an SwTextNode.
///
/// This is the primary way how direct character formats are stored inside a paragraph.
class SW_DLLPUBLIC SwFormatAutoFormat final : public SfxPoolItem
{
    std::shared_ptr<SfxItemSet> mpHandle;

public:
    DECLARE_ITEM_TYPE_FUNCTION(SwFormatAutoFormat)
    SwFormatAutoFormat( sal_uInt16 nWhich = RES_TXTATR_AUTOFMT );

    /// "pure virtual methods" of SfxPoolItem
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual SwFormatAutoFormat* Clone( SfxItemPool* pPool = nullptr ) const override;
    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText,
                                  const IntlWrapper& rIntl ) const override;

    virtual bool QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    void SetStyleHandle( const std::shared_ptr<SfxItemSet>& pHandle ) { mpHandle = pHandle; }
    const std::shared_ptr<SfxItemSet>& GetStyleHandle() const { return mpHandle; }

    void dumpAsXml(xmlTextWriterPtr pWriter) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

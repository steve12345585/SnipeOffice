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
#ifndef INCLUDED_SW_INC_FCHRFMT_HXX
#define INCLUDED_SW_INC_FCHRFMT_HXX

#include <svl/poolitem.hxx>
#include <svl/listener.hxx>
#include "format.hxx"
#include "charfmt.hxx"

class SwTextCharFormat;
class IntlWrapper;

/// This pool item subclass can appear in the hint array of a text node. It refers to a character
/// style. It's owned by SwTextCharFormat.
class SW_DLLPUBLIC SwFormatCharFormat final : public SfxPoolItem, public SvtListener
{
    friend class SwTextCharFormat;
    SwTextCharFormat* m_pTextAttribute;     ///< My text attribute.
    SwCharFormat* m_pCharFormat;

public:
    /// single argument ctors shall be explicit.
    DECLARE_ITEM_TYPE_FUNCTION(SwFormatCharFormat)
    explicit SwFormatCharFormat( SwCharFormat *pFormat );
    virtual ~SwFormatCharFormat() override;

    /// @@@ public copy ctor, but no copy assignment?
    SwFormatCharFormat( const SwFormatCharFormat& rAttr );

private:
    virtual void Notify(const SfxHint&) override;

    /// @@@ public copy ctor, but no copy assignment?
    SwFormatCharFormat & operator= (const SwFormatCharFormat &) = delete;
public:


    /// "pure virtual methods" of SfxPoolItem
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual SwFormatCharFormat* Clone( SfxItemPool* pPool = nullptr ) const override;
    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText,
                                  const IntlWrapper&    rIntl ) const override;

    virtual bool QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    void SetCharFormat( SwCharFormat* pFormat )
    {
        assert(!pFormat->IsDefault()); // expose cases that lead to use-after-free
        EndListeningAll();
        StartListening(pFormat->GetNotifier());
        m_pCharFormat = pFormat;
    }
    SwCharFormat* GetCharFormat() const { return m_pCharFormat; }

    void dumpAsXml(xmlTextWriterPtr pWriter) const override;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

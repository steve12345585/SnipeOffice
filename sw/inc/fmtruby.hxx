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
#ifndef INCLUDED_SW_INC_FMTRUBY_HXX
#define INCLUDED_SW_INC_FMTRUBY_HXX

#include "swdllapi.h"
#include <svl/poolitem.hxx>
#include <com/sun/star/text/RubyAdjust.hpp>

class SwTextRuby;

/// SfxPoolItem subclass that is owned by an SwTextRuby and contains info entered in Format -> Asian
/// Phonetic Guide. This is a character property, i.e. appears in the SwpHints of an SwTextNode.
class SW_DLLPUBLIC SwFormatRuby final : public SfxPoolItem
{
    friend class SwTextRuby;

    OUString m_sRubyText;                     ///< The ruby text.
    UIName m_sCharFormatName;                 ///< Name of the charformat.
    SwTextRuby* m_pTextAttr;                  ///< The TextAttribute.
    sal_uInt16 m_nCharFormatId;               ///< PoolId of the charformat.
    sal_uInt16 m_nPosition;                   ///< Position of the Ruby-character.
    css::text::RubyAdjust m_eAdjustment;      ///< Specific adjustment of the Ruby-ch.

public:
    DECLARE_ITEM_TYPE_FUNCTION(SwFormatRuby)
    SwFormatRuby( OUString aRubyText );
    SwFormatRuby( const SwFormatRuby& rAttr );
    virtual ~SwFormatRuby() override;

    SwFormatRuby& operator=( const SwFormatRuby& rAttr );

    // "Pure virtual methods" of SfxPoolItem.
    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual SwFormatRuby*   Clone( SfxItemPool* pPool = nullptr ) const override;

    virtual bool GetPresentation( SfxItemPresentation ePres,
                                  MapUnit eCoreMetric,
                                  MapUnit ePresMetric,
                                  OUString &rText,
                                  const IntlWrapper& rIntl ) const override;

    virtual bool QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    const SwTextRuby* GetTextRuby() const         { return m_pTextAttr; }

    const OUString& GetText() const                    { return m_sRubyText; }
    void SetText( const OUString& rText )        { m_sRubyText = rText; }

    const UIName& GetCharFormatName() const             { return m_sCharFormatName; }
    void SetCharFormatName( const UIName& rNm )  { m_sCharFormatName = rNm; }

    sal_uInt16 GetCharFormatId() const                 { return m_nCharFormatId; }
    void SetCharFormatId( sal_uInt16 nNew )            { m_nCharFormatId = nNew; }

    sal_uInt16 GetPosition() const                  { return m_nPosition; }
    void SetPosition( sal_uInt16 nNew )             { m_nPosition = nNew; }

    css::text::RubyAdjust GetAdjustment() const       { return m_eAdjustment; }
    void SetAdjustment( css::text::RubyAdjust nNew )  { m_eAdjustment = nNew; }
    void dumpAsXml(xmlTextWriterPtr pWriter) const override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

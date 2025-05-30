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

#pragma once

#include <svl/poolitem.hxx>
#include <unotools/configitem.hxx>
#include "scdllapi.h"

class SC_DLLPUBLIC ScPrintOptions
{
private:
    bool    bSkipEmpty;
    bool    bAllSheets;
    bool    bForceBreaks;

public:
                ScPrintOptions();

    bool    GetSkipEmpty() const            { return bSkipEmpty; }
    void    SetSkipEmpty( bool bVal )       { bSkipEmpty = bVal; }
    bool    GetAllSheets() const            { return bAllSheets; }
    void    SetAllSheets( bool bVal )       { bAllSheets = bVal; }
    bool    GetForceBreaks() const              { return bForceBreaks; }
    void    SetForceBreaks( bool bVal )     { bForceBreaks = bVal; }

    void    SetDefaults();

    bool                    operator== ( const ScPrintOptions& rOpt ) const;
};

// item for the dialog / options page

class SC_DLLPUBLIC ScTpPrintItem final : public SfxPoolItem
{
public:
                ScTpPrintItem( const ScPrintOptions& rOpt );
                virtual ~ScTpPrintItem() override;

    DECLARE_ITEM_TYPE_FUNCTION(ScTpPrintItem)
    ScTpPrintItem(ScTpPrintItem const &) = default;
    ScTpPrintItem(ScTpPrintItem &&) = default;
    ScTpPrintItem & operator =(ScTpPrintItem const &) = delete; // due to SfxPoolItem
    ScTpPrintItem & operator =(ScTpPrintItem &&) = delete; // due to SfxPoolItem

    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual ScTpPrintItem*  Clone( SfxItemPool *pPool = nullptr ) const override;

    const ScPrintOptions&   GetPrintOptions() const { return theOptions; }

private:
    ScPrintOptions theOptions;
};

// config item

class ScPrintCfg final : private ScPrintOptions, public utl::ConfigItem
{
private:
    static css::uno::Sequence<OUString> GetPropertyNames();
    void ReadCfg();
    virtual void    ImplCommit() override;

public:
            ScPrintCfg();

    const ScPrintOptions& GetOptions() const { return *this; }
    void            SetOptions( const ScPrintOptions& rNew );

    virtual void Notify( const css::uno::Sequence< OUString >& aPropertyNames ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

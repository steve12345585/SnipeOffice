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

#include "refreshtimer.hxx"
#include "address.hxx"
#include <sfx2/lnkbase.hxx>
#include "scdllapi.h"

class SfxObjectShell;
class ScDocShell;

class SAL_DLLPUBLIC_RTTI ScAreaLink final : public ::sfx2::SvBaseLink, public ScRefreshTimer
{
private:
    ScDocShell*     m_pDocSh;
    OUString        aFileName;
    OUString        aFilterName;
    OUString        aOptions;
    OUString        aSourceArea;
    ScRange         aDestArea;
    bool            bAddUndo;
    bool            bInCreate;
    bool            bDoInsert;      // is set to FALSE for first update
    static bool FindExtRange( ScRange& rRange, const ScDocument& rSrcDoc, const OUString& rAreaName );

public:
    SC_DLLPUBLIC ScAreaLink( ScDocShell* pShell, OUString aFile,
                    OUString aFilter, OUString aOpt,
                    OUString aArea, const ScRange& rDest, sal_Int32 nRefreshDelaySeconds );
    virtual ~ScAreaLink() override;

    virtual void Closed() override;
    virtual ::sfx2::SvBaseLink::UpdateResult DataChanged(
        const OUString& rMimeType, const css::uno::Any & rValue ) override;

    virtual void    Edit(weld::Window*, const Link<SvBaseLink&,void>& rEndEditHdl) override;

    bool    Refresh( const OUString& rNewFile, const OUString& rNewFilter,
                    const OUString& rNewArea, sal_Int32 nRefreshDelaySeconds );

    void    SetInCreate(bool bSet)                  { bInCreate = bSet; }
    void    SetDoInsert(bool bSet)                  { bDoInsert = bSet; }
    void    SetDestArea(const ScRange& rNew);
    void    SetSource(const OUString& rDoc, const OUString& rFlt, const OUString& rOpt,
                        const OUString& rArea);

    bool IsEqual( std::u16string_view rFile, std::u16string_view rFilter, std::u16string_view rOpt,
                  std::u16string_view rSource, const ScRange& rDest ) const;

    const OUString& GetFile() const         { return aFileName;     }
    const OUString& GetFilter() const       { return aFilterName;   }
    const OUString& GetOptions() const      { return aOptions;      }
    const OUString& GetSource() const       { return aSourceArea;   }
    const ScRange&  GetDestArea() const     { return aDestArea;     }

    DECL_DLLPRIVATE_LINK( RefreshHdl, Timer*, void );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

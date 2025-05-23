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

#include <rtl/ustring.hxx>
#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <svl/poolitem.hxx>
#include <config_features.h>

#if HAVE_FEATURE_SCRIPTING

class BasicManager;

class SFX2_DLLPUBLIC SfxMacroInfoItem final : public SfxPoolItem
{
    const BasicManager*     pBasicManager;
    OUString                aLibName;
    OUString                aModuleName;
    OUString                aMethodName;
    OUString                aCommentText;
    OUString aLocationName;

public:
    static SfxPoolItem* CreateDefault();
    DECLARE_ITEM_TYPE_FUNCTION(SfxMacroInfoItem)
    SfxMacroInfoItem( sal_uInt16 nWhich,
                    const BasicManager* pMgr,
                    OUString aLibName,
                    OUString aModuleName,
                    OUString aMethodName,
                    OUString aComment,
                    OUString aLocation = OUString());

    virtual SfxMacroInfoItem* Clone( SfxItemPool *pPool = nullptr ) const override;
    virtual bool            operator==( const SfxPoolItem& ) const override;
    const OUString&         GetMethod() const
                                { return aMethodName; }
    void                    SetMethod( const OUString& r )
                                { aMethodName = r; }
    const OUString&         GetModule() const
                                { return aModuleName; }
    void                    SetModule( const OUString& r )
                                { aModuleName = r; }
    const OUString&         GetLib() const
                                { return aLibName; }
    void                    SetLib( const OUString& r )
                                { aLibName = r; }
    const BasicManager*     GetBasicManager() const
                            { return pBasicManager; }
    const OUString& GetLocation() const { return aLocationName; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

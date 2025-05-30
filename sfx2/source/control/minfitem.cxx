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

#include <sfx2/minfitem.hxx>
#include <sal/log.hxx>
#include <utility>
#include <config_features.h>

#if HAVE_FEATURE_SCRIPTING

SfxPoolItem* SfxMacroInfoItem::CreateDefault() { SAL_WARN( "sfx", "No SfxMacroInfItem factory available"); return nullptr; }


SfxMacroInfoItem::SfxMacroInfoItem(
    sal_uInt16 nWhichId,        // Slot-ID
    const BasicManager* pMgr,
    OUString _aLibName,
    OUString _aModuleName,
    OUString _aMethodName,
    OUString _aComment,
    OUString _aLocationName) :
    SfxPoolItem(nWhichId),
    pBasicManager(pMgr),
    aLibName(std::move(_aLibName)),
    aModuleName(std::move(_aModuleName)),
    aMethodName(std::move(_aMethodName)),
    aCommentText(std::move(_aComment)),
    aLocationName(std::move(_aLocationName))
{
}

// op ==

bool SfxMacroInfoItem::operator==( const SfxPoolItem& rCmp) const
{
    const SfxMacroInfoItem rItem = static_cast<const SfxMacroInfoItem&>(rCmp);
    return SfxPoolItem::operator==(rCmp) &&
            pBasicManager == rItem.pBasicManager &&
            aLibName == rItem.aLibName &&
            aModuleName == rItem.aModuleName &&
            aMethodName == rItem.aMethodName &&
            aCommentText == rItem.aCommentText;
}

SfxMacroInfoItem* SfxMacroInfoItem::Clone( SfxItemPool *) const
{
    return new SfxMacroInfoItem(*this);
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

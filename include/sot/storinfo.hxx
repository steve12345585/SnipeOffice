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

#ifndef INCLUDED_SOT_STORINFO_HXX
#define INCLUDED_SOT_STORINFO_HXX

#include <rtl/ustring.hxx>
#include <utility>
#include <vector>
#include <sot/sotdllapi.h>
#include <sot/formats.hxx>

class StgDirEntry;
class SvStream;

class SvStorageInfo
{
    friend class SotStorage;
    OUString aName;
    sal_uInt64 nSize;
    bool bStream;
    bool bStorage;

public:
    SvStorageInfo(const StgDirEntry&);
    SvStorageInfo(OUString _aName, sal_uInt64 nSz, bool bIsStorage)
        : aName(std::move(_aName))
        , nSize(nSz)
        , bStream(!bIsStorage)
        , bStorage(bIsStorage)
    {
    }

    const OUString& GetName() const { return aName; }
    bool IsStream() const { return bStream; }
    bool IsStorage() const { return bStorage; }
    sal_uInt64 GetSize() const { return nSize; }
};

typedef std::vector<SvStorageInfo> SvStorageInfoList;

SotClipboardFormatId ReadClipboardFormat(SvStream& rStm);
SOT_DLLPUBLIC void WriteClipboardFormat(SvStream& rStm, SotClipboardFormatId nFormat);

#endif // _STORINFO_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <sal/config.h>

#include <algorithm>
#include <iostream>
#include "WW8Sttbf.hxx"
#include <osl/endian.h>
#include <o3tl/make_shared.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <tools/stream.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>

namespace ww8
{
    WW8Struct::WW8Struct(SvStream& rSt, sal_uInt32 nPos, sal_uInt32 nSize)
        : mn_offset(0), mn_size(0)
    {
        if (checkSeek(rSt, nPos))
        {
            std::size_t nRemainingSize = rSt.remainingSize();
            nSize = std::min<sal_uInt32>(nRemainingSize, nSize);
            m_pData = o3tl::make_shared_array<sal_uInt8>(nSize);
            mn_size = rSt.ReadBytes(m_pData.get(), nSize);
        }
        OSL_ENSURE(mn_size == nSize, "short read in WW8Struct::WW8Struct");
    }

    WW8Struct::WW8Struct(WW8Struct const * pStruct, sal_uInt32 nPos, sal_uInt32 nSize)
        : m_pData(pStruct->m_pData), mn_offset(pStruct->mn_offset + nPos)
        , mn_size(nSize)
    {
    }

    WW8Struct::~WW8Struct()
    {
    }

    sal_uInt8 WW8Struct::getU8(sal_uInt32 nOffset)
    {
        sal_uInt8 nResult = 0;

        if (nOffset < mn_size)
        {
            nResult = m_pData.get()[mn_offset + nOffset];
        }

        return nResult;
    }

    OUString WW8Struct::getUString(sal_uInt32 nOffset,
                                          sal_Int32 nCount)
    {
        OUString aResult;

        if (nCount > 0)
        {
            //clip to available
            sal_uInt32 nStartOff = mn_offset + nOffset;
            if (nStartOff >= mn_size)
                return aResult;
            sal_uInt32 nAvailable = (mn_size - nStartOff)/sizeof(sal_Unicode);
            if (o3tl::make_unsigned(nCount) > nAvailable)
                nCount = nAvailable;
            OUStringBuffer aBuf(nCount);
            for (sal_Int32 i = 0; i < nCount; ++i)
                aBuf.append(static_cast<sal_Unicode>(getU16(nStartOff+i*2)));
            aResult = aBuf.makeStringAndClear();
        }

        SAL_INFO( "sw.ww8.level2", "<WW8Struct-getUString offset=\"" << nOffset
            << "\" count=\"" << nCount << "\">" << aResult << "</WW8Struct-getUString>" );

        return aResult;

    }
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

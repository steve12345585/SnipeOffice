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

#include <sal/log.hxx>
#include <sal/types.h>

#include <bitset.hxx>

#include <string.h>

// creates the asymmetric difference with another bitset

IndexBitSet& IndexBitSet::operator-=(sal_uInt16 nBit)
{
    sal_uInt16 nBlock = nBit / 32;
    sal_uInt32 nBitVal = 1U << (nBit % 32);

    if ( nBlock >= nBlocks )
      return *this;

    if ( pBitmap[nBlock] & nBitVal )
    {
        pBitmap[nBlock] &= ~nBitVal;
    }

    return *this;
}

// unify with a single bit

IndexBitSet& IndexBitSet::operator|=( sal_uInt16 nBit )
{
    sal_uInt16 nBlock = nBit / 32;
    sal_uInt32 nBitVal = 1U << (nBit % 32);

    if ( nBlock >= nBlocks )
    {
        sal_uInt32 *pNewMap = new sal_uInt32[nBlock+1];
        memset( pNewMap + nBlocks, 0, 4 * (nBlock - nBlocks + 1) );

        if ( pBitmap )
        {
            memcpy( pNewMap, pBitmap.get(), 4 * nBlocks );
        }
        pBitmap.reset(pNewMap);
        nBlocks = nBlock+1;
    }

    if ( (pBitmap[nBlock] & nBitVal) == 0 )
    {
        pBitmap[nBlock] |= nBitVal;
    }

    return *this;
}


// determines if the bit is set (may be the only one)

bool IndexBitSet::Contains( sal_uInt16 nBit ) const
{
    sal_uInt16 nBlock = nBit / 32;
    sal_uInt32 nBitVal = 1U << (nBit % 32);

    if ( nBlock >= nBlocks )
        return false;
    return ( nBitVal & pBitmap[nBlock] ) == nBitVal;
}

IndexBitSet::IndexBitSet()
{
    nBlocks = 0;
}

IndexBitSet::~IndexBitSet()
{
}

sal_uInt16 IndexBitSet::GetFreeIndex()
{
  for(sal_uInt16 i=0;i<SAL_MAX_UINT16;i++)
      if(!Contains(i))
      {
        *this|=i;
        return i;
      }
  SAL_WARN( "sfx", "IndexBitSet contains more than SAL_MAX_UINT16 entries");
  return 0;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

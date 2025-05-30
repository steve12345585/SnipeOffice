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

#include <sal/config.h>

#include <memory>

#include <sal/types.h>

#include <store/types.h>

#include "storbase.hxx"

namespace store
{

class OStorePageBIOS;

struct OStoreBTreeEntry
{
    typedef OStorePageKey  K;
    typedef OStorePageLink L;

    K          m_aKey;
    L          m_aLink;
    sal_uInt32 m_nAttrib;

    explicit OStoreBTreeEntry (
        K const &  rKey    = K(),
        L const &  rLink   = L())
        : m_aKey    (rKey),
          m_aLink   (rLink),
          m_nAttrib (store::htonl(0))
    {}

    enum CompareResult
    {
        COMPARE_LESS    = -1,
        COMPARE_EQUAL   =  0,
        COMPARE_GREATER =  1
    };

    CompareResult compare (const OStoreBTreeEntry& rOther) const
    {
        if (m_aKey < rOther.m_aKey)
            return COMPARE_LESS;
        else if (m_aKey == rOther.m_aKey)
            return COMPARE_EQUAL;
        else
            return COMPARE_GREATER;
    }
};

#define STORE_MAGIC_BTREENODE sal_uInt32(0x58190322)

struct OStoreBTreeNodeData : public store::PageData
{
    typedef PageData      base;
    typedef OStoreBTreeNodeData self;

    typedef OStorePageGuard     G;
    typedef OStoreBTreeEntry    T;

    G m_aGuard;
    T m_pData[1];

    static const sal_uInt32 theTypeId = STORE_MAGIC_BTREENODE;

    static const size_t     theSize     = sizeof(G);
    static const sal_uInt16 thePageSize = base::theSize + self::theSize;
    static_assert(STORE_MINIMUM_PAGESIZE >= self::thePageSize, "got to be at least equal in size");

    sal_uInt16 capacity() const
    {
        return static_cast<sal_uInt16>(store::ntohs(base::m_aDescr.m_nSize) - self::thePageSize);
    }

    /** capacityCount (must be even).
    */
    sal_uInt16 capacityCount() const
    {
        return sal_uInt16(capacity() / sizeof(T));
    }

    sal_uInt16 usage() const
    {
        return static_cast<sal_uInt16>(store::ntohs(base::m_aDescr.m_nUsed) - self::thePageSize);
    }

    sal_uInt16 usageCount() const
    {
        return sal_uInt16(usage() / sizeof(T));
    }
    void usageCount (sal_uInt16 nCount)
    {
        size_t const nBytes = self::thePageSize + nCount * sizeof(T);
        base::m_aDescr.m_nUsed = store::htons(sal::static_int_cast< sal_uInt16 >(nBytes));
    }

    explicit OStoreBTreeNodeData (sal_uInt16 nPageSize);

    void guard()
    {
        sal_uInt32 nCRC32 = rtl_crc32 (0, &m_aGuard.m_nMagic, sizeof(sal_uInt32));
        nCRC32 = rtl_crc32 (nCRC32, m_pData, capacity());
        m_aGuard.m_nCRC32 = store::htonl(nCRC32);
    }

    storeError verify() const
    {
        sal_uInt32 nCRC32 = rtl_crc32 (0, &m_aGuard.m_nMagic, sizeof(sal_uInt32));
        nCRC32 = rtl_crc32 (nCRC32, m_pData, capacity());
        if (m_aGuard.m_nCRC32 != store::htonl(nCRC32))
            return store_E_InvalidChecksum;
        else
            return store_E_None;
    }

    sal_uInt32 depth() const
    {
        return store::ntohl(self::m_aGuard.m_nMagic);
    }
    void depth (sal_uInt32 nDepth)
    {
        self::m_aGuard.m_nMagic = store::htonl(nDepth);
    }

    bool querySplit() const
    {
        return usageCount() >= capacityCount();
    }

    sal_uInt16 find   (const T& t) const;
    void       insert (sal_uInt16 i, const T& t);
    void       remove (sal_uInt16 i);

    /** split (left half copied from right half of left page).
    */
    void split (const self& rPageL);

    /** truncate (to n elements).
    */
    void truncate (sal_uInt16 n);
};

class OStoreBTreeNodeObject : public store::OStorePageObject
{
    typedef OStorePageObject      base;
    typedef OStoreBTreeNodeObject self;
    typedef OStoreBTreeNodeData   page;

    typedef OStoreBTreeEntry      T;

public:
    explicit OStoreBTreeNodeObject (std::shared_ptr<PageData> const & rxPage = std::shared_ptr<PageData>())
        : OStorePageObject (rxPage)
    {}

    virtual storeError guard  (sal_uInt32 nAddr) override;
    virtual storeError verify (sal_uInt32 nAddr) const override;

    /** split.
     *
     *  @param rxPageL [inout] left child to be split
     */
    storeError split (
        sal_uInt16                 nIndexL,
        PageHolderObject< page > & rxPageL,
        OStorePageBIOS &           rBIOS);

    /** remove (down to leaf node, recursive).
    */
    storeError remove (
        sal_uInt16         nIndexL,
        OStoreBTreeEntry & rEntryL,
        OStorePageBIOS &   rBIOS);
};

class OStoreBTreeRootObject : public store::OStoreBTreeNodeObject
{
    typedef OStoreBTreeNodeObject base;
    typedef OStoreBTreeNodeData   page;

    typedef OStoreBTreeEntry      T;

public:
    explicit OStoreBTreeRootObject (std::shared_ptr<PageData> const & rxPage = std::shared_ptr<PageData>())
        : OStoreBTreeNodeObject (rxPage)
    {}

    storeError loadOrCreate (
        sal_uInt32       nAddr,
        OStorePageBIOS & rBIOS);

    /** find_lookup (w/o split()).
     *  Precond: root node page loaded.
     */
    storeError find_lookup (
        OStoreBTreeNodeObject & rNode,  // [out]
        sal_uInt16 &            rIndex, // [out]
        OStorePageKey const &   rKey,
        OStorePageBIOS &        rBIOS) const;

    /** find_insert (possibly with split()).
     *  Precond: root node page loaded.
     */
    storeError find_insert (
        OStoreBTreeNodeObject & rNode,
        sal_uInt16 &            rIndex,
        OStorePageKey const &   rKey,
        OStorePageBIOS &        rBIOS);

private:
    /** testInvariant.
     *  Precond: root node page loaded.
     */
    void testInvariant (char const * message) const;

    /** change (Root).
     *
     *  @param rxPageL [out] prev. root (needs split)
     */
    storeError change (
        PageHolderObject< page > & rxPageL,
        OStorePageBIOS &           rBIOS);
};

} // namespace store

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

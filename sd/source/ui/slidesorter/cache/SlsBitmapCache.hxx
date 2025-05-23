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

#include <vcl/bitmapex.hxx>
#include <mutex>
#include <memory>

class SdrPage;

namespace sd::slidesorter::cache
{
class CacheCompactor;
class BitmapCompressor;

/** This low level cache is the actual bitmap container.  It supports a
    precious flag for every preview bitmap and keeps track of total sizes
    for all previews with/without this flag.  The precious flag is used by
    compaction algorithms to determine which previews may be compressed or
    even discarded and which have to remain in their original form.  The
    precious flag is usually set for the visible previews.

    Additionally to the actual preview there is an optional marked preview.
    This is used for slides excluded from the slide show which have a preview
    that shows a mark (some sort of bitmap overlay) to that effect.
*/
class BitmapCache
{
public:
    /** The key for looking up preview bitmaps is a pointer to an SdrPage
        object.  The prior use of PageObjectViewObjectContact objects (which
        ultimately use them) turned out to be less suitable because their
        life time is shorter then that of the page objects.  Frequent
        destruction and re-creation of the preview bitmaps was the result.
    */
    typedef const SdrPage* CacheKey;
    class CacheEntry;
    class CacheBitmapContainer;
    typedef ::std::vector<CacheKey> CacheIndex;

    /** Create a new cache for bitmap objects.
        The default value from the configuration is used.
        When that does not exist then an internal default value is
        used.
    */
    explicit BitmapCache();

    /** The destructor clears the cache and releases all bitmaps still in it.
    */
    ~BitmapCache();

    /** Remove all preview bitmaps from the cache.  After this call the
        cache is empty.
    */
    void Clear();

    /** Return <TRUE/> when the cache is full, i.e. the cache compactor had
        to be run.
    */
    bool IsFull() const { return mbIsFull; }

    /** Return the memory size that is occupied by all non-precious bitmaps
        in the cache.
    */
    sal_Int32 GetSize() const { return mnNormalCacheSize; }

    /** Return <TRUE/> when a preview bitmap exists for the given key.
    */
    bool HasBitmap(const CacheKey& rKey);

    /** Return <TRUE/> when a preview bitmap exists for the given key and
        when it is up-to-date.
    */
    bool BitmapIsUpToDate(const CacheKey& rKey);

    /** Return the preview bitmap for the given contact object.
    */
    BitmapEx GetBitmap(const CacheKey& rKey);

    /** Return the marked preview bitmap for the given contact object.
    */
    BitmapEx GetMarkedBitmap(const CacheKey& rKey);

    /** Release the reference to the preview bitmap that is associated with
        the given key.
    */
    void ReleaseBitmap(const CacheKey& rKey);

    /** Mark the specified preview bitmap as not being up-to-date
        anymore.
        @return
            When the key references a page in the cache then
            return <TRUE/>.  When the key is not known then <FALSE/>
            is returned.
    */
    bool InvalidateBitmap(const CacheKey& rKey);

    /** Mark all preview bitmaps as not being up-to-date anymore.
    */
    void InvalidateCache();

    /** Add or replace a bitmap for the given key.
    */
    void SetBitmap(const CacheKey& rKey, const BitmapEx& rPreview, bool bIsPrecious);

    /** Add or replace a marked bitmap for the given key.
    */
    void SetMarkedBitmap(const CacheKey& rKey, const BitmapEx& rPreview);

    /** Mark the specified preview bitmap as precious, i.e. that it must not
        be compressed or otherwise removed from the cache.
    */
    void SetPrecious(const CacheKey& rKey, bool bIsPrecious);

    /** Calculate the cache size.  This should rarely be necessary because
        the cache size is tracked with each modification of preview
        bitmaps.
    */
    void ReCalculateTotalCacheSize();

    /** Use the previews in the given cache to initialize missing previews.
    */
    void Recycle(const BitmapCache& rCache);

    /** Return a list of sorted cache keys that represent an index into (a
        part of) the cache.  The entries of the index are sorted according
        to last access times with the least recently access time first.
        Entries with the precious flag set are omitted.
        Entries with that have no preview bitmaps are omitted.
    */
    CacheIndex GetCacheIndex() const;

    /** Compress the specified preview bitmap with the given bitmap
        compressor.  A reference to the compressor is stored for later
        decompression.
    */
    void Compress(const CacheKey& rKey, const std::shared_ptr<BitmapCompressor>& rpCompressor);

private:
    mutable std::mutex maMutex;

    std::unique_ptr<CacheBitmapContainer> mpBitmapContainer;

    /** Total size of bytes that are occupied by bitmaps in the cache for
        whom the slides are currently not inside the visible area.
    */
    sal_Int32 mnNormalCacheSize;

    /** Total size of bytes that are occupied by bitmaps in the cache for
        whom the slides are currently visible.
    */
    sal_Int32 mnPreciousCacheSize;

    /** At the moment the access time is not an actual time or date value
        but a counter that is increased with every access.  It thus defines
        the same ordering as a true time.
    */
    sal_Int32 mnCurrentAccessTime;

    /** The maximal cache size for the off-screen preview bitmaps.  When
        mnNormalCacheSize grows larger than this value then the
        mpCacheCompactor member is used to reduce the cache size.
    */
    sal_Int32 mnMaximalNormalCacheSize;

    /** The cache compactor is used to reduce the number of bytes used by
        off-screen preview bitmaps.
    */
    ::std::unique_ptr<CacheCompactor> mpCacheCompactor;

    /** This flag stores if the cache is or recently was full, i.e. the
        cache compactor has or had to be run in order to reduce the cache
        size to the allowed value.
    */
    bool mbIsFull;

    /** Update mnNormalCacheSize or mnPreciousCacheSize according to the
        precious flag of the specified preview bitmap and the specified
        operation.
    */
    enum CacheOperation
    {
        ADD,
        REMOVE
    };
    void UpdateCacheSize(std::unique_lock<std::mutex>& rGuard, const CacheEntry& rKey,
                         CacheOperation eOperation);

    void ReCalculateTotalCacheSize(std::unique_lock<std::mutex>& rGuard);

    void SetBitmap(std::unique_lock<std::mutex>& rGuard, const CacheKey& rKey,
                   const BitmapEx& rPreview, bool bIsPrecious);
};

} // end of namespace ::sd::slidesorter::cache

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

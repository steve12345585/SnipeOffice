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

#include <cache/SlsCacheContext.hxx>
#include <model/SlsSharedPageDescriptor.hxx>

namespace sd::slidesorter::model
{
class SlideSorterModel;
}
namespace sd::slidesorter
{
class SlideSorter;
}

namespace sd::slidesorter::view
{
/** The cache context for the SlideSorter as used by Draw and Impress.  See
    the base class for documentation of the individual methods.
*/
class ViewCacheContext : public cache::CacheContext
{
public:
    explicit ViewCacheContext(SlideSorter& rSlideSorter);
    virtual ~ViewCacheContext() override;
    virtual void NotifyPreviewCreation(cache::CacheKey aKey) override;
    virtual bool IsIdle() override;
    virtual bool IsVisible(cache::CacheKey aKey) override;
    virtual const SdrPage* GetPage(cache::CacheKey aKey) override;
    virtual std::shared_ptr<std::vector<cache::CacheKey>> GetEntryList(bool bVisible) override;
    virtual sal_Int32 GetPriority(cache::CacheKey aKey) override;
    virtual SdXImpressDocument* GetModel() override;

private:
    model::SlideSorterModel& mrModel;
    SlideSorter& mrSlideSorter;

    model::SharedPageDescriptor GetDescriptor(cache::CacheKey aKey);
};

} // end of namespace ::sd::slidesorter::view

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

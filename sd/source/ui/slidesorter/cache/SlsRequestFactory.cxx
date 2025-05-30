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

#include "SlsRequestFactory.hxx"
#include "SlsRequestQueue.hxx"

namespace sd::slidesorter::cache {

void RequestFactory::operator()(
    RequestQueue& rRequestQueue,
    const SharedCacheContext& rpCacheContext)
{
    std::shared_ptr<std::vector<CacheKey> > aKeys;

    // Add the requests for the visible pages.
    aKeys = rpCacheContext->GetEntryList(true);
    if (aKeys != nullptr)
    {
        for (const auto& rKey : *aKeys)
            rRequestQueue.AddRequest(rKey, VISIBLE_NO_PREVIEW);
    }

    // Add the requests for the non-visible pages.
    aKeys = rpCacheContext->GetEntryList(false);
    if (aKeys != nullptr)
    {
        for (const auto& rKey : *aKeys)
            rRequestQueue.AddRequest(rKey, NOT_VISIBLE);
    }
}

} // end of namespace ::sd::slidesorter::cache

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

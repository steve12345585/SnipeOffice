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

#include <com/sun/star/container/XIndexAccess.hpp>
#include <memory>
#include <vector>

struct GroupEntry
{
    sal_uInt32                  mnCurrentPos;
    sal_uInt32                  mnCount;
    css::uno::Reference< css::container::XIndexAccess >           mXIndexAccess;

    explicit GroupEntry( css::uno::Reference< css::container::XIndexAccess > const & rIndex )
      : mnCurrentPos(0),
        mnCount(rIndex->getCount()),
        mXIndexAccess(rIndex)
    {
    };

    explicit GroupEntry( sal_uInt32 nCount )
      :  mnCurrentPos(0),
         mnCount(nCount)
    {
    };
};

class GroupTable
{
        sal_uInt32              mnIndex;
        sal_uInt32              mnGroupsClosed;
        std::vector<GroupEntry> mvGroupEntry;

    public:

        sal_uInt32              GetCurrentGroupIndex() const { return mnIndex; };
        sal_Int32               GetCurrentGroupLevel() const { return mvGroupEntry.size() - 1; };
        const css::uno::Reference< css::container::XIndexAccess > &
                                GetCurrentGroupAccess() const { return mvGroupEntry.back().mXIndexAccess; };
        sal_uInt32              GetGroupsClosed();
        void                    ResetGroupTable( sal_uInt32 nCount );
        void                    ClearGroupTable();
        bool                    EnterGroup( css::uno::Reference< css::container::XIndexAccess > const & rIndex );
        bool                    GetNextGroupEntry();
                                GroupTable();
                                ~GroupTable();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

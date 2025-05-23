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
#ifndef INCLUDED_SW_INC_SORTOPT_HXX
#define INCLUDED_SW_INC_SORTOPT_HXX

#include <rtl/ustring.hxx>
#include <i18nlangtag/lang.h>
#include "swdllapi.h"
#include <vector>

enum class SwSortOrder     { Ascending, Descending };
enum class SwSortDirection { Columns, Rows };

struct SW_DLLPUBLIC SwSortKey
{
    SwSortKey();
    SwSortKey( sal_uInt16 nId, const OUString& rSrtType, SwSortOrder eOrder );

    OUString        sSortType;
    SwSortOrder     eSortOrder;
    sal_uInt16      nColumnId;
    bool            bIsNumeric;
};

struct SW_DLLPUBLIC SwSortOptions
{
    SwSortOptions();
    ~SwSortOptions();
    SwSortOptions(const SwSortOptions& rOpt);

    SwSortOptions& operator=( SwSortOptions const & ) = delete; // MSVC2015 workaround

    std::vector<SwSortKey>  aKeys;
    SwSortDirection         eDirection;
    sal_Unicode             cDeli;
    LanguageType            nLanguage;
    bool                    bTable;
    bool                    bIgnoreCase;
};

#endif // INCLUDED_SW_INC_SORTOPT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

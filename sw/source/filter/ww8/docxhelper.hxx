/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_SOURCE_FILTER_WW8_DOCXHELPER_HXX
#define INCLUDED_SW_SOURCE_FILTER_WW8_DOCXHELPER_HXX

#include <sal/config.h>

#include <string_view>

#include <sal/types.h>

struct DocxStringTokenMap
{
    const char* pToken;
    sal_Int32 nToken;
};

sal_Int32 DocxStringGetToken(DocxStringTokenMap const* pMap, std::u16string_view rName);

#endif // INCLUDED_SW_SOURCE_FILTER_WW8_DOCXHELPER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

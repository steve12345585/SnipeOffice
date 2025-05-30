/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <string_view>

#include <rtl/ustring.hxx>

namespace dbahsql::utils
{
OUString convertToUTF8(std::string_view original);

OUString getTableNameFromStmt(std::u16string_view sSql);

void ensureFirebirdTableLength(std::u16string_view sName);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

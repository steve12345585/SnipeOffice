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

#include "scdllapi.h"
#include <rtl/ustring.hxx>
#include <svl/sharedstring.hxx>

class Color;
class ScAddress;
class ScDocument;
struct ScInterpreterContext;
struct ScRefCellValue;
namespace svl { class SharedStringPool; }

class SC_DLLPUBLIC ScCellFormat
{
public:

    static OUString GetString(
        const ScRefCellValue& rCell, sal_uInt32 nFormat,
        const Color** ppColor, ScInterpreterContext* pContext, const ScDocument& rDoc, bool bNullVals = true,
        bool bFormula  = false, bool bUseStarFormat = false );

    static OUString GetString(
        ScDocument& rDoc, const ScAddress& rPos, sal_uInt32 nFormat,
        const Color** ppColor, ScInterpreterContext* pContext, bool bNullVals = true,
        bool bFormula  = false );

    // Similar to GetInputSharedString, but can be used to visit the source of the
    // svl::SharedString to avoid reference counting overhead
    template <typename TFunctor>
    static auto visitInputSharedString(
        const ScRefCellValue& rCell, sal_uInt32 nFormat, ScInterpreterContext* pContext,
        const ScDocument& rDoc, svl::SharedStringPool& rStrPool,
        bool bFiltering, bool bForceSystemLocale, const TFunctor& rOper);

    static OUString GetInputString(
        const ScRefCellValue& rCell, sal_uInt32 nFormat, ScInterpreterContext* pContext,
        const ScDocument& rDoc, bool bFiltering = false, bool bForceSystemLocale = false );

    static OUString GetOutputString(
        ScDocument& rDoc, const ScAddress& rPos, const ScRefCellValue& rCell );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

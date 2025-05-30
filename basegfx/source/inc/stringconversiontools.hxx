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

#include <string_view>

#include <sal/types.h>
#include <rtl/ustring.hxx>

namespace basegfx::internal
    {
        void skipSpaces(sal_Int32&      io_rPos,
                        std::u16string_view rStr,
                        const sal_Int32 nLen);

        inline bool isOnNumberChar(const sal_Unicode aChar,
                                   bool              bSignAllowed)
        {
            const bool bPredicate( (u'0' <= aChar && u'9' >= aChar)
                                    || (bSignAllowed && u'+' == aChar)
                                    || (bSignAllowed && u'-' == aChar)
                                    || (u'.' == aChar));

            return bPredicate;
        }

        inline bool isOnNumberChar(std::u16string_view rStr,
                                   const sal_Int32 nPos)
        {
            return isOnNumberChar(rStr[nPos], true/*bSignAllowed*/);
        }

        bool importDoubleAndSpaces(double&          o_fRetval,
                                   sal_Int32&       io_rPos,
                                   std::u16string_view  rStr,
                                   const sal_Int32  nLen );

        bool importFlagAndSpaces(sal_Int32&      o_nRetval,
                                 sal_Int32&      io_rPos,
                                 std::u16string_view rStr,
                                 const sal_Int32 nLen);

} // namespace basegfx::internal

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

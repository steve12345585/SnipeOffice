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

#include <config_options.h>
#include <com/sun/star/lang/Locale.hpp>

#include <connectivity/IParseContext.hxx>
#include <rtl/ustring.hxx>
#include <svx/svxdllapi.h>
#include <vector>

namespace svxform
{

    //= OSystemParseContext

    class SAL_DLLPUBLIC_RTTI OSystemParseContext : public ::connectivity::IParseContext
    {
    protected:

        ::std::vector< OUString > m_aLocalizedKeywords;
        OSystemParseContext(bool bInit);

    public:
        SVXCORE_DLLPUBLIC OSystemParseContext();
        SVXCORE_DLLPUBLIC virtual ~OSystemParseContext();

        // retrieves language specific error messages
        virtual OUString getErrorMessage(ErrorCode _eCodes) const override;

        // retrieves language specific keyword strings (only ASCII allowed)
        virtual OString getIntlKeywordAscii(InternationalKeyCode _eKey) const override;

        // finds out, if we have an international keyword (only ASCII allowed)
        virtual InternationalKeyCode getIntlKeyCode(const OString& rToken) const override;

        /** gets a locale instance which should be used when parsing in the context specified by this instance
            <p>if this is not overridden by derived classes, it returns the static default locale.</p>
        */
        SVXCORE_DLLPUBLIC virtual css::lang::Locale getPreferredLocale( ) const override;

    };

    class SAL_DLLPUBLIC_RTTI ONeutralParseContext final : public OSystemParseContext
    {
    public:
        SVXCORE_DLLPUBLIC ONeutralParseContext();
        SVXCORE_DLLPUBLIC virtual ~ONeutralParseContext();
    };

    //= OParseContextClient

    /** helper class which needs access to a (shared and ref-counted) OSystemParseContext
        instance.
    */
    class UNLESS_MERGELIBS_MORE(SVXCORE_DLLPUBLIC) OParseContextClient
    {
    protected:
        OParseContextClient();
        virtual ~OParseContextClient();

        const OSystemParseContext* getParseContext() const;
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

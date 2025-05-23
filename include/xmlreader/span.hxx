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

#include <cstring>

#include <sal/types.h>
#include <xmlreader/detail/xmlreaderdllapi.hxx>

namespace rtl { class OUString; }

namespace xmlreader {

struct SAL_WARN_UNUSED OOO_DLLPUBLIC_XMLREADER Span {
    char const * begin;
    sal_Int32 length;

    Span(): begin(nullptr), length(0) {}
        // init length to avoid compiler warnings

    Span(char const * theBegin, sal_Int32 theLength):
        begin(theBegin), length(theLength) {}

    template< std::size_t N > explicit Span(char const (& literal)[N]):
        begin(literal), length(N - 1)
    {}

    void clear() noexcept { begin = nullptr; }

    bool is() const { return begin != nullptr; }

    bool operator==(Span const & text) const {
        return length == text.length
            && std::memcmp(begin, text.begin, text.length) == 0;
    }

    bool operator!=(Span const & text) const {
        return !(operator==(text));
    }

    bool equals(char const * textBegin, sal_Int32 textLength) const {
        return operator==(Span(textBegin, textLength));
    }

    template< std::size_t N > bool operator==(char const (& literal)[N])
        const
    {
        return operator==(Span(literal, N - 1));
    }

    template< std::size_t N > bool operator!=(char const (& literal)[N])
        const
    {
        return operator!=(Span(literal, N - 1));
    }

    rtl::OUString convertFromUtf8() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

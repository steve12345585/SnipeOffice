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

#ifndef INCLUDED_STOC_SOURCE_URIPROC_URIREFERENCE_HXX
#define INCLUDED_STOC_SOURCE_URIPROC_URIREFERENCE_HXX

#include <mutex>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <rtl/ustrbuf.hxx>

namespace stoc::uriproc {

class UriReference {
public:
    UriReference(
        OUString scheme, bool hasAuthority,
        OUString const & authority, OUString path,
        bool hasQuery, OUString const & query);

    ~UriReference();

    /// @throws css::uno::RuntimeException
    OUString getUriReference();

    /// @throws css::uno::RuntimeException
    bool isAbsolute() const;

    /// @throws css::uno::RuntimeException
    const OUString& getScheme() const { return m_scheme;}

    /// @throws css::uno::RuntimeException
    OUString getSchemeSpecificPart();

    /// @throws css::uno::RuntimeException
    bool isHierarchical();

    /// @throws css::uno::RuntimeException
    bool hasAuthority() const;

    /// @throws css::uno::RuntimeException
    const OUString& getAuthority() const;

    /// @throws css::uno::RuntimeException
    OUString getPath();

    /// @throws css::uno::RuntimeException
    bool hasRelativePath();

    /// @throws css::uno::RuntimeException
    sal_Int32 getPathSegmentCount();

    /// @throws css::uno::RuntimeException
    OUString getPathSegment(sal_Int32 index);

    /// @throws css::uno::RuntimeException
    bool hasQuery() const;

    /// @throws css::uno::RuntimeException
    const OUString& getQuery() const;

    /// @throws css::uno::RuntimeException
    bool hasFragment();

    /// @throws css::uno::RuntimeException
    OUString getFragment();

    /// @throws css::uno::RuntimeException
    void setFragment(OUString const & fragment);

    /// @throws css::uno::RuntimeException
    void clearFragment();

    std::mutex m_mutex;
    OUString m_path;

private:
    UriReference(UriReference const &) = delete;
    void operator =(UriReference const &) = delete;

    void appendSchemeSpecificPart(OUStringBuffer & buffer) const;

    OUString m_scheme;
    OUString m_authority;
    OUString m_query;
    OUString m_fragment;
    bool m_hasAuthority;
    bool m_hasQuery;
    bool m_hasFragment;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

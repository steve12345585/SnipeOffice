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

#include <deque>
#include <com/sun/star/uno/Reference.h>
#include <regexpmap.hxx>

namespace com::sun::star::ucb {
    class XContentProvider;
}


class ProviderListEntry_Impl
{
    css::uno::Reference<
        css::ucb::XContentProvider > m_xProvider;
    mutable css::uno::Reference<
        css::ucb::XContentProvider > m_xResolvedProvider;

private:
    css::uno::Reference< css::ucb::XContentProvider > const & resolveProvider() const;

public:
    explicit ProviderListEntry_Impl(
        css::uno::Reference< css::ucb::XContentProvider > xProvider )
    : m_xProvider( std::move(xProvider) ) {}

    const css::uno::Reference< css::ucb::XContentProvider >& getProvider() const
    { return m_xProvider; }
    inline css::uno::Reference< css::ucb::XContentProvider > const & getResolvedProvider() const;
};

inline css::uno::Reference< css::ucb::XContentProvider > const &
ProviderListEntry_Impl::getResolvedProvider() const
{
    return m_xResolvedProvider.is() ? m_xResolvedProvider : resolveProvider();
}


typedef std::deque< ProviderListEntry_Impl > ProviderList_Impl;


typedef ucb_impl::RegexpMap< ProviderList_Impl > ProviderMap_Impl;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <cppuhelper/implbase.hxx>
#include <o3tl/hash_combine.hxx>
#include <unordered_map>
#include <utility>

enum class XmlStyleFamily;

struct StyleNameKey_Impl
{
    XmlStyleFamily m_nFamily;
    OUString       m_aName;

    StyleNameKey_Impl( XmlStyleFamily nFamily,
                       OUString sName ) :
        m_nFamily( nFamily ),
        m_aName(std::move( sName ))
    {
    }
};

struct StyleNameHash_Impl
{
    inline size_t operator()( const StyleNameKey_Impl& r ) const;
    inline bool operator()( const StyleNameKey_Impl& r1,
                               const StyleNameKey_Impl& r2 ) const;
};

inline size_t StyleNameHash_Impl::operator()( const StyleNameKey_Impl& r ) const
{
    std::size_t seed = 0;
    o3tl::hash_combine(seed, r.m_nFamily);
    o3tl::hash_combine(seed, r.m_aName.hashCode());
    return seed;
}

inline bool StyleNameHash_Impl::operator()(
        const StyleNameKey_Impl& r1,
        const StyleNameKey_Impl& r2 ) const
{
    return r1.m_nFamily == r2.m_nFamily && r1.m_aName == r2.m_aName;
}

class StyleMap final :
    public ::cppu::WeakImplHelper<>,
    public std::unordered_map< StyleNameKey_Impl, OUString,
                            StyleNameHash_Impl, StyleNameHash_Impl >
{

public:

    StyleMap();
    virtual ~StyleMap() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

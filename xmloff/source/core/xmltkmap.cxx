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

#include <rtl/ustring.hxx>
#include <xmloff/xmltkmap.hxx>
#include <xmloff/xmltoken.hxx>
#include <o3tl/hash_combine.hxx>

#include <unordered_map>
#include <utility>

using namespace ::xmloff::token;

class SvXMLTokenMap_Impl
{
private:
    struct PairHash
    {
        std::size_t operator()(const std::pair<sal_uInt16,OUString> &pair) const
        {
            std::size_t seed = 0;
            o3tl::hash_combine(seed, pair.first);
            o3tl::hash_combine(seed, pair.second.hashCode());
            return seed;
        }
    };
    std::unordered_map< std::pair<sal_uInt16, OUString>,
                        sal_uInt16, PairHash> m_aPrefixAndNameToTokenMap;

public:
    void insert( const SvXMLTokenMapEntry& rEntry );
    sal_uInt16 get( sal_uInt16 nKeyPrefix, const OUString& rLName ) const;
};

void SvXMLTokenMap_Impl::insert( const SvXMLTokenMapEntry& rEntry )
{
    bool inserted = m_aPrefixAndNameToTokenMap.insert( std::make_pair( std::make_pair( rEntry.nPrefixKey,
                                       GetXMLToken( rEntry.eLocalName ) ), rEntry.nToken ) ).second;
    assert(inserted && "duplicate token");
    (void)inserted;
}

sal_uInt16 SvXMLTokenMap_Impl::get( sal_uInt16 nKeyPrefix, const OUString& rLName ) const
{
    auto aIter( m_aPrefixAndNameToTokenMap.find( std::make_pair( nKeyPrefix, rLName ) ) );
    if ( aIter != m_aPrefixAndNameToTokenMap.end() )
        return (*aIter).second;
    else
        return XML_TOK_UNKNOWN;
}

SvXMLTokenMap::SvXMLTokenMap( const SvXMLTokenMapEntry *pMap )
    : m_pImpl( new SvXMLTokenMap_Impl )
{
    while( pMap->eLocalName != XML_TOKEN_INVALID )
    {
        m_pImpl->insert( *pMap );
        pMap++;
    }
}

SvXMLTokenMap::~SvXMLTokenMap()
{
}

sal_uInt16 SvXMLTokenMap::Get( sal_uInt16 nKeyPrefix,
                               const OUString& rLName ) const
{
    return m_pImpl->get( nKeyPrefix, rLName );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

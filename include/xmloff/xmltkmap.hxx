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

#ifndef INCLUDED_XMLOFF_XMLTKMAP_HXX
#define INCLUDED_XMLOFF_XMLTKMAP_HXX

#include <config_options.h>
#include <sal/config.h>
#include <xmloff/dllapi.h>
#include <sal/types.h>
#include <xmloff/xmltoken.hxx>
#include <memory>


class SvXMLTokenMap_Impl;

#define XML_TOK_UNKNOWN 0xffffU
#define XML_TOKEN_MAP_END { 0xffffU, xmloff::token::XML_TOKEN_INVALID, 0U }

struct SvXMLTokenMapEntry
{
    enum xmloff::token::XMLTokenEnum eLocalName;
    sal_uInt16 nPrefixKey;
    sal_uInt16 nToken;

    SvXMLTokenMapEntry( sal_uInt16 nPrefix, xmloff::token::XMLTokenEnum eName,
                        sal_uInt16 nTok ) :
        eLocalName( eName ),
        nPrefixKey( nPrefix ),
        nToken( nTok )
    {
    }
};

class UNLESS_MERGELIBS_MORE(XMLOFF_DLLPUBLIC) SvXMLTokenMap
{
private:
    std::unique_ptr<SvXMLTokenMap_Impl>  m_pImpl;

public:

    SvXMLTokenMap( const SvXMLTokenMapEntry* pMap );
    ~SvXMLTokenMap();

    sal_uInt16 Get( sal_uInt16 nPrefix, const OUString& rLName ) const;
};

#endif // INCLUDED_XMLOFF_XMLTKMAP_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

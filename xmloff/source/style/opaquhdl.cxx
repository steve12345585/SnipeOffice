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

#include "opaquhdl.hxx"
#include <com/sun/star/uno/Any.hxx>

#include <xmloff/xmltoken.hxx>
#include <xmloff/xmluconv.hxx>

using namespace ::com::sun::star::uno;
using namespace ::xmloff::token;


XMLOpaquePropHdl::~XMLOpaquePropHdl()
{
    // nothing to do
}

bool XMLOpaquePropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bValue = IsXMLToken( rStrImpValue, XML_FOREGROUND );
    rValue <<= bValue;

    return true;
}

bool XMLOpaquePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    bool bValue;

    if (rValue >>= bValue)
    {
        if( bValue )
            rStrExpValue = GetXMLToken( XML_FOREGROUND );
        else
            rStrExpValue = GetXMLToken( XML_BACKGROUND );

        bRet = true;
    }

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

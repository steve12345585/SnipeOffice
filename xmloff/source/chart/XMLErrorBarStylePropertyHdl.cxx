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

#include "XMLErrorBarStylePropertyHdl.hxx"

#include <xmloff/xmluconv.hxx>
#include <unotools/saveopt.hxx>

#include <com/sun/star/chart/ErrorBarStyle.hpp>
#include <com/sun/star/uno/Any.hxx>

using namespace com::sun::star;

XMLErrorBarStylePropertyHdl::XMLErrorBarStylePropertyHdl(  const SvXMLEnumMapEntry<sal_Int32>* pEnumMap )
        : XMLEnumPropertyHdl( pEnumMap )
{
}

XMLErrorBarStylePropertyHdl::~XMLErrorBarStylePropertyHdl()
{
}

bool XMLErrorBarStylePropertyHdl::exportXML( OUString& rStrExpValue,
    const uno::Any& rValue, const SvXMLUnitConverter& rUnitConverter) const
{
    uno::Any aValue(rValue);
    const SvtSaveOptions::ODFSaneDefaultVersion nCurrentVersion(rUnitConverter.getSaneDefaultVersion());
    if (nCurrentVersion < SvtSaveOptions::ODFSVER_012)
    {
        sal_Int32 nValue = 0;
        if(rValue >>= nValue )
        {
            if( nValue == css::chart::ErrorBarStyle::STANDARD_ERROR
                || nValue == css::chart::ErrorBarStyle::FROM_DATA )
            {
                nValue = css::chart::ErrorBarStyle::NONE;
                aValue <<= nValue;
            }
        }
    }

    return XMLEnumPropertyHdl::exportXML( rStrExpValue, aValue, rUnitConverter );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

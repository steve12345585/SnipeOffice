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

#ifndef INCLUDED_COMPHELPER_PROPERTYINFOHASH_HXX
#define INCLUDED_COMPHELPER_PROPERTYINFOHASH_HXX

#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Type.h>
#include <unordered_map>

namespace comphelper
{
    struct PropertyInfo
    {
        OUString maName;
        css::uno::Type maType;
        sal_Int32 mnHandle;
        sal_Int16 mnAttributes;

        PropertyInfo(OUString const & aName, sal_Int32 nHandle, css::uno::Type const & aType, sal_Int16 nAttributes)
            : maName(aName), maType(aType), mnHandle(nHandle), mnAttributes(nAttributes) {}
        PropertyInfo(OUString && aName, sal_Int32 nHandle, css::uno::Type const & aType, sal_Int16 nAttributes)
            : maName(std::move(aName)), maType(aType), mnHandle(nHandle), mnAttributes(nAttributes) {}
    };
    struct PropertyData
    {
        sal_uInt8 mnMapId;
        const PropertyInfo *mpInfo;
        PropertyData ( sal_uInt8 nMapId, PropertyInfo const *pInfo )
        : mnMapId ( nMapId )
        , mpInfo ( pInfo ) {}
    };
}

typedef std::unordered_map < OUString,
                        ::comphelper::PropertyInfo const * > PropertyInfoHash;
typedef std::unordered_map < OUString,
                        ::comphelper::PropertyData* > PropertyDataHash;
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

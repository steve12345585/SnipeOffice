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

#include <comphelper/ChainablePropertySetInfo.hxx>
#include <sal/log.hxx>

using ::comphelper::PropertyInfo;
using ::comphelper::ChainablePropertySetInfo;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::beans::Property;
using ::com::sun::star::beans::UnknownPropertyException;

ChainablePropertySetInfo::ChainablePropertySetInfo( PropertyInfo const * pMap )
{
    for( ; !pMap->maName.isEmpty(); ++pMap )
    {
        SAL_WARN_IF(
            maMap.contains(pMap->maName),
            "comphelper", "Duplicate property name \"" << pMap->maName << "\"");
        maMap[pMap->maName] = pMap;
    }
}

ChainablePropertySetInfo::~ChainablePropertySetInfo()
    noexcept
{
}

void ChainablePropertySetInfo::remove( const OUString& aName )
{
    maMap.erase ( aName );
    if ( maProperties.hasElements() )
         maProperties.realloc( 0 );
}

Sequence< ::Property > SAL_CALL ChainablePropertySetInfo::getProperties()
{
    sal_Int32 nSize = maMap.size();
    if( maProperties.getLength() != nSize )
    {
        maProperties.realloc ( nSize );
        Property* pProperties = maProperties.getArray();

        for (auto const& elem : maMap)
        {
            PropertyInfo const * pInfo = elem.second;

            pProperties->Name = pInfo->maName;
            pProperties->Handle = pInfo->mnHandle;
            pProperties->Type = pInfo->maType;
            pProperties->Attributes = pInfo->mnAttributes;
            ++pProperties;
        }
    }
    return maProperties;
}

Property SAL_CALL ChainablePropertySetInfo::getPropertyByName( const OUString& rName )
{
    PropertyInfoHash::iterator aIter = maMap.find( rName );

    if ( maMap.end() == aIter )
        throw UnknownPropertyException( rName, *this );

    PropertyInfo const *pInfo = (*aIter).second;
    Property aProperty;
    aProperty.Name   = pInfo->maName;
    aProperty.Handle = pInfo->mnHandle;
    aProperty.Type = pInfo->maType;
    aProperty.Attributes = pInfo->mnAttributes;
    return aProperty;
}

sal_Bool SAL_CALL ChainablePropertySetInfo::hasPropertyByName( const OUString& rName )
{
    return maMap.contains( rName );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

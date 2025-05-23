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


#include <helper/property.hxx>
#include <map>

#include <helper/unopropertyarrayhelper.hxx>



UnoPropertyArrayHelper::UnoPropertyArrayHelper( const css::uno::Sequence<sal_Int32>& rIDs )
{
    for ( const sal_Int32 nID : rIDs )
        maIDs.insert( nID );
}

UnoPropertyArrayHelper::UnoPropertyArrayHelper( const std::vector< sal_uInt16 > &rIDs )
{
    for (const auto& rId : rIDs)
      maIDs.insert( rId );
}

bool UnoPropertyArrayHelper::ImplHasProperty( sal_uInt16 nPropId ) const
{
    if ( ( nPropId >= BASEPROPERTY_FONTDESCRIPTORPART_START ) && ( nPropId <= BASEPROPERTY_FONTDESCRIPTORPART_END ) )
        nPropId = BASEPROPERTY_FONTDESCRIPTOR;

    return maIDs.find( nPropId ) != maIDs.end();
}

// ::cppu::IPropertyArrayHelper
sal_Bool UnoPropertyArrayHelper::fillPropertyMembersByHandle( OUString * pPropName, sal_Int16 * pAttributes, sal_Int32 nPropId )
{
    sal_uInt16 id = sal::static_int_cast< sal_uInt16 >(nPropId);
    bool bValid = ImplHasProperty( id );
    if ( bValid )
    {
        if ( pPropName )
            *pPropName = GetPropertyName( id );
        if ( pAttributes )
            *pAttributes = GetPropertyAttribs( id );
    }
    return bValid;
}

css::uno::Sequence< css::beans::Property > UnoPropertyArrayHelper::getProperties()
{
    // Sort by names ...

    std::map<OUString, sal_uInt16> aSortedPropsIds;
    for (const auto& rId : maIDs)
    {
        sal_uInt16 nId = sal::static_int_cast< sal_uInt16 >(rId);
        aSortedPropsIds.emplace(GetPropertyName( nId ), nId);

        if ( nId == BASEPROPERTY_FONTDESCRIPTOR )
        {
            // single properties ...
            for ( sal_uInt16 i = BASEPROPERTY_FONTDESCRIPTORPART_START; i <= BASEPROPERTY_FONTDESCRIPTORPART_END; i++ )
                aSortedPropsIds.emplace(GetPropertyName( i ), i);
        }
    }

    sal_uInt32 nProps = aSortedPropsIds.size();   // could be more now
    css::uno::Sequence< css::beans::Property> aProps( nProps );
    css::beans::Property* pProps = aProps.getArray();

    sal_uInt32 n = 0;
    for ( const auto& rPropIds : aSortedPropsIds )
    {
        sal_uInt16 nId = rPropIds.second;
        pProps[n].Name = rPropIds.first;
        pProps[n].Handle = nId;
        pProps[n].Type = *GetPropertyType( nId );
        pProps[n].Attributes = GetPropertyAttribs( nId );
        ++n;
    }

    return aProps;
}

css::beans::Property UnoPropertyArrayHelper::getPropertyByName(const OUString& rPropertyName)
{
    css::beans::Property aProp;
    sal_uInt16 nId = GetPropertyId( rPropertyName );
    if ( ImplHasProperty( nId ) )
    {
        aProp.Name = rPropertyName;
        aProp.Handle = -1;
        aProp.Type = *GetPropertyType( nId );
        aProp.Attributes = GetPropertyAttribs( nId );
    }

    return aProp;
}

sal_Bool UnoPropertyArrayHelper::hasPropertyByName(const OUString& rPropertyName)
{
    return ImplHasProperty( GetPropertyId( rPropertyName ) );
}

sal_Int32 UnoPropertyArrayHelper::getHandleByName( const OUString & rPropertyName )
{
    sal_Int32 nId = static_cast<sal_Int32>(GetPropertyId( rPropertyName ));
    return nId ? nId : -1;
}

sal_Int32 UnoPropertyArrayHelper::fillHandles( sal_Int32* pHandles, const css::uno::Sequence< OUString > & rPropNames )
{
    const OUString* pNames = rPropNames.getConstArray();
    sal_Int32 nValues = rPropNames.getLength();
    sal_Int32 nValidHandles = 0;

    for ( sal_Int32 n = 0; n < nValues; n++ )
    {
        sal_uInt16 nPropId = GetPropertyId( pNames[n] );
        if ( nPropId && ImplHasProperty( nPropId ) )
        {
            pHandles[n] = nPropId;
            nValidHandles++;
        }
        else
        {
            pHandles[n] = -1;
        }
    }
    return nValidHandles;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

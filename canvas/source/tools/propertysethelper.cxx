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

#include <sal/config.h>

#include <string_view>

#include <propertysethelper.hxx>
#include <com/sun/star/beans/PropertyVetoException.hpp>
#include <com/sun/star/beans/UnknownPropertyException.hpp>

using namespace ::com::sun::star;

namespace canvas
{
    namespace
    {
        void throwUnknown( std::u16string_view aPropertyName )
        {
            throw beans::UnknownPropertyException(
                OUString::Concat("PropertySetHelper: property ") +
                aPropertyName + " not found."
                );
        }

        void throwVeto( std::u16string_view aPropertyName )
        {
            throw beans::PropertyVetoException(
                OUString::Concat("PropertySetHelper: property ") +
                aPropertyName + " access was vetoed." );
        }

        struct EntryComparator
        {
            bool operator()( const PropertySetHelper::MapType::MapEntry& rLHS,
                             const PropertySetHelper::MapType::MapEntry& rRHS )
            {
                return strcmp( rLHS.maKey,
                               rRHS.maKey ) < 0;
            }
        };
    }

    PropertySetHelper::PropertySetHelper()
    {
    }

    void PropertySetHelper::initProperties( InputMap&& rMap )
    {
        mpMap.reset();
        maMapEntries = std::move(rMap);

        std::sort( maMapEntries.begin(),
                   maMapEntries.end(),
                   EntryComparator() );

        if( !maMapEntries.empty() )
            mpMap.reset( new MapType(maMapEntries.data(),
                                     maMapEntries.size(),
                                     true) );
    }

    void PropertySetHelper::addProperties( const InputMap& rMap )
    {
        InputMap aMerged( maMapEntries );
        aMerged.insert( aMerged.end(),
                        rMap.begin(),
                        rMap.end() );

        initProperties( std::move(aMerged) );
    }

    bool PropertySetHelper::isPropertyName( const OUString& aPropertyName ) const
    {
        if (!mpMap)
            return false;

        Callbacks aDummy;
        return mpMap->lookup( aPropertyName,
                              aDummy );
    }

    uno::Reference< beans::XPropertySetInfo > PropertySetHelper::getPropertySetInfo() const
    {
        // we're a stealth property set
        return uno::Reference< beans::XPropertySetInfo >();
    }

    void PropertySetHelper::setPropertyValue( const OUString& aPropertyName,
                                              const uno::Any&        aValue )
    {
        Callbacks aCallbacks;
        if (!mpMap || !mpMap->lookup(aPropertyName, aCallbacks))
        {
            throwUnknown( aPropertyName );
        }

        if (!aCallbacks.setter)
            throwVeto( aPropertyName );

        aCallbacks.setter(aValue);
    }

    uno::Any PropertySetHelper::getPropertyValue( const OUString& aPropertyName ) const
    {
        Callbacks aCallbacks;
        if (!mpMap || !mpMap->lookup(aPropertyName, aCallbacks))
        {
            throwUnknown( aPropertyName );
        }

        if (aCallbacks.getter)
            return aCallbacks.getter();

        // TODO(Q1): subtlety, empty getter method silently returns
        // the empty any
        return uno::Any();
    }

    void PropertySetHelper::addPropertyChangeListener( const OUString&                                  aPropertyName,
                                                       const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/ )
    {
        // check validity of property, but otherwise ignore the
        // request
        if( !isPropertyName( aPropertyName ) )
            throwUnknown( aPropertyName );
    }

    void PropertySetHelper::addVetoableChangeListener( const OUString&                                  aPropertyName,
                                                       const uno::Reference< beans::XVetoableChangeListener >& /*xListener*/ )
    {
        // check validity of property, but otherwise ignore the
        // request
        if( !isPropertyName( aPropertyName ) )
            throwUnknown( aPropertyName );
    }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

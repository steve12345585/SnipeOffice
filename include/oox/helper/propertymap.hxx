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

#ifndef INCLUDED_OOX_HELPER_PROPERTYMAP_HXX
#define INCLUDED_OOX_HELPER_PROPERTYMAP_HXX

#include <map>
#include <utility>
#include <vector>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <oox/dllapi.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star::beans {
    struct PropertyValue;
    class XPropertySet;
}

namespace oox {


typedef ::std::map< OUString, css::uno::Any > PropertyNameMap;

/** A helper that maps property identifiers to property values.

    The property identifiers are generated on compile time and refer to the
    property name strings that are held by a static vector. The identifier to
    name mapping is done internally while the properties are written to
    property sets.
 */
class OOX_DLLPUBLIC PropertyMap
{
public:
    PropertyMap();

    /** Returns the name of the passed property identifier. */
    static const OUString& getPropertyName( sal_Int32 nPropId );

    /** Returns the property identifier of the passed name. */
    static sal_Int32 getPropertyId( std::u16string_view sPropName );

    /** Returns true, if the map contains a property with the passed identifier. */
    bool                hasProperty( sal_Int32 nPropId ) const;

    /** Sets the specified property to the passed value. Does nothing, if the
        identifier is invalid. */
    bool                setAnyProperty( sal_Int32 nPropId, const css::uno::Any& rValue );

    /** Sets the specified property to the passed value. Does nothing, if the
        identifier is invalid. */
    template< typename Type >
    bool                setProperty( sal_Int32 nPropId, Type&& rValue )
    {
        if( nPropId < 0 )
            return false;

        maProperties[ nPropId ] <<= std::forward<Type>(rValue);
        return true;
    }

    /** setAnyProperty should be used */
    bool                setProperty( sal_Int32, const css::uno::Any& ) = delete;

    css::uno::Any       getProperty( sal_Int32 nPropId );

    void                erase( sal_Int32 nPropId );

    bool                empty() const;

    /** Inserts all properties contained in the passed property map. */
    void                assignUsed( const PropertyMap& rPropMap );

    /** Inserts all properties contained in the passed property map */
    void                assignAll( const PropertyMap& rPropMap );

    /** Returns a sequence of property values, filled with all contained properties. */
    css::uno::Sequence< css::beans::PropertyValue >
                        makePropertyValueSequence() const;

    /** Fills the passed sequences of names and anys with all contained properties. */
    void                fillSequences(
                            css::uno::Sequence< OUString >& rNames,
                            css::uno::Sequence< css::uno::Any >& rValues ) const;

    void                fillPropertyNameMap(PropertyNameMap& rMap) const;

    /** Creates a property set supporting the XPropertySet interface and inserts all properties. */
    css::uno::Reference< css::beans::XPropertySet >
                        makePropertySet() const;

#if OSL_DEBUG_LEVEL > 0
#ifdef DBG_UTIL
  static void dump( const css::uno::Reference< css::beans::XPropertySet >& rXPropSet);
#endif
  static void dumpCode( const css::uno::Reference< css::beans::XPropertySet >& rXPropSet);
  static void dumpData( const css::uno::Reference<css::beans::XPropertySet>& rXPropSet);
#endif
private:
    const std::vector<OUString>* mpPropNames;
    std::map< sal_Int32, css::uno::Any > maProperties;
};


} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

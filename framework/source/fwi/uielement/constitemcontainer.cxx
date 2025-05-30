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

#include <uielement/constitemcontainer.hxx>
#include <uielement/itemcontainer.hxx>

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>

#include <comphelper/propertysetinfo.hxx>
#include <comphelper/servicehelper.hxx>
#include <rtl/ref.hxx>

using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::container;

const int PROPHANDLE_UINAME     = 1;
constexpr OUString PROPNAME_UINAME = u"UIName"_ustr;

namespace framework
{

ConstItemContainer::ConstItemContainer()
{
}

ConstItemContainer::ConstItemContainer( const ItemContainer& rItemContainer )
{
    ShareGuard aLock( rItemContainer.m_aShareMutex );
    copyItemContainer( rItemContainer.m_aItemVector );
}

ConstItemContainer::ConstItemContainer( const Reference< XIndexAccess >& rSourceContainer, bool bFastCopy )
{
    // We also have to copy the UIName property
    try
    {
        Reference< XPropertySet > xPropSet( rSourceContainer, UNO_QUERY );
        if ( xPropSet.is() )
        {
            xPropSet->getPropertyValue(u"UIName"_ustr) >>= m_aUIName;
        }
    }
    catch ( const Exception& )
    {
    }

    if ( !rSourceContainer.is() )
        return;

    try
    {
        sal_Int32 nCount = rSourceContainer->getCount();
        m_aItemVector.reserve(nCount);
        if ( bFastCopy )
        {
            for ( sal_Int32 i = 0; i < nCount; i++ )
            {
                Sequence< PropertyValue > aPropSeq;
                if ( rSourceContainer->getByIndex( i ) >>= aPropSeq )
                    m_aItemVector.push_back( aPropSeq );
            }
        }
        else
        {
            for ( sal_Int32 i = 0; i < nCount; i++ )
            {
                Sequence< PropertyValue > aPropSeq;
                if ( rSourceContainer->getByIndex( i ) >>= aPropSeq )
                {
                    sal_Int32 nContainerIndex = -1;
                    Reference< XIndexAccess > xIndexAccess;
                    for ( sal_Int32 j = 0; j < aPropSeq.getLength(); j++ )
                    {
                        if ( aPropSeq[j].Name == "ItemDescriptorContainer" )
                        {
                            aPropSeq[j].Value >>= xIndexAccess;
                            nContainerIndex = j;
                            break;
                        }
                    }

                    if ( xIndexAccess.is() && nContainerIndex >= 0 )
                        aPropSeq.getArray()[nContainerIndex].Value <<= Reference<XIndexAccess>(deepCopyContainer( xIndexAccess ));

                    m_aItemVector.push_back( aPropSeq );
                }
            }
        }
    }
    catch ( const IndexOutOfBoundsException& )
    {
    }
}

ConstItemContainer::~ConstItemContainer()
{
}

// private
void ConstItemContainer::copyItemContainer( const std::vector< Sequence< PropertyValue > >& rSourceVector )
{
    const sal_uInt32 nCount = rSourceVector.size();
    for ( sal_uInt32 i = 0; i < nCount; i++ )
    {
        sal_Int32 nContainerIndex = -1;
        Sequence< PropertyValue > aPropSeq( rSourceVector[i] );
        Reference< XIndexAccess > xIndexAccess;
        for ( sal_Int32 j = 0; j < aPropSeq.getLength(); j++ )
        {
            if ( aPropSeq[j].Name == "ItemDescriptorContainer" )
            {
                aPropSeq[j].Value >>= xIndexAccess;
                nContainerIndex = j;
                break;
            }
        }

        if ( xIndexAccess.is() && nContainerIndex >= 0 )
            aPropSeq.getArray()[nContainerIndex].Value <<= Reference<XIndexAccess>(deepCopyContainer( xIndexAccess ));

        m_aItemVector.push_back( aPropSeq );
    }
}

rtl::Reference< ConstItemContainer > ConstItemContainer::deepCopyContainer( const Reference< XIndexAccess >& rSubContainer )
{
    rtl::Reference< ConstItemContainer > xReturn;
    if ( rSubContainer.is() )
    {
        ItemContainer* pSource = dynamic_cast<ItemContainer*>( rSubContainer.get() );
        if ( pSource )
            xReturn = new ConstItemContainer( *pSource );
        else
            xReturn = new ConstItemContainer( rSubContainer );
    }

    return xReturn;
}

// XElementAccess
sal_Bool SAL_CALL ConstItemContainer::hasElements()
{
    return ( !m_aItemVector.empty() );
}

// XIndexAccess
sal_Int32 SAL_CALL ConstItemContainer::getCount()
{
    return m_aItemVector.size();
}

Any SAL_CALL ConstItemContainer::getByIndex( sal_Int32 Index )
{
    if ( sal_Int32( m_aItemVector.size()) <= Index )
        throw IndexOutOfBoundsException( OUString(), static_cast<OWeakObject *>(this) );
    return Any( m_aItemVector[Index] );
}

namespace
{
    std::vector<comphelper::PropertyMapEntry> makePropertyMap(const css::uno::Sequence<css::beans::Property>& rProps)
    {
        std::vector<comphelper::PropertyMapEntry> aEntries;
        for (auto const& it : rProps)
            aEntries.emplace_back(it.Name, it.Handle, it.Type, it.Attributes, 0);
        return aEntries;
    }
}

// XPropertySet
Reference< XPropertySetInfo > SAL_CALL ConstItemContainer::getPropertySetInfo()
{
    // Create structure of propertysetinfo for baseclass "OPropertySetHelper".
    // (Use method "getInfoHelper()".)
    static std::vector<comphelper::PropertyMapEntry> aPropertyInfos(makePropertyMap(getInfoHelper().getProperties()));
    static Reference< XPropertySetInfo > xInfo(new comphelper::PropertySetInfo(aPropertyInfos));

    return xInfo;
}

void SAL_CALL ConstItemContainer::setPropertyValue( const OUString&, const Any& )
{
}

Any SAL_CALL ConstItemContainer::getPropertyValue( const OUString& PropertyName )
{
    if ( PropertyName == PROPNAME_UINAME )
        return Any( m_aUIName );

    throw UnknownPropertyException(PropertyName);
}

void SAL_CALL ConstItemContainer::addPropertyChangeListener( const OUString&, const css::uno::Reference< css::beans::XPropertyChangeListener >& )
{
}

void SAL_CALL ConstItemContainer::removePropertyChangeListener( const OUString&, const css::uno::Reference< css::beans::XPropertyChangeListener >& )
{
    // Only read-only properties - do nothing
}

void SAL_CALL ConstItemContainer::addVetoableChangeListener( const OUString&, const css::uno::Reference< css::beans::XVetoableChangeListener >& )
{
    // Only read-only properties - do nothing
}

void SAL_CALL ConstItemContainer::removeVetoableChangeListener( const OUString&, const css::uno::Reference< css::beans::XVetoableChangeListener >& )
{
    // Only read-only properties - do nothing
}

// XFastPropertySet
void SAL_CALL ConstItemContainer::setFastPropertyValue( sal_Int32, const css::uno::Any& )
{
}

Any SAL_CALL ConstItemContainer::getFastPropertyValue( sal_Int32 nHandle )
{
    if ( nHandle == PROPHANDLE_UINAME )
        return Any( m_aUIName );

    throw UnknownPropertyException(OUString::number(nHandle));
}

::cppu::IPropertyArrayHelper& ConstItemContainer::getInfoHelper()
{
    // Define static member to give structure of properties to baseclass "OPropertySetHelper".
    // "impl_getStaticPropertyDescriptor" is a non exported and static function, who will define a static propertytable.
    // "true" say: Table is sorted by name.
    static ::cppu::OPropertyArrayHelper ourInfoHelper( impl_getStaticPropertyDescriptor(), true );

    return ourInfoHelper;
}

css::uno::Sequence< css::beans::Property > ConstItemContainer::impl_getStaticPropertyDescriptor()
{
    // Create a property array to initialize sequence!
    // Table of all predefined properties of this class. It's used from OPropertySetHelper-class!
    // Don't forget to change the defines (see begin of this file), if you add, change or delete a property in this list!!!
    // It's necessary for methods of OPropertySetHelper.
    // ATTENTION:
    //      YOU MUST SORT FOLLOW TABLE BY NAME ALPHABETICAL !!!

    return
    {
        css::beans::Property( PROPNAME_UINAME, PROPHANDLE_UINAME ,
                              cppu::UnoType<OUString>::get(),
                              css::beans::PropertyAttribute::TRANSIENT | css::beans::PropertyAttribute::READONLY  )
    };
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

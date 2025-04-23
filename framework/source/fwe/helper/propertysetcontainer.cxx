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

#include <helper/propertysetcontainer.hxx>

#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <cppuhelper/queryinterface.hxx>
#include <vcl/svapp.hxx>

constexpr OUString WRONG_TYPE_EXCEPTION = u"Only XPropertSet allowed!"_ustr;

using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::container;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;

namespace framework
{

PropertySetContainer::PropertySetContainer()
{
}

PropertySetContainer::~PropertySetContainer()
{
}

// XIndexContainer
void SAL_CALL PropertySetContainer::insertByIndex( sal_Int32 Index, const css::uno::Any& Element )
{
    std::unique_lock g(m_aMutex);

    sal_Int32 nSize = m_aPropertySetVector.size();

    if ( nSize < Index )
        throw IndexOutOfBoundsException( OUString(), static_cast<OWeakObject *>(this) );

    Reference< XPropertySet > aPropertySetElement;

    if ( !(Element >>= aPropertySetElement) )
    {
        throw IllegalArgumentException(
            WRONG_TYPE_EXCEPTION,
            static_cast<OWeakObject *>(this), 2 );
    }

    if ( nSize == Index )
        m_aPropertySetVector.push_back( aPropertySetElement );
    else
    {
        PropertySetVector::iterator aIter = m_aPropertySetVector.begin();
        aIter += Index;
        m_aPropertySetVector.insert( aIter, aPropertySetElement );
    }
}

void SAL_CALL PropertySetContainer::removeByIndex( sal_Int32 nIndex )
{
    std::unique_lock g(m_aMutex);

    if ( static_cast<sal_Int32>(m_aPropertySetVector.size()) <= nIndex )
        throw IndexOutOfBoundsException( OUString(), static_cast<OWeakObject *>(this) );

    m_aPropertySetVector.erase(m_aPropertySetVector.begin() +  nIndex);
}

// XIndexReplace
void SAL_CALL PropertySetContainer::replaceByIndex( sal_Int32 Index, const css::uno::Any& Element )
{
    std::unique_lock g(m_aMutex);

    if ( static_cast<sal_Int32>(m_aPropertySetVector.size()) <= Index )
        throw IndexOutOfBoundsException( OUString(), static_cast<OWeakObject *>(this) );

    Reference< XPropertySet > aPropertySetElement;

    if ( !(Element >>= aPropertySetElement) )
    {
        throw IllegalArgumentException(
            WRONG_TYPE_EXCEPTION,
            static_cast<OWeakObject *>(this), 2 );
    }

    m_aPropertySetVector[ Index ] = std::move(aPropertySetElement);
}

// XIndexAccess
sal_Int32 SAL_CALL PropertySetContainer::getCount()
{
    std::unique_lock g(m_aMutex);

    return m_aPropertySetVector.size();
}

Any SAL_CALL PropertySetContainer::getByIndex( sal_Int32 Index )
{
    std::unique_lock g(m_aMutex);

    if ( static_cast<sal_Int32>(m_aPropertySetVector.size()) <= Index )
        throw IndexOutOfBoundsException( OUString(), static_cast<OWeakObject *>(this) );

    return Any(m_aPropertySetVector[ Index ]);
}

// XElementAccess
sal_Bool SAL_CALL PropertySetContainer::hasElements()
{
    std::unique_lock g(m_aMutex);

    return !( m_aPropertySetVector.empty() );
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

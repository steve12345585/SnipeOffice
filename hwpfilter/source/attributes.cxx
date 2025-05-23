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


#include <assert.h>
#include <utility>
#include <vector>
#include "attributes.hxx"

namespace {

struct TagAttribute
{
    TagAttribute( OUString aName, OUString aType, OUString aValue )
       : sName(std::move(aName)), sType(std::move(aType)), sValue(std::move(aValue))
    {
    }

    OUString sName;
    OUString sType;
    OUString sValue;
};

}

struct AttributeListImpl_impl
{
    AttributeListImpl_impl()
    {
// performance improvement during adding
        vecAttribute.reserve(20);
    }
    std::vector<struct TagAttribute> vecAttribute;
};

sal_Int16 SAL_CALL AttributeListImpl::getLength()
{
    return static_cast<sal_Int16>(m_pImpl->vecAttribute.size());
}


AttributeListImpl::AttributeListImpl( const AttributeListImpl &r )
 : cppu::WeakImplHelper<css::xml::sax::XAttributeList>( r ),
   m_pImpl( new AttributeListImpl_impl )
{
    *m_pImpl = *(r.m_pImpl);
}


OUString AttributeListImpl::getNameByIndex(sal_Int16 i)
{
    sal_uInt32 i2 = sal::static_int_cast<sal_Int16>(i);
    if( i >= 0 &&  i2 < m_pImpl->vecAttribute.size() )
    {
        return m_pImpl->vecAttribute[i].sName;
    }
    return OUString();
}


OUString AttributeListImpl::getTypeByIndex(sal_Int16 i)
{
    sal_uInt32 i2 = sal::static_int_cast<sal_Int16>(i);
    if( i >= 0 &&  i2 < m_pImpl->vecAttribute.size() )
    {
        return m_pImpl->vecAttribute[i].sType;
    }
    return OUString();
}


OUString AttributeListImpl::getValueByIndex(sal_Int16 i)
{
    sal_uInt32 i2 = sal::static_int_cast<sal_Int16>(i);
    if( i >= 0 &&  i2 < m_pImpl->vecAttribute.size() )
    {
        return m_pImpl->vecAttribute[i].sValue;
    }
    return OUString();

}


OUString AttributeListImpl::getTypeByName( const OUString& sName )
{
    for (auto const& elem : m_pImpl->vecAttribute)
    {
        if( elem.sName == sName )
        {
            return elem.sType;
        }
    }
    return OUString();
}


OUString AttributeListImpl::getValueByName(const OUString& sName)
{
    for (auto const& elem : m_pImpl->vecAttribute)
    {
        if( elem.sName == sName )
        {
            return elem.sValue;
        }
    }
    return OUString();
}


AttributeListImpl::AttributeListImpl()
    : m_pImpl( new AttributeListImpl_impl )
{
}


AttributeListImpl::~AttributeListImpl()
{
}


void AttributeListImpl::addAttribute(   const OUString &sName ,
const OUString &sType ,
const OUString &sValue )
{
    m_pImpl->vecAttribute.emplace_back( sName , sType , sValue );
}


void AttributeListImpl::clear()
{
    std::vector<struct TagAttribute>().swap(m_pImpl->vecAttribute);

    assert( ! getLength() );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

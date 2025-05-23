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

#include <comphelper/attributelist.hxx>
#include "MutableAttrList.hxx"


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;

comphelper::AttributeList* XMLMutableAttributeList::GetMutableAttrList()
{
    if( !m_pMutableAttrList )
    {
        m_pMutableAttrList = new comphelper::AttributeList( m_xAttrList );
        m_xAttrList = m_pMutableAttrList;
    }

    return m_pMutableAttrList.get();
}

XMLMutableAttributeList::XMLMutableAttributeList() :
    m_pMutableAttrList( new comphelper::AttributeList )
{
    m_xAttrList = m_pMutableAttrList;
}

XMLMutableAttributeList::XMLMutableAttributeList( const Reference<
        XAttributeList> & rAttrList, bool bClone ) :
    m_xAttrList( rAttrList.is() ? rAttrList : new comphelper::AttributeList )
{
    if( bClone )
        GetMutableAttrList();
}


XMLMutableAttributeList::~XMLMutableAttributeList()
{
    m_xAttrList = nullptr;
}

sal_Int16 SAL_CALL XMLMutableAttributeList::getLength()
{
    return m_xAttrList->getLength();
}


OUString SAL_CALL XMLMutableAttributeList::getNameByIndex(sal_Int16 i)
{
    return m_xAttrList->getNameByIndex( i );
}


OUString SAL_CALL XMLMutableAttributeList::getTypeByIndex(sal_Int16 i)
{
    return m_xAttrList->getTypeByIndex( i );
}

OUString SAL_CALL  XMLMutableAttributeList::getValueByIndex(sal_Int16 i)
{
    return m_xAttrList->getValueByIndex( i );
}

OUString SAL_CALL XMLMutableAttributeList::getTypeByName(
        const OUString& rName )
{
    return m_xAttrList->getTypeByName( rName );
}

OUString SAL_CALL XMLMutableAttributeList::getValueByName(
        const OUString& rName)
{
    return m_xAttrList->getValueByName( rName );
}


Reference< XCloneable > XMLMutableAttributeList::createClone()
{
    // A cloned list will be a read only list!
    Reference< XCloneable >  r = new comphelper::AttributeList( m_xAttrList );
    return r;
}

void XMLMutableAttributeList::SetValueByIndex( sal_Int16 i,
                                               const OUString& rValue )
{
    GetMutableAttrList()->SetValueByIndex( i, rValue );
}

void XMLMutableAttributeList::AddAttribute( const OUString &rName ,
                                            const OUString &rValue )
{
    GetMutableAttrList()->AddAttribute( rName, rValue );
}

void XMLMutableAttributeList::RemoveAttributeByIndex( sal_Int16 i )
{
    GetMutableAttrList()->RemoveAttributeByIndex( i );
}

void XMLMutableAttributeList::RenameAttributeByIndex( sal_Int16 i,
                                                      const OUString& rNewName )
{
    GetMutableAttrList()->RenameAttributeByIndex( i, rNewName );
}

void XMLMutableAttributeList::AppendAttributeList(
        const Reference< css::xml::sax::XAttributeList >& r )
{
    GetMutableAttrList()->AppendAttributeList( r );
}

sal_Int16 XMLMutableAttributeList::GetIndexByName( const OUString& rName ) const
{
    sal_Int16 nIndex = -1;
    if( m_pMutableAttrList )
    {
        nIndex = m_pMutableAttrList->GetIndexByName( rName );
    }
    else
    {
        sal_Int16 nCount = m_xAttrList->getLength();
        for( sal_Int16 i=0; nIndex==-1 && i<nCount ; ++i )
        {
            if( m_xAttrList->getNameByIndex(i) == rName )
                nIndex = i;
        }
    }
    return nIndex;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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


#include <svl/slstitm.hxx>
#include <svl/poolitem.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <comphelper/sequence.hxx>
#include <osl/diagnose.h>
#include <rtl/ustrbuf.hxx>
#include <tools/lineend.hxx>

SfxPoolItem* SfxStringListItem::CreateDefault() { return new SfxStringListItem; }

SfxStringListItem::SfxStringListItem() :
    SfxPoolItem( 0 )
{
}


SfxStringListItem::SfxStringListItem( sal_uInt16 which, const std::vector<OUString>* pList ) :
    SfxPoolItem( which )
{
    // FIXME: Putting an empty list does not work
    // Therefore the query after the count is commented out
    if( pList /*!!! && pList->Count() */ )
    {
        mpList = std::make_shared<std::vector<OUString>>(*pList);
    }
}


SfxStringListItem::~SfxStringListItem()
{
}


std::vector<OUString>& SfxStringListItem::GetList()
{
    if( !mpList )
        mpList = std::make_shared<std::vector<OUString>>();
    return *mpList;
}

const std::vector<OUString>& SfxStringListItem::GetList () const
{
    return const_cast< SfxStringListItem * >(this)->GetList();
}


bool SfxStringListItem::operator==( const SfxPoolItem& rItem ) const
{
    assert(SfxPoolItem::operator==(rItem));

    const SfxStringListItem& rSSLItem = static_cast<const SfxStringListItem&>(rItem);

    return mpList == rSSLItem.mpList;
}


bool SfxStringListItem::GetPresentation
(
    SfxItemPresentation     /*ePresentation*/,
    MapUnit                 /*eCoreMetric*/,
    MapUnit                 /*ePresentationMetric*/,
    OUString&               rText,
    const IntlWrapper&
)   const
{
    rText = "(List)";
    return false;
}

SfxStringListItem* SfxStringListItem::Clone( SfxItemPool *) const
{
    return new SfxStringListItem( *this );
}

void SfxStringListItem::SetString( const OUString& rStr )
{
    mpList = std::make_shared<std::vector<OUString>>();

    OUString aStr(convertLineEnd(rStr, LINEEND_CR));
    // put last string only if not empty
    for (sal_Int32 nStart = 0; nStart >= 0 && nStart < aStr.getLength();)
        mpList->push_back(aStr.getToken(0, '\r', nStart));
}


OUString SfxStringListItem::GetString()
{
    OUStringBuffer aStr;
    if ( mpList )
    {
        for (auto iter = mpList->begin(), end = mpList->end(); iter != end;)
        {
            aStr.append(*iter);
            ++iter;

            if (iter == end)
                break;

            aStr.append(SAL_NEWLINE_STRING);
        }
    }
    return aStr.makeStringAndClear();
}


void SfxStringListItem::SetStringList( const css::uno::Sequence< OUString >& rList )
{
    mpList = std::make_shared<std::vector<OUString>>(
        comphelper::sequenceToContainer<std::vector<OUString>>(rList));
}

void SfxStringListItem::GetStringList( css::uno::Sequence< OUString >& rList ) const
{
    size_t nCount = mpList->size();

    rList.realloc( nCount );
    auto pList = rList.getArray();
    for( size_t i=0; i < nCount; i++ )
        pList[i] = (*mpList)[i];
}

// virtual
bool SfxStringListItem::PutValue( const css::uno::Any& rVal, sal_uInt8 )
{
    css::uno::Sequence< OUString > aValue;
    if ( rVal >>= aValue )
    {
        SetStringList( aValue );
        return true;
    }

    OSL_FAIL( "SfxStringListItem::PutValue - Wrong type!" );
    return false;
}

// virtual
bool SfxStringListItem::QueryValue( css::uno::Any& rVal, sal_uInt8 ) const
{
    css::uno::Sequence< OUString > aStringList;
    GetStringList( aStringList );
    rVal <<= aStringList;
    return true;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

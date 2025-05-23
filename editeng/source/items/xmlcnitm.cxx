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

#include <memory>

#include <comphelper/servicehelper.hxx>
#include <com/sun/star/xml/AttributeData.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <o3tl/any.hxx>
#include <xmloff/xmlcnimp.hxx>
#include <xmloff/unoatrcn.hxx>
#include <editeng/xmlcnitm.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::xml;


SvXMLAttrContainerItem::SvXMLAttrContainerItem( sal_uInt16 _nWhich ) :
    SfxPoolItem( _nWhich )
{
}

SvXMLAttrContainerItem::SvXMLAttrContainerItem(
                                        const SvXMLAttrContainerItem& rItem ) :
    SfxPoolItem( rItem ),
    maContainerData( rItem.maContainerData )
{
}

SvXMLAttrContainerItem::~SvXMLAttrContainerItem()
{
}

bool SvXMLAttrContainerItem::operator==( const SfxPoolItem& rItem ) const
{
    return SfxPoolItem::operator==(rItem) &&
        maContainerData == static_cast<const SvXMLAttrContainerItem&>(rItem).maContainerData;
}

bool SvXMLAttrContainerItem::GetPresentation(
                    SfxItemPresentation /*ePresentation*/,
                    MapUnit /*eCoreMetric*/,
                    MapUnit /*ePresentationMetric*/,
                    OUString & /*rText*/,
                    const IntlWrapper& /*rIntlWrapper*/ ) const
{
    return false;
}

bool SvXMLAttrContainerItem::QueryValue( css::uno::Any& rVal, sal_uInt8 /*nMemberId*/ ) const
{
    Reference<XNameContainer> xContainer
        = new SvUnoAttributeContainer(std::make_unique<SvXMLAttrContainerData>(maContainerData));

    rVal <<= xContainer;
    return true;
}

bool SvXMLAttrContainerItem::PutValue( const css::uno::Any& rVal, sal_uInt8 /*nMemberId*/ )
{
    Reference<XInterface> xTunnel(rVal, UNO_QUERY);
    if (auto pContainer = dynamic_cast<SvUnoAttributeContainer*>(xTunnel.get()))
    {
        maContainerData = *pContainer->GetContainerImpl();
    }
    else
    {
        SvXMLAttrContainerData aNewImpl;

        try
        {
            Reference<XNameContainer> xContainer( rVal, UNO_QUERY );
            if( !xContainer.is() )
                return false;

            const Sequence< OUString > aNameSequence( xContainer->getElementNames() );
            const OUString* pNames = aNameSequence.getConstArray();
            const sal_Int32 nCount = aNameSequence.getLength();
            Any aAny;
            sal_Int32 nAttr;

            for( nAttr = 0; nAttr < nCount; nAttr++ )
            {
                const OUString aName( *pNames++ );

                aAny = xContainer->getByName( aName );
                auto pData = o3tl::tryAccess<AttributeData>(aAny);
                if( !pData )
                    return false;

                sal_Int32 pos = aName.indexOf( ':' );
                if( pos != -1 )
                {
                    const OUString aPrefix( aName.copy( 0, pos ));
                    const OUString aLName( aName.copy( pos+1 ));

                    if( pData->Namespace.isEmpty() )
                    {
                        if( !aNewImpl.AddAttr( aPrefix, aLName, pData->Value ) )
                            break;
                    }
                    else
                    {
                        if( !aNewImpl.AddAttr( aPrefix, pData->Namespace, aLName, pData->Value ) )
                            break;
                    }
                }
                else
                {
                    if( !aNewImpl.AddAttr( aName, pData->Value ) )
                        break;
                }
            }

            if( nAttr == nCount )
                maContainerData = std::move(aNewImpl);
            else
                return false;
        }
        catch(...)
        {
            return false;
        }
    }
    return true;
}


bool SvXMLAttrContainerItem::AddAttr( const OUString& rLName,
                                        const OUString& rValue )
{
    return maContainerData.AddAttr( rLName, rValue );
}

bool SvXMLAttrContainerItem::AddAttr( const OUString& rPrefix,
          const OUString& rNamespace, const OUString& rLName,
          const OUString& rValue )
{
    return maContainerData.AddAttr( rPrefix, rNamespace, rLName, rValue );
}

sal_uInt16 SvXMLAttrContainerItem::GetAttrCount() const
{
    return static_cast<sal_uInt16>(maContainerData.GetAttrCount());
}

OUString SvXMLAttrContainerItem::GetAttrNamespace( sal_uInt16 i ) const
{
    return maContainerData.GetAttrNamespace( i );
}

OUString SvXMLAttrContainerItem::GetAttrPrefix( sal_uInt16 i ) const
{
    return maContainerData.GetAttrPrefix( i );
}

const OUString& SvXMLAttrContainerItem::GetAttrLName( sal_uInt16 i ) const
{
    return maContainerData.GetAttrLName( i );
}

const OUString& SvXMLAttrContainerItem::GetAttrValue( sal_uInt16 i ) const
{
    return maContainerData.GetAttrValue( i );
}


sal_uInt16 SvXMLAttrContainerItem::GetFirstNamespaceIndex() const
{
    return maContainerData.GetFirstNamespaceIndex();
}

sal_uInt16 SvXMLAttrContainerItem::GetNextNamespaceIndex( sal_uInt16 nIdx ) const
{
    return maContainerData.GetNextNamespaceIndex( nIdx );
}

const OUString& SvXMLAttrContainerItem::GetNamespace( sal_uInt16 i ) const
{
    return maContainerData.GetNamespace( i );
}

const OUString& SvXMLAttrContainerItem::GetPrefix( sal_uInt16 i ) const
{
    return maContainerData.GetPrefix( i );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

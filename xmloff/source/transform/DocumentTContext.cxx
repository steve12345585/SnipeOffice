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

#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <xmloff/namespacemap.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>

#include "TransformerBase.hxx"
#include "MutableAttrList.hxx"

#include "DocumentTContext.hxx"


using namespace ::xmloff::token;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;
using namespace ::com::sun::star::beans;

XMLDocumentTransformerContext::XMLDocumentTransformerContext( XMLTransformerBase& rImp,
                                                const OUString& rQName ) :
    XMLTransformerContext( rImp, rQName )
{
}

void XMLDocumentTransformerContext::StartElement( const Reference< XAttributeList >& rAttrList )
{
    Reference< XAttributeList > xAttrList( rAttrList );

    bool bMimeFound = false;
    OUString aClass;
    OUString aClassQName(
                    GetTransformer().GetNamespaceMap().GetQNameByKey(
                                XML_NAMESPACE_OFFICE, GetXMLToken(XML_CLASS ) ) );

    rtl::Reference<XMLMutableAttributeList> pMutableAttrList;
    sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
    for( sal_Int16 i=0; i < nAttrCount; i++ )
    {
        const OUString aAttrName = xAttrList->getNameByIndex( i );
        OUString aLocalName;
        sal_uInt16 nPrefix =
            GetTransformer().GetNamespaceMap().GetKeyByAttrName( aAttrName,
                                                                 &aLocalName );
        if( XML_NAMESPACE_OFFICE == nPrefix &&
            IsXMLToken( aLocalName, XML_MIMETYPE ) )
        {
            const OUString aValue = xAttrList->getValueByIndex( i );
            static constexpr std::string_view aTmp[]
            {
                "application/vnd.oasis.openoffice.",
                "application/x-vnd.oasis.openoffice.",
                "application/vnd.oasis.opendocument.",
                "application/x-vnd.oasis.document."
            };
            for (const auto & rPrefix : aTmp)
            {
                if( aValue.matchAsciiL( rPrefix.data(), rPrefix.size() ) )
                {
                    aClass = aValue.copy( rPrefix.size() );
                    break;
                }
            }

            if( !pMutableAttrList )
            {
                pMutableAttrList = new XMLMutableAttributeList( xAttrList );
                xAttrList = pMutableAttrList;
            }
            pMutableAttrList->SetValueByIndex( i, aClass );
            pMutableAttrList->RenameAttributeByIndex(i, aClassQName );
            bMimeFound = true;
            break;
        }
    }

    if( !bMimeFound )
    {
        const Reference< XPropertySet > rPropSet =
            GetTransformer().GetPropertySet();

        if( rPropSet.is() )
        {
            Reference< XPropertySetInfo > xPropSetInfo(
                rPropSet->getPropertySetInfo() );
            OUString aPropName(u"Class"_ustr);
            if( xPropSetInfo.is() && xPropSetInfo->hasPropertyByName( aPropName ) )
            {
                Any aAny = rPropSet->getPropertyValue( aPropName );
                aAny >>= aClass;
            }
        }

        if( !aClass.isEmpty() )
        {
            if( !pMutableAttrList )
            {
                pMutableAttrList = new XMLMutableAttributeList( xAttrList );
                xAttrList = pMutableAttrList;
            }

            pMutableAttrList->AddAttribute( aClassQName, aClass );
        }
    }
    XMLTransformerContext::StartElement( xAttrList );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

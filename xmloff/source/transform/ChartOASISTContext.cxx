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

#include "ChartOASISTContext.hxx"
#include "MutableAttrList.hxx"
#include <xmloff/xmlnamespace.hxx>
#include "ActionMapTypesOASIS.hxx"
#include "AttrTransformerAction.hxx"
#include "TransformerActions.hxx"
#include "TransformerBase.hxx"
#include <osl/diagnose.h>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;
using namespace ::xmloff::token;

XMLChartOASISTransformerContext::XMLChartOASISTransformerContext(
        XMLTransformerBase& rImp,
        const OUString& rQName ) :
    XMLTransformerContext( rImp, rQName )
{
}

XMLChartOASISTransformerContext::~XMLChartOASISTransformerContext()
{
}

void XMLChartOASISTransformerContext::StartElement(
    const Reference< XAttributeList >& rAttrList )
{
    XMLTransformerActions *pActions =
        GetTransformer().GetUserDefinedActions( OASIS_CHART_ACTIONS );
    OSL_ENSURE( pActions, "go no actions" );

    OUString aAddInName;
    Reference< XAttributeList > xAttrList( rAttrList );
    rtl::Reference<XMLMutableAttributeList> pMutableAttrList;
    sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
    for( sal_Int16 i=0; i < nAttrCount; i++ )
    {
        const OUString aAttrName = xAttrList->getNameByIndex( i );
        OUString aLocalName;
        sal_uInt16 nPrefix =
            GetTransformer().GetNamespaceMap().GetKeyByAttrName( aAttrName,
                                                                 &aLocalName );
        XMLTransformerActions::key_type aKey( nPrefix, aLocalName );
        XMLTransformerActions::const_iterator aIter =
            pActions->find( aKey );
        if( aIter != pActions->end() )
        {
            if( !pMutableAttrList )
            {
                pMutableAttrList =
                        new XMLMutableAttributeList( xAttrList );
                xAttrList = pMutableAttrList;
            }
            const OUString aAttrValue = xAttrList->getValueByIndex( i );
            switch( (*aIter).second.m_nActionType )
            {
            case XML_ATACTION_IN2INCH:
                {
                    OUString aAttrValue2( aAttrValue );
                    if( XMLTransformerBase::ReplaceSingleInWithInch(
                                aAttrValue2 ) )
                        pMutableAttrList->SetValueByIndex( i, aAttrValue2 );
                }
                break;
            case XML_ATACTION_DECODE_STYLE_NAME_REF:
                {
                    OUString aAttrValue2( aAttrValue );
                    if( XMLTransformerBase::DecodeStyleName(aAttrValue2) )
                        pMutableAttrList->SetValueByIndex( i, aAttrValue2 );
                }
                break;
            case XML_ATACTION_REMOVE_ANY_NAMESPACE_PREFIX:
                OSL_ENSURE( IsXMLToken( aLocalName, XML_CLASS ),
                               "unexpected class token" );
                {
                    OUString aChartClass;
                    sal_uInt16 nValuePrefix =
                        GetTransformer().GetNamespaceMap().GetKeyByAttrName(
                            aAttrValue,
                            &aChartClass );
                    if( XML_NAMESPACE_CHART == nValuePrefix )
                    {
                        pMutableAttrList->SetValueByIndex( i, aChartClass );
                    }
                    else if ( XML_NAMESPACE_OOO == nValuePrefix )
                    {
                        pMutableAttrList->SetValueByIndex( i,
                                                GetXMLToken(XML_ADD_IN ) );
                        aAddInName = aChartClass;
                    }
                }
                break;
            default:
                OSL_ENSURE( false, "unknown action" );
                break;
            }
        }
    }

    if( !aAddInName.isEmpty() )
    {
        OUString aAttrQName( GetTransformer().GetNamespaceMap().GetQNameByKey(
                                XML_NAMESPACE_CHART,
                                GetXMLToken( XML_ADD_IN_NAME ) ) );
        assert(pMutableAttrList && "coverity[var_deref_model] - pMutableAttrList should be assigned in a superset of the enclosing if condition entry logic");
        pMutableAttrList->AddAttribute( aAttrQName, aAddInName );
    }

    XMLTransformerContext::StartElement( xAttrList );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

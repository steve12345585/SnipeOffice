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

#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/xml/AttributeData.hpp>
#include <com/sun/star/uno/Any.hxx>

#include <AttributeContainerHandler.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::container;




XMLAttributeContainerHandler::~XMLAttributeContainerHandler()
{
    // nothing to do
}

bool XMLAttributeContainerHandler::equals(
        const Any& r1,
        const Any& r2 ) const
{
    Reference< XNameContainer > xContainer1;
    Reference< XNameContainer > xContainer2;

    if( ( r1 >>= xContainer1 ) && ( r2 >>= xContainer2 ) )
    {
        const uno::Sequence< OUString > aAttribNames1( xContainer1->getElementNames() );
        uno::Sequence< OUString > aAttribNames2( xContainer2->getElementNames() );

        if( aAttribNames1.getLength() == aAttribNames2.getLength() )
        {
            xml::AttributeData aData1;
            xml::AttributeData aData2;

            for( const OUString& rAttribName : aAttribNames1 )
            {
                if( !xContainer2->hasByName( rAttribName ) )
                    return false;

                xContainer1->getByName( rAttribName ) >>= aData1;
                xContainer2->getByName( rAttribName ) >>= aData2;

                if( ( aData1.Namespace != aData2.Namespace ) ||
                    ( aData1.Type      != aData2.Type      ) ||
                    ( aData1.Value     != aData2.Value     ) )
                    return false;
            }

            return true;
        }
    }

    return false;
}

bool XMLAttributeContainerHandler::importXML( const OUString& /*rStrImpValue*/, Any& /*rValue*/, const SvXMLUnitConverter& /*rUnitConverter*/ ) const
{
    return true;
}

bool XMLAttributeContainerHandler::exportXML( OUString& /*rStrExpValue*/, const Any& /*rValue*/, const SvXMLUnitConverter& /*rUnitConverter*/ ) const
{
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

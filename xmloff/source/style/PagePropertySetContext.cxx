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

#include <tools/debug.hxx>
#include "PagePropertySetContext.hxx"
#include <XMLBackgroundImageContext.hxx>
#include <XMLTextColumnsContext.hxx>
#include <PageMasterStyleMap.hxx>
#include "XMLFootnoteSeparatorImport.hxx"
#include <xmloff/xmlimppr.hxx>
#include <xmloff/xmlprmap.hxx>


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;

PagePropertySetContext::PagePropertySetContext(
                 SvXMLImport& rImport, sal_Int32 nElement,
                 const Reference< xml::sax::XFastAttributeList > & xAttrList,
                 sal_uInt32 nFam,
                 ::std::vector< XMLPropertyState > &rProps,
                 SvXMLImportPropertyMapper* pMap,
                 sal_Int32 nStartIndex, sal_Int32 nEndIndex,
                 const PageContextType aTempType ) :
    SvXMLPropertySetContext( rImport, nElement, xAttrList, nFam,
                             rProps, pMap, nStartIndex, nEndIndex )
{
    aType = aTempType;
}

PagePropertySetContext::~PagePropertySetContext()
{
}

css::uno::Reference< css::xml::sax::XFastContextHandler > PagePropertySetContext::createFastChildContext(
    sal_Int32 nElement,
    const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList,
    ::std::vector< XMLPropertyState > &rProperties,
    const XMLPropertyState& rProp )
{
    sal_Int32 nPos = CTF_PM_GRAPHICPOSITION;
    sal_Int32 nFil = CTF_PM_GRAPHICFILTER;
    switch ( aType )
    {
        case Header:
        {
            nPos = CTF_PM_HEADERGRAPHICPOSITION;
            nFil = CTF_PM_HEADERGRAPHICFILTER;
        }
        break;
        case Footer:
        {
            nPos = CTF_PM_FOOTERGRAPHICPOSITION;
            nFil = CTF_PM_FOOTERGRAPHICFILTER;
        }
        break;
        default:
            break;
    }

    switch( mpMapper->getPropertySetMapper()
                    ->GetEntryContextId( rProp.mnIndex ) )
    {
    case CTF_PM_GRAPHICURL:
    case CTF_PM_HEADERGRAPHICURL:
    case CTF_PM_FOOTERGRAPHICURL:
        DBG_ASSERT( rProp.mnIndex >= 2 &&
                    nPos  == mpMapper->getPropertySetMapper()
                        ->GetEntryContextId( rProp.mnIndex-2 ) &&
                    nFil  == mpMapper->getPropertySetMapper()
                        ->GetEntryContextId( rProp.mnIndex-1 ),
                    "invalid property map!");
        return
            new XMLBackgroundImageContext( GetImport(), nElement,
                                           xAttrList,
                                           rProp,
                                           rProp.mnIndex-2,
                                           rProp.mnIndex-1,
                                           -1,
                                           mpMapper->getPropertySetMapper()->FindEntryIndex(CTF_PM_FILLBITMAPMODE),
                                           rProperties );
        break;

    case CTF_PM_TEXTCOLUMNS:
        return new XMLTextColumnsContext(GetImport(), nElement, xAttrList, rProp, rProperties);
        break;

    case CTF_PM_FTN_LINE_WEIGHT:
        return new XMLFootnoteSeparatorImport(
            GetImport(), nElement, rProperties,
            mpMapper->getPropertySetMapper(), rProp.mnIndex);
        break;
    }

    return SvXMLPropertySetContext::createFastChildContext( nElement,
                                                            xAttrList,
                                                            rProperties, rProp );
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

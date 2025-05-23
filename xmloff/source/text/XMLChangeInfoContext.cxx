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

#include "XMLChangeInfoContext.hxx"
#include "XMLChangedRegionImportContext.hxx"
#include <XMLStringBufferImportContext.hxx>
#include <com/sun/star/uno/Reference.h>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlimp.hxx>
#include <sal/log.hxx>


using namespace ::xmloff::token;

using ::com::sun::star::uno::Reference;


XMLChangeInfoContext::XMLChangeInfoContext(
    SvXMLImport& rImport,
    XMLChangedRegionImportContext& rPParent,
    const OUString& rChangeType)
:   SvXMLImportContext(rImport)
,   rType(rChangeType)
,   rChangedRegion(rPParent)
{
}

XMLChangeInfoContext::~XMLChangeInfoContext()
{
}

css::uno::Reference< css::xml::sax::XFastContextHandler > XMLChangeInfoContext::createFastChildContext(
    sal_Int32 nElement,
    const css::uno::Reference< css::xml::sax::XFastAttributeList >& )
{
    SvXMLImportContextRef xContext;

    switch (nElement)
    {
        case XML_ELEMENT(DC, XML_CREATOR):
            xContext = new XMLStringBufferImportContext(GetImport(), sAuthorBuffer);
            break;
        case XML_ELEMENT(DC, XML_DATE):
            xContext = new XMLStringBufferImportContext(GetImport(), sDateTimeBuffer);
            break;
        case XML_ELEMENT(LO_EXT, XML_MOVE_ID):
            xContext = new XMLStringBufferImportContext(GetImport(), sMovedIDBuffer);
            break;
        case XML_ELEMENT(TEXT, XML_P):
        case XML_ELEMENT(LO_EXT, XML_P):
            xContext = new XMLStringBufferImportContext(GetImport(), sCommentBuffer);
            break;
        default:
            XMLOFF_WARN_UNKNOWN_ELEMENT("xmloff", nElement);
    }

    return xContext;
}

void XMLChangeInfoContext::endFastElement(sal_Int32 )
{
    // set values at changed region context
    rChangedRegion.SetChangeInfo(rType, sAuthorBuffer.makeStringAndClear(),
                                 sCommentBuffer.makeStringAndClear(), sDateTimeBuffer,
                                 sMovedIDBuffer.makeStringAndClear());
    sDateTimeBuffer.setLength(0);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include "xmlConnectionResource.hxx"
#include "xmlfilter.hxx"
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <strings.hxx>
#include <comphelper/diagnose_ex.hxx>

namespace dbaxml
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::xml::sax;

OXMLConnectionResource::OXMLConnectionResource( ODBFilter& rImport,
                const Reference< XFastAttributeList > & _xAttrList) :
    SvXMLImportContext( rImport )
{
    Reference<XPropertySet> xDataSource = rImport.getDataSource();

    PropertyValue aProperty;

    if (!xDataSource.is())
        return;

    for (auto &aIter : sax_fastparser::castToFastAttributeList( _xAttrList ))
    {
        aProperty.Name.clear();
        aProperty.Value = Any();

        switch( aIter.getToken() )
        {
            case XML_ELEMENT(XLINK, XML_HREF):
                try
                {
                    xDataSource->setPropertyValue(PROPERTY_URL,Any(aIter.toString()));
                }
                catch(const Exception&)
                {
                    DBG_UNHANDLED_EXCEPTION("dbaccess");
                }
                break;
            case XML_ELEMENT(XLINK, XML_TYPE):
                aProperty.Name = PROPERTY_TYPE;
                break;
            case XML_ELEMENT(XLINK, XML_SHOW):
                aProperty.Name = "Show";
                break;
            case XML_ELEMENT(XLINK, XML_ACTUATE):
                aProperty.Name = "Actuate";
                break;
            default:
                XMLOFF_WARN_UNKNOWN("dbaccess", aIter);
        }
        if ( !aProperty.Name.isEmpty() )
        {
            if ( !aProperty.Value.hasValue() )
                aProperty.Value <<= aIter.toString();
            rImport.addInfo(aProperty);
        }
    }
}

OXMLConnectionResource::~OXMLConnectionResource()
{

}

} // namespace dbaxml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

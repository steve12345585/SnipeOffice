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
#include "xmlComponent.hxx"
#include "xmlfilter.hxx"
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>

namespace rptxml
{
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::report;
    using namespace ::com::sun::star::xml::sax;
OXMLComponent::OXMLComponent( ORptFilter& _rImport
                ,const Reference< XFastAttributeList > & _xAttrList
                ,const Reference< XReportComponent > & _xComponent
                ) :
    SvXMLImportContext( _rImport )
    ,m_xComponent(_xComponent)
{
    OSL_ENSURE(m_xComponent.is(),"Component is NULL!");

    for (auto &aIter : sax_fastparser::castToFastAttributeList( _xAttrList ))
    {
        try
        {
            switch( aIter.getToken() )
            {
                case XML_ELEMENT(DRAW, XML_NAME):
                    m_xComponent->setName(aIter.toString());
                    break;
                default:
                    XMLOFF_WARN_UNKNOWN("reportdesign", aIter);
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while putting props into report component!");
        }
    }
}


OXMLComponent::~OXMLComponent()
{
}


} // namespace rptxml


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

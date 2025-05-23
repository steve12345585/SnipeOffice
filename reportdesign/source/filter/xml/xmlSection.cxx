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
#include "xmlSection.hxx"
#include "xmlfilter.hxx"
#include <utility>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmluconv.hxx>
#include "xmlHelper.hxx"
#include <com/sun/star/report/ReportPrintOption.hpp>
#include "xmlTable.hxx"
#include <comphelper/diagnose_ex.hxx>


namespace rptxml
{
    using namespace ::xmloff;
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::xml::sax;

    static sal_Int16 lcl_getReportPrintOption(std::string_view _sValue)
    {
        sal_Int16 nRet = report::ReportPrintOption::ALL_PAGES;
        const SvXMLEnumMapEntry<sal_Int16>* aXML_EnumMap = OXMLHelper::GetReportPrintOptions();
        (void)SvXMLUnitConverter::convertEnum( nRet, _sValue, aXML_EnumMap );
        return nRet;
    }


OXMLSection::OXMLSection( ORptFilter& rImport,
                const uno::Reference< xml::sax::XFastAttributeList > & _xAttrList
                ,uno::Reference< report::XSection > _xSection
                ,bool _bPageHeader)
:SvXMLImportContext( rImport )
,m_xSection(std::move(_xSection))
{

    if (!m_xSection.is())
        return;
    try
    {
        for (auto &aIter : sax_fastparser::castToFastAttributeList( _xAttrList ))
        {
            switch( aIter.getToken() )
            {
                case XML_ELEMENT(REPORT, XML_PAGE_PRINT_OPTION):
                    if ( _bPageHeader )
                        m_xSection->getReportDefinition()->setPageHeaderOption(lcl_getReportPrintOption(aIter.toView()));
                    else
                        m_xSection->getReportDefinition()->setPageFooterOption(lcl_getReportPrintOption(aIter.toView()));
                    break;
                case XML_ELEMENT(REPORT, XML_REPEAT_SECTION):
                    m_xSection->setRepeatSection(IsXMLToken(aIter, XML_TRUE));
                    break;
                default:
                    XMLOFF_WARN_UNKNOWN("reportdesign", aIter);
            }
        }
    }
    catch(Exception&)
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while filling the section props");
    }
}

OXMLSection::~OXMLSection()
{
}

css::uno::Reference< css::xml::sax::XFastContextHandler > OXMLSection::createFastChildContext(
        sal_Int32 nElement,
        const Reference< XFastAttributeList > & xAttrList )
{
    css::uno::Reference< css::xml::sax::XFastContextHandler > xContext;
    ORptFilter& rImport = GetOwnImport();

    switch( nElement )
    {
        case XML_ELEMENT(TABLE, XML_TABLE):
            xContext = new OXMLTable( rImport, xAttrList, m_xSection);
            break;
        default:
            break;
    }

    return xContext;
}

ORptFilter& OXMLSection::GetOwnImport()
{
    return static_cast<ORptFilter&>(GetImport());
}


} // namespace rptxml


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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
#include "xmlGroup.hxx"
#include "xmlSection.hxx"
#include "xmlFunction.hxx"
#include "xmlfilter.hxx"
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/ProgressBarHelper.hxx>
#include "xmlHelper.hxx"
#include "xmlEnums.hxx"
#include <com/sun/star/report/GroupOn.hpp>
#include <com/sun/star/report/KeepTogether.hpp>
#include <o3tl/string_view.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>

namespace rptxml
{
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::report;
    using namespace ::com::sun::star::xml::sax;

    static sal_Int16 lcl_getKeepTogetherOption(std::string_view _sValue)
    {
        sal_Int16 nRet = report::KeepTogether::NO;
        const SvXMLEnumMapEntry<sal_Int16>* aXML_EnumMap = OXMLHelper::GetKeepTogetherOptions();
        (void)SvXMLUnitConverter::convertEnum( nRet, _sValue, aXML_EnumMap );
        return nRet;
    }

OXMLGroup::OXMLGroup( ORptFilter& _rImport
                ,const Reference< XFastAttributeList > & _xAttrList
                ) :
    SvXMLImportContext( _rImport )
{

    m_xGroups = _rImport.getReportDefinition()->getGroups();
    OSL_ENSURE(m_xGroups.is(),"Groups is NULL!");
    m_xGroup = m_xGroups->createGroup();

    m_xGroup->setSortAscending(false);// the default value has to be set
    for (auto &aIter : sax_fastparser::castToFastAttributeList( _xAttrList ))
    {
        try
        {
            switch( aIter.getToken() )
            {
                case XML_ELEMENT(REPORT, XML_START_NEW_COLUMN):
                    m_xGroup->setStartNewColumn(IsXMLToken(aIter, XML_TRUE));
                    break;
                case XML_ELEMENT(REPORT, XML_RESET_PAGE_NUMBER):
                    m_xGroup->setResetPageNumber(IsXMLToken(aIter, XML_TRUE));
                    break;
                case XML_ELEMENT(REPORT, XML_SORT_ASCENDING):
                    m_xGroup->setSortAscending(IsXMLToken(aIter, XML_TRUE));
                    break;
                case XML_ELEMENT(REPORT, XML_GROUP_EXPRESSION):
                    {
                        OUString sValue = aIter.toString();
                        sal_Int32 nLen = sValue.getLength();
                        if ( nLen )
                        {

                            static const char s_sChanged[] = "rpt:HASCHANGED(\"";
                            sal_Int32 nPos = sValue.indexOf(s_sChanged);
                            if ( nPos == -1 )
                                nPos = 5;
                            else
                            {
                                nPos = strlen(s_sChanged);
                                static const char s_sQuote[] = "\"\"";
                                sal_Int32 nIndex = sValue.indexOf(s_sQuote,nPos);
                                while ( nIndex > -1 )
                                {
                                    sValue = sValue.replaceAt(nIndex,2, u"\"");
                                    nIndex = sValue.indexOf(s_sQuote,nIndex+2);
                                }
                                nLen = sValue.getLength() - 1;
                            }
                            sValue = sValue.copy(nPos,nLen-nPos-1);
                            const ORptFilter::TGroupFunctionMap& aFunctions = _rImport.getFunctions();
                            ORptFilter::TGroupFunctionMap::const_iterator aFind = aFunctions.find(sValue);
                            if ( aFind != aFunctions.end() )
                            {
                                const OUString sCompleteFormula = aFind->second->getFormula();
                                OUString sExpression = sCompleteFormula.getToken(1,'[');
                                sExpression = sExpression.getToken(0,']');
                                sal_Int32 nIndex = 0;
                                const std::u16string_view sFormula = o3tl::getToken(sCompleteFormula, 0,'(',nIndex);
                                ::sal_Int16 nGroupOn = report::GroupOn::DEFAULT;

                                if ( sFormula == u"rpt:LEFT")
                                {
                                    nGroupOn = report::GroupOn::PREFIX_CHARACTERS;
                                    std::u16string_view sInterval = o3tl::getToken(sCompleteFormula, 1,';',nIndex);
                                    sInterval = o3tl::getToken(sInterval, 0,')');
                                    m_xGroup->setGroupInterval(o3tl::toInt32(sInterval));
                                }
                                else if ( sFormula == u"rpt:YEAR")
                                    nGroupOn = report::GroupOn::YEAR;
                                else if ( sFormula == u"rpt:MONTH")
                                {
                                    nGroupOn = report::GroupOn::MONTH;
                                }
                                else if ( sCompleteFormula.matchIgnoreAsciiCase("rpt:INT((MONTH",0)
                                       && sCompleteFormula.endsWithIgnoreAsciiCase("-1)/3)+1") )
                                {
                                    nGroupOn = report::GroupOn::QUARTAL;
                                }
                                else if ( sFormula == u"rpt:WEEK")
                                    nGroupOn = report::GroupOn::WEEK;
                                else if ( sFormula == u"rpt:DAY")
                                    nGroupOn = report::GroupOn::DAY;
                                else if ( sFormula == u"rpt:HOUR")
                                    nGroupOn = report::GroupOn::HOUR;
                                else if ( sFormula == u"rpt:MINUTE")
                                    nGroupOn = report::GroupOn::MINUTE;
                                else if ( sFormula == u"rpt:INT")
                                {
                                    nGroupOn = report::GroupOn::INTERVAL;
                                    _rImport.removeFunction(sExpression);
                                    sExpression = sExpression.copy(std::string_view("INT_count_").size());
                                    OUString sInterval = sCompleteFormula.getToken(1,'/');
                                    sInterval = sInterval.getToken(0,')');
                                    m_xGroup->setGroupInterval(sInterval.toInt32());
                                }

                                m_xGroup->setGroupOn(nGroupOn);

                                _rImport.removeFunction(sValue);
                                sValue = sExpression;
                            }
                            m_xGroup->setExpression(sValue);
                        }
                    }
                    break;
                case XML_ELEMENT(REPORT, XML_KEEP_TOGETHER):
                    m_xGroup->setKeepTogether(lcl_getKeepTogetherOption(aIter.toView()));
                    break;
                default:
                    XMLOFF_WARN_UNKNOWN("reportdesign", aIter);
                    break;
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while putting group props!");
        }
    }
}


OXMLGroup::~OXMLGroup()
{

}

css::uno::Reference< css::xml::sax::XFastContextHandler > OXMLGroup::createFastChildContext(
    sal_Int32 nElement,
    const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList )
{
    css::uno::Reference< css::xml::sax::XFastContextHandler > xContext;
    ORptFilter& rImport = GetOwnImport();

    switch( nElement )
    {
        case XML_ELEMENT(REPORT, XML_FUNCTION):
            {
                rImport.GetProgressBarHelper()->Increment( PROGRESS_BAR_STEP );
                xContext = new OXMLFunction( rImport,xAttrList,m_xGroup);
            }
            break;
        case XML_ELEMENT(REPORT, XML_GROUP_HEADER):
            {
                rImport.GetProgressBarHelper()->Increment( PROGRESS_BAR_STEP );
                m_xGroup->setHeaderOn(true);
                xContext = new OXMLSection( rImport,xAttrList,m_xGroup->getHeader());
            }
            break;
        case XML_ELEMENT(REPORT, XML_GROUP):
            rImport.GetProgressBarHelper()->Increment( PROGRESS_BAR_STEP );
            xContext = new OXMLGroup( rImport,xAttrList);
            break;
        case XML_ELEMENT(REPORT, XML_DETAIL):
            {
                rImport.GetProgressBarHelper()->Increment( PROGRESS_BAR_STEP );
                Reference<XReportDefinition> xComponent = rImport.getReportDefinition();
                xContext = new OXMLSection( rImport,xAttrList, xComponent->getDetail());
            }
            break;

        case XML_ELEMENT(REPORT, XML_GROUP_FOOTER):
            {
                rImport.GetProgressBarHelper()->Increment( PROGRESS_BAR_STEP );
                m_xGroup->setFooterOn(true);
                xContext = new OXMLSection( rImport,xAttrList,m_xGroup->getFooter());
            }
            break;
        default:
            break;
    }

    return xContext;
}

ORptFilter& OXMLGroup::GetOwnImport()
{
    return static_cast<ORptFilter&>(GetImport());
}

void OXMLGroup::endFastElement(sal_Int32 )
{
    try
    {
        // the group elements end in the reverse order
        m_xGroups->insertByIndex(0,uno::Any(m_xGroup));
    }catch(uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "");
    }
}


} // namespace rptxml


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

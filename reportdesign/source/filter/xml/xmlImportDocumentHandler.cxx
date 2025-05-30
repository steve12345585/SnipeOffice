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
#include "xmlImportDocumentHandler.hxx"
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/chart2/data/XDatabaseDataProvider.hpp>
#include <com/sun/star/chart2/data/XDataReceiver.hpp>
#include <com/sun/star/chart2/data/XDataSource.hpp>
#include <com/sun/star/chart/XComplexDescriptionAccess.hpp>
#include <com/sun/star/chart/ChartDataRowSource.hpp>
#include <com/sun/star/reflection/ProxyFactory.hpp>
#include <comphelper/attributelist.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <utility>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlement.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/xmltkmap.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <rtl/ref.hxx>

#include "xmlHelper.hxx"
#include "xmlEnums.hxx"
#include "xmlExportDocumentHandler.hxx"

namespace rptxml
{
using namespace ::com::sun::star;
using namespace ::xmloff::token;

ImportDocumentHandler::ImportDocumentHandler(uno::Reference< uno::XComponentContext > context)
    :m_bImportedChart( false )
    ,m_xContext(std::move(context))
{
}

ImportDocumentHandler::~ImportDocumentHandler()
{
    if ( m_xProxy.is() )
    {
        m_xProxy->setDelegator( nullptr );
        m_xProxy.clear();
    }
}
IMPLEMENT_GET_IMPLEMENTATION_ID(ImportDocumentHandler)

OUString SAL_CALL ImportDocumentHandler::getImplementationName(  )
{
    return u"com.sun.star.comp.report.ImportDocumentHandler"_ustr;
}

sal_Bool SAL_CALL ImportDocumentHandler::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

uno::Sequence< OUString > SAL_CALL ImportDocumentHandler::getSupportedServiceNames(  )
{
    uno::Sequence< OUString > aSupported;
    if ( m_xServiceInfo.is() )
        aSupported = m_xServiceInfo->getSupportedServiceNames();
    return ::comphelper::concatSequences( uno::Sequence<OUString> { u"com.sun.star.report.ImportDocumentHandler"_ustr }, aSupported);
}


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
reportdesign_ImportDocumentHandler_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new ImportDocumentHandler(context));
}

// xml::sax::XDocumentHandler:
void SAL_CALL ImportDocumentHandler::startDocument()
{
    m_xDelegatee->startDocument();
}

void SAL_CALL ImportDocumentHandler::endDocument()
{
    m_xDelegatee->endDocument();
    uno::Reference< chart2::data::XDataReceiver > xReceiver(m_xModel,uno::UNO_QUERY_THROW);
    if ( !m_bImportedChart )
        return;

    // this fills the chart again
    ::comphelper::NamedValueCollection aArgs;
    aArgs.put( u"CellRangeRepresentation"_ustr, u"all"_ustr );
    aArgs.put( u"FirstCellAsLabel"_ustr, uno::Any( true ) );
    aArgs.put( u"DataRowSource"_ustr, uno::Any( chart::ChartDataRowSource_COLUMNS ) );

    bool bHasCategories = false;

    uno::Reference< chart2::data::XDataSource > xDataSource(m_xModel, uno::UNO_QUERY);
    if( xDataSource.is())
    {
        const uno::Sequence< uno::Reference< chart2::data::XLabeledDataSequence > > aSequences(xDataSource->getDataSequences());
        for( const auto& rSequence : aSequences )
        {
            if( rSequence.is() )
            {
                uno::Reference< beans::XPropertySet > xSeqProp( rSequence->getValues(), uno::UNO_QUERY );
                OUString aRole;
                if  (   xSeqProp.is()
                    &&  ( xSeqProp->getPropertyValue( u"Role"_ustr ) >>= aRole )
                    &&  aRole == "categories"
                    )
                {
                    bHasCategories = true;
                    break;
                }
            }
        }
    }
    aArgs.put( u"HasCategories"_ustr, uno::Any( bHasCategories ) );

    uno::Reference< chart::XComplexDescriptionAccess > xDataProvider(m_xModel->getDataProvider(),uno::UNO_QUERY);
    if ( xDataProvider.is() )
    {
        const uno::Sequence< OUString > aColumnNames = xDataProvider->getColumnDescriptions();
        aArgs.put( u"ColumnDescriptions"_ustr, uno::Any( aColumnNames ) );
    }

    xReceiver->attachDataProvider( m_xDatabaseDataProvider );
    xReceiver->setArguments( aArgs.getPropertyValues() );
}

void SAL_CALL ImportDocumentHandler::startElement(const OUString & _sName, const uno::Reference< xml::sax::XAttributeList > & _xAttrList)
{
    uno::Reference< xml::sax::XAttributeList > xNewAttribs = _xAttrList;
    bool bExport = true;
    if ( _sName == "office:report" )
    {
        const sal_Int16 nLength = (_xAttrList.is()) ? _xAttrList->getLength() : 0;
        static const OUString s_sTRUE = ::xmloff::token::GetXMLToken(XML_TRUE);
        try
        {
            for(sal_Int16 i = 0; i < nLength; ++i)
            {
                OUString sLocalName;
                const OUString sAttrName = _xAttrList->getNameByIndex( i );
                const sal_Int32 nColonPos = sAttrName.indexOf( ':' );
                if( -1 == nColonPos )
                    sLocalName = sAttrName;
                else
                    sLocalName = sAttrName.copy( nColonPos + 1 );
                const OUString sValue = _xAttrList->getValueByIndex( i );

                switch( m_pReportElemTokenMap->Get( XML_NAMESPACE_REPORT, sLocalName ) )
                {
                    case XML_TOK_COMMAND_TYPE:
                        {
                            sal_Int32 nRet = sdb::CommandType::COMMAND;
                            const SvXMLEnumMapEntry<sal_Int32>* aXML_EnumMap = OXMLHelper::GetCommandTypeOptions();
                            (void)SvXMLUnitConverter::convertEnum( nRet, sValue, aXML_EnumMap );
                            m_xDatabaseDataProvider->setCommandType(nRet);
                        }
                        break;
                    case XML_TOK_COMMAND:
                        m_xDatabaseDataProvider->setCommand(sValue);
                        break;
                    case XML_TOK_FILTER:
                        m_xDatabaseDataProvider->setFilter(sValue);
                        break;
                    case XML_TOK_ESCAPE_PROCESSING:
                        m_xDatabaseDataProvider->setEscapeProcessing(sValue == s_sTRUE);
                        break;
                    default:
                        break;
                }
            }
        }
        catch(uno::Exception&)
        {
        }
        m_xDelegatee->startElement(lcl_createAttribute(XML_NP_OFFICE,XML_CHART),nullptr);
        bExport = false;
        m_bImportedChart = true;
    }
    else if ( _sName == "rpt:master-detail-field" )
    {
        const sal_Int16 nLength = (_xAttrList.is()) ? _xAttrList->getLength() : 0;
        ::std::unique_ptr<SvXMLTokenMap> pMasterElemTokenMap( OXMLHelper::GetSubDocumentElemTokenMap());
        try
        {
            OUString sMasterField,sDetailField;
            for(sal_Int16 i = 0; i < nLength; ++i)
            {
                OUString sLocalName;
                const OUString sAttrName = _xAttrList->getNameByIndex( i );
                const sal_Int32 nColonPos = sAttrName.indexOf( ':' );
                if( -1 == nColonPos )
                    sLocalName = sAttrName;
                else
                    sLocalName = sAttrName.copy( nColonPos + 1 );
                const OUString sValue = _xAttrList->getValueByIndex( i );

                switch( pMasterElemTokenMap->Get( XML_NAMESPACE_REPORT, sLocalName ) )
                {
                    case XML_TOK_MASTER:
                        sMasterField = sValue;
                        break;
                    case XML_TOK_SUB_DETAIL:
                        sDetailField = sValue;
                        break;
                }
            }
            if ( sDetailField.isEmpty() )
                sDetailField = sMasterField;
            m_aMasterFields.push_back(sMasterField);
            m_aDetailFields.push_back(sDetailField);
        }
        catch(uno::Exception&)
        {
            TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while filling the report definition props");
        }
        bExport = false;
    }
    else if ( _sName == "rpt:detail"
        ||    _sName == "rpt:formatted-text"
        ||    _sName == "rpt:master-detail-fields"
        ||    _sName == "rpt:report-component"
        ||    _sName == "rpt:report-element")
        bExport = false;
    else if ( _sName == "chart:plot-area" )
    {
        bool bHasCategories = true;
        const sal_Int16 nLength = (_xAttrList.is()) ? _xAttrList->getLength() : 0;
        for(sal_Int16 i = 0; i < nLength; ++i)
        {
            std::u16string_view sLocalName;
            const OUString sAttrName = _xAttrList->getNameByIndex( i );
            const sal_Int32 nColonPos = sAttrName.indexOf( ':' );
            if( -1 == nColonPos )
                sLocalName = sAttrName;
            else
                sLocalName = sAttrName.subView( nColonPos + 1 );
            if ( sLocalName == u"data-source-has-labels" )
            {
                const OUString sValue = _xAttrList->getValueByIndex( i );
                bHasCategories = sValue == "both";
                break;
            }
        }
        for(beans::PropertyValue & propVal : asNonConstRange(m_aArguments))
        {
            if ( propVal.Name == "HasCategories" )
            {
                propVal.Value <<= bHasCategories;
                break;
            }
        }

        rtl::Reference<comphelper::AttributeList> pList = new comphelper::AttributeList();
        xNewAttribs = pList;
        pList->AppendAttributeList(_xAttrList);
        pList->AddAttribute(u"table:cell-range-address"_ustr,u"local-table.$A$1:.$Z$65536"_ustr);

    }

    if ( bExport )
        m_xDelegatee->startElement(_sName,xNewAttribs);
}

void SAL_CALL ImportDocumentHandler::endElement(const OUString & _sName)
{
    bool bExport = true;
    OUString sNewName = _sName;
    if ( _sName == "office:report" )
    {
        sNewName = lcl_createAttribute(XML_NP_OFFICE,XML_CHART);
    }
    else if ( _sName == "rpt:master-detail-fields" )
    {
        if ( !m_aMasterFields.empty() )
            m_xDatabaseDataProvider->setMasterFields(uno::Sequence< OUString>(&*m_aMasterFields.begin(),m_aMasterFields.size()));
        if ( !m_aDetailFields.empty() )
            m_xDatabaseDataProvider->setDetailFields(uno::Sequence< OUString>(&*m_aDetailFields.begin(),m_aDetailFields.size()));
        bExport = false;
    }
    else if ( _sName == "rpt:detail"
        ||    _sName == "rpt:formatted-text"
        ||    _sName == "rpt:master-detail-field"
        ||    _sName == "rpt:report-component"
        ||    _sName == "rpt:report-element")
        bExport = false;

    if ( bExport )
        m_xDelegatee->endElement(sNewName);
}

void SAL_CALL ImportDocumentHandler::characters(const OUString & aChars)
{
    m_xDelegatee->characters(aChars);
}

void SAL_CALL ImportDocumentHandler::ignorableWhitespace(const OUString & aWhitespaces)
{
    m_xDelegatee->ignorableWhitespace(aWhitespaces);
}

void SAL_CALL ImportDocumentHandler::processingInstruction(const OUString & aTarget, const OUString & aData)
{
    m_xDelegatee->processingInstruction(aTarget,aData);
}

void SAL_CALL ImportDocumentHandler::setDocumentLocator(const uno::Reference< xml::sax::XLocator > & xLocator)
{
    m_xDelegatee->setDocumentLocator(xLocator);
}
void SAL_CALL ImportDocumentHandler::initialize( const uno::Sequence< uno::Any >& _aArguments )
{
    comphelper::SequenceAsHashMap aArgs(_aArguments);
    m_xDocumentHandler = aArgs.getUnpackedValueOrDefault(u"DocumentHandler"_ustr,m_xDocumentHandler);
    m_xModel = aArgs.getUnpackedValueOrDefault(u"Model"_ustr,m_xModel);

    OSL_ENSURE(m_xDocumentHandler.is(), "No document handler available!");
    if (!m_xDocumentHandler.is() || !m_xModel.is())
        throw uno::Exception(u"no delegatee and no model"_ustr, nullptr);

    m_xDelegatee.set(new SvXMLLegacyToFastDocHandler(dynamic_cast<SvXMLImport*>(m_xDocumentHandler.get())));

    m_xDatabaseDataProvider.set(m_xModel->getDataProvider(),uno::UNO_QUERY);
    if ( !m_xDatabaseDataProvider.is() )
    {
        // tdf#117162 reportbuilder needs the DataProvider to exist to progress further
        setDataProvider(m_xModel, OUString());
        m_xDatabaseDataProvider.set(m_xModel->getDataProvider(), uno::UNO_QUERY_THROW);
    }

    m_aArguments = m_xDatabaseDataProvider->detectArguments(nullptr);

    uno::Reference< reflection::XProxyFactory > xProxyFactory = reflection::ProxyFactory::create( m_xContext );
    m_xProxy = xProxyFactory->createProxy(m_xDelegatee);
    ::comphelper::query_aggregation(m_xProxy,m_xDelegatee);
    m_xTypeProvider.set(m_xDelegatee,uno::UNO_QUERY);
    m_xServiceInfo.set(m_xDelegatee,uno::UNO_QUERY);

    // set ourself as delegator
    m_xProxy->setDelegator( *this );

    m_pReportElemTokenMap = OXMLHelper::GetReportElemTokenMap();
}

uno::Any SAL_CALL ImportDocumentHandler::queryInterface( const uno::Type& _rType )
{
    uno::Any aReturn = ImportDocumentHandler_BASE::queryInterface(_rType);
    return aReturn.hasValue() ? aReturn : (m_xProxy.is() ? m_xProxy->queryAggregation(_rType) : aReturn);
}

uno::Sequence< uno::Type > SAL_CALL ImportDocumentHandler::getTypes(  )
{
    if ( m_xTypeProvider.is() )
        return ::comphelper::concatSequences(
            ImportDocumentHandler_BASE::getTypes(),
            m_xTypeProvider->getTypes()
        );
    return ImportDocumentHandler_BASE::getTypes();
}


} // namespace rptxml


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

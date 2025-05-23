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

#include <MetaExportComponent.hxx>
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/util/MeasureUnit.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/genericpropertyset.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <osl/diagnose.h>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/namespacemap.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlmetae.hxx>
#include <PropertySetMerger.hxx>

#include <unotools/docinfohelper.hxx>


using namespace ::com::sun::star;
using namespace ::xmloff::token;

XMLMetaExportComponent::XMLMetaExportComponent(
    const css::uno::Reference< css::uno::XComponentContext >& xContext,
    OUString const & implementationName, SvXMLExportFlags nFlags )
:   SvXMLExport( xContext, implementationName, util::MeasureUnit::CM, XML_TEXT, nFlags )
{
}

XMLMetaExportComponent::~XMLMetaExportComponent()
{
}

void SAL_CALL XMLMetaExportComponent::setSourceDocument( const css::uno::Reference< css::lang::XComponent >& xDoc )
{
    try
    {
        SvXMLExport::setSourceDocument( xDoc );
    }
    catch( lang::IllegalArgumentException& )
    {
        // allow to use document properties service without model access
        // this is required for document properties exporter
        mxDocProps =
            uno::Reference< document::XDocumentProperties >::query( xDoc );
        if( !mxDocProps.is() )
            throw lang::IllegalArgumentException();
    }
}

ErrCode XMLMetaExportComponent::exportDoc( enum XMLTokenEnum )
{
    uno::Reference< xml::sax::XDocumentHandler > xDocHandler = GetDocHandler();

    if( !(getExportFlags() & SvXMLExportFlags::OASIS) )
    {
        uno::Reference< uno::XComponentContext > xContext = getComponentContext();
        try
        {
            static const ::comphelper::PropertyMapEntry aInfoMap[] =
            {
                { u"Class"_ustr, 0,
                    ::cppu::UnoType<OUString>::get(),
                    beans::PropertyAttribute::MAYBEVOID, 0},
            };
            uno::Reference< beans::XPropertySet > xConvPropSet(
                ::comphelper::GenericPropertySet_CreateInstance(
                        new ::comphelper::PropertySetInfo( aInfoMap ) ) );

            xConvPropSet->setPropertyValue(u"Class"_ustr, uno::Any(GetXMLToken( XML_TEXT )) );

            uno::Reference< beans::XPropertySet > xPropSet =
                getExportInfo().is()
                    ?  PropertySetMerger_CreateInstance( getExportInfo(),
                                                      xConvPropSet )
                    : getExportInfo();

            uno::Sequence< uno::Any > aArgs{ uno::Any(xDocHandler), uno::Any(xPropSet),
                                             uno::Any(GetModel()) };

            // get filter component
            xDocHandler.set(
                xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                    u"com.sun.star.comp.Oasis2OOoTransformer"_ustr, aArgs, xContext),
                uno::UNO_QUERY_THROW );

            SetDocHandler( xDocHandler );
        }
        catch( css::uno::Exception& )
        {
            OSL_FAIL( "Cannot instantiate com.sun.star.comp.Oasis2OOoTransformer!");
        }
    }


    xDocHandler->startDocument();

    addChaffWhenEncryptedStorage();

    {

        const SvXMLNamespaceMap& rMap = GetNamespaceMap();
        sal_uInt16 nPos = rMap.GetFirstKey();
        while( USHRT_MAX != nPos )
        {
            GetAttrList().AddAttribute( rMap.GetAttrNameByKey( nPos ), rMap.GetNameByKey( nPos ) );
            nPos = GetNamespaceMap().GetNextKey( nPos );
        }

        const char*const pVersion = GetODFVersionAttributeValue();

        if( pVersion )
            AddAttribute( XML_NAMESPACE_OFFICE, XML_VERSION,
                            OUString::createFromAscii(pVersion) );

        SvXMLElementExport aDocElem( *this, XML_NAMESPACE_OFFICE, XML_DOCUMENT_META,
                    true, true );

        // NB: office:meta is now written by _ExportMeta
        ExportMeta_();
    }
    xDocHandler->endDocument();
    return ERRCODE_NONE;
}

void XMLMetaExportComponent::ExportMeta_()
{
    if (mxDocProps.is()) {
        OUString generator( ::utl::DocInfoHelper::GetGeneratorString() );
        // update generator here
        mxDocProps->setGenerator(generator);
        rtl::Reference<SvXMLMetaExport> pMeta = new SvXMLMetaExport(*this, mxDocProps);
        pMeta->Export();
    } else {
        SvXMLExport::ExportMeta_();
    }
}

// methods without content:
void XMLMetaExportComponent::ExportAutoStyles_() {}
void XMLMetaExportComponent::ExportMasterStyles_() {}
void XMLMetaExportComponent::ExportContent_() {}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
XMLMetaExportComponent_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new XMLMetaExportComponent(context, u"XMLMetaExportComponent"_ustr, SvXMLExportFlags::META|SvXMLExportFlags::OASIS));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
XMLMetaExportOOo_get_implementation(css::uno::XComponentContext* context,
                                    css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(
        new XMLMetaExportComponent(context, u"XMLMetaExportOOo"_ustr, SvXMLExportFlags::META));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

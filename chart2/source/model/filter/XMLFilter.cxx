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

#include <XMLFilter.hxx>
#include <MediaDescriptorHelper.hxx>

#include <officecfg/Office/Common.hxx>
#include <svtools/sfxecode.hxx>
#include <comphelper/genericpropertyset.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <comphelper/documentconstants.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/sequence.hxx>

#include <osl/diagnose.h>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/xml/sax/Writer.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/StorageFactory.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/xml/sax/Parser.hpp>
#include <com/sun/star/xml/sax/SAXParseException.hpp>
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#include <com/sun/star/xml/sax/XFastParser.hpp>
#include <com/sun/star/packages/zip/ZipIOException.hpp>
#include <com/sun/star/document/GraphicStorageHandler.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::osl::MutexGuard;

namespace
{
constexpr OUString sXML_metaStreamName = u"meta.xml"_ustr;
constexpr OUString sXML_styleStreamName = u"styles.xml"_ustr;
constexpr OUString sXML_contentStreamName = u"content.xml"_ustr;


uno::Reference< embed::XStorage > lcl_getWriteStorage(
    const Sequence< beans::PropertyValue >& rMediaDescriptor,
    const uno::Reference< uno::XComponentContext >& xContext,const OUString& _sMediaType)
{
    uno::Reference< embed::XStorage > xStorage;
    try
    {
        apphelper::MediaDescriptorHelper aMDHelper( rMediaDescriptor );
        if( aMDHelper.ISSET_Storage )
        {
            xStorage = aMDHelper.Storage;
        }
        else
        {
            Reference< lang::XSingleServiceFactory > xStorageFact( embed::StorageFactory::create( xContext ) );

            std::vector< beans::PropertyValue > aPropertiesForStorage;

            for( sal_Int32 i=rMediaDescriptor.getLength(); i--; )
            {
                // properties understood by storage factory
                // (see package/source/xstor/xfactory.cxx for details)
                if ( rMediaDescriptor[i].Name == "InteractionHandler" || rMediaDescriptor[i].Name == "Password" || rMediaDescriptor[i].Name == "RepairPackage" )
                {
                    aPropertiesForStorage.push_back( rMediaDescriptor[i] );
                }
            }

            Sequence< uno::Any > aStorageArgs{
                aMDHelper.ISSET_OutputStream ? uno::Any(aMDHelper.OutputStream)
                                             : uno::Any(aMDHelper.URL),
                uno::Any(embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE),
                uno::Any(comphelper::containerToSequence( aPropertiesForStorage ))
            };

            xStorage.set(
                xStorageFact->createInstanceWithArguments( aStorageArgs ),
                uno::UNO_QUERY_THROW );
        }

        // set correct media type at storage
        uno::Reference<beans::XPropertySet> xProp(xStorage,uno::UNO_QUERY);
        OUString aMediaType;
        if ( ! xProp.is() ||
             ! ( xProp->getPropertyValue( u"MediaType"_ustr) >>= aMediaType ) ||
             ( aMediaType.isEmpty() ))
        {
            xProp->setPropertyValue( u"MediaType"_ustr, uno::Any( _sMediaType ));
        }
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
    return xStorage;
}

uno::Reference< embed::XStorage > lcl_getReadStorage(
    const Sequence< beans::PropertyValue >& rMediaDescriptor,
    const uno::Reference< uno::XComponentContext >& xContext)
{
    uno::Reference< embed::XStorage > xStorage;

    try
    {
        apphelper::MediaDescriptorHelper aMDHelper( rMediaDescriptor );
        if( aMDHelper.ISSET_Storage )
        {
            xStorage = aMDHelper.Storage;
        }
        else
        {
            // get XStream from MediaDescriptor
            uno::Reference< io::XInputStream > xStream;
            std::vector< beans::PropertyValue > aPropertiesForStorage;
            for( sal_Int32 i=rMediaDescriptor.getLength(); i--; )
            {
                if( rMediaDescriptor[i].Name == "InputStream" )
                    xStream.set( rMediaDescriptor[i].Value, uno::UNO_QUERY );

                // properties understood by storage factory
                // (see package/source/xstor/xfactory.cxx for details)
                if ( rMediaDescriptor[i].Name == "InteractionHandler" || rMediaDescriptor[i].Name == "Password" || rMediaDescriptor[i].Name == "RepairPackage" )
                {
                    aPropertiesForStorage.push_back( rMediaDescriptor[i] );
                }
            }
            OSL_ENSURE( xStream.is(), "No Stream" );
            if( ! xStream.is())
                return xStorage;

            // convert XInputStream to XStorage via the storage factory
            Reference< lang::XSingleServiceFactory > xStorageFact( embed::StorageFactory::create( xContext ) );
            Sequence< uno::Any > aStorageArgs{
                uno::Any(xStream),
                uno::Any(embed::ElementModes::READ | embed::ElementModes::NOCREATE),
                uno::Any(comphelper::containerToSequence( aPropertiesForStorage ))
            };
            xStorage.set(
                xStorageFact->createInstanceWithArguments( aStorageArgs ), uno::UNO_QUERY_THROW );
        }

        OSL_ENSURE( xStorage.is(), "No Storage" );
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }

    return xStorage;
}

} // anonymous namespace

namespace chart
{

XMLFilter::XMLFilter( Reference< uno::XComponentContext > const & xContext ) :
        m_xContext( xContext )
{}

XMLFilter::~XMLFilter()
{}

// ____ XFilter ____
sal_Bool SAL_CALL XMLFilter::filter(
    const Sequence< beans::PropertyValue >& aDescriptor )
{
    bool bResult = false;

    MutexGuard aGuard( m_aMutex );

    if( m_xSourceDoc.is())
    {
        OSL_ENSURE( ! m_xTargetDoc.is(), "source doc is set -> target document should not be set" );
        if( impl_Export( m_xSourceDoc,
                         aDescriptor ) == ERRCODE_NONE )
        {
            m_xSourceDoc = nullptr;
            bResult = true;
        }
    }
    else if( m_xTargetDoc.is())
    {
        if( impl_Import( m_xTargetDoc,
                         aDescriptor ) == ERRCODE_NONE )
        {
            m_xTargetDoc = nullptr;
            bResult = true;
        }
    }
    else
    {
        OSL_FAIL( "filter() called with no document set" );
    }

    return bResult;
}

void SAL_CALL XMLFilter::cancel()
{
}

// ____ XImporter ____
void SAL_CALL XMLFilter::setTargetDocument(
    const Reference< lang::XComponent >& Document )
{
    MutexGuard aGuard( m_aMutex );
    OSL_ENSURE( ! m_xSourceDoc.is(), "Setting target doc while source doc is set" );

    m_xTargetDoc = Document;
}

// ____ XExporter ____
void SAL_CALL XMLFilter::setSourceDocument(
    const Reference< lang::XComponent >& Document )
{
    MutexGuard aGuard( m_aMutex );
    OSL_ENSURE( ! m_xTargetDoc.is(), "Setting source doc while target doc is set" );

    m_xSourceDoc = Document;
}

ErrCode XMLFilter::impl_Import(
    const Reference< lang::XComponent > & xDocumentComp,
    const Sequence< beans::PropertyValue > & rMediaDescriptor )
{
    ErrCode nWarning = ERRCODE_NONE;

    OSL_ENSURE( xDocumentComp.is(), "Import: No Model" );
    OSL_ENSURE( m_xContext.is(), "Import: No ComponentContext" );

    if( ! (xDocumentComp.is() &&
           m_xContext.is()))
        return nWarning;

    try
    {
        Reference< lang::XServiceInfo > xServInfo( xDocumentComp, uno::UNO_QUERY_THROW );
        if( ! xServInfo->supportsService( u"com.sun.star.chart2.ChartDocument"_ustr))
        {
            OSL_FAIL( "Import: No ChartDocument" );
            return ERRCODE_SFX_GENERAL;
        }

        Reference< lang::XMultiComponentFactory > xFactory( m_xContext->getServiceManager());
        OSL_ENSURE( xFactory.is(), "Import: No Factory" );
        if( ! xFactory.is())
            return ERRCODE_SFX_GENERAL;

        bool bOasis = true;
        isOasisFormat( rMediaDescriptor, bOasis );
        Reference< embed::XStorage > xStorage( lcl_getReadStorage( rMediaDescriptor, m_xContext));
        if( ! xStorage.is())
            return ERRCODE_SFX_GENERAL;

        uno::Reference<document::XGraphicStorageHandler> xGraphicStorageHandler;
        uno::Reference<lang::XMultiServiceFactory> xServiceFactory(xFactory, uno::UNO_QUERY);
        if (xServiceFactory.is())
        {
            uno::Sequence<uno::Any> aArgs{ uno::Any(xStorage) };
            xGraphicStorageHandler.set(
                xServiceFactory->createInstanceWithArguments(
                    u"com.sun.star.comp.Svx.GraphicImportHelper"_ustr, aArgs), uno::UNO_QUERY);
        }

        // create XPropertySet with extra information for the filter
        /** property map for import info set */
        static comphelper::PropertyMapEntry const aImportInfoMap[] =
        {
            // necessary properties for XML progress bar at load time
            { u"ProgressRange"_ustr,   0, cppu::UnoType<sal_Int32>::get(),  css::beans::PropertyAttribute::MAYBEVOID, 0},
            { u"ProgressMax"_ustr,     0, cppu::UnoType<sal_Int32>::get(),  css::beans::PropertyAttribute::MAYBEVOID, 0},
            { u"ProgressCurrent"_ustr, 0, cppu::UnoType<sal_Int32>::get(),  css::beans::PropertyAttribute::MAYBEVOID, 0},
            { u"PrivateData"_ustr,     0, cppu::UnoType<XInterface>::get(), css::beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"BaseURI"_ustr,         0, cppu::UnoType<OUString>::get(),   css::beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"StreamRelPath"_ustr,   0, cppu::UnoType<OUString>::get(),   css::beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"StreamName"_ustr,      0, cppu::UnoType<OUString>::get(),   css::beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"BuildId"_ustr,         0, cppu::UnoType<OUString>::get(),   css::beans::PropertyAttribute::MAYBEVOID, 0 },
        };
        uno::Reference< beans::XPropertySet > xImportInfo(
                    comphelper::GenericPropertySet_CreateInstance(
                                new comphelper::PropertySetInfo( aImportInfoMap ) ) );

        // Set base URI and Hierarchical Name
        OUString aHierarchName, aBaseUri;
        // why retrieve this from the model when it's available as rMediaDescriptor?
        uno::Reference<frame::XModel> const xModel(m_xTargetDoc, uno::UNO_QUERY);
        if( xModel.is() )
        {
            const uno::Sequence< beans::PropertyValue > aModProps = xModel->getArgs();
            for( beans::PropertyValue const & prop : aModProps )
            {
                if( prop.Name == "HierarchicalDocumentName" )
                {
                    // Actually this argument only has meaning for embedded documents
                    prop.Value >>= aHierarchName;
                }
                else if( prop.Name == "DocumentBaseURL" )
                {
                    prop.Value >>= aBaseUri;
                }
            }
        }

        // needed for relative URLs, but in clipboard copy/paste there may be none
        SAL_INFO_IF(aBaseUri.isEmpty(), "chart2", "chart::XMLFilter: no base URL");
        if( !aBaseUri.isEmpty() )
            xImportInfo->setPropertyValue( u"BaseURI"_ustr, uno::Any( aBaseUri ) );

        if( !aHierarchName.isEmpty() )
            xImportInfo->setPropertyValue( u"StreamRelPath"_ustr, uno::Any( aHierarchName ) );

        // import meta information
        if( bOasis )
            nWarning = impl_ImportStream(
                sXML_metaStreamName,
                u"com.sun.star.comp.Chart.XMLOasisMetaImporter"_ustr,
                xStorage, xFactory, xGraphicStorageHandler, xImportInfo );

        // import styles
        ErrCode nTmpErr = impl_ImportStream(
            sXML_styleStreamName,
            bOasis
            ? u"com.sun.star.comp.Chart.XMLOasisStylesImporter"_ustr
            : u"com.sun.star.comp.Chart.XMLStylesImporter"_ustr,
            xStorage, xFactory, xGraphicStorageHandler, xImportInfo );
        nWarning = nWarning != ERRCODE_NONE ? nWarning : nTmpErr;

        // import content
        ErrCode nContentWarning = impl_ImportStream(
            sXML_contentStreamName,
            bOasis
            ? u"com.sun.star.comp.Chart.XMLOasisContentImporter"_ustr
            : u"com.sun.star.comp.Chart.XMLContentImporter"_ustr,
            xStorage, xFactory, xGraphicStorageHandler, xImportInfo );
        nWarning = nWarning != ERRCODE_NONE ? nWarning : nContentWarning;
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("chart2");

        // something went awry
        nWarning = ERRCODE_SFX_GENERAL;
    }

    return nWarning;
}

ErrCode XMLFilter::impl_ImportStream(
    const OUString & rStreamName,
    const OUString & rServiceName,
    const Reference< embed::XStorage > & xStorage,
    const Reference< lang::XMultiComponentFactory > & xFactory,
    const Reference< document::XGraphicStorageHandler > & xGraphicStorageHandler,
    uno::Reference< beans::XPropertySet > const & xImportInfo )
{
    ErrCode nWarning = ERRCODE_SFX_GENERAL;

    if( ! (xStorage.is() &&
           xStorage->hasByName( rStreamName )))
        return ERRCODE_NONE;

    if( xImportInfo.is() )
        xImportInfo->setPropertyValue( u"StreamName"_ustr, uno::Any( rStreamName ) );

    if( xStorage.is() &&
        xStorage->isStreamElement( rStreamName ) )
    {
        try
        {
            auto xInputStream =
                xStorage->openStreamElement(
                    rStreamName,
                    embed::ElementModes::READ | embed::ElementModes::NOCREATE );

            // todo: encryption

            if( xInputStream.is())
            {
                sal_Int32 nArgs = 0;
                if( xGraphicStorageHandler.is())
                    nArgs++;
                if( xImportInfo.is())
                    nArgs++;

                uno::Sequence< uno::Any > aFilterCompArgs( nArgs );
                auto aFilterCompArgsRange = asNonConstRange(aFilterCompArgs);

                nArgs = 0;
                if( xGraphicStorageHandler.is())
                    aFilterCompArgsRange[nArgs++] <<= xGraphicStorageHandler;
                if( xImportInfo.is())
                    aFilterCompArgsRange[ nArgs++ ] <<= xImportInfo;

                // the underlying SvXMLImport implements XFastParser, XImporter, XFastDocumentHandler
                Reference< XInterface  > xFilter =
                    xFactory->createInstanceWithArgumentsAndContext( rServiceName, aFilterCompArgs, m_xContext );
                assert(xFilter);
                Reference< document::XImporter > xImporter( xFilter, uno::UNO_QUERY );
                assert(xImporter);
                xImporter->setTargetDocument( Reference< lang::XComponent >( m_xTargetDoc, uno::UNO_SET_THROW ));

                if ( !m_sDocumentHandler.isEmpty() )
                {
                    try
                    {
                        uno::Sequence< uno::Any > aArgs{
                            uno::Any(beans::NamedValue(u"DocumentHandler"_ustr, uno::Any(xFilter))),
                            uno::Any(beans::NamedValue(u"Model"_ustr, uno::Any(m_xTargetDoc)))
                        };

                        xFilter = xFactory->createInstanceWithArgumentsAndContext(m_sDocumentHandler,aArgs,m_xContext);
                    }
                    catch (const uno::Exception&)
                    {
                        TOOLS_WARN_EXCEPTION("chart2", "failed to instantiate " << m_sDocumentHandler);
                    }
                }
                xml::sax::InputSource aParserInput;
                aParserInput.aInputStream.set(xInputStream, uno::UNO_QUERY_THROW);

                // the underlying SvXMLImport implements XFastParser, XImporter, XFastDocumentHandler
                Reference< xml::sax::XFastParser > xFastParser(xFilter, uno::UNO_QUERY);
                if (xFastParser.is())
                    xFastParser->parseStream(aParserInput);
                else
                {
                    Reference<xml::sax::XParser> xParser = xml::sax::Parser::create(m_xContext);
                    xParser->setDocumentHandler( uno::Reference<xml::sax::XDocumentHandler>(xFilter, uno::UNO_QUERY_THROW) );
                    xParser->parseStream(aParserInput);
                }
            }

            // load was successful
            nWarning = ERRCODE_NONE;
        }
        catch (const xml::sax::SAXParseException&)
        {
            // todo: if encrypted: ERRCODE_SFX_WRONGPASSWORD
        }
        catch (const xml::sax::SAXException&)
        {
            // todo: if encrypted: ERRCODE_SFX_WRONGPASSWORD
        }
        catch (const packages::zip::ZipIOException&)
        {
            nWarning = ERRCODE_IO_BROKENPACKAGE;
        }
        catch (const io::IOException&)
        {
            TOOLS_WARN_EXCEPTION("chart2", "");
        }
        catch (const uno::Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }

    return nWarning;
}

ErrCode XMLFilter::impl_Export(
    const Reference< lang::XComponent > & xDocumentComp,
    const Sequence< beans::PropertyValue > & rMediaDescriptor )
{
    m_aMediaDescriptor = rMediaDescriptor;
    //save

    ErrCode nWarning = ERRCODE_NONE;

    OSL_ENSURE( xDocumentComp.is(), "Export: No Model" );
    OSL_ENSURE( m_xContext.is(), "Export: No ComponentContext" );

    if( !xDocumentComp.is() || !m_xContext.is() )
        return nWarning;

    try
    {
        Reference< lang::XServiceInfo > xServInfo( xDocumentComp, uno::UNO_QUERY_THROW );
        if( ! xServInfo->supportsService( u"com.sun.star.chart2.ChartDocument"_ustr))
        {
            OSL_FAIL( "Export: No ChartDocument" );
            return ERRCODE_SFX_GENERAL;
        }

        Reference< lang::XMultiComponentFactory > xFactory( m_xContext->getServiceManager());
        OSL_ENSURE( xFactory.is(), "Export: No Factory" );
        if( ! xFactory.is())
            return ERRCODE_SFX_GENERAL;
        uno::Reference< lang::XMultiServiceFactory > xServiceFactory( m_xContext->getServiceManager(), uno::UNO_QUERY);
        if( ! xServiceFactory.is())
            return ERRCODE_SFX_GENERAL;

        uno::Reference< xml::sax::XWriter > xSaxWriter = xml::sax::Writer::create(m_xContext);

        bool bOasis = true;
        isOasisFormat( rMediaDescriptor, bOasis );

        uno::Reference< embed::XStorage > xStorage( lcl_getWriteStorage( rMediaDescriptor, m_xContext, getMediaType(bOasis) ) );
        OSL_ENSURE( xStorage.is(), "No Storage" );
        if( ! xStorage.is())
            return ERRCODE_SFX_GENERAL;

        uno::Reference< xml::sax::XDocumentHandler> xDocHandler = xSaxWriter;

        if ( !m_sDocumentHandler.isEmpty() )
        {
            try
            {
                uno::Sequence< uno::Any > aArgs{
                    uno::Any(beans::NamedValue(u"DocumentHandler"_ustr, uno::Any(xDocHandler))),
                    uno::Any(beans::NamedValue(u"Model"_ustr, uno::Any(xDocumentComp)))
                };

                xDocHandler.set(xServiceFactory->createInstanceWithArguments(m_sDocumentHandler,aArgs), uno::UNO_QUERY );
                xSaxWriter.set(xDocHandler,uno::UNO_QUERY);
            }
            catch (const uno::Exception&)
            {
                TOOLS_WARN_EXCEPTION( "chart2", "Exception caught!");
            }
        }

        Reference<document::XGraphicStorageHandler> xGraphicStorageHandler;
        xGraphicStorageHandler.set(document::GraphicStorageHandler::createWithStorage(m_xContext, xStorage));

        // property map for export info set
        static comphelper::PropertyMapEntry const aExportInfoMap[] =
        {
            { u"UsePrettyPrinting"_ustr, 0, cppu::UnoType<bool>::get(), beans::PropertyAttribute::MAYBEVOID, 0},
            { u"BaseURI"_ustr, 0, ::cppu::UnoType<OUString>::get(), beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"StreamRelPath"_ustr, 0, ::cppu::UnoType<OUString>::get(), beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"StreamName"_ustr, 0, ::cppu::UnoType<OUString>::get(), beans::PropertyAttribute::MAYBEVOID, 0 },
            { u"ExportTableNumberList"_ustr, 0, cppu::UnoType<bool>::get(), beans::PropertyAttribute::MAYBEVOID, 0 },
        };

        uno::Reference< beans::XPropertySet > xInfoSet =
            comphelper::GenericPropertySet_CreateInstance( new comphelper::PropertySetInfo( aExportInfoMap ) );

        bool bUsePrettyPrinting( officecfg::Office::Common::Save::Document::PrettyPrinting::get() );
        xInfoSet->setPropertyValue( u"UsePrettyPrinting"_ustr, uno::Any( bUsePrettyPrinting ) );
        if( ! bOasis )
            xInfoSet->setPropertyValue( u"ExportTableNumberList"_ustr, uno::Any( true ));

        sal_Int32 nArgs = 2;
        if( xGraphicStorageHandler.is())
            nArgs++;

        uno::Sequence< uno::Any > aFilterProperties( nArgs );
        {
            auto pFilterProperties = aFilterProperties.getArray();
            nArgs = 0;
            pFilterProperties[ nArgs++ ] <<= xInfoSet;
            pFilterProperties[ nArgs++ ] <<= xDocHandler;
            if( xGraphicStorageHandler.is())
                pFilterProperties[ nArgs++ ] <<= xGraphicStorageHandler;
        }

        // export meta information
        if( bOasis )
            nWarning = impl_ExportStream(
                sXML_metaStreamName,
                u"com.sun.star.comp.Chart.XMLOasisMetaExporter"_ustr,
                xStorage, xSaxWriter, xServiceFactory, aFilterProperties );

        // export styles
        ErrCode nTmp = impl_ExportStream(
            sXML_styleStreamName,
            bOasis
            ? u"com.sun.star.comp.Chart.XMLOasisStylesExporter"_ustr
            : u"com.sun.star.comp.Chart.XMLStylesExporter"_ustr, // soffice 6/7
            xStorage, xSaxWriter, xServiceFactory, aFilterProperties );
        nWarning = nWarning != ERRCODE_NONE ? nWarning : nTmp;

        // export content
        ErrCode nContentWarning = impl_ExportStream(
            sXML_contentStreamName,
            bOasis
            ? u"com.sun.star.comp.Chart.XMLOasisContentExporter"_ustr
            : u"com.sun.star.comp.Chart.XMLContentExporter"_ustr,
            xStorage, xSaxWriter, xServiceFactory, aFilterProperties );
        nWarning = nWarning != ERRCODE_NONE ? nWarning : nContentWarning;

        Reference< lang::XComponent > xComp(xGraphicStorageHandler, uno::UNO_QUERY);
        if (xComp.is())
            xComp->dispose();

        uno::Reference<embed::XTransactedObject> xTransact( xStorage ,uno::UNO_QUERY);
        if ( xTransact.is() )
            xTransact->commit();
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("chart2");

        // something went awry
        nWarning = ERRCODE_SFX_GENERAL;
    }

    return nWarning;
}

ErrCode XMLFilter::impl_ExportStream(
    const OUString & rStreamName,
    const OUString & rServiceName,
    const Reference< embed::XStorage > & xStorage,
    const uno::Reference< xml::sax::XWriter >& xActiveDataSource,
    const Reference< lang::XMultiServiceFactory >& xServiceFactory,
    const Sequence< uno::Any > & rFilterProperties )
{
    try
    {
        if( !xServiceFactory.is() )
            return ERRCODE_SFX_GENERAL;
        if( !xStorage.is() )
            return ERRCODE_SFX_GENERAL;
        if ( !xActiveDataSource.is() )
            return ERRCODE_SFX_GENERAL;

        uno::Reference< io::XStream > xStream( xStorage->openStreamElement(
            rStreamName, embed::ElementModes::READWRITE | embed::ElementModes::TRUNCATE ) );
        if ( !xStream.is() )
            return ERRCODE_SFX_GENERAL;
        uno::Reference< io::XOutputStream > xOutputStream( xStream->getOutputStream() );
        if ( !xOutputStream.is() )
            return ERRCODE_SFX_GENERAL;

        uno::Reference< beans::XPropertySet > xStreamProp( xOutputStream, uno::UNO_QUERY );
        if(xStreamProp.is()) try
        {
            xStreamProp->setPropertyValue( u"MediaType"_ustr, uno::Any( u"text/xml"_ustr ) );
            xStreamProp->setPropertyValue( u"Compressed"_ustr, uno::Any( true ) );//@todo?
            xStreamProp->setPropertyValue( u"UseCommonStoragePasswordEncryption"_ustr, uno::Any( true ) );
        }
        catch (const uno::Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }

        xActiveDataSource->setOutputStream(xOutputStream);

        // set Base URL
        {
            uno::Reference< beans::XPropertySet > xInfoSet;
            if( rFilterProperties.hasElements() )
                rFilterProperties[0] >>= xInfoSet;
            OSL_ENSURE( xInfoSet.is(), "missing infoset for export" );
            if( xInfoSet.is() )
                xInfoSet->setPropertyValue( u"StreamName"_ustr, uno::Any( rStreamName ) );
        }

        Reference< XExporter > xExporter( xServiceFactory->createInstanceWithArguments(
            rServiceName, rFilterProperties ), uno::UNO_QUERY);
        if ( !xExporter.is() )
            return ERRCODE_SFX_GENERAL;

        xExporter->setSourceDocument( m_xSourceDoc );

        uno::Reference< document::XFilter > xFilter( xExporter, uno::UNO_QUERY );
        if ( !xFilter.is() )
            return ERRCODE_SFX_GENERAL;

        xFilter->filter(m_aMediaDescriptor);
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("chart2");
    }
    return ERRCODE_NONE;
}

void XMLFilter::isOasisFormat(const Sequence< beans::PropertyValue >& _rMediaDescriptor, bool & rOutOASIS )
{
    apphelper::MediaDescriptorHelper aMDHelper( _rMediaDescriptor );
    if( aMDHelper.ISSET_FilterName )
        rOutOASIS = aMDHelper.FilterName == "chart8";
}
OUString XMLFilter::getMediaType(bool _bOasis)
{
    return _bOasis ? MIMETYPE_OASIS_OPENDOCUMENT_CHART_ASCII : MIMETYPE_VND_SUN_XML_CHART_ASCII;
}

OUString SAL_CALL XMLFilter::getImplementationName()
{
    return u"com.sun.star.comp.chart2.XMLFilter"_ustr;
}

sal_Bool SAL_CALL XMLFilter::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService(this, rServiceName);
}

css::uno::Sequence< OUString > SAL_CALL XMLFilter::getSupportedServiceNames()
{
    return {
        u"com.sun.star.document.ImportFilter"_ustr,
        u"com.sun.star.document.ExportFilter"_ustr
    };
    // todo: services are incomplete.  Missing:
    // XInitialization, XNamed
}

void XMLReportFilterHelper::isOasisFormat(const Sequence< beans::PropertyValue >& _rMediaDescriptor, bool & rOutOASIS )
{
    apphelper::MediaDescriptorHelper aMDHelper( _rMediaDescriptor );
    if( aMDHelper.ISSET_FilterName )
        rOutOASIS = aMDHelper.FilterName == "StarOffice XML (Base) Report Chart";
}
OUString XMLReportFilterHelper::getMediaType(bool )
{
    return MIMETYPE_OASIS_OPENDOCUMENT_REPORT_CHART_ASCII;
}

} //  namespace chart

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_chart2_XMLFilter_get_implementation(css::uno::XComponentContext *context,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new ::chart::XMLFilter(context));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_chart2_report_XMLFilter_get_implementation(css::uno::XComponentContext *context,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new ::chart::XMLReportFilterHelper(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

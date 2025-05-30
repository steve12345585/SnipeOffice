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

#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <osl/diagnose.h>
#include <svl/macitem.hxx>
#include <svtools/unoevent.hxx>
#include <sfx2/docfile.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/fileformat.h>
#include <comphelper/processfactory.hxx>
#include <comphelper/lok.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/xml/sax/FastParser.hpp>
#include <com/sun/star/xml/sax/FastToken.hpp>
#include <com/sun/star/xml/sax/Parser.hpp>
#include <com/sun/star/xml/sax/Writer.hpp>
#include <com/sun/star/xml/sax/SAXParseException.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <doc.hxx>
#include <docsh.hxx>
#include <shellio.hxx>
#include <SwXMLTextBlocks.hxx>
#include <SwXMLBlockImport.hxx>
#include <SwXMLBlockExport.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <sfx2/event.hxx>
#include <swerror.h>

constexpr OUString XMLN_BLOCKLIST = u"BlockList.xml"_ustr;

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::container;
using namespace css::xml::sax;
using namespace xmloff::token;

using ::xmloff::token::XML_BLOCK_LIST;
using ::xmloff::token::XML_UNFORMATTED_TEXT;
using ::xmloff::token::GetXMLToken;

ErrCode SwXMLTextBlocks::GetDoc( sal_uInt16 nIdx )
{
    OUString aFolderName ( GetPackageName ( nIdx ) );

    if (!IsOnlyTextBlock ( nIdx ) )
    {
        try
        {
            m_xRoot = m_xBlkRoot->openStorageElement( aFolderName, embed::ElementModes::READ );
            m_xMedium = new SfxMedium( m_xRoot, GetBaseURL(), u"writer8"_ustr );
            SwReader aReader( *m_xMedium, aFolderName, m_xDoc.get() );
            ReadXML->SetBlockMode( true );
            aReader.Read( *ReadXML );
            ReadXML->SetBlockMode( false );
            // Ole objects fail to display when inserted into the document, as
            // the ObjectReplacement folder and contents are missing
            OUString sObjReplacements( u"ObjectReplacements"_ustr );
            if ( m_xRoot->hasByName( sObjReplacements ) )
            {
                if (SwDocShell* pShell = m_xDoc->GetDocShell())
                {
                    uno::Reference< document::XStorageBasedDocument > xDocStor( pShell->GetModel(), uno::UNO_QUERY );
                    if (xDocStor)
                    {
                        uno::Reference< embed::XStorage > xStr( xDocStor->getDocumentStorage() );
                        if ( xStr.is() )
                        {
                            m_xRoot->copyElementTo( sObjReplacements, xStr, sObjReplacements );
                            uno::Reference< embed::XTransactedObject > xTrans( xStr, uno::UNO_QUERY );
                            if ( xTrans.is() )
                                xTrans->commit();
                        }
                    }
                }
            }
        }
        catch( uno::Exception& )
        {
        }

        m_xRoot = nullptr;
    }
    else
    {
        OUString aStreamName = aFolderName + ".xml";
        try
        {
            m_xRoot = m_xBlkRoot->openStorageElement( aFolderName, embed::ElementModes::READ );
            uno::Reference < io::XStream > xStream = m_xRoot->openStreamElement( aStreamName, embed::ElementModes::READ );

            const uno::Reference< uno::XComponentContext >& xContext =
                comphelper::getProcessComponentContext();

            xml::sax::InputSource aParserInput;
            aParserInput.sSystemId = m_aNames[nIdx]->m_aPackageName;

            aParserInput.aInputStream = xStream->getInputStream();

            // get filter
            uno::Reference< xml::sax::XFastDocumentHandler > xFilter = new SwXMLTextBlockImport( xContext, m_aCurrentText, true );
            uno::Reference< xml::sax::XFastTokenHandler > xTokenHandler = new SwXMLTextBlockTokenHandler();

            // connect parser and filter
            uno::Reference< xml::sax::XFastParser > xParser = xml::sax::FastParser::create(xContext);
            xParser->setFastDocumentHandler( xFilter );
            xParser->setTokenHandler( xTokenHandler );

            xParser->registerNamespace( u"http://openoffice.org/2000/text"_ustr, FastToken::NAMESPACE | XML_NAMESPACE_TEXT );
            xParser->registerNamespace( u"http://openoffice.org/2000/office"_ustr, FastToken::NAMESPACE | XML_NAMESPACE_OFFICE );

            // parse
            try
            {
                xParser->parseStream( aParserInput );
            }
            catch( xml::sax::SAXParseException&  )
            {
                // re throw ?
            }
            catch( xml::sax::SAXException&  )
            {
                // re throw ?
            }
            catch( io::IOException& )
            {
                // re throw ?
            }

            m_bInfoChanged = false;
            MakeBlockText(m_aCurrentText);
        }
        catch( uno::Exception& )
        {
        }

        m_xRoot = nullptr;
    }
    return ERRCODE_NONE;
}

// event description for autotext events; this constant should really be
// taken from unocore/unoevents.cxx or ui/unotxt.cxx
const struct SvEventDescription aAutotextEvents[] =
{
    { SvMacroItemId::SwStartInsGlossary,  "OnInsertStart" },
    { SvMacroItemId::SwEndInsGlossary,    "OnInsertDone" },
    { SvMacroItemId::NONE, nullptr }
};

ErrCode SwXMLTextBlocks::GetMacroTable( sal_uInt16 nIdx,
                                      SvxMacroTableDtor& rMacroTable )
{
    // set current auto text
    m_aShort = m_aNames[nIdx]->m_aShort;
    m_aLong = m_aNames[nIdx]->m_aLong;
    m_aPackageName = m_aNames[nIdx]->m_aPackageName;

    // open stream in proper sub-storage
    CloseFile();
    if ( OpenFile() != ERRCODE_NONE )
        return ERR_SWG_READ_ERROR;

    if (comphelper::LibreOfficeKit::isActive())
        return ERR_SWG_READ_ERROR;

    try
    {
        m_xRoot = m_xBlkRoot->openStorageElement( m_aPackageName, embed::ElementModes::READ );
        bool bOasis = SotStorage::GetVersion( m_xRoot ) > SOFFICE_FILEFORMAT_60;

        uno::Reference < io::XStream > xDocStream = m_xRoot->openStreamElement(
            u"atevent.xml"_ustr, embed::ElementModes::READ );
        SAL_INFO("sw", "Can't open atevent.xml stream");
        if ( !xDocStream.is() )
            return ERR_SWG_READ_ERROR;

        // prepare ParserInputSource
        xml::sax::InputSource aParserInput;
        aParserInput.sSystemId = m_aName;
        aParserInput.aInputStream = xDocStream->getInputStream();

        // get service factory
        const uno::Reference< uno::XComponentContext >& xContext =
            comphelper::getProcessComponentContext();

        // create descriptor and reference to it. Either
        // both or neither must be kept because of the
        // reference counting!
        rtl::Reference<SvMacroTableEventDescriptor> pDescriptor =
            new SvMacroTableEventDescriptor(aAutotextEvents);
        Sequence<Any> aFilterArguments{ Any(uno::Reference<XNameReplace>(pDescriptor)) };

        // get filter
        OUString sFilterComponent = bOasis
            ? u"com.sun.star.comp.Writer.XMLOasisAutotextEventsImporter"_ustr
            : u"com.sun.star.comp.Writer.XMLAutotextEventsImporter"_ustr;
        uno::Reference< XInterface > xFilterInt =
            xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                sFilterComponent, aFilterArguments, xContext);


        // parse the stream
        try
        {
            Reference<css::xml::sax::XFastParser> xFastParser(xFilterInt, UNO_QUERY);
            Reference<css::xml::sax::XFastDocumentHandler> xFastDocHandler(xFilterInt, UNO_QUERY);
            if (xFastParser)
            {
                xFastParser->parseStream(aParserInput);
            }
            else if (xFastDocHandler)
            {
                Reference<css::xml::sax::XFastParser> xParser
                    = css::xml::sax::FastParser::create(xContext);
                xParser->setFastDocumentHandler(xFastDocHandler);
                xParser->parseStream(aParserInput);
            }
            else
            {
                Reference<css::xml::sax::XDocumentHandler> xDocHandler(xFilterInt, UNO_QUERY);
                OSL_ENSURE( xDocHandler.is(), "can't instantiate autotext-events filter");
                if ( !xDocHandler.is() )
                    return ERR_SWG_READ_ERROR;
                Reference<css::xml::sax::XParser> xParser = css::xml::sax::Parser::create(xContext);
                xParser->setDocumentHandler(xDocHandler);
                xParser->parseStream(aParserInput);
            }
        }
        catch( xml::sax::SAXParseException& )
        {
            // workaround for #83452#: SetSize doesn't work
            // nRet = ERR_SWG_READ_ERROR;
        }
        catch( xml::sax::SAXException& )
        {
            TOOLS_WARN_EXCEPTION("sw", "");
            return ERR_SWG_READ_ERROR;
        }
        catch( io::IOException& )
        {
            TOOLS_WARN_EXCEPTION("sw", "");
            return ERR_SWG_READ_ERROR;
        }

        // and finally, copy macro into table
        pDescriptor->copyMacrosIntoTable(rMacroTable);
    }
    catch( uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("sw", "");
        return ERR_SWG_READ_ERROR;
    }

    // success!
    return ERRCODE_NONE;
}

ErrCode SwXMLTextBlocks::GetBlockText( std::u16string_view rShort, OUString& rText )
{
    OUString aFolderName = GeneratePackageName ( rShort );
    OUString aStreamName = aFolderName + ".xml";
    rText.clear();

    try
    {
        bool bTextOnly = true;

        m_xRoot = m_xBlkRoot->openStorageElement( aFolderName, embed::ElementModes::READ );
        if ( !m_xRoot->hasByName( aStreamName ) || !m_xRoot->isStreamElement( aStreamName ) )
        {
            bTextOnly = false;
            aStreamName = "content.xml";
        }

        uno::Reference < io::XStream > xContents = m_xRoot->openStreamElement( aStreamName, embed::ElementModes::READ );
        const uno::Reference< uno::XComponentContext >& xContext =
            comphelper::getProcessComponentContext();

        xml::sax::InputSource aParserInput;
        aParserInput.sSystemId = m_aName;
        aParserInput.aInputStream = xContents->getInputStream();

        // get filter
        uno::Reference< xml::sax::XFastDocumentHandler > xFilter = new SwXMLTextBlockImport( xContext, rText, bTextOnly );
        uno::Reference< xml::sax::XFastTokenHandler > xTokenHandler = new SwXMLTextBlockTokenHandler();

        // connect parser and filter
        uno::Reference< xml::sax::XFastParser > xParser = xml::sax::FastParser::create(xContext);
        xParser->setFastDocumentHandler( xFilter );
        xParser->setTokenHandler( xTokenHandler );

        xParser->registerNamespace( u"urn:oasis:names:tc:opendocument:xmlns:office:1.0"_ustr, FastToken::NAMESPACE | XML_NAMESPACE_OFFICE );
        xParser->registerNamespace( u"urn:oasis:names:tc:opendocument:xmlns:text:1.0"_ustr, FastToken::NAMESPACE | XML_NAMESPACE_TEXT );

        // parse
        try
        {
            xParser->parseStream( aParserInput );
        }
        catch( xml::sax::SAXParseException&  )
        {
            // re throw ?
        }
        catch( xml::sax::SAXException&  )
        {
            // re throw ?
        }
        catch( io::IOException& )
        {
            // re throw ?
        }

        m_xRoot = nullptr;
    }
    catch ( uno::Exception& )
    {
        SAL_WARN("sw", "Tried to open non-existent folder or stream: " << aStreamName << " derived from autocorr of: " << OUString(rShort));
    }

    return ERRCODE_NONE;
}

ErrCode SwXMLTextBlocks::PutBlockText( const OUString& rShort,
                                         std::u16string_view rText,  const OUString& rPackageName )
{
    GetIndex ( rShort );
    /*
    if (xBlkRoot->IsContained ( rPackageName ) )
    {
        xBlkRoot->Remove ( rPackageName );
        xBlkRoot->Commit ( );
    }
    */
    OUString aStreamName = rPackageName + ".xml";

    const uno::Reference< uno::XComponentContext >& xContext =
        comphelper::getProcessComponentContext();

    uno::Reference < xml::sax::XWriter > xWriter = xml::sax::Writer::create(xContext);
    ErrCode nRes = ERRCODE_NONE;

    try
    {
    m_xRoot = m_xBlkRoot->openStorageElement( rPackageName, embed::ElementModes::WRITE );
    uno::Reference < io::XStream > xDocStream = m_xRoot->openStreamElement( aStreamName,
                embed::ElementModes::WRITE | embed::ElementModes::TRUNCATE );

    uno::Reference < beans::XPropertySet > xSet( xDocStream, uno::UNO_QUERY );
    xSet->setPropertyValue(u"MediaType"_ustr, Any(u"text/xml"_ustr) );
    uno::Reference < io::XOutputStream > xOut = xDocStream->getOutputStream();
    xWriter->setOutputStream(xOut);

    rtl::Reference<SwXMLTextBlockExport> xExp( new SwXMLTextBlockExport( xContext, *this, GetXMLToken ( XML_UNFORMATTED_TEXT ), xWriter) );

    xExp->exportDoc( rText );

    uno::Reference < embed::XTransactedObject > xTrans( m_xRoot, uno::UNO_QUERY );
    if ( xTrans.is() )
        xTrans->commit();

    if (! (m_nFlags & SwXmlFlags::NoRootCommit) )
    {
        uno::Reference < embed::XTransactedObject > xTmpTrans( m_xBlkRoot, uno::UNO_QUERY );
        if ( xTmpTrans.is() )
            xTmpTrans->commit();
    }
    }
    catch ( uno::Exception& )
    {
        nRes = ERR_SWG_WRITE_ERROR;
    }

    m_xRoot = nullptr;

    //TODO/LATER: error handling
    /*
    sal_uLong nErr = xBlkRoot->GetError();
    sal_uLong nRes = 0;
    if( nErr == SVSTREAM_DISK_FULL )
        nRes = ERR_W4W_WRITE_FULL;
    else if( nErr != ERRCODE_NONE )
        nRes = ERR_SWG_WRITE_ERROR;
    */
    if( !nRes ) // So that we can access the Doc via GetText & nCur
        MakeBlockText( rText );

    return nRes;
}

void SwXMLTextBlocks::ReadInfo()
{
    const OUString sDocName( XMLN_BLOCKLIST );
    try
    {
        if ( !m_xBlkRoot.is() || !m_xBlkRoot->hasByName( sDocName ) || !m_xBlkRoot->isStreamElement( sDocName ) )
            return;

        const uno::Reference< uno::XComponentContext >& xContext =
                comphelper::getProcessComponentContext();

        xml::sax::InputSource aParserInput;
        aParserInput.sSystemId = sDocName;

        uno::Reference < io::XStream > xDocStream = m_xBlkRoot->openStreamElement( sDocName, embed::ElementModes::READ );
        aParserInput.aInputStream = xDocStream->getInputStream();

        // get filter
        uno::Reference< xml::sax::XFastDocumentHandler > xFilter = new SwXMLBlockListImport( xContext, *this );
        uno::Reference< xml::sax::XFastTokenHandler > xTokenHandler = new SwXMLBlockListTokenHandler();

        // connect parser and filter
        uno::Reference< xml::sax::XFastParser > xParser = xml::sax::FastParser::create(xContext);
        xParser->setFastDocumentHandler( xFilter );
        xParser->registerNamespace( u"http://openoffice.org/2001/block-list"_ustr, FastToken::NAMESPACE | XML_NAMESPACE_BLOCKLIST );
        xParser->setTokenHandler( xTokenHandler );

        // parse
        xParser->parseStream( aParserInput );
    }
    catch ( uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("sw", "when loading " << sDocName);
        // re throw ?
    }
}
void SwXMLTextBlocks::WriteInfo()
{
    if ( !(m_xBlkRoot.is() || ERRCODE_NONE == OpenFile ( false )) )
        return;

    const uno::Reference< uno::XComponentContext >& xContext =
        comphelper::getProcessComponentContext();

    uno::Reference < xml::sax::XWriter > xWriter = xml::sax::Writer::create(xContext);

    /*
    if ( xBlkRoot->IsContained( sDocName) )
    {
        xBlkRoot->Remove ( sDocName );
        xBlkRoot->Commit();
    }
    */

    try
    {
    uno::Reference < io::XStream > xDocStream = m_xBlkRoot->openStreamElement( XMLN_BLOCKLIST,
                embed::ElementModes::WRITE | embed::ElementModes::TRUNCATE );

    uno::Reference < beans::XPropertySet > xSet( xDocStream, uno::UNO_QUERY );
    xSet->setPropertyValue(u"MediaType"_ustr, Any(u"text/xml"_ustr) );
    uno::Reference < io::XOutputStream > xOut = xDocStream->getOutputStream();
    xWriter->setOutputStream(xOut);

    rtl::Reference<SwXMLBlockListExport> xExp(new SwXMLBlockListExport( xContext, *this, XMLN_BLOCKLIST, xWriter) );

    xExp->exportDoc( XML_BLOCK_LIST );

    uno::Reference < embed::XTransactedObject > xTrans( m_xBlkRoot, uno::UNO_QUERY );
    if ( xTrans.is() )
        xTrans->commit();
    }
    catch ( uno::Exception& )
    {
    }

    m_bInfoChanged = false;
    return;
}

ErrCode SwXMLTextBlocks::SetMacroTable(
    sal_uInt16 nIdx,
    const SvxMacroTableDtor& rMacroTable )
{
    // set current autotext
    m_aShort = m_aNames[nIdx]->m_aShort;
    m_aLong = m_aNames[nIdx]->m_aLong;
    m_aPackageName = m_aNames[nIdx]->m_aPackageName;

    // start XML autotext event export
    ErrCode nRes = ERRCODE_NONE;

    const uno::Reference< uno::XComponentContext >& xContext =
        comphelper::getProcessComponentContext();

    SwDocShell* pShell = m_xDoc->GetDocShell();
    if (!pShell)
        return ERR_SWG_WRITE_ERROR;

    // Get model
    uno::Reference< lang::XComponent > xModelComp = pShell->GetModel();
    OSL_ENSURE( xModelComp.is(), "XMLWriter::Write: got no model" );
    if( !xModelComp.is() )
        return ERR_SWG_WRITE_ERROR;

    // open stream in proper sub-storage
    CloseFile(); // close (it may be open in read-only-mode)
    nRes = OpenFile ( false );

    if ( ERRCODE_NONE == nRes )
    {
        try
        {
            m_xRoot = m_xBlkRoot->openStorageElement( m_aPackageName, embed::ElementModes::WRITE );
            bool bOasis = SotStorage::GetVersion( m_xRoot ) > SOFFICE_FILEFORMAT_60;

            uno::Reference < io::XStream > xDocStream = m_xRoot->openStreamElement( u"atevent.xml"_ustr,
                        embed::ElementModes::WRITE | embed::ElementModes::TRUNCATE );

            uno::Reference < beans::XPropertySet > xSet( xDocStream, uno::UNO_QUERY );
            xSet->setPropertyValue(u"MediaType"_ustr, Any(u"text/xml"_ustr) );
            uno::Reference < io::XOutputStream > xOutputStream = xDocStream->getOutputStream();

            // get XML writer
            uno::Reference< xml::sax::XWriter > xSaxWriter =
                xml::sax::Writer::create( xContext );

            // connect XML writer to output stream
            xSaxWriter->setOutputStream( xOutputStream );

            // construct events object
            uno::Reference<XNameAccess> xEvents =
                new SvMacroTableEventDescriptor(rMacroTable,aAutotextEvents);

            // prepare arguments (prepend doc handler to given arguments)
            Sequence<Any> aParams{ Any(xSaxWriter), Any(xEvents) };

            // get filter component
            OUString sFilterComponent = bOasis
                ? u"com.sun.star.comp.Writer.XMLOasisAutotextEventsExporter"_ustr
                : u"com.sun.star.comp.Writer.XMLAutotextEventsExporter"_ustr;
            uno::Reference< document::XExporter > xExporter(
                xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                    sFilterComponent, aParams, xContext), UNO_QUERY);
            OSL_ENSURE( xExporter.is(),
                    "can't instantiate export filter component" );
            if( xExporter.is() )
            {
                // connect model and filter
                xExporter->setSourceDocument( xModelComp );

                // filter!
                Sequence<beans::PropertyValue> aFilterProps( 0 );
                uno::Reference < document::XFilter > xFilter( xExporter,
                                                         UNO_QUERY );
                xFilter->filter( aFilterProps );
            }
            else
                nRes = ERR_SWG_WRITE_ERROR;

            // finally, commit stream, sub-storage and storage
            uno::Reference < embed::XTransactedObject > xTmpTrans( m_xRoot, uno::UNO_QUERY );
            if ( xTmpTrans.is() )
                xTmpTrans->commit();

            uno::Reference < embed::XTransactedObject > xTrans( m_xBlkRoot, uno::UNO_QUERY );
            if ( xTrans.is() )
                xTrans->commit();

            m_xRoot = nullptr;
        }
        catch ( uno::Exception& )
        {
            nRes = ERR_SWG_WRITE_ERROR;
        }

        CloseFile();
    }
    else
        nRes = ERR_SWG_WRITE_ERROR;

    return nRes;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

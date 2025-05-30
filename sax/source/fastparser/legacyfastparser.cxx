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

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/xml/sax/XParser.hpp>
#include <com/sun/star/xml/sax/FastParser.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/beans/Pair.hpp>
#include <comphelper/attributelist.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/processfactory.hxx>
#include <rtl/ref.hxx>
#include <memory>
#include <utility>
#include <vector>

using namespace ::cppu;
using namespace css;
using namespace uno;
using namespace lang;
using namespace xml::sax;
using namespace io;

namespace {

class NamespaceHandler : public WeakImplHelper< XFastNamespaceHandler >
{
private:
    struct NamespaceDefine
    {
        OUString    m_aPrefix;
        OUString    m_aNamespaceURI;

        NamespaceDefine( OUString aPrefix, OUString aNamespaceURI ) : m_aPrefix(std::move( aPrefix )), m_aNamespaceURI(std::move( aNamespaceURI )) {}
    };
    std::vector< std::unique_ptr< NamespaceDefine > > m_aNamespaceDefines;

public:
    NamespaceHandler();
    void addNSDeclAttributes( rtl::Reference < comphelper::AttributeList > const & rAttrList );

    //XFastNamespaceHandler
    virtual void SAL_CALL registerNamespace( const OUString& rNamespacePrefix, const OUString& rNamespaceURI ) override;
    virtual OUString SAL_CALL getNamespaceURI( const OUString& rNamespacePrefix ) override;
};

NamespaceHandler::NamespaceHandler()
{
}

void NamespaceHandler::addNSDeclAttributes( rtl::Reference < comphelper::AttributeList > const & rAttrList )
{
    for(const auto& aNamespaceDefine : m_aNamespaceDefines)
    {
        OUString& rPrefix = aNamespaceDefine->m_aPrefix;
        OUString& rNamespaceURI = aNamespaceDefine->m_aNamespaceURI;
        OUString sDecl;
        if ( rPrefix.isEmpty() )
            sDecl = "xmlns";
        else
            sDecl = "xmlns:" + rPrefix;
        rAttrList->AddAttribute( sDecl, rNamespaceURI );
    }
    m_aNamespaceDefines.clear();
}

void NamespaceHandler::registerNamespace( const OUString& rNamespacePrefix, const OUString& rNamespaceURI )
{
    m_aNamespaceDefines.push_back( std::make_unique<NamespaceDefine>(
                                    rNamespacePrefix, rNamespaceURI) );
}

OUString NamespaceHandler::getNamespaceURI( const OUString&/* rNamespacePrefix */ )
{
    return OUString();
}

class SaxLegacyFastParser : public WeakImplHelper< XInitialization, XServiceInfo, XParser >
{
private:
    rtl::Reference< NamespaceHandler > m_aNamespaceHandler;
public:
    SaxLegacyFastParser();

// css::lang::XInitialization:
    virtual void SAL_CALL initialize(css::uno::Sequence<css::uno::Any> const& rArguments) override;

// The SAX-Parser-Interface
    virtual void SAL_CALL parseStream(  const InputSource& structSource) override;
    virtual void SAL_CALL setDocumentHandler(const Reference< XDocumentHandler > & xHandler) override;
    virtual void SAL_CALL setErrorHandler(const Reference< XErrorHandler > & xHandler) override;
    virtual void SAL_CALL setDTDHandler(const Reference < XDTDHandler > & xHandler) override;
    virtual void SAL_CALL setEntityResolver(const Reference<  XEntityResolver >& xResolver) override;
    virtual void SAL_CALL setLocale( const Locale &locale ) override;

// XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
    sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;

private:
    Reference< XFastParser > m_xParser;
    Reference< XDocumentHandler > m_xDocumentHandler;
    Reference< XFastTokenHandler > m_xTokenHandler;

};


class CallbackDocumentHandler : public WeakImplHelper< XFastDocumentHandler >
{
private:
    Reference< XDocumentHandler > m_xDocumentHandler;
    Reference< XFastTokenHandler > m_xTokenHandler;
    rtl::Reference< NamespaceHandler > m_aNamespaceHandler;
    OUString getNamespacePrefixFromToken( sal_Int32 nToken );
    OUString getNameFromToken( sal_Int32 nToken );

    static constexpr OUString aDefaultNamespace = u""_ustr;
    static constexpr OUString aNamespaceSeparator = u":"_ustr;

public:
    CallbackDocumentHandler( Reference< XDocumentHandler > const & xDocumentHandler,
                             rtl::Reference< NamespaceHandler > const & rNamespaceHandler,
                             Reference< XFastTokenHandler > const & xTokenHandler);

    // XFastDocumentHandler
    virtual void SAL_CALL startDocument() override;
    virtual void SAL_CALL endDocument() override;
    virtual void SAL_CALL processingInstruction( const OUString& rTarget, const OUString& rData ) override;
    virtual void SAL_CALL setDocumentLocator( const Reference< XLocator >& xLocator ) override;

    // XFastContextHandler
    virtual void SAL_CALL startFastElement( sal_Int32 nElement, const Reference< XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL startUnknownElement( const OUString& Namespace, const OUString& Name, const Reference< XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL endFastElement( sal_Int32 Element ) override;
    virtual void SAL_CALL endUnknownElement( const OUString& Namespace, const OUString& Name ) override;
    virtual Reference< XFastContextHandler > SAL_CALL createFastChildContext( sal_Int32 nElement, const Reference< XFastAttributeList >& Attribs ) override;
    virtual Reference< XFastContextHandler > SAL_CALL createUnknownChildContext( const OUString& Namespace, const OUString& Name, const Reference< XFastAttributeList >& Attribs ) override;
    virtual void SAL_CALL characters( const OUString& aChars ) override;

};

OUString CallbackDocumentHandler::getNamespacePrefixFromToken( sal_Int32 nToken )
{
    if ( ( nToken & 0xffff0000 ) != 0 )
    {
        Sequence< sal_Int8 > aSeq = m_xTokenHandler->getUTF8Identifier( nToken & 0xffff0000 );
        return OUString( reinterpret_cast< const char* >(
                        aSeq.getConstArray() ), aSeq.getLength(), RTL_TEXTENCODING_UTF8 );
    }
    else
        return OUString();
}

OUString CallbackDocumentHandler::getNameFromToken( sal_Int32 nToken )
{
    Sequence< sal_Int8 > aSeq = m_xTokenHandler->getUTF8Identifier( nToken & 0xffff );
    return OUString( reinterpret_cast< const char* >(
                    aSeq.getConstArray() ), aSeq.getLength(), RTL_TEXTENCODING_UTF8 );
}

CallbackDocumentHandler::CallbackDocumentHandler( Reference< XDocumentHandler > const & xDocumentHandler,
                                                  rtl::Reference< NamespaceHandler > const & rNamespaceHandler,
                                                  Reference< XFastTokenHandler > const & xTokenHandler)
{
   m_xDocumentHandler.set( xDocumentHandler );
   m_aNamespaceHandler = rNamespaceHandler;
   m_xTokenHandler.set( xTokenHandler );
}

void SAL_CALL CallbackDocumentHandler::startDocument()
{
    if ( m_xDocumentHandler.is() )
        m_xDocumentHandler->startDocument();
}

void SAL_CALL CallbackDocumentHandler::endDocument()
{
    if ( m_xDocumentHandler.is() )
        m_xDocumentHandler->endDocument();
}

void SAL_CALL CallbackDocumentHandler::processingInstruction( const OUString& rTarget, const OUString& rData )
{
    if ( m_xDocumentHandler.is() )
        m_xDocumentHandler->processingInstruction( rTarget, rData );
}

void SAL_CALL CallbackDocumentHandler::setDocumentLocator( const Reference< XLocator >& xLocator )
{
    if ( m_xDocumentHandler.is() )
        m_xDocumentHandler->setDocumentLocator( xLocator );
}

void SAL_CALL CallbackDocumentHandler::startFastElement( sal_Int32 nElement , const Reference< XFastAttributeList >& Attribs  )
{
    const OUString aPrefix = CallbackDocumentHandler::getNamespacePrefixFromToken( nElement );
    const OUString aLocalName = CallbackDocumentHandler::getNameFromToken( nElement );
    startUnknownElement( aDefaultNamespace, (aPrefix.isEmpty())? aLocalName : aPrefix + aNamespaceSeparator + aLocalName, Attribs );
}

void SAL_CALL CallbackDocumentHandler::startUnknownElement( const OUString& /*Namespace*/, const OUString& Name, const Reference< XFastAttributeList >& Attribs  )
{
    if ( !m_xDocumentHandler.is() )
        return;

    rtl::Reference < comphelper::AttributeList > rAttrList = new comphelper::AttributeList;
    m_aNamespaceHandler->addNSDeclAttributes( rAttrList );

    const Sequence< xml::FastAttribute > fastAttribs = Attribs->getFastAttributes();
    for (const auto& rAttr : fastAttribs)
    {
        const OUString& rAttrValue = rAttr.Value;
        sal_Int32 nToken = rAttr.Token;
        const OUString aAttrNamespacePrefix = CallbackDocumentHandler::getNamespacePrefixFromToken( nToken );
        OUString sAttrName = CallbackDocumentHandler::getNameFromToken( nToken );
        if ( !aAttrNamespacePrefix.isEmpty() )
            sAttrName = aAttrNamespacePrefix + aNamespaceSeparator + sAttrName;

        rAttrList->AddAttribute( sAttrName, rAttrValue );
    }

    const Sequence< xml::Attribute > unknownAttribs = Attribs->getUnknownAttributes();
    for (const auto& rAttr : unknownAttribs)
    {
        const OUString& rAttrValue = rAttr.Value;
        const OUString& rAttrName = rAttr.Name;

        rAttrList->AddAttribute( rAttrName, rAttrValue );
    }
    m_xDocumentHandler->startElement( Name, rAttrList );
}

void SAL_CALL CallbackDocumentHandler::endFastElement( sal_Int32 nElement )
{
    const OUString aPrefix = CallbackDocumentHandler::getNamespacePrefixFromToken( nElement );
    const OUString aLocalName = CallbackDocumentHandler::getNameFromToken( nElement );
    endUnknownElement( aDefaultNamespace, (aPrefix.isEmpty())? aLocalName : aPrefix + aNamespaceSeparator + aLocalName );
}


void SAL_CALL CallbackDocumentHandler::endUnknownElement( const OUString& /*Namespace*/, const OUString& Name )
{
    if ( m_xDocumentHandler.is() )
    {
        m_xDocumentHandler->endElement( Name );
    }
}

Reference< XFastContextHandler > SAL_CALL CallbackDocumentHandler::createFastChildContext( sal_Int32/* nElement */, const Reference< XFastAttributeList >&/* Attribs */ )
{
    return this;
}


Reference< XFastContextHandler > SAL_CALL CallbackDocumentHandler::createUnknownChildContext( const OUString&/* Namespace */, const OUString&/* Name */, const Reference< XFastAttributeList >&/* Attribs */ )
{
    return this;
}

void SAL_CALL CallbackDocumentHandler::characters( const OUString& aChars )
{
    if ( m_xDocumentHandler.is() )
        m_xDocumentHandler->characters( aChars );
}

SaxLegacyFastParser::SaxLegacyFastParser( ) : m_aNamespaceHandler( new NamespaceHandler ),
  m_xParser(FastParser::create(::comphelper::getProcessComponentContext() ))
{
    m_xParser->setNamespaceHandler( m_aNamespaceHandler );
}

void SAL_CALL SaxLegacyFastParser::initialize(Sequence< Any > const& rArguments )
{
    if (!rArguments.hasElements())
        return;

    Reference< XFastTokenHandler > xTokenHandler;
    OUString str;
    if ( ( rArguments[0] >>= xTokenHandler ) && xTokenHandler.is() )
    {
        m_xTokenHandler.set( xTokenHandler );
    }
    else if ( ( rArguments[0] >>= str ) && "registerNamespaces" == str )
    {
        css::beans::Pair< OUString, sal_Int32 > rPair;
        for (sal_Int32 i = 1; i < rArguments.getLength(); i++ )
        {
            rArguments[i] >>= rPair;
            m_xParser->registerNamespace( rPair.First, rPair.Second );
        }
    }
    else
    {
        uno::Reference<lang::XInitialization> const xInit(m_xParser,
                        uno::UNO_QUERY_THROW);
        xInit->initialize( rArguments );
    }
}

void SaxLegacyFastParser::parseStream( const InputSource& structSource )
{
    m_xParser->setFastDocumentHandler( new CallbackDocumentHandler( m_xDocumentHandler,
                                       m_aNamespaceHandler, m_xTokenHandler ) );
    m_xParser->setTokenHandler( m_xTokenHandler );
    m_xParser->parseStream( structSource );
}

void SaxLegacyFastParser::setDocumentHandler( const Reference< XDocumentHandler > & xHandler )
{
    m_xDocumentHandler = xHandler;
}

void SaxLegacyFastParser::setErrorHandler( const Reference< XErrorHandler > & xHandler )
{
    m_xParser->setErrorHandler( xHandler );
}

void SaxLegacyFastParser::setDTDHandler( const Reference < XDTDHandler > &/* xHandler */ )
{

}

void SaxLegacyFastParser::setEntityResolver( const Reference<  XEntityResolver >& xResolver )
{
    m_xParser->setEntityResolver( xResolver );
}

void SaxLegacyFastParser::setLocale( const Locale &locale )
{
    m_xParser->setLocale( locale );
}

OUString SaxLegacyFastParser::getImplementationName()
{
    return u"com.sun.star.comp.extensions.xml.sax.LegacyFastParser"_ustr;
}

sal_Bool SaxLegacyFastParser::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SaxLegacyFastParser::getSupportedServiceNames()
{
    return { u"com.sun.star.xml.sax.LegacyFastParser"_ustr };
}

} //namespace

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_extensions_xml_sax_LegacyFastParser_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new SaxLegacyFastParser);
}

 /* vim:set shiftwidth=4 softtabstop=4 expandtab: */

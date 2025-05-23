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

#include <sal/config.h>

#include <string_view>

#include <config_folders.h>

#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <comphelper/sequence.hxx>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/deployment/UpdateInformationEntry.hpp>
#include <com/sun/star/deployment/XUpdateInformationProvider.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/ucb/UniversalContentBroker.hpp>
#include <com/sun/star/ucb/XWebDAVCommandEnvironment.hpp>
#include <com/sun/star/ucb/XCommandProcessor2.hpp>
#include <com/sun/star/ucb/OpenCommandArgument3.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/task/PasswordContainerInteractionHandler.hpp>
#include <com/sun/star/xml/dom/DocumentBuilder.hpp>
#include <com/sun/star/xml/xpath/XPathAPI.hpp>
#include <com/sun/star/xml/xpath/XPathException.hpp>
#include <rtl/ref.hxx>
#include <rtl/bootstrap.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <osl/conditn.hxx>
#include <utility>
#include <vcl/svapp.hxx>

namespace beans = css::beans ;
namespace container = css::container ;
namespace deployment = css::deployment ;
namespace io = css::io ;
namespace lang = css::lang ;
namespace task = css::task ;
namespace ucb = css::ucb ;
namespace uno = css::uno ;
namespace xml = css::xml ;


namespace
{

#if OSL_DEBUG_LEVEL >= 2

class InputStreamWrapper : public ::cppu::WeakImplHelper< io::XInputStream >
{
    uno::Reference< io::XInputStream > m_xStream;

public:
    explicit InputStreamWrapper(const uno::Reference< io::XInputStream >& rxStream) :
        m_xStream(rxStream) {};

    virtual sal_Int32 SAL_CALL readBytes(uno::Sequence< sal_Int8 >& aData, sal_Int32 nBytesToRead)
        {
            sal_Int32 n = m_xStream->readBytes(aData, nBytesToRead);
            return n;
        };
    virtual sal_Int32 SAL_CALL readSomeBytes(uno::Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead)
        {
            sal_Int32 n = m_xStream->readSomeBytes(aData, nMaxBytesToRead);
            return n;
        };
    virtual void SAL_CALL skipBytes( sal_Int32 nBytesToSkip )
        { m_xStream->skipBytes(nBytesToSkip); };
    virtual sal_Int32 SAL_CALL available()
        { return m_xStream->available(); };
    virtual void SAL_CALL closeInput( )
        {};
};

#define INPUT_STREAM(i) new InputStreamWrapper(i)
#else
#define INPUT_STREAM(i) i
#endif


class ActiveDataSink : public ::cppu::WeakImplHelper< io::XActiveDataSink >
{
    uno::Reference< io::XInputStream > m_xStream;

public:
    ActiveDataSink() {};

    virtual uno::Reference< io::XInputStream > SAL_CALL getInputStream() override { return m_xStream; };
    virtual void SAL_CALL setInputStream( uno::Reference< io::XInputStream > const & rStream ) override { m_xStream = rStream; };
};


class UpdateInformationProvider :
    public ::cppu::WeakImplHelper< deployment::XUpdateInformationProvider,
                                    ucb::XWebDAVCommandEnvironment,
                                    lang::XServiceInfo >
{
    OUString getUserAgent(bool bExtended);
    bool isUserAgentExtended() const;
public:
    uno::Reference< xml::dom::XElement > getDocumentRoot(const uno::Reference< xml::dom::XNode >& rxNode);
    uno::Reference< xml::dom::XNode > getChildNode(const uno::Reference< xml::dom::XNode >& rxNode, std::u16string_view rName);


    // XUpdateInformationService
    virtual uno::Sequence< uno::Reference< xml::dom::XElement > > SAL_CALL
    getUpdateInformation(
        uno::Sequence< OUString > const & repositories,
        OUString const & extensionId
    ) override;

    virtual void SAL_CALL cancel() override;

    virtual void SAL_CALL setInteractionHandler(
        uno::Reference< task::XInteractionHandler > const & handler ) override;

    virtual uno::Reference< container::XEnumeration > SAL_CALL
    getUpdateInformationEnumeration(
        uno::Sequence< OUString > const & repositories,
        OUString const & extensionId
    ) override;

    // XCommandEnvironment
    virtual uno::Reference< task::XInteractionHandler > SAL_CALL getInteractionHandler() override;

    virtual uno::Reference< ucb::XProgressHandler > SAL_CALL getProgressHandler() override { return  uno::Reference< ucb::XProgressHandler >(); };

    // XWebDAVCommandEnvironment
    virtual uno::Sequence< beans::StringPair > SAL_CALL getUserRequestHeaders(
        const OUString&,  ucb::WebDAVHTTPMethod ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(OUString const & serviceName) override;
    virtual uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    UpdateInformationProvider(uno::Reference<uno::XComponentContext> xContext,
                              uno::Reference< ucb::XUniversalContentBroker > xUniversalContentBroker,
                              uno::Reference< xml::dom::XDocumentBuilder > xDocumentBuilder,
                              uno::Reference< xml::xpath::XXPathAPI > xXPathAPI);

protected:

    virtual ~UpdateInformationProvider() override;
    static OUString getConfigurationItem(uno::Reference<lang::XMultiServiceFactory> const & configurationProvider, OUString const & node, OUString const & item);
    static uno::Any getConfigurationItemAny(uno::Reference<lang::XMultiServiceFactory> const & configurationProvider, OUString const & node, OUString const & item);

private:
    uno::Reference< io::XInputStream > load(const OUString& rURL);

    void storeCommandInfo( sal_Int32 nCommandId,
        uno::Reference< ucb::XCommandProcessor > const & rxCommandProcessor);

    const uno::Reference< uno::XComponentContext> m_xContext;

    const uno::Reference< ucb::XUniversalContentBroker > m_xUniversalContentBroker;
    const uno::Reference< xml::dom::XDocumentBuilder > m_xDocumentBuilder;
    const uno::Reference< xml::xpath::XXPathAPI > m_xXPathAPI;

    uno::Sequence< beans::StringPair > m_aRequestHeaderList;

    uno::Reference< ucb::XCommandProcessor > m_xCommandProcessor;
    uno::Reference< task::XInteractionHandler > m_xInteractionHandler;
    uno::Reference< task::XInteractionHandler > m_xPwContainerInteractionHandler;

    osl::Mutex m_aMutex;
    osl::Condition m_bCancelled;

    sal_Int32 m_nCommandId;
};


class UpdateInformationEnumeration : public ::cppu::WeakImplHelper< container::XEnumeration >
{
public:
    UpdateInformationEnumeration(const uno::Reference< xml::dom::XNodeList >& xNodeList,
                                 rtl::Reference< UpdateInformationProvider > xUpdateInformationProvider) :
        m_xUpdateInformationProvider(std::move(xUpdateInformationProvider)),
        m_xNodeList(xNodeList),
        m_nNodes(xNodeList.is() ? xNodeList->getLength() : 0),
        m_nCount(0)
    {
    };

    // XEnumeration
    sal_Bool SAL_CALL hasMoreElements() override { return m_nCount < m_nNodes; };
    uno::Any SAL_CALL nextElement() override
    {
        OSL_ASSERT( m_xNodeList.is() );
        OSL_ASSERT( m_xUpdateInformationProvider.is() );

        if( m_nCount >= m_nNodes )
            throw container::NoSuchElementException(OUString::number(m_nCount), *this);

        try
        {
            deployment::UpdateInformationEntry aEntry;

            uno::Reference< xml::dom::XNode > xAtomEntryNode( m_xNodeList->item(m_nCount++) );

            uno::Reference< xml::dom::XNode > xSummaryNode(
                m_xUpdateInformationProvider->getChildNode( xAtomEntryNode, u"summary/text()" )
            );

            if( xSummaryNode.is() )
                aEntry.Description = xSummaryNode->getNodeValue();

            uno::Reference< xml::dom::XNode > xContentNode(
                m_xUpdateInformationProvider->getChildNode( xAtomEntryNode, u"content" ) );

            if( xContentNode.is() )
                aEntry.UpdateDocument = m_xUpdateInformationProvider->getDocumentRoot(xContentNode);

            return uno::Any(aEntry);
        }
        catch( ucb::CommandAbortedException const &)
        {
            // action has been aborted
            css::uno::Any anyEx = cppu::getCaughtException();
            throw lang::WrappedTargetException( u"Command aborted"_ustr, *this, anyEx );
        }
        catch( uno::RuntimeException const & )
        {
            // let runtime exception pass
            throw;
        }
        catch( uno::Exception const &)
        {
            // document not accessible
            css::uno::Any anyEx = cppu::getCaughtException();
            throw lang::WrappedTargetException( u"Document not accessible"_ustr, *this, anyEx );
        }
    }

private:
    const rtl::Reference< UpdateInformationProvider > m_xUpdateInformationProvider;
    const uno::Reference< xml::dom::XNodeList > m_xNodeList;
    const sal_Int32 m_nNodes;
    sal_Int32 m_nCount;
};


class SingleUpdateInformationEnumeration : public ::cppu::WeakImplHelper< container::XEnumeration >
{
public:
    explicit SingleUpdateInformationEnumeration(const uno::Reference< xml::dom::XElement >& xElement)
        : m_nCount(0) { m_aEntry.UpdateDocument = xElement; };

    // XEnumeration
    sal_Bool SAL_CALL hasMoreElements() override { return 0 == m_nCount; };
    uno::Any SAL_CALL nextElement() override
    {
        if( m_nCount > 0 )
            throw container::NoSuchElementException(OUString::number(m_nCount), *this);

        ++m_nCount;
        return uno::Any(m_aEntry);
    };

private:
    sal_Int32 m_nCount;
    deployment::UpdateInformationEntry m_aEntry;
};

UpdateInformationProvider::UpdateInformationProvider(
    uno::Reference<uno::XComponentContext> xContext,
    uno::Reference< ucb::XUniversalContentBroker > xUniversalContentBroker,
    uno::Reference< xml::dom::XDocumentBuilder > xDocumentBuilder,
    uno::Reference< xml::xpath::XXPathAPI > xXPathAPI)
    : m_xContext(std::move(xContext))
    , m_xUniversalContentBroker(std::move(xUniversalContentBroker))
    , m_xDocumentBuilder(std::move(xDocumentBuilder))
    , m_xXPathAPI(std::move(xXPathAPI))
    , m_aRequestHeaderList(2)
    , m_nCommandId(0)
{
    uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider(
        css::configuration::theDefaultProvider::get(m_xContext));

    auto pRequestHeaderList = m_aRequestHeaderList.getArray();
    pRequestHeaderList[0].First = "Accept-Language";
    pRequestHeaderList[0].Second = getConfigurationItem( xConfigurationProvider, u"org.openoffice.Setup/L10N"_ustr, u"ooLocale"_ustr );
}

bool
UpdateInformationProvider::isUserAgentExtended() const
{
    bool bExtendedUserAgent = false;
    try {
        uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider(
            css::configuration::theDefaultProvider::get(m_xContext));

        uno::Any aExtended = getConfigurationItemAny(
            xConfigurationProvider,
            u"org.openoffice.Office.Jobs/Jobs/UpdateCheck/Arguments"_ustr,
            u"ExtendedUserAgent"_ustr);
        aExtended >>= bExtendedUserAgent;
    } catch (const uno::RuntimeException &) {
        SAL_WARN("extensions.update", "Online update disabled");
    }
    return bExtendedUserAgent;
}

OUString UpdateInformationProvider::getUserAgent(bool bExtended)
{
    uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider(
        css::configuration::theDefaultProvider::get(m_xContext));

    OUStringBuffer buf;
    buf.append(
        getConfigurationItem(
            xConfigurationProvider,
            u"org.openoffice.Setup/Product"_ustr,
            u"ooName"_ustr));
    buf.append(' ');
    buf.append(
        getConfigurationItem(
            xConfigurationProvider,
            u"org.openoffice.Setup/Product"_ustr,
            u"ooSetupVersion"_ustr));

    OUString extension(
        getConfigurationItem(
            xConfigurationProvider,
            u"org.openoffice.Setup/Product"_ustr,
            u"ooSetupExtension"_ustr));
    if (!extension.isEmpty())
        buf.append(extension);

    OUString product(buf.makeStringAndClear());

    OUString aUserAgent( u"${$BRAND_BASE_DIR/" LIBO_ETC_FOLDER "/" SAL_CONFIGFILE("version") ":UpdateUserAgent}"_ustr );
    OUString aExtended;
    if( bExtended )
    {
        aExtended = Application::GetHWOSConfInfo();
    }
    rtl::Bootstrap::expandMacros( aUserAgent );
    aUserAgent = aUserAgent.replaceAll("<PRODUCT>", product);
    aUserAgent = aUserAgent.replaceAll("<OPTIONAL_OS_HW_DATA>", aExtended);
    SAL_INFO("extensions.update", "UpdateUserAgent: " << aUserAgent);
    // if you want to debug online updates from a dev version, then uncommenting this (adjust for platform)
    // might be helpful
    // return "LibreOffice 7.3.5.2 (184fe81b8c8c30d8b5082578aee2fed2ea847c01; Linux; X86_64; )";
    return aUserAgent;
}

uno::Sequence< beans::StringPair > SAL_CALL UpdateInformationProvider::getUserRequestHeaders(
    const OUString &aURL, ucb::WebDAVHTTPMethod )
{
    bool bExtendedUserAgent;
    uno::Sequence< beans::StringPair > aPair = m_aRequestHeaderList;

    // Internal use from cui/ some magic URLs
    if( aURL.startsWith( "useragent:" ) )
        bExtendedUserAgent = (aURL == "useragent:extended");
    else
        bExtendedUserAgent = isUserAgentExtended();

    OUString aUserAgent = getUserAgent(bExtendedUserAgent);

    if( aUserAgent.isEmpty() )
        aPair.realloc(1);
    else
    {
        auto pPair = aPair.getArray();
        pPair[1].First = "User-Agent";
        pPair[1].Second = aUserAgent;
    }

    return aPair;
};

UpdateInformationProvider::~UpdateInformationProvider()
{
}

uno::Any
UpdateInformationProvider::getConfigurationItemAny(uno::Reference<lang::XMultiServiceFactory> const & configurationProvider, OUString const & node, OUString const & item)
{
    beans::PropertyValue aProperty;
    aProperty.Name  = "nodepath";
    aProperty.Value <<= node;

    uno::Sequence< uno::Any > aArgumentList{ uno::Any(aProperty) };
    uno::Reference< container::XNameAccess > xNameAccess(
        configurationProvider->createInstanceWithArguments(
            u"com.sun.star.configuration.ConfigurationAccess"_ustr,
            aArgumentList ),
        uno::UNO_QUERY_THROW);

    return xNameAccess->getByName(item);
}

OUString
UpdateInformationProvider::getConfigurationItem(uno::Reference<lang::XMultiServiceFactory> const & configurationProvider, OUString const & node, OUString const & item)
{
    OUString sRet;
    getConfigurationItemAny(configurationProvider, node, item) >>= sRet;
    return sRet;
}

void
UpdateInformationProvider::storeCommandInfo(
    sal_Int32 nCommandId,
    uno::Reference< ucb::XCommandProcessor > const & rxCommandProcessor)
{
    osl::MutexGuard aGuard(m_aMutex);

    m_nCommandId = nCommandId;
    m_xCommandProcessor = rxCommandProcessor;
}

uno::Reference< io::XInputStream >
UpdateInformationProvider::load(const OUString& rURL)
{
    uno::Reference< ucb::XContentIdentifier > xId = m_xUniversalContentBroker->createContentIdentifier(rURL);

    if( !xId.is() )
        throw uno::RuntimeException(
            u"unable to obtain universal content id"_ustr, *this);

    uno::Reference< ucb::XCommandProcessor > xCommandProcessor(m_xUniversalContentBroker->queryContent(xId), uno::UNO_QUERY_THROW);
    rtl::Reference< ActiveDataSink > aSink(new ActiveDataSink());

    ucb::OpenCommandArgument3 aOpenArgument;
    aOpenArgument.Mode = ucb::OpenMode::DOCUMENT;
    aOpenArgument.Priority = 32768;
    aOpenArgument.Sink = *aSink;
    // Disable KeepAlive in webdav - don't want millions of office
    // instances phone home & clog up servers
    aOpenArgument.OpeningFlags = { { u"KeepAlive"_ustr, uno::Any(false) } };

    ucb::Command aCommand;
    aCommand.Name = "open";
    aCommand.Argument <<= aOpenArgument;

    sal_Int32 nCommandId = xCommandProcessor->createCommandIdentifier();

    storeCommandInfo(nCommandId, xCommandProcessor);
    try
    {
        xCommandProcessor->execute(aCommand, nCommandId,
            static_cast < XCommandEnvironment *> (this));
    }
    catch( const uno::Exception & /* e */ )
    {
        storeCommandInfo(0, uno::Reference< ucb::XCommandProcessor > ());

        uno::Reference< ucb::XCommandProcessor2 > xCommandProcessor2(xCommandProcessor, uno::UNO_QUERY);
        if( xCommandProcessor2.is() )
            xCommandProcessor2->releaseCommandIdentifier(nCommandId);

        throw;
    }
    storeCommandInfo(0, uno::Reference< ucb::XCommandProcessor > ());

    uno::Reference< ucb::XCommandProcessor2 > xCommandProcessor2(xCommandProcessor, uno::UNO_QUERY);
    if( xCommandProcessor2.is() )
        xCommandProcessor2->releaseCommandIdentifier(nCommandId);

    return INPUT_STREAM(aSink->getInputStream());
}


// TODO: docu content node

uno::Reference< xml::dom::XElement >
UpdateInformationProvider::getDocumentRoot(const uno::Reference< xml::dom::XNode >& rxNode)
{
    OSL_ASSERT(m_xDocumentBuilder.is());

    uno::Reference< xml::dom::XElement > xElement(rxNode, uno::UNO_QUERY_THROW);

    // load the document referenced in 'src' attribute ..
    if( xElement->hasAttribute( u"src"_ustr ) )
    {
        uno::Reference< xml::dom::XDocument > xUpdateXML =
            m_xDocumentBuilder->parse(load(xElement->getAttribute( u"src"_ustr )));

        OSL_ASSERT( xUpdateXML.is() );

        if( xUpdateXML.is() )
            return xUpdateXML->getDocumentElement();
    }
    // .. or return the (single) child element
    else
    {
        uno::Reference< xml::dom::XNodeList> xChildNodes = rxNode->getChildNodes();

        // ignore possible #text nodes
        sal_Int32 nmax = xChildNodes->getLength();
        for(sal_Int32 n=0; n < nmax; n++)
        {
            uno::Reference< xml::dom::XElement > xChildElement(xChildNodes->item(n), uno::UNO_QUERY);
            if( xChildElement.is() )
            {
                /* Copy the content to a dedicated document since XXPathAPI->selectNodeList
                 * seems to evaluate expression always relative to the root node.
                 */
                uno::Reference< xml::dom::XDocument > xUpdateXML = m_xDocumentBuilder->newDocument();
                xUpdateXML->appendChild( xUpdateXML->importNode(xChildElement, true ) );
                return xUpdateXML->getDocumentElement();
            }
        }
    }

    return uno::Reference< xml::dom::XElement > ();
}


uno::Reference< xml::dom::XNode >
UpdateInformationProvider::getChildNode(const uno::Reference< xml::dom::XNode >& rxNode,
                                        std::u16string_view rName)
{
    OSL_ASSERT(m_xXPathAPI.is());
    try {
        return m_xXPathAPI->selectSingleNode(rxNode, OUString::Concat("./atom:") + rName);
    } catch (const xml::xpath::XPathException &) {
        // ignore
        return nullptr;
    }
}


uno::Reference< container::XEnumeration > SAL_CALL
UpdateInformationProvider::getUpdateInformationEnumeration(
    uno::Sequence< OUString > const & repositories,
    OUString const & extensionId
)
{
    OSL_ASSERT(m_xDocumentBuilder.is());

    // reset cancelled flag
    m_bCancelled.reset();

    for(sal_Int32 n=0; n<repositories.getLength(); n++)
    {
        try
        {
            uno::Reference< xml::dom::XDocument > xDocument = m_xDocumentBuilder->parse(load(repositories[n]));
            uno::Reference< xml::dom::XElement > xElement;

            if( xDocument.is() )
                xElement = xDocument->getDocumentElement();

            if( xElement.is() )
            {
                if( xElement->getNodeName() == "feed" )
                {
                    OUString aXPathExpression;

                    if( !extensionId.isEmpty() )
                        aXPathExpression = "//atom:entry/atom:category[@term=\'" + extensionId + "\']/..";
                    else
                        aXPathExpression = "//atom:entry";

                    uno::Reference< xml::dom::XNodeList > xNodeList;
                    try {
                        xNodeList = m_xXPathAPI->selectNodeList(xDocument,
                            aXPathExpression);
                    } catch (const xml::xpath::XPathException &) {
                        // ignore
                    }

                    return new UpdateInformationEnumeration(xNodeList, this);
                }
                else
                {
                    return new SingleUpdateInformationEnumeration(xElement);
                }
            }

            if( m_bCancelled.check() )
                break;
        }
        catch( uno::RuntimeException const& /*e*/)
        {
            // #i118675# ignore runtime exceptions for now
            // especially the "unsatisfied query for interface of
            // type com.sun.star.ucb.XCommandProcessor!" exception
        }

        // rethrow only if last url in the list
        catch( uno::Exception const & )
        {
            if( n+1 >= repositories.getLength() )
                throw;
        }
    }

    return uno::Reference< container::XEnumeration >();
}


uno::Sequence< uno::Reference< xml::dom::XElement > > SAL_CALL
UpdateInformationProvider::getUpdateInformation(
    uno::Sequence< OUString > const & repositories,
    OUString const & extensionId
)
{
    uno::Reference< container::XEnumeration > xEnumeration(
        getUpdateInformationEnumeration(repositories, extensionId)
    );

    std::vector< uno::Reference< xml::dom::XElement > > aRet;

    if( xEnumeration.is() )
    {
        while( xEnumeration->hasMoreElements() )
        {
            try
            {
                deployment::UpdateInformationEntry aEntry;
                if( (xEnumeration->nextElement() >>= aEntry ) && aEntry.UpdateDocument.is() )
                {
                    aRet.push_back(aEntry.UpdateDocument);
                }
            }

            catch( const lang::WrappedTargetException& e )
            {
                // command aborted, return what we have got so far
                if( e.TargetException.isExtractableTo( ::cppu::UnoType< css::ucb::CommandAbortedException >::get() ) )
                {
                    break;
                }

                // ignore files that can't be loaded
            }
        }
    }

    return comphelper::containerToSequence(aRet);
}


void SAL_CALL
UpdateInformationProvider::cancel()
{
    m_bCancelled.set();

    osl::MutexGuard aGuard(m_aMutex);
    if( m_xCommandProcessor.is() )
        m_xCommandProcessor->abort(m_nCommandId);
}


void SAL_CALL
UpdateInformationProvider::setInteractionHandler(
        uno::Reference< task::XInteractionHandler > const & handler )
{
    osl::MutexGuard aGuard(m_aMutex);
    m_xInteractionHandler = handler;
}


uno::Reference< task::XInteractionHandler > SAL_CALL
UpdateInformationProvider::getInteractionHandler()
{
    osl::MutexGuard aGuard( m_aMutex );

    if ( m_xInteractionHandler.is() )
        return m_xInteractionHandler;
    else
    {
        try
        {
            // Supply an interaction handler that uses the password container
            // service to obtain credentials without displaying a password gui.

            if ( !m_xPwContainerInteractionHandler.is() )
                m_xPwContainerInteractionHandler
                    = task::PasswordContainerInteractionHandler::create(
                        m_xContext );
        }
        catch ( uno::RuntimeException const & )
        {
            throw;
        }
        catch ( uno::Exception const & )
        {
        }
        return m_xPwContainerInteractionHandler;
    }
}



OUString SAL_CALL
UpdateInformationProvider::getImplementationName()
{
    return u"vnd.sun.UpdateInformationProvider"_ustr;
}


uno::Sequence< OUString > SAL_CALL
UpdateInformationProvider::getSupportedServiceNames()
{
    return { u"com.sun.star.deployment.UpdateInformationProvider"_ustr };
}

sal_Bool SAL_CALL
UpdateInformationProvider::supportsService( OUString const & serviceName )
{
    return cppu::supportsService(this, serviceName);
}

} // anonymous namespace

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
extensions_update_UpdateInformationProvider_get_implementation(
    css::uno::XComponentContext* xContext , css::uno::Sequence<css::uno::Any> const&)
{
    uno::Reference< ucb::XUniversalContentBroker > xUniversalContentBroker =
        ucb::UniversalContentBroker::create(xContext);

    uno::Reference< xml::dom::XDocumentBuilder > xDocumentBuilder(
        xml::dom::DocumentBuilder::create(xContext));

    uno::Reference< xml::xpath::XXPathAPI > xXPath = xml::xpath::XPathAPI::create( xContext );

    xXPath->registerNS( u"atom"_ustr, u"http://www.w3.org/2005/Atom"_ustr );

    return cppu::acquire(
        new UpdateInformationProvider(xContext, xUniversalContentBroker, xDocumentBuilder, xXPath));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

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

#include <dp_descriptioninfoset.hxx>

#include <dp_resource.h>

#include <comphelper/sequence.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <optional>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/deployment/DeploymentException.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/io/SequenceInputStream.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/XProgressHandler.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/xml/dom/DOMException.hpp>
#include <com/sun/star/xml/dom/XNode.hpp>
#include <com/sun/star/xml/dom/XNodeList.hpp>
#include <com/sun/star/xml/dom/DocumentBuilder.hpp>
#include <com/sun/star/xml/xpath/XPathAPI.hpp>
#include <com/sun/star/xml/xpath/XPathException.hpp>
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <ucbhelper/content.hxx>
#include <o3tl/string_view.hxx>

namespace {

using css::uno::Reference;

class EmptyNodeList:
    public cppu::WeakImplHelper<css::xml::dom::XNodeList>
{
public:
    EmptyNodeList();

    EmptyNodeList(const EmptyNodeList&) = delete;
    const EmptyNodeList& operator=(const EmptyNodeList&) = delete;

    virtual ::sal_Int32 SAL_CALL getLength() override;

    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL
    item(::sal_Int32 index) override;
};

EmptyNodeList::EmptyNodeList() {}

::sal_Int32 EmptyNodeList::getLength() {
    return 0;
}

css::uno::Reference< css::xml::dom::XNode > EmptyNodeList::item(::sal_Int32)
{
    throw css::uno::RuntimeException(u"bad EmptyNodeList com.sun.star.xml.dom.XNodeList.item call"_ustr,
        static_cast< ::cppu::OWeakObject * >(this));
}

OUString getNodeValue(
    css::uno::Reference< css::xml::dom::XNode > const & node)
{
    OSL_ASSERT(node.is());
    try {
        return node->getNodeValue();
    } catch (const css::xml::dom::DOMException & e) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw css::lang::WrappedTargetRuntimeException(
            "com.sun.star.xml.dom.DOMException: " + e.Message,
            nullptr, anyEx );
    }
}

/**The class uses the UCB to access the description.xml file in an
   extension. The UCB must have been initialized already. It also
   requires that the extension has already be unzipped to a particular
   location.
 */
class ExtensionDescription
{
public:
    /**throws an exception if the description.xml is not
        available, cannot be read, does not contain the expected data,
        or any other error occurred. Therefore it should only be used with
        new extensions.

        Throws css::uno::RuntimeException,
        css::deployment::DeploymentException,
        dp_registry::backend::bundle::NoDescriptionException.
     */
    ExtensionDescription(
        const css::uno::Reference<css::uno::XComponentContext>& xContext,
        std::u16string_view installDir,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xCmdEnv);

    const css::uno::Reference<css::xml::dom::XNode>& getRootElement() const
    {
        return m_xRoot;
    }

private:
    css::uno::Reference<css::xml::dom::XNode> m_xRoot;
};

class NoDescriptionException
{
};

class FileDoesNotExistFilter
    : public ::cppu::WeakImplHelper< css::ucb::XCommandEnvironment,
                                      css::task::XInteractionHandler >

{
    bool m_bExist;
    css::uno::Reference< css::ucb::XCommandEnvironment > m_xCommandEnv;

public:
    explicit FileDoesNotExistFilter(
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xCmdEnv);

    bool exist() { return m_bExist;}
    // XCommandEnvironment
    virtual css::uno::Reference<css::task::XInteractionHandler > SAL_CALL
    getInteractionHandler() override;
    virtual css::uno::Reference<css::ucb::XProgressHandler >
    SAL_CALL getProgressHandler() override;

    // XInteractionHandler
    virtual void SAL_CALL handle(
        css::uno::Reference<css::task::XInteractionRequest > const & xRequest ) override;
};

ExtensionDescription::ExtensionDescription(
    const Reference<css::uno::XComponentContext>& xContext,
    std::u16string_view installDir,
    const Reference< css::ucb::XCommandEnvironment >& xCmdEnv)
{
    try {
        //may throw css::ucb::ContentCreationException
        //If there is no description.xml then ucb will start an interaction which
        //brings up a dialog.We want to prevent this. Therefore we wrap the xCmdEnv
        //and filter the respective exception out.
        OUString sDescriptionUri(OUString::Concat(installDir) + "/description.xml");
        Reference<css::ucb::XCommandEnvironment> xFilter = new FileDoesNotExistFilter(xCmdEnv);
        ::ucbhelper::Content descContent(sDescriptionUri, xFilter, xContext);

        //throws a css::uno::Exception if the file is not available
        Reference<css::io::XInputStream> xIn;
        try
        {   //throws com.sun.star.ucb.InteractiveIOException
            xIn = descContent.openStream();
        }
        catch ( const css::uno::Exception& )
        {
            if ( ! static_cast<FileDoesNotExistFilter*>(xFilter.get())->exist())
                throw NoDescriptionException();
            throw;
        }
        if (!xIn.is())
        {
            throw css::uno::Exception(
                "Could not get XInputStream for description.xml of extension " +
                sDescriptionUri, nullptr);
        }

        //get root node of description.xml
        Reference<css::xml::dom::XDocumentBuilder> xDocBuilder(
            css::xml::dom::DocumentBuilder::create(xContext) );

        if (!xDocBuilder->isNamespaceAware())
        {
            throw css::uno::Exception(
                u"Service com.sun.star.xml.dom.DocumentBuilder is not namespace aware."_ustr, nullptr);
        }

        Reference<css::xml::dom::XDocument> xDoc = xDocBuilder->parse(xIn);
        if (!xDoc.is())
        {
            throw css::uno::Exception(sDescriptionUri + " contains data which cannot be parsed. ", nullptr);
        }

        //check for proper root element and namespace
        Reference<css::xml::dom::XElement> xRoot = xDoc->getDocumentElement();
        if (!xRoot.is())
        {
            throw css::uno::Exception(
                sDescriptionUri + " contains no root element.", nullptr);
        }

        if ( xRoot->getTagName() != "description")
        {
            throw css::uno::Exception(
                sDescriptionUri + " does not contain the root element <description>.", nullptr);
        }

        m_xRoot.set(xRoot, css::uno::UNO_QUERY_THROW);
        OUString nsDescription = xRoot->getNamespaceURI();

        //check if this namespace is supported
        if ( nsDescription != "http://openoffice.org/extensions/description/2006")
        {
            throw css::uno::Exception(sDescriptionUri + " contains a root element with an unsupported namespace. ", nullptr);
        }
    } catch (const css::uno::RuntimeException &) {
        throw;
    } catch (const css::deployment::DeploymentException &) {
        throw;
    } catch (const css::uno::Exception & e) {
        css::uno::Any a(cppu::getCaughtException());
        throw css::deployment::DeploymentException(
            e.Message, Reference< css::uno::XInterface >(), a);
    }
}

FileDoesNotExistFilter::FileDoesNotExistFilter(
    const Reference< css::ucb::XCommandEnvironment >& xCmdEnv):
    m_bExist(true), m_xCommandEnv(xCmdEnv)
{}

    // XCommandEnvironment
Reference<css::task::XInteractionHandler >
    FileDoesNotExistFilter::getInteractionHandler()
{
    return static_cast<css::task::XInteractionHandler*>(this);
}

Reference<css::ucb::XProgressHandler >
    FileDoesNotExistFilter::getProgressHandler()
{
    return m_xCommandEnv.is()
        ? m_xCommandEnv->getProgressHandler()
        : Reference<css::ucb::XProgressHandler>();
}

// XInteractionHandler
//If the interaction was caused by a non-existing file which is specified in the ctor
//of FileDoesNotExistFilter, then we do nothing
void  FileDoesNotExistFilter::handle(
        Reference<css::task::XInteractionRequest > const & xRequest )
{
    css::uno::Any request( xRequest->getRequest() );

    css::ucb::InteractiveIOException ioexc;
    if ((request>>= ioexc)
        && (ioexc.Code == css::ucb::IOErrorCode_NOT_EXISTING
            || ioexc.Code == css::ucb::IOErrorCode_NOT_EXISTING_PATH))
    {
        m_bExist = false;
        return;
    }
    Reference<css::task::XInteractionHandler> xInteraction;
    if (m_xCommandEnv.is()) {
        xInteraction = m_xCommandEnv->getInteractionHandler();
    }
    if (xInteraction.is()) {
        xInteraction->handle(xRequest);
    }
}

}

namespace dp_misc {

DescriptionInfoset getDescriptionInfoset(std::u16string_view sExtensionFolderURL)
{
    Reference< css::xml::dom::XNode > root;
    const Reference<css::uno::XComponentContext>& context(
        comphelper::getProcessComponentContext());
    try {
        root =
            ExtensionDescription(
                context, sExtensionFolderURL,
                Reference< css::ucb::XCommandEnvironment >()).
            getRootElement();
    } catch (const NoDescriptionException &) {
    } catch (const css::deployment::DeploymentException & e) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw css::lang::WrappedTargetRuntimeException(
            "com.sun.star.deployment.DeploymentException: " + e.Message,
            nullptr, anyEx );
    }
    return DescriptionInfoset(context, root);
}

DescriptionInfoset::DescriptionInfoset(
    css::uno::Reference< css::uno::XComponentContext > const & context,
    css::uno::Reference< css::xml::dom::XNode > const & element):
    m_context(context),
    m_element(element)
{
    if (m_element.is()) {
        m_xpath = css::xml::xpath::XPathAPI::create(context);
        m_xpath->registerNS(u"desc"_ustr, element->getNamespaceURI());
        m_xpath->registerNS(u"xlink"_ustr, u"http://www.w3.org/1999/xlink"_ustr);
    }
}

DescriptionInfoset::~DescriptionInfoset() {}

::std::optional< OUString > DescriptionInfoset::getIdentifier() const {
    return getOptionalValue(u"desc:identifier/@value"_ustr);
}

OUString DescriptionInfoset::getNodeValueFromExpression(OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, expression);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return n.is() ? getNodeValue(n) : OUString();
}

void DescriptionInfoset::checkDenylist() const
{
    if (!m_element.is())
        return;

    std::optional< OUString > id(getIdentifier());
    if (!id)
        return; // nothing to check
    OUString currentversion(getVersion());
    if (currentversion.getLength() == 0)
        return;  // nothing to check

    css::uno::Sequence<css::uno::Any> args(comphelper::InitAnyPropertySequence(
    {
        {"nodepath", css::uno::Any(u"/org.openoffice.Office.ExtensionDependencies/Extensions"_ustr)}
    }));
    css::uno::Reference< css::container::XNameAccess > denylist(
        (css::configuration::theDefaultProvider::get(m_context)
         ->createInstanceWithArguments(
             u"com.sun.star.configuration.ConfigurationAccess"_ustr, args)),
        css::uno::UNO_QUERY_THROW);

    // check first if a denylist entry is available
    if (!(denylist.is() && denylist->hasByName(*id)))        return;

    css::uno::Reference< css::beans::XPropertySet > extProps(
        denylist->getByName(*id), css::uno::UNO_QUERY_THROW);

    css::uno::Any anyValue = extProps->getPropertyValue(u"Versions"_ustr);

    css::uno::Sequence< OUString > blversions;
    anyValue >>= blversions;

    // check if the current version requires further dependency checks from the denylist
    if (!checkDenylistVersion(currentversion, blversions))        return;

    anyValue = extProps->getPropertyValue(u"Dependencies"_ustr);
    OUString udeps;
    anyValue >>= udeps;

    if (udeps.getLength() == 0)
        return; // nothing todo

    OString xmlDependencies = OUStringToOString(udeps, RTL_TEXTENCODING_UNICODE);

    css::uno::Reference< css::xml::dom::XDocumentBuilder> docbuilder(
        m_context->getServiceManager()->createInstanceWithContext(u"com.sun.star.xml.dom.DocumentBuilder"_ustr, m_context),
        css::uno::UNO_QUERY_THROW);

    css::uno::Sequence< sal_Int8 > byteSeq(reinterpret_cast<const sal_Int8*>(xmlDependencies.getStr()), xmlDependencies.getLength());

    css::uno::Reference< css::io::XInputStream> inputstream( css::io::SequenceInputStream::createStreamFromSequence(m_context, byteSeq),
                                                             css::uno::UNO_QUERY_THROW);

    css::uno::Reference< css::xml::dom::XDocument > xDocument(docbuilder->parse(inputstream));
    css::uno::Reference< css::xml::dom::XElement > xElement(xDocument->getDocumentElement());
    css::uno::Reference< css::xml::dom::XNodeList > xDeps(xElement->getChildNodes());
    sal_Int32 nLen = xDeps->getLength();

    // get the parent xml document  of current description info for the import
    css::uno::Reference< css::xml::dom::XDocument > xCurrentDescInfo(m_element->getOwnerDocument());

    // get dependency node of current description info to merge the new dependencies from the denylist
    css::uno::Reference< css::xml::dom::XNode > xCurrentDeps(
        m_xpath->selectSingleNode(m_element, u"desc:dependencies"_ustr));

    // if no dependency node exists, create a new one in the current description info
    if (!xCurrentDeps.is()) {
        css::uno::Reference< css::xml::dom::XNode > xNewDepNode(
            xCurrentDescInfo->createElementNS(
                u"http://openoffice.org/extensions/description/2006"_ustr,
                u"dependencies"_ustr), css::uno::UNO_QUERY_THROW);
        m_element->appendChild(xNewDepNode);
        xCurrentDeps = m_xpath->selectSingleNode(m_element, u"desc:dependencies"_ustr);
    }

    for (sal_Int32 i=0; i<nLen; i++) {
        css::uno::Reference< css::xml::dom::XNode > xNode(xDeps->item(i));
        css::uno::Reference< css::xml::dom::XElement > xDep(xNode, css::uno::UNO_QUERY);
        if (xDep.is()) {
            // found valid denylist dependency, import the node first and append it to the existing dependency node
            css::uno::Reference< css::xml::dom::XNode > importedNode = xCurrentDescInfo->importNode(xNode, true);
            xCurrentDeps->appendChild(importedNode);
        }
    }
}

bool DescriptionInfoset::checkDenylistVersion(
    std::u16string_view currentversion,
    css::uno::Sequence< OUString > const & versions)
{
    sal_Int32 nLen = versions.getLength();
    for (sal_Int32 i=0; i<nLen; i++) {
        if (currentversion == versions[i])
            return true;
    }

    return false;
}

OUString DescriptionInfoset::getVersion() const
{
    return getNodeValueFromExpression( u"desc:version/@value"_ustr );
}

css::uno::Sequence< OUString > DescriptionInfoset::getSupportedPlatforms() const
{
    //When there is no description.xml then we assume that we support all platforms
    if (! m_element.is())
    {
        return { u"all"_ustr };
    }

    //Check if the <platform> element was provided. If not the default is "all" platforms
    css::uno::Reference< css::xml::dom::XNode > nodePlatform(
        m_xpath->selectSingleNode(m_element, u"desc:platform"_ustr));
    if (!nodePlatform.is())
    {
        return { u"all"_ustr };
    }

    //There is a platform element.
    const OUString value = getNodeValueFromExpression(u"desc:platform/@value"_ustr);
    //parse the string, it can contained multiple strings separated by commas
    std::vector< OUString> vec;
    sal_Int32 nIndex = 0;
    do
    {
        const OUString aToken( o3tl::trim(o3tl::getToken(value, 0, ',', nIndex )) );
        if (!aToken.isEmpty())
            vec.push_back(aToken);

    }
    while (nIndex >= 0);

    return comphelper::containerToSequence(vec);
}

css::uno::Reference< css::xml::dom::XNodeList >
DescriptionInfoset::getDependencies() const {
    if (m_element.is()) {
        try {
            // check the extension denylist first and expand the dependencies if applicable
            checkDenylist();

            return m_xpath->selectNodeList(m_element, u"desc:dependencies/*"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return new EmptyNodeList;
}

css::uno::Sequence< OUString >
DescriptionInfoset::getUpdateInformationUrls() const {
    return getUrls(u"desc:update-information/desc:src/@xlink:href"_ustr);
}

css::uno::Sequence< OUString >
DescriptionInfoset::getUpdateDownloadUrls() const
{
    return getUrls(u"desc:update-download/desc:src/@xlink:href"_ustr);
}

OUString DescriptionInfoset::getIconURL( bool bHighContrast ) const
{
    css::uno::Sequence< OUString > aStrList = getUrls( u"desc:icon/desc:default/@xlink:href"_ustr );
    css::uno::Sequence< OUString > aStrListHC = getUrls( u"desc:icon/desc:high-contrast/@xlink:href"_ustr );

    if ( bHighContrast && aStrListHC.hasElements() && !aStrListHC[0].isEmpty() )
        return aStrListHC[0];

    if ( aStrList.hasElements() && !aStrList[0].isEmpty() )
        return aStrList[0];

    return OUString();
}

::std::optional< OUString > DescriptionInfoset::getLocalizedUpdateWebsiteURL()
    const
{
    bool bParentExists = false;
    const OUString sURL (getLocalizedHREFAttrFromChild(u"/desc:description/desc:update-website"_ustr, &bParentExists ));

    if (!sURL.isEmpty())
        return ::std::optional< OUString >(sURL);
    else
        return bParentExists ? ::std::optional< OUString >(OUString()) :
            ::std::optional< OUString >();
}

::std::optional< OUString > DescriptionInfoset::getOptionalValue(
    OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, expression);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return n.is()
        ? ::std::optional< OUString >(getNodeValue(n))
        : ::std::optional< OUString >();
}

css::uno::Sequence< OUString > DescriptionInfoset::getUrls(
    OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNodeList > ns;
    if (m_element.is()) {
        try {
            ns = m_xpath->selectNodeList(m_element, expression);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    css::uno::Sequence< OUString > urls(ns.is() ? ns->getLength() : 0);
    auto urlsRange = asNonConstRange(urls);
    for (::sal_Int32 i = 0; i < urls.getLength(); ++i) {
        urlsRange[i] = getNodeValue(ns->item(i));
    }
    return urls;
}

std::pair< OUString, OUString > DescriptionInfoset::getLocalizedPublisherNameAndURL() const
{
    css::uno::Reference< css::xml::dom::XNode > node =
        getLocalizedChild(u"desc:publisher"_ustr);

    OUString sPublisherName;
    OUString sURL;
    if (node.is())
    {
        css::uno::Reference< css::xml::dom::XNode > xPathName;
        try {
            xPathName = m_xpath->selectSingleNode(node, u"text()"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xPathName.is());
        if (xPathName.is())
            sPublisherName = xPathName->getNodeValue();

        css::uno::Reference< css::xml::dom::XNode > xURL;
        try {
            xURL = m_xpath->selectSingleNode(node, u"@xlink:href"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xURL.is());
        if (xURL.is())
           sURL = xURL->getNodeValue();
    }
    return std::make_pair(sPublisherName, sURL);
}

OUString DescriptionInfoset::getLocalizedReleaseNotesURL() const
{
    return getLocalizedHREFAttrFromChild(u"/desc:description/desc:release-notes"_ustr, nullptr);
}

OUString DescriptionInfoset::getLocalizedDisplayName() const
{
    css::uno::Reference< css::xml::dom::XNode > node =
        getLocalizedChild(u"desc:display-name"_ustr);
    if (node.is())
    {
        css::uno::Reference< css::xml::dom::XNode > xtext;
        try {
            xtext = m_xpath->selectSingleNode(node, u"text()"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        if (xtext.is())
            return xtext->getNodeValue();
    }
    return OUString();
}

OUString DescriptionInfoset::getLocalizedLicenseURL() const
{
    return getLocalizedHREFAttrFromChild(u"/desc:description/desc:registration/desc:simple-license"_ustr, nullptr);

}

::std::optional<SimpleLicenseAttributes>
DescriptionInfoset::getSimpleLicenseAttributes() const
{
    //Check if the node exist
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, u"/desc:description/desc:registration/desc:simple-license/@accept-by"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        if (n.is())
        {
            SimpleLicenseAttributes attributes;
            attributes.acceptBy =
                getNodeValueFromExpression(u"/desc:description/desc:registration/desc:simple-license/@accept-by"_ustr);

            ::std::optional< OUString > suppressOnUpdate = getOptionalValue(u"/desc:description/desc:registration/desc:simple-license/@suppress-on-update"_ustr);
            if (suppressOnUpdate)
                attributes.suppressOnUpdate = o3tl::equalsIgnoreAsciiCase(o3tl::trim(*suppressOnUpdate), u"true");
            else
                attributes.suppressOnUpdate = false;

            ::std::optional< OUString > suppressIfRequired = getOptionalValue(u"/desc:description/desc:registration/desc:simple-license/@suppress-if-required"_ustr);
            if (suppressIfRequired)
                attributes.suppressIfRequired = o3tl::equalsIgnoreAsciiCase(o3tl::trim(*suppressIfRequired), u"true");
            else
                attributes.suppressIfRequired = false;

            return ::std::optional<SimpleLicenseAttributes>(attributes);
        }
    }
    return ::std::optional<SimpleLicenseAttributes>();
}

OUString DescriptionInfoset::getLocalizedDescriptionURL() const
{
    return getLocalizedHREFAttrFromChild(u"/desc:description/desc:extension-description"_ustr, nullptr);
}

css::uno::Reference< css::xml::dom::XNode >
DescriptionInfoset::getLocalizedChild( const OUString & sParent) const
{
    if ( ! m_element.is() || sParent.isEmpty())
        return css::uno::Reference< css::xml::dom::XNode > ();

    css::uno::Reference< css::xml::dom::XNode > xParent;
    try {
        xParent = m_xpath->selectSingleNode(m_element, sParent);
    } catch (const css::xml::xpath::XPathException &) {
        // ignore
    }
    css::uno::Reference<css::xml::dom::XNode> nodeMatch;
    if (xParent.is())
    {
        nodeMatch = matchLanguageTag(xParent, getOfficeLanguageTag().getBcp47());

        //office: en-DE, en, en-DE-altmark
        if (! nodeMatch.is())
        {
            // Already tried full tag, continue with first fallback.
            const std::vector< OUString > aFallbacks( getOfficeLanguageTag().getFallbackStrings( false));
            for (auto const& fallback : aFallbacks)
            {
                nodeMatch = matchLanguageTag(xParent, fallback);
                if (nodeMatch.is())
                    break;
            }
            if (! nodeMatch.is())
                nodeMatch = getChildWithDefaultLocale(xParent);
        }
    }

    return nodeMatch;
}

css::uno::Reference<css::xml::dom::XNode>
DescriptionInfoset::matchLanguageTag(
    css::uno::Reference< css::xml::dom::XNode > const & xParent, std::u16string_view rTag) const
{
    OSL_ASSERT(xParent.is());
    css::uno::Reference<css::xml::dom::XNode> nodeMatch;

    //first try exact match for lang
    const OUString exp1(OUString::Concat("*[@lang=\"") + rTag + "\"]");
    try {
        nodeMatch = m_xpath->selectSingleNode(xParent, exp1);
    } catch (const css::xml::xpath::XPathException &) {
        // ignore
    }

    //try to match in strings that also have a country and/or variant, for
    //example en  matches in en-US-montana, en-US, en-montana
    if (!nodeMatch.is())
    {
        const OUString exp2(
            OUString::Concat("*[starts-with(@lang,\"") + rTag + "-\")]");
        try {
            nodeMatch = m_xpath->selectSingleNode(xParent, exp2);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return nodeMatch;
}

css::uno::Reference<css::xml::dom::XNode>
DescriptionInfoset::getChildWithDefaultLocale(css::uno::Reference< css::xml::dom::XNode >
                                    const & xParent) const
{
    OSL_ASSERT(xParent.is());
    if ( xParent->getNodeName() == "simple-license" )
    {
        css::uno::Reference<css::xml::dom::XNode> nodeDefault;
        try {
            nodeDefault = m_xpath->selectSingleNode(xParent, u"@default-license-id"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        if (nodeDefault.is())
        {
            //The old way
            const OUString exp1("desc:license-text[@license-id = \""
                + nodeDefault->getNodeValue()
                + "\"]");
            try {
                return m_xpath->selectSingleNode(xParent, exp1);
            } catch (const css::xml::xpath::XPathException &) {
                // ignore
            }
        }
    }

    try {
        return m_xpath->selectSingleNode(xParent, u"*[1]"_ustr);
    } catch (const css::xml::xpath::XPathException &) {
        // ignore
        return nullptr;
    }
}

OUString DescriptionInfoset::getLocalizedHREFAttrFromChild(
    OUString const & sXPathParent, bool * out_bParentExists)
    const
{
    css::uno::Reference< css::xml::dom::XNode > node =
        getLocalizedChild(sXPathParent);

    OUString sURL;
    if (node.is())
    {
        if (out_bParentExists)
            *out_bParentExists = true;
        css::uno::Reference< css::xml::dom::XNode > xURL;
        try {
            xURL = m_xpath->selectSingleNode(node, u"@xlink:href"_ustr);
        } catch (const css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xURL.is());
        if (xURL.is())
            sURL = xURL->getNodeValue();
    }
    else
    {
        if (out_bParentExists)
            *out_bParentExists = false;
    }
    return sURL;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

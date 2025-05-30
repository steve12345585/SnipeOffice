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


#include <sfx2/DocumentMetadataAccess.hxx>

#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/frame/XTransientDocumentsDocumentContentIdentifierFactory.hpp>
#include <com/sun/star/task/ErrorCodeIOException.hpp>
#include <com/sun/star/ucb/InteractiveAugmentedIOException.hpp>
#include <com/sun/star/rdf/FileFormat.hpp>
#include <com/sun/star/rdf/ParseException.hpp>
#include <com/sun/star/rdf/RepositoryException.hpp>
#include <com/sun/star/rdf/URIs.hpp>
#include <com/sun/star/rdf/Statement.hpp>
#include <com/sun/star/rdf/URI.hpp>
#include <com/sun/star/rdf/Repository.hpp>

#include <rtl/ustrbuf.hxx>
#include <rtl/uri.hxx>
#include <rtl/bootstrap.hxx>
#include <sal/log.hxx>

#include <comphelper/interaction.hxx>
#include <unotools/mediadescriptor.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/storagehelper.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <o3tl/string_view.hxx>

#include <sfx2/docfile.hxx>
#include <sfx2/XmlIdRegistry.hxx>
#include <sfx2/objsh.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <libxml/tree.h>

#include <utility>
#include <vector>
#include <set>
#include <string_view>

#include <com/sun/star/uri/XUriReference.hpp>
#include <com/sun/star/uri/UriReferenceFactory.hpp>


/*
 Note: in the context of this implementation, all rdf.QueryExceptions and
 rdf.RepositoryExceptions are RuntimeExceptions, and will be reported as such.

 This implementation assumes that it is only used with ODF documents, not mere
 ODF packages. In other words, we enforce that metadata files must not be
 called reserved names.
 */

using namespace ::com::sun::star;

namespace sfx2 {


bool isValidNCName(std::u16string_view i_rIdref)
{
    const OString id(
        OUStringToOString(i_rIdref, RTL_TEXTENCODING_UTF8) );
    return !(xmlValidateNCName(
        reinterpret_cast<const unsigned char*>(id.getStr()), 0));
}


constexpr OUString s_content = u"content.xml"_ustr;
constexpr OUString s_styles = u"styles.xml"_ustr;
constexpr OUString s_manifest = u"manifest.rdf"_ustr;
const char s_odfmime [] = "application/vnd.oasis.opendocument.";


static bool isContentFile(std::u16string_view i_rPath)
{
    return i_rPath == s_content;
}

static bool isStylesFile (std::u16string_view i_rPath)
{
    return i_rPath == s_styles;
}

bool isValidXmlId(std::u16string_view i_rStreamName,
    std::u16string_view i_rIdref)
{
    return isValidNCName(i_rIdref)
        && (isContentFile(i_rStreamName) || isStylesFile(i_rStreamName));
}

static bool isReservedFile(std::u16string_view i_rPath)
{
    return isContentFile(i_rPath) || isStylesFile(i_rPath) || i_rPath == u"meta.xml" || i_rPath == u"settings.xml";
}


uno::Reference<rdf::XURI> createBaseURI(
    uno::Reference<uno::XComponentContext> const & i_xContext,
    uno::Reference<frame::XModel> const & i_xModel,
    OUString const & i_rPkgURI, std::u16string_view i_rSubDocument)
{
    if (!i_xContext.is() || (!i_xModel.is() && i_rPkgURI.isEmpty())) {
        throw uno::RuntimeException();
    }

    OUString pkgURI(i_rPkgURI);

    // tdf#123293 chicken/egg problem when loading from stream: there is no URI,
    // and also the model doesn't have a storage yet, so we need to get the
    // tdoc URI without a storage...
    if (pkgURI.isEmpty())
    {
        assert(i_xModel.is());
        uno::Reference<frame::XTransientDocumentsDocumentContentIdentifierFactory>
            const xTDDCIF(
                    i_xContext->getServiceManager()->createInstanceWithContext(
                        u"com.sun.star.ucb.TransientDocumentsContentProvider"_ustr,
                        i_xContext),
                uno::UNO_QUERY_THROW);
        uno::Reference<ucb::XContentIdentifier> const xContentId(
            xTDDCIF->createDocumentContentIdentifier(i_xModel));
        SAL_WARN_IF(!xContentId.is(), "sfx", "createBaseURI: cannot create ContentIdentifier");
        if (!xContentId.is())
        {
            throw uno::RuntimeException(u"createBaseURI: cannot create ContentIdentifier"_ustr);
        }
        pkgURI = xContentId->getContentIdentifier();
        assert(!pkgURI.isEmpty());
        if (!pkgURI.isEmpty() && !pkgURI.endsWith("/"))
        {
            pkgURI += "/";
        }
    }

    // #i108078# workaround non-hierarchical vnd.sun.star.expand URIs
    // this really should be done somewhere else, not here.
    if (pkgURI.startsWithIgnoreAsciiCase("vnd.sun.star.expand:", &pkgURI))
    {
        // expand it here (makeAbsolute requires hierarchical URI)
        if (!pkgURI.isEmpty()) {
            pkgURI = ::rtl::Uri::decode(
                    pkgURI, rtl_UriDecodeStrict, RTL_TEXTENCODING_UTF8);
            if (pkgURI.isEmpty()) {
                throw uno::RuntimeException();
            }
            ::rtl::Bootstrap::expandMacros(pkgURI);
        }
    }

    const uno::Reference<uri::XUriReferenceFactory> xUriFactory =
        uri::UriReferenceFactory::create( i_xContext);
    uno::Reference< uri::XUriReference > xBaseURI;

    const uno::Reference< uri::XUriReference > xPkgURI(
        xUriFactory->parse(pkgURI), uno::UNO_SET_THROW );
    xPkgURI->clearFragment();

    // need to know whether the storage is a FileSystemStorage
    // XServiceInfo would be better, but it is not implemented
//    if ( pkgURI.getLength() && ::utl::UCBContentHelper::IsFolder(pkgURI) )
    if (true) {
        xBaseURI.set( xPkgURI, uno::UNO_SET_THROW );
    }
    OUStringBuffer buf(64);
    if (!xBaseURI->getUriReference().endsWith("/"))
    {
        const sal_Int32 count( xBaseURI->getPathSegmentCount() );
        if (count > 0)
        {
            buf.append(xBaseURI->getPathSegment(count - 1));
        }
        buf.append('/');
    }
    if (!i_rSubDocument.empty())
    {
        buf.append(OUString::Concat(i_rSubDocument) + "/");
    }
    if (!buf.isEmpty())
    {
        const uno::Reference< uri::XUriReference > xPathURI(
            xUriFactory->parse(buf.makeStringAndClear()), uno::UNO_SET_THROW );
        xBaseURI.set(
            xUriFactory->makeAbsolute(xBaseURI, xPathURI,
                true, uri::RelativeUriExcessParentSegments_ERROR),
            uno::UNO_SET_THROW);
    }

    return rdf::URI::create(i_xContext, xBaseURI->getUriReference());
}


struct DocumentMetadataAccess_Impl
{
    // note: these are all initialized in constructor, and loadFromStorage
    const uno::Reference<uno::XComponentContext> m_xContext;
    const SfxObjectShell & m_rXmlIdRegistrySupplier;
    uno::Reference<rdf::XURI> m_xBaseURI;
    uno::Reference<rdf::XRepository> m_xRepository;
    uno::Reference<rdf::XNamedGraph> m_xManifest;
    DocumentMetadataAccess_Impl(
            uno::Reference<uno::XComponentContext> i_xContext,
            SfxObjectShell const & i_rRegistrySupplier)
      : m_xContext(std::move(i_xContext))
      , m_rXmlIdRegistrySupplier(i_rRegistrySupplier)
    {
        OSL_ENSURE(m_xContext.is(), "context null");
    }
};

// this is... a hack.
template<sal_Int16 Constant>
static uno::Reference<rdf::XURI> const &
getURI(uno::Reference< uno::XComponentContext > const & i_xContext)
{
    static uno::Reference< rdf::XURI > xURI(
        rdf::URI::createKnown(i_xContext, Constant), uno::UNO_SET_THROW);
    return xURI;
}


/** would storing the file to a XStorage succeed? */
static bool isFileNameValid(std::u16string_view i_rFileName)
{
    if (i_rFileName.empty()) return false;
    if (i_rFileName[0] == '/')        return false; // no absolute paths!
    sal_Int32 idx(0);
    do {
      const OUString segment(
        o3tl::getToken(i_rFileName, 0, u'/', idx) );
      if (segment.isEmpty()      ||  // no empty segments
          segment == "."         ||  // no . segments
          segment == ".."        ||  // no .. segments
          !::comphelper::OStorageHelper::IsValidZipEntryFileName(
              segment, false))      // no invalid characters
                                      return false;
    } while (idx >= 0);
    return true;
}

/** split a uri hierarchy into first segment and rest */
static bool
splitPath(OUString const & i_rPath,
    OUString & o_rDir, OUString& o_rRest)
{
    const sal_Int32 idx(i_rPath.indexOf(u'/'));
    if (idx < 0 || idx >= i_rPath.getLength()) {
        o_rDir.clear();
        o_rRest = i_rPath;
        return true;
    } else if (idx == 0 || idx == i_rPath.getLength() - 1) {
        // input must not start or end with '/'
        return false;
    } else {
        o_rDir  = i_rPath.copy(0, idx);
        o_rRest = i_rPath.copy(idx+1);
        return true;
    }
}

static bool
splitXmlId(std::u16string_view i_XmlId,
    OUString & o_StreamName, OUString& o_Idref )
{
    const size_t idx(i_XmlId.find(u'#'));
    if (idx == std::u16string_view::npos)
        return false;
    o_StreamName = i_XmlId.substr(0, idx);
    o_Idref      = i_XmlId.substr(idx+1);
    return isValidXmlId(o_StreamName, o_Idref);
}


static uno::Reference<rdf::XURI>
getURIForStream(struct DocumentMetadataAccess_Impl const & i_rImpl,
    OUString const& i_rPath)
{
    const uno::Reference<rdf::XURI> xURI(
        rdf::URI::createNS( i_rImpl.m_xContext,
            i_rImpl.m_xBaseURI->getStringValue(), i_rPath),
        uno::UNO_SET_THROW);
    return xURI;
}

/** add statements declaring i_xResource to be a file of type i_xType with
    path i_rPath to manifest, with optional additional types i_pTypes */
static void
addFile(struct DocumentMetadataAccess_Impl const & i_rImpl,
    uno::Reference<rdf::XURI> const& i_xType,
    OUString const & i_rPath,
    const uno::Sequence < uno::Reference< rdf::XURI > > * i_pTypes)
{
    try {
        const uno::Reference<rdf::XURI> xURI( getURIForStream(
            i_rImpl, i_rPath) );

        i_rImpl.m_xManifest->addStatement(i_rImpl.m_xBaseURI,
            getURI<rdf::URIs::PKG_HASPART>(i_rImpl.m_xContext),
            xURI);
        i_rImpl.m_xManifest->addStatement(xURI,
            getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext),
            i_xType);
        if (i_pTypes) {
            for (const auto& rType : *i_pTypes) {
                i_rImpl.m_xManifest->addStatement(xURI,
                    getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext),
                    rType);
            }
        }
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"addFile: exception"_ustr, /*this*/nullptr, anyEx);
    }
}

/** add content.xml or styles.xml to manifest */
static bool
addContentOrStylesFileImpl(struct DocumentMetadataAccess_Impl const & i_rImpl,
    const OUString & i_rPath)
{
    uno::Reference<rdf::XURI> xType;
    if (isContentFile(i_rPath)) {
        xType.set(getURI<rdf::URIs::ODF_CONTENTFILE>(i_rImpl.m_xContext));
    } else if (isStylesFile(i_rPath)) {
        xType.set(getURI<rdf::URIs::ODF_STYLESFILE>(i_rImpl.m_xContext));
    } else {
        return false;
    }
    addFile(i_rImpl, xType, i_rPath, nullptr);
    return true;
}

/** add metadata file to manifest */
static void
addMetadataFileImpl(struct DocumentMetadataAccess_Impl const & i_rImpl,
    const OUString & i_rPath,
    const uno::Sequence < uno::Reference< rdf::XURI > > & i_rTypes)
{
    addFile(i_rImpl,
            getURI<rdf::URIs::PKG_METADATAFILE>(i_rImpl.m_xContext),
            i_rPath, &i_rTypes);
}

/** remove a file from the manifest */
static void
removeFile(struct DocumentMetadataAccess_Impl const & i_rImpl,
    uno::Reference<rdf::XURI> const& i_xPart)
{
    if (!i_xPart.is()) throw uno::RuntimeException();
    try {
        i_rImpl.m_xManifest->removeStatements(i_rImpl.m_xBaseURI,
            getURI<rdf::URIs::PKG_HASPART>(i_rImpl.m_xContext),
            i_xPart);
        i_rImpl.m_xManifest->removeStatements(i_xPart,
            getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext), nullptr);
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"removeFile: exception"_ustr,
            nullptr, anyEx);
    }
}

static ::std::vector< uno::Reference< rdf::XURI > >
getAllParts(struct DocumentMetadataAccess_Impl const & i_rImpl)
{
    ::std::vector< uno::Reference< rdf::XURI > > ret;
    try {
        const uno::Reference<container::XEnumeration> xEnum(
            i_rImpl.m_xManifest->getStatements( i_rImpl.m_xBaseURI,
                getURI<rdf::URIs::PKG_HASPART>(i_rImpl.m_xContext), nullptr),
            uno::UNO_SET_THROW);
        while (xEnum->hasMoreElements()) {
            rdf::Statement stmt;
            if (!(xEnum->nextElement() >>= stmt)) {
                throw uno::RuntimeException();
            }
            const uno::Reference<rdf::XURI> xPart(stmt.Object,
                uno::UNO_QUERY);
            if (!xPart.is()) continue;
            ret.push_back(xPart);
        }
        return ret;
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"getAllParts: exception"_ustr,
            nullptr, anyEx);
    }
}

static bool
isPartOfType(struct DocumentMetadataAccess_Impl const & i_rImpl,
    uno::Reference<rdf::XURI> const & i_xPart,
    uno::Reference<rdf::XURI> const & i_xType)
{
    if (!i_xPart.is() || !i_xType.is()) throw uno::RuntimeException();
    try {
        const uno::Reference<container::XEnumeration> xEnum(
            i_rImpl.m_xManifest->getStatements(i_xPart,
                getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext),
                i_xType),
            uno::UNO_SET_THROW);
        return xEnum->hasMoreElements();
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"isPartOfType: exception"_ustr,
            nullptr, anyEx);
    }
}

static ::std::vector<uno::Reference<rdf::XURI>>
getAllParts(struct DocumentMetadataAccess_Impl const& i_rImpl,
            const uno::Reference<rdf::XURI>& i_xType)
{
    ::std::vector<uno::Reference<rdf::XURI>> ret;
    try
    {
        const uno::Reference<container::XEnumeration> xEnum(
            i_rImpl.m_xManifest->getStatements(i_rImpl.m_xBaseURI,
                                               getURI<rdf::URIs::PKG_HASPART>(i_rImpl.m_xContext),
                                               nullptr),
            uno::UNO_SET_THROW);
        while (xEnum->hasMoreElements())
        {
            rdf::Statement stmt;
            if (!(xEnum->nextElement() >>= stmt))
            {
                throw uno::RuntimeException();
            }
            const uno::Reference<rdf::XURI> xPart(stmt.Object, uno::UNO_QUERY);
            if (!xPart.is())
                continue;

            const uno::Reference<container::XEnumeration> xEnum2(
                i_rImpl.m_xManifest->getStatements(
                    xPart, getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext), i_xType),
                uno::UNO_SET_THROW);
            if (xEnum2->hasMoreElements())
                ret.emplace_back(xPart);
        }
        return ret;
    }
    catch (const uno::RuntimeException&)
    {
        throw;
    }
    catch (const uno::Exception& e)
    {
        throw lang::WrappedTargetRuntimeException(u"getAllParts: exception"_ustr, nullptr,
                                                  uno::Any(e));
    }
}

static ucb::InteractiveAugmentedIOException
mkException( OUString const & i_rMessage,
    ucb::IOErrorCode const i_ErrorCode,
    OUString const & i_rUri, OUString const & i_rResource)
{
    const beans::PropertyValue uriProp(u"Uri"_ustr,
        -1, uno::Any(i_rUri), static_cast<beans::PropertyState>(0));
    const beans::PropertyValue rnProp(
        u"ResourceName"_ustr,
        -1, uno::Any(i_rResource), static_cast<beans::PropertyState>(0));
    return ucb::InteractiveAugmentedIOException(i_rMessage, {},
                                                task::InteractionClassification_ERROR, i_ErrorCode,
                                                { uno::Any(uriProp), uno::Any(rnProp) });
}

/** error handling policy.
    <p>If a handler is given, ask it how to proceed:
    <ul><li>(default:) cancel import, raise exception</li>
        <li>ignore the error and continue</li>
        <li>retry the action that led to the error</li></ul></p>
    N.B.: must not be called before DMA is fully initialized!
    @returns true iff caller should retry
 */
static bool
handleError( ucb::InteractiveAugmentedIOException const & i_rException,
    const uno::Reference<task::XInteractionHandler> & i_xHandler)
{
    if (!i_xHandler.is()) {
        throw lang::WrappedTargetException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: exception"_ustr,
            /* *this*/ nullptr, uno::Any(i_rException));
    }

    ::rtl::Reference< ::comphelper::OInteractionRequest > pRequest(
        new ::comphelper::OInteractionRequest(uno::Any(i_rException)) );
    ::rtl::Reference< ::comphelper::OInteractionRetry > pRetry(
        new ::comphelper::OInteractionRetry );
    ::rtl::Reference< ::comphelper::OInteractionApprove > pApprove(
        new ::comphelper::OInteractionApprove );
    ::rtl::Reference< ::comphelper::OInteractionAbort > pAbort(
        new ::comphelper::OInteractionAbort );

    pRequest->addContinuation( pApprove );
    pRequest->addContinuation( pAbort );
    // actually call the handler
    i_xHandler->handle( pRequest );
    if (pRetry->wasSelected()) {
        return true;
    } else if (pApprove->wasSelected()) {
        return false;
    } else {
        OSL_ENSURE(pAbort->wasSelected(), "no continuation selected?");
        throw lang::WrappedTargetException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: exception"_ustr,
            /* *this*/ nullptr, uno::Any(i_rException));
    }
}

/** check if storage has content.xml/styles.xml;
    e.g. ODB files seem to only have content.xml */
static void
collectFilesFromStorage(uno::Reference<embed::XStorage> const& i_xStorage,
    std::set< OUString > & o_rFiles)
{
    try {
        if (i_xStorage->hasByName(s_content) &&
            i_xStorage->isStreamElement(s_content))
        {
            o_rFiles.insert(s_content);
        }
        if (i_xStorage->hasByName(s_styles) &&
            i_xStorage->isStreamElement(s_styles))
        {
            o_rFiles.insert(s_styles);
        }
    } catch (const uno::Exception &) {
        TOOLS_WARN_EXCEPTION("sfx", "collectFilesFromStorage");
    }
}

/** import a metadata file into repository */
static void
readStream(struct DocumentMetadataAccess_Impl & i_rImpl,
    uno::Reference< embed::XStorage > const & i_xStorage,
    OUString const & i_rPath,
    OUString const & i_rBaseURI)
{
    try {
        OUString dir;
        OUString rest;
        if (!splitPath(i_rPath, dir, rest)) throw uno::RuntimeException();
        if (dir.isEmpty()) {
            if (!i_xStorage->isStreamElement(i_rPath)) {
                throw mkException(
                    u"readStream: is not a stream"_ustr,
                    ucb::IOErrorCode_NO_FILE, i_rBaseURI + i_rPath, i_rPath);
            }
            const uno::Reference<io::XStream> xStream(
                i_xStorage->openStreamElement(i_rPath,
                    embed::ElementModes::READ), uno::UNO_SET_THROW);
            const uno::Reference<io::XInputStream> xInStream(
                xStream->getInputStream(), uno::UNO_SET_THROW );
            const uno::Reference<rdf::XURI> xBaseURI(
                rdf::URI::create(i_rImpl.m_xContext, i_rBaseURI));
            const uno::Reference<rdf::XURI> xURI(
                rdf::URI::createNS(i_rImpl.m_xContext,
                    i_rBaseURI, i_rPath));
            i_rImpl.m_xRepository->importGraph(rdf::FileFormat::RDF_XML,
                xInStream, xURI, xBaseURI);
        } else {
            if (!i_xStorage->isStorageElement(dir)) {
                throw mkException(
                    u"readStream: is not a directory"_ustr,
                    ucb::IOErrorCode_NO_DIRECTORY, i_rBaseURI + dir, dir);
            }
            const uno::Reference<embed::XStorage> xDir(
                i_xStorage->openStorageElement(dir,
                    embed::ElementModes::READ));
            const uno::Reference< beans::XPropertySet > xDirProps(xDir,
                uno::UNO_QUERY_THROW);
            try {
                OUString mimeType;
                xDirProps->getPropertyValue(
                        utl::MediaDescriptor::PROP_MEDIATYPE )
                    >>= mimeType;
                if (mimeType.startsWith(s_odfmime)) {
                    SAL_WARN("sfx", "readStream: refusing to recurse into embedded document");
                    return;
                }
            } catch (const uno::Exception &) { }
            readStream(i_rImpl, xDir, rest, i_rBaseURI+dir+"/" );
        }
    } catch (const container::NoSuchElementException & e) {
        throw mkException(e.Message, ucb::IOErrorCode_NOT_EXISTING_PATH,
            i_rBaseURI + i_rPath, i_rPath);
    } catch (const io::IOException & e) {
        throw mkException(e.Message, ucb::IOErrorCode_CANT_READ,
            i_rBaseURI + i_rPath, i_rPath);
    } catch (const rdf::ParseException & e) {
        throw mkException(e.Message, ucb::IOErrorCode_WRONG_FORMAT,
            i_rBaseURI + i_rPath, i_rPath);
    }
}

/** import a metadata file into repository */
static void
importFile(struct DocumentMetadataAccess_Impl & i_rImpl,
    uno::Reference<embed::XStorage> const & i_xStorage,
    OUString const & i_rBaseURI,
    uno::Reference<task::XInteractionHandler> const & i_xHandler,
    const OUString& i_rPath)
{
retry:
    try {
        readStream(i_rImpl, i_xStorage, i_rPath, i_rBaseURI);
    } catch (const ucb::InteractiveAugmentedIOException & e) {
        if (handleError(e, i_xHandler)) goto retry;
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"importFile: exception"_ustr,
            nullptr, anyEx);
    }
}

/** actually write a metadata file to the storage */
static void
exportStream(struct DocumentMetadataAccess_Impl const & i_rImpl,
    uno::Reference< embed::XStorage > const & i_xStorage,
    uno::Reference<rdf::XURI> const & i_xGraphName,
    OUString const & i_rFileName,
    OUString const & i_rBaseURI)
{
    const uno::Reference<io::XStream> xStream(
        i_xStorage->openStreamElement(i_rFileName,
            embed::ElementModes::WRITE | embed::ElementModes::TRUNCATE),
        uno::UNO_SET_THROW);
    const uno::Reference< beans::XPropertySet > xStreamProps(xStream,
        uno::UNO_QUERY);
    if (xStreamProps.is()) { // this is NOT supported in FileSystemStorage
        xStreamProps->setPropertyValue(
            u"MediaType"_ustr,
            uno::Any(u"application/rdf+xml"_ustr));
    }
    const uno::Reference<io::XOutputStream> xOutStream(
        xStream->getOutputStream(), uno::UNO_SET_THROW );
    const uno::Reference<rdf::XURI> xBaseURI(
        rdf::URI::create(i_rImpl.m_xContext, i_rBaseURI));
    i_rImpl.m_xRepository->exportGraph(rdf::FileFormat::RDF_XML,
        xOutStream, i_xGraphName, xBaseURI);
}

/** write a metadata file to the storage */
static void
writeStream(struct DocumentMetadataAccess_Impl & i_rImpl,
    uno::Reference< embed::XStorage > const & i_xStorage,
    uno::Reference<rdf::XURI> const & i_xGraphName,
    OUString const & i_rPath,
    OUString const & i_rBaseURI)
{
    OUString dir;
    OUString rest;
    if (!splitPath(i_rPath, dir, rest)) throw uno::RuntimeException();
    try {
        if (dir.isEmpty()) {
            exportStream(i_rImpl, i_xStorage, i_xGraphName, i_rPath,
                i_rBaseURI);
        } else {
            const uno::Reference<embed::XStorage> xDir(
                i_xStorage->openStorageElement(dir,
                    embed::ElementModes::WRITE));
            const uno::Reference< beans::XPropertySet > xDirProps(xDir,
                uno::UNO_QUERY_THROW);
            try {
                OUString mimeType;
                xDirProps->getPropertyValue(
                        utl::MediaDescriptor::PROP_MEDIATYPE )
                    >>= mimeType;
                if (mimeType.startsWith(s_odfmime)) {
                    SAL_WARN("sfx", "writeStream: refusing to recurse into embedded document");
                    return;
                }
            } catch (const uno::Exception &) { }
            writeStream(i_rImpl, xDir, i_xGraphName, rest, i_rBaseURI+dir+"/");
            uno::Reference<embed::XTransactedObject> const xTransaction(
                xDir, uno::UNO_QUERY);
            if (xTransaction.is()) {
                xTransaction->commit();
            }
        }
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const io::IOException &) {
        throw;
    }
}

static void
initLoading(struct DocumentMetadataAccess_Impl & i_rImpl,
    const uno::Reference< embed::XStorage > & i_xStorage,
    const uno::Reference<rdf::XURI> & i_xBaseURI,
    const uno::Reference<task::XInteractionHandler> & i_xHandler)
{
retry:
    // clear old data
    i_rImpl.m_xManifest.clear();
    // init BaseURI
    i_rImpl.m_xBaseURI = i_xBaseURI;

    // create repository
    i_rImpl.m_xRepository.clear();
    i_rImpl.m_xRepository.set(rdf::Repository::create(i_rImpl.m_xContext),
            uno::UNO_SET_THROW);

    // try to delay raising errors until after initialization is done
    uno::Any rterr;
    ucb::InteractiveAugmentedIOException iaioe;
    bool err(false);

    const uno::Reference <rdf::XURI> xManifest(
        getURIForStream(i_rImpl, s_manifest));
    try {
        readStream(i_rImpl, i_xStorage, s_manifest, i_xBaseURI->getStringValue());
    } catch (const ucb::InteractiveAugmentedIOException & e) {
        // no manifest.rdf: this is not an error in ODF < 1.2
        if (ucb::IOErrorCode_NOT_EXISTING_PATH != e.Code) {
            iaioe = e;
            err = true;
        }
    } catch (const uno::Exception & e) {
        rterr <<= e;
    }

    // init manifest graph
    const uno::Reference<rdf::XNamedGraph> xManifestGraph(
        i_rImpl.m_xRepository->getGraph(xManifest));
    i_rImpl.m_xManifest.set(xManifestGraph.is() ? xManifestGraph :
        i_rImpl.m_xRepository->createGraph(xManifest), uno::UNO_SET_THROW);

    // document statement
    i_rImpl.m_xManifest->addStatement(i_rImpl.m_xBaseURI,
        getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext),
        getURI<rdf::URIs::PKG_DOCUMENT>(i_rImpl.m_xContext));

    OSL_ENSURE(i_rImpl.m_xBaseURI.is(), "base URI is null");
    OSL_ENSURE(i_rImpl.m_xRepository.is(), "repository is null");
    OSL_ENSURE(i_rImpl.m_xManifest.is(), "manifest is null");

    if (rterr.hasValue()) {
        throw lang::WrappedTargetRuntimeException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: "
            "exception"_ustr, nullptr, rterr);
    }

    if (err && handleError(iaioe, i_xHandler))
        goto retry;
}

/** init Impl struct */
static void init(struct DocumentMetadataAccess_Impl & i_rImpl)
{
    try {

        i_rImpl.m_xManifest.set(i_rImpl.m_xRepository->createGraph(
            getURIForStream(i_rImpl, s_manifest)),
            uno::UNO_SET_THROW);

        // insert the document statement
        i_rImpl.m_xManifest->addStatement(i_rImpl.m_xBaseURI,
            getURI<rdf::URIs::RDF_TYPE>(i_rImpl.m_xContext),
            getURI<rdf::URIs::PKG_DOCUMENT>(i_rImpl.m_xContext));
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"init: unexpected exception"_ustr, nullptr,
            anyEx);
    }

    // add top-level content files
    if (!addContentOrStylesFileImpl(i_rImpl, s_content)) {
        throw uno::RuntimeException();
    }
    if (!addContentOrStylesFileImpl(i_rImpl, s_styles)) {
        throw uno::RuntimeException();
    }
}


DocumentMetadataAccess::DocumentMetadataAccess(
        uno::Reference< uno::XComponentContext > const & i_xContext,
        const SfxObjectShell & i_rRegistrySupplier)
    : m_pImpl(new DocumentMetadataAccess_Impl(i_xContext, i_rRegistrySupplier))
{
    // no initialization: must call loadFrom...
}

DocumentMetadataAccess::DocumentMetadataAccess(
        uno::Reference< uno::XComponentContext > const & i_xContext,
        const SfxObjectShell & i_rRegistrySupplier,
        OUString const & i_rURI)
    : m_pImpl(new DocumentMetadataAccess_Impl(i_xContext, i_rRegistrySupplier))
{
    OSL_ENSURE(!i_rURI.isEmpty(), "DMA::DMA: no URI given!");
    OSL_ENSURE(i_rURI.endsWith("/"), "DMA::DMA: URI without / given!");
    if (!i_rURI.endsWith("/")) throw uno::RuntimeException();
    m_pImpl->m_xBaseURI.set(rdf::URI::create(m_pImpl->m_xContext, i_rURI));
    m_pImpl->m_xRepository.set(rdf::Repository::create(m_pImpl->m_xContext),
            uno::UNO_SET_THROW);

    // init repository
    init(*m_pImpl);

    OSL_ENSURE(m_pImpl->m_xBaseURI.is(), "base URI is null");
    OSL_ENSURE(m_pImpl->m_xRepository.is(), "repository is null");
    OSL_ENSURE(m_pImpl->m_xManifest.is(), "manifest is null");
}

DocumentMetadataAccess::~DocumentMetadataAccess()
{
}

// css::rdf::XRepositorySupplier:
uno::Reference< rdf::XRepository > SAL_CALL
DocumentMetadataAccess::getRDFRepository()
{
    OSL_ENSURE(m_pImpl->m_xRepository.is(), "repository not initialized");
    return m_pImpl->m_xRepository;
}

// css::rdf::XNode:
OUString SAL_CALL
DocumentMetadataAccess::getStringValue()
{
    return m_pImpl->m_xBaseURI->getStringValue();
}

// css::rdf::XURI:
OUString SAL_CALL
DocumentMetadataAccess::getNamespace()
{
    return m_pImpl->m_xBaseURI->getNamespace();
}

OUString SAL_CALL
DocumentMetadataAccess::getLocalName()
{
    return m_pImpl->m_xBaseURI->getLocalName();
}

// css::rdf::XDocumentMetadataAccess:
uno::Reference< rdf::XMetadatable > SAL_CALL
DocumentMetadataAccess::getElementByMetadataReference(
    const css::beans::StringPair & i_rReference)
{
    const IXmlIdRegistry * pReg(
        m_pImpl->m_rXmlIdRegistrySupplier.GetXmlIdRegistry() );
    if (!pReg) {
        throw uno::RuntimeException(
            u"DocumentMetadataAccess::getElementByXmlId: no registry"_ustr, *this);
    }
    return pReg->GetElementByMetadataReference(i_rReference);
}

uno::Reference< rdf::XMetadatable > SAL_CALL
DocumentMetadataAccess::getElementByURI(
    const uno::Reference< rdf::XURI > & i_xURI )
{
    if (!i_xURI.is()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::getElementByURI: URI is null"_ustr, *this, 0);
    }

    const OUString baseURI( m_pImpl->m_xBaseURI->getStringValue() );
    const OUString name( i_xURI->getStringValue() );
    if (!name.match(baseURI)) {
        return nullptr;
    }
    OUString path;
    OUString idref;
    if (!splitXmlId(name.subView(baseURI.getLength()), path, idref)) {
        return nullptr;
    }

    return getElementByMetadataReference( beans::StringPair(path, idref) );
}

uno::Sequence<uno::Reference<rdf::XURI>> SAL_CALL
DocumentMetadataAccess::getMetadataGraphsWithType(const uno::Reference<rdf::XURI>& i_xType)
{
    if (!i_xType.is())
    {
        throw lang::IllegalArgumentException(u"DocumentMetadataAccess::getMetadataGraphsWithType: "
                                             "type is null"_ustr,
                                             *this, 0);
    }

    return ::comphelper::containerToSequence(getAllParts(*m_pImpl, i_xType));
}

uno::Reference<rdf::XURI> SAL_CALL
DocumentMetadataAccess::addMetadataFile(const OUString & i_rFileName,
    const uno::Sequence < uno::Reference< rdf::XURI > > & i_rTypes)
{
    if (!isFileNameValid(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::addMetadataFile: invalid FileName"_ustr,
            *this, 0);
    }
    if (isReservedFile(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::addMetadataFile:"
            "invalid FileName: reserved"_ustr, *this, 0);
    }
    if (std::any_of(i_rTypes.begin(), i_rTypes.end(),
            [](const uno::Reference< rdf::XURI >& rType) { return !rType.is(); })) {
        throw lang::IllegalArgumentException(
                u"DocumentMetadataAccess::addMetadataFile: "
                "null type"_ustr, *this, 2);
    }

    const uno::Reference<rdf::XURI> xGraphName(
        getURIForStream(*m_pImpl, i_rFileName) );

    try {
        m_pImpl->m_xRepository->createGraph(xGraphName);
    } catch (const rdf::RepositoryException &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"DocumentMetadataAccess::addMetadataFile: exception"_ustr,
            *this, anyEx);
        // note: all other exceptions are propagated
    }

    addMetadataFileImpl(*m_pImpl, i_rFileName, i_rTypes);
    return xGraphName;
}

uno::Reference<rdf::XURI> SAL_CALL
DocumentMetadataAccess::importMetadataFile(::sal_Int16 i_Format,
    const uno::Reference< io::XInputStream > & i_xInStream,
    const OUString & i_rFileName,
    const uno::Reference< rdf::XURI > & i_xBaseURI,
    const uno::Sequence < uno::Reference< rdf::XURI > > & i_rTypes)
{
    if (!isFileNameValid(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::importMetadataFile: invalid FileName"_ustr,
            *this, 0);
    }
    if (isReservedFile(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::importMetadataFile:"
            "invalid FileName: reserved"_ustr, *this, 0);
    }
    if (std::any_of(i_rTypes.begin(), i_rTypes.end(),
            [](const uno::Reference< rdf::XURI >& rType) { return !rType.is(); })) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::importMetadataFile: null type"_ustr,
            *this, 5);
    }

    const uno::Reference<rdf::XURI> xGraphName(
        getURIForStream(*m_pImpl, i_rFileName) );

    try {
        m_pImpl->m_xRepository->importGraph(
            i_Format, i_xInStream, xGraphName, i_xBaseURI);
    } catch (const rdf::RepositoryException &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                u"DocumentMetadataAccess::importMetadataFile: "
                "RepositoryException"_ustr, *this, anyEx);
        // note: all other exceptions are propagated
    }

    // add to manifest
    addMetadataFileImpl(*m_pImpl, i_rFileName, i_rTypes);
    return xGraphName;
}

void SAL_CALL
DocumentMetadataAccess::removeMetadataFile(
    const uno::Reference< rdf::XURI > & i_xGraphName)
{
    try {
        m_pImpl->m_xRepository->destroyGraph(i_xGraphName);
    } catch (const rdf::RepositoryException &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                u"DocumentMetadataAccess::removeMetadataFile: "
                "RepositoryException"_ustr, *this, anyEx);
        // note: all other exceptions are propagated
    }

    // remove file from manifest
    removeFile(*m_pImpl, i_xGraphName);
}

void SAL_CALL
DocumentMetadataAccess::addContentOrStylesFile(
    const OUString & i_rFileName)
{
    if (!isFileNameValid(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::addContentOrStylesFile: "
            "invalid FileName"_ustr, *this, 0);
    }

    if (!addContentOrStylesFileImpl(*m_pImpl, i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::addContentOrStylesFile: "
            "invalid FileName: must end with content.xml or styles.xml"_ustr,
            *this, 0);
    }
}

void SAL_CALL
DocumentMetadataAccess::removeContentOrStylesFile(
    const OUString & i_rFileName)
{
    if (!isFileNameValid(i_rFileName)) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::removeContentOrStylesFile: "
            "invalid FileName"_ustr, *this, 0);
    }

    try {
        const uno::Reference<rdf::XURI> xPart(
            getURIForStream(*m_pImpl, i_rFileName) );
        const uno::Reference<container::XEnumeration> xEnum(
            m_pImpl->m_xManifest->getStatements( m_pImpl->m_xBaseURI,
                getURI<rdf::URIs::PKG_HASPART>(m_pImpl->m_xContext),
                xPart),
            uno::UNO_SET_THROW);
        if (!xEnum->hasMoreElements()) {
            throw container::NoSuchElementException(
                "DocumentMetadataAccess::removeContentOrStylesFile: "
                "cannot find stream in manifest graph: " + i_rFileName,
                *this);
        }

        // remove file from manifest
        removeFile(*m_pImpl, xPart);

    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
            u"DocumentMetadataAccess::removeContentOrStylesFile: exception"_ustr,
            *this, anyEx);
    }
}

void SAL_CALL DocumentMetadataAccess::loadMetadataFromStorage(
    const uno::Reference< embed::XStorage > & i_xStorage,
    const uno::Reference<rdf::XURI> & i_xBaseURI,
    const uno::Reference<task::XInteractionHandler> & i_xHandler)
{
    if (!i_xStorage.is()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: "
            "storage is null"_ustr, *this, 0);
    }
    if (!i_xBaseURI.is()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: "
            "base URI is null"_ustr, *this, 1);
    }
    const OUString baseURI( i_xBaseURI->getStringValue());
    if (baseURI.indexOf('#') >= 0) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: "
            "base URI not absolute"_ustr, *this, 1);
    }
    if (!baseURI.endsWith("/")) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::loadMetadataFromStorage: "
            "base URI does not end with slash"_ustr, *this, 1);
    }

    initLoading(*m_pImpl, i_xStorage, i_xBaseURI, i_xHandler);

    std::set< OUString > StgFiles;
    collectFilesFromStorage(i_xStorage, StgFiles);

    std::vector< OUString > MfstMetadataFiles;

    try {
        const ::std::vector< uno::Reference< rdf::XURI > > parts(
            getAllParts(*m_pImpl) );
        const uno::Reference<rdf::XURI>& xContentFile(
            getURI<rdf::URIs::ODF_CONTENTFILE>(m_pImpl->m_xContext));
        const uno::Reference<rdf::XURI>& xStylesFile(
            getURI<rdf::URIs::ODF_STYLESFILE>(m_pImpl->m_xContext));
        const uno::Reference<rdf::XURI>& xMetadataFile(
            getURI<rdf::URIs::PKG_METADATAFILE>(m_pImpl->m_xContext));
        const sal_Int32 len( baseURI.getLength() );
        for (const auto& rxPart : parts) {
            const OUString name(rxPart->getStringValue());
            if (!name.match(baseURI)) {
                SAL_WARN("sfx", "loadMetadataFromStorage: graph not in document: " << name);
                continue;
            }
            const OUString relName( name.copy(len) );
            if (relName == s_manifest) {
                SAL_WARN("sfx", "loadMetadataFromStorage: found ourselves a recursive manifest!");
                continue;
            }
            // remove found items from StgFiles
            StgFiles.erase(relName);
            if (isContentFile(relName)) {
                if (!isPartOfType(*m_pImpl, rxPart, xContentFile)) {
                    const uno::Reference <rdf::XURI> xName(
                        getURIForStream(*m_pImpl, relName) );
                    // add missing type statement
                    m_pImpl->m_xManifest->addStatement(xName,
                        getURI<rdf::URIs::RDF_TYPE>(m_pImpl->m_xContext),
                        xContentFile);
                }
            } else if (isStylesFile(relName)) {
                if (!isPartOfType(*m_pImpl, rxPart, xStylesFile)) {
                    const uno::Reference <rdf::XURI> xName(
                        getURIForStream(*m_pImpl, relName) );
                    // add missing type statement
                    m_pImpl->m_xManifest->addStatement(xName,
                        getURI<rdf::URIs::RDF_TYPE>(m_pImpl->m_xContext),
                        xStylesFile);
                }
            } else if (isReservedFile(relName)) {
                SAL_WARN("sfx", "loadMetadataFromStorage: reserved file name in manifest");
            } else {
                if (isPartOfType(*m_pImpl, rxPart, xMetadataFile)) {
                    MfstMetadataFiles.push_back(relName);
                }
                // do not add statement for MetadataFile; it could be
                // something else! just ignore it...
            }
        }
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                u"DocumentMetadataAccess::loadMetadataFromStorage: "
                "exception"_ustr, *this, anyEx);
    }

    for (const auto& aStgFile : StgFiles)
        addContentOrStylesFileImpl(*m_pImpl, aStgFile);

    for (const auto& aMfstMetadataFile : MfstMetadataFiles)
        importFile(*m_pImpl, i_xStorage, baseURI, i_xHandler, aMfstMetadataFile);
}

void SAL_CALL DocumentMetadataAccess::storeMetadataToStorage(
    const uno::Reference< embed::XStorage > & i_xStorage)
{
    if (!i_xStorage.is()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::storeMetadataToStorage: "
            "storage is null"_ustr, *this, 0);
    }

    // export manifest
    const uno::Reference <rdf::XURI> xManifest(
        getURIForStream(*m_pImpl, s_manifest) );
    const OUString baseURI( m_pImpl->m_xBaseURI->getStringValue() );
    try {
        writeStream(*m_pImpl, i_xStorage, xManifest, s_manifest, baseURI);
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const io::IOException &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetException(
            u"storeMetadataToStorage: IO exception"_ustr, *this, anyEx);
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                u"storeMetadataToStorage: exception"_ustr, *this, anyEx);
    }

    // export metadata streams
    try {
        const uno::Sequence<uno::Reference<rdf::XURI> > graphs(
            m_pImpl->m_xRepository->getGraphNames());
        const sal_Int32 len( baseURI.getLength() );
        for (const uno::Reference<rdf::XURI>& xName : graphs) {
            const OUString name(xName->getStringValue());
            if (!name.match(baseURI)) {
                SAL_WARN("sfx", "storeMetadataToStorage: graph not in document: " << name);
                continue;
            }
            const OUString relName( name.copy(len) );
            if (relName == s_manifest) {
                continue;
            }
            if (!isFileNameValid(relName) || isReservedFile(relName)) {
                SAL_WARN("sfx", "storeMetadataToStorage: invalid file name: " << relName);
                continue;
            }
            try {
                writeStream(*m_pImpl, i_xStorage, xName, relName, baseURI);
            } catch (const uno::RuntimeException &) {
                throw;
            } catch (const io::IOException &) {
                css::uno::Any anyEx = cppu::getCaughtException();
                throw lang::WrappedTargetException(
                    u"storeMetadataToStorage: IO exception"_ustr,
                    *this, anyEx);
            } catch (const uno::Exception &) {
                css::uno::Any anyEx = cppu::getCaughtException();
                throw lang::WrappedTargetRuntimeException(
                    u"storeMetadataToStorage: exception"_ustr,
                    *this, anyEx);
            }
        }
    } catch (const rdf::RepositoryException &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetRuntimeException(
                u"storeMetadataToStorage: exception"_ustr, *this, anyEx);
    }
}

void SAL_CALL
DocumentMetadataAccess::loadMetadataFromMedium(
    const uno::Sequence< beans::PropertyValue > & i_rMedium)
{
    uno::Reference<io::XInputStream> xIn;
    utl::MediaDescriptor md(i_rMedium);
    OUString URL;
    md[ utl::MediaDescriptor::PROP_URL ] >>= URL;
    OUString BaseURL;
    md[ utl::MediaDescriptor::PROP_DOCUMENTBASEURL ] >>= BaseURL;
    if (md.addInputStream()) {
        md[ utl::MediaDescriptor::PROP_INPUTSTREAM ] >>= xIn;
    }
    if (!xIn.is() && URL.isEmpty()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::loadMetadataFromMedium: "
            "invalid medium: no URL, no input stream"_ustr, *this, 0);
    }
    uno::Reference<embed::XStorage> xStorage;
    try {
        if (xIn.is()) {
            xStorage = ::comphelper::OStorageHelper::GetStorageFromInputStream(
                            xIn, m_pImpl->m_xContext);
        } else { // fallback to url
            xStorage = ::comphelper::OStorageHelper::GetStorageFromURL2(
                            URL, embed::ElementModes::READ, m_pImpl->m_xContext);
        }
    } catch (const uno::RuntimeException &) {
        throw;
    } catch (const io::IOException &) {
        throw;
    } catch (const uno::Exception &) {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw lang::WrappedTargetException(
                    u"DocumentMetadataAccess::loadMetadataFromMedium: "
                    "exception"_ustr, *this, anyEx);
    }
    if (!xStorage.is()) {
        throw uno::RuntimeException(
            u"DocumentMetadataAccess::loadMetadataFromMedium: "
            "cannot get Storage"_ustr, *this);
    }
    uno::Reference<rdf::XURI> xBaseURI;
    try {
        xBaseURI = createBaseURI(m_pImpl->m_xContext, nullptr, BaseURL);
    } catch (const uno::Exception &) {
        // fall back to URL
        try {
            xBaseURI = createBaseURI(m_pImpl->m_xContext, nullptr, URL);
        } catch (const uno::Exception &) {
            OSL_FAIL("cannot create base URI");
        }
    }
    uno::Reference<task::XInteractionHandler> xIH;
    md[ utl::MediaDescriptor::PROP_INTERACTIONHANDLER ] >>= xIH;
    loadMetadataFromStorage(xStorage, xBaseURI, xIH);
}

void SAL_CALL
DocumentMetadataAccess::storeMetadataToMedium(
    const uno::Sequence< beans::PropertyValue > & i_rMedium)
{
    utl::MediaDescriptor md(i_rMedium);
    OUString URL;
    md[ utl::MediaDescriptor::PROP_URL ] >>= URL;
    if (URL.isEmpty()) {
        throw lang::IllegalArgumentException(
            u"DocumentMetadataAccess::storeMetadataToMedium: "
            "invalid medium: no URL"_ustr, *this, 0);
    }

    SfxMedium aMedium(i_rMedium);
    uno::Reference<embed::XStorage> xStorage(aMedium.GetOutputStorage());

    bool sfx(false);
    if (xStorage.is()) {
        sfx = true;
    } else {
        xStorage = ::comphelper::OStorageHelper::GetStorageFromURL2(
                        URL, embed::ElementModes::WRITE, m_pImpl->m_xContext);
    }

    if (!xStorage.is()) {
        throw uno::RuntimeException(
            u"DocumentMetadataAccess::storeMetadataToMedium: "
            "cannot get Storage"_ustr, *this);
    }
    // set MIME type of the storage
    utl::MediaDescriptor::const_iterator iter
        = md.find(utl::MediaDescriptor::PROP_MEDIATYPE);
    if (iter != md.end()) {
        uno::Reference< beans::XPropertySet > xProps(xStorage,
            uno::UNO_QUERY_THROW);
        try {
            // this is NOT supported in FileSystemStorage
            xProps->setPropertyValue(
                utl::MediaDescriptor::PROP_MEDIATYPE,
                iter->second);
        } catch (const uno::Exception &) { }
    }
    storeMetadataToStorage(xStorage);

    if (!sfx)
        return;

    const bool bOk = aMedium.Commit();
    aMedium.Close();
    if ( !bOk ) {
        ErrCodeMsg nError = aMedium.GetErrorIgnoreWarning();
        if ( nError == ERRCODE_NONE ) {
            nError = ERRCODE_IO_GENERAL;
        }
        task::ErrorCodeIOException ex(
            "DocumentMetadataAccess::storeMetadataToMedium Commit failed: " + nError.toString(),
            uno::Reference< uno::XInterface >(), sal_uInt32(nError.GetCode()));
        throw lang::WrappedTargetException(OUString(), *this,
                uno::Any(ex));
    }
}

} // namespace sfx2

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

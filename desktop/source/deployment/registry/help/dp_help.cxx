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
#include <config_features.h>

#include <strings.hrc>
#include <dp_backend.h>
#include <dp_misc.h>
#include "dp_helpbackenddb.hxx"
#include <dp_ucb.h>
#include <rtl/uri.hxx>
#include <osl/file.hxx>
#include <ucbhelper/content.hxx>
#include <svl/inettype.hxx>
#include <unotools/pathoptions.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/string_view.hxx>

#if HAVE_FEATURE_XMLHELP
#include <helpcompiler/compilehelp.hxx>
#include <helpcompiler/HelpIndexer.hxx>
#endif
#include <com/sun/star/deployment/DeploymentException.hpp>
#include <com/sun/star/deployment/ExtensionRemovedException.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/util/XMacroExpander.hpp>
#include <optional>
#include <string_view>

using namespace ::dp_misc;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;

namespace dp_registry::backend::help {
namespace {


class BackendImpl : public ::dp_registry::backend::PackageRegistryBackend
{
    class PackageImpl : public ::dp_registry::backend::Package
    {
        BackendImpl * getMyBackend() const;

        // Package
        virtual beans::Optional< beans::Ambiguous<sal_Bool> > isRegistered_(
            ::osl::ResettableMutexGuard & guard,
            ::rtl::Reference<AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv ) override;
        virtual void processPackage_(
            ::osl::ResettableMutexGuard & guard,
            bool registerPackage,
            bool startup,
            ::rtl::Reference<AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv ) override;


    public:
        PackageImpl(
            ::rtl::Reference<PackageRegistryBackend> const & myBackend,
            OUString const & url, OUString const & name,
            Reference<deployment::XPackageTypeInfo> const & xPackageType,
            bool bRemoved, OUString const & identifier);

        bool extensionContainsCompiledHelp();

        //XPackage
        virtual css::beans::Optional< OUString > SAL_CALL getRegistrationDataURL() override;
    };
    friend class PackageImpl;

    // PackageRegistryBackend
    virtual Reference<deployment::XPackage> bindPackage_(
        OUString const & url, OUString const & mediaType,
        bool bRemoved, OUString const & identifier,
        Reference<XCommandEnvironment> const & xCmdEnv ) override;

    void implProcessHelp( PackageImpl * package, bool doRegisterPackage,
                          Reference<ucb::XCommandEnvironment> const & xCmdEnv);
    void implCollectXhpFiles( const OUString& aDir,
        std::vector< OUString >& o_rXhpFileVector );

    ::std::optional<HelpBackendDb::Data> readDataFromDb(std::u16string_view url);
    bool hasActiveEntry(std::u16string_view url);
    bool activateEntry(std::u16string_view url);

    Reference< ucb::XSimpleFileAccess3 > const & getFileAccess();
    Reference< ucb::XSimpleFileAccess3 > m_xSFA;

    const Reference<deployment::XPackageTypeInfo> m_xHelpTypeInfo;
    Sequence< Reference<deployment::XPackageTypeInfo> > m_typeInfos;
    std::unique_ptr<HelpBackendDb> m_backendDb;

public:
    BackendImpl( Sequence<Any> const & args,
                 Reference<XComponentContext> const & xComponentContext );

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XPackageRegistry
    virtual Sequence< Reference<deployment::XPackageTypeInfo> > SAL_CALL
        getSupportedPackageTypes() override;
    virtual void SAL_CALL packageRemoved(OUString const & url, OUString const & mediaType) override;

};


BackendImpl::BackendImpl(
    Sequence<Any> const & args,
    Reference<XComponentContext> const & xComponentContext )
    : PackageRegistryBackend( args, xComponentContext ),
      m_xHelpTypeInfo( new Package::TypeInfo(u"application/vnd.sun.star.help"_ustr,
                               OUString(),
                               DpResId(RID_STR_HELP)
                               ) ),
      m_typeInfos{ m_xHelpTypeInfo }
{
    if (transientMode())
        return;

    OUString dbFile = makeURL(getCachePath(), u"backenddb.xml"_ustr);
    m_backendDb.reset(
        new HelpBackendDb(getComponentContext(), dbFile));

    //clean up data folders which are no longer used.
    //This must not be done in the same process where the help files
    //are still registers. Only after revoking and restarting OOo the folders
    //can be removed. This works now, because the extension manager is a singleton
    //and the backends are only create once per process.
    std::vector<OUString> folders = m_backendDb->getAllDataUrls();
    deleteUnusedFolders(folders);
}

// XServiceInfo
OUString BackendImpl::getImplementationName()
{
    return u"com.sun.star.comp.deployment.help.PackageRegistryBackend"_ustr;
}

sal_Bool BackendImpl::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence< OUString > BackendImpl::getSupportedServiceNames()
{
    return { BACKEND_SERVICE_NAME };
}

// XPackageRegistry

Sequence< Reference<deployment::XPackageTypeInfo> >
BackendImpl::getSupportedPackageTypes()
{
    return m_typeInfos;
}

void BackendImpl::packageRemoved(OUString const & url, OUString const & /*mediaType*/)
{
    if (m_backendDb)
        m_backendDb->removeEntry(url);
}

// PackageRegistryBackend

Reference<deployment::XPackage> BackendImpl::bindPackage_(
    OUString const & url, OUString const & mediaType_,
    bool bRemoved, OUString const & identifier,
    Reference<XCommandEnvironment> const & xCmdEnv )
{
    // we don't support auto detection:
    if (mediaType_.isEmpty())
        throw lang::IllegalArgumentException(
            StrCannotDetectMediaType() + url,
            static_cast<OWeakObject *>(this), static_cast<sal_Int16>(-1) );

    OUString type, subType;
    INetContentTypeParameterList params;
    if (INetContentTypes::parse( mediaType_, type, subType, &params ))
    {
        if (type.equalsIgnoreAsciiCase("application"))
        {
            OUString name;
            if (!bRemoved)
            {
                ::ucbhelper::Content ucbContent(
                    url, xCmdEnv, getComponentContext() );
                name = StrTitle::getTitle( ucbContent );
            }

            if (subType.equalsIgnoreAsciiCase( "vnd.sun.star.help"))
            {
                return new PackageImpl(
                    this, url, name, m_xHelpTypeInfo, bRemoved,
                    identifier);
            }
        }
    }
    throw lang::IllegalArgumentException(
        StrUnsupportedMediaType() + mediaType_,
        static_cast<OWeakObject *>(this),
        static_cast<sal_Int16>(-1) );
}

::std::optional<HelpBackendDb::Data> BackendImpl::readDataFromDb(
    std::u16string_view url)
{
    ::std::optional<HelpBackendDb::Data> data;
    if (m_backendDb)
        data = m_backendDb->getEntry(url);
    return data;
}

bool BackendImpl::hasActiveEntry(std::u16string_view url)
{
    if (m_backendDb)
        return m_backendDb->hasActiveEntry(url);
    return false;
}

bool BackendImpl::activateEntry(std::u16string_view url)
{
    if (m_backendDb)
        return m_backendDb->activateEntry(url);
    return false;
}


BackendImpl::PackageImpl::PackageImpl(
    ::rtl::Reference<PackageRegistryBackend> const & myBackend,
    OUString const & url, OUString const & name,
    Reference<deployment::XPackageTypeInfo> const & xPackageType,
    bool bRemoved, OUString const & identifier)
    : Package( myBackend, url, name, name, xPackageType, bRemoved,
               identifier)
{
}

// Package
BackendImpl * BackendImpl::PackageImpl::getMyBackend() const
{
    BackendImpl * pBackend = static_cast<BackendImpl *>(m_myBackend.get());
    if (nullptr == pBackend)
    {
        //May throw a DisposedException
        check();
        //We should never get here...
        throw RuntimeException(u"Failed to get the BackendImpl"_ustr,
            static_cast<OWeakObject*>(const_cast<PackageImpl *>(this)));
    }
    return pBackend;
}

bool BackendImpl::PackageImpl::extensionContainsCompiledHelp()
{
    bool bCompiled = true;
    OUString aExpandedHelpURL = dp_misc::expandUnoRcUrl(getURL());

    ::osl::Directory helpFolder(aExpandedHelpURL);
    if ( helpFolder.open() == ::osl::File::E_None)
    {
        //iterate over the contents of the help folder
        //We assume that all folders within the help folder contain language specific
        //help files. If just one of them does not contain compiled help then this
        //function returns false.
        ::osl::DirectoryItem item;
        ::osl::File::RC errorNext = ::osl::File::E_None;
        while ((errorNext = helpFolder.getNextItem(item)) == ::osl::File::E_None)
        {
            //No find the language folders
            ::osl::FileStatus stat(osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileName |osl_FileStatus_Mask_FileURL);
            if (item.getFileStatus(stat) == ::osl::File::E_None)
            {
                if (stat.getFileType() != ::osl::FileStatus::Directory)
                    continue;

                //look if there is the folder help.idxl in the language folder
                OUString compUrl(stat.getFileURL() + "/help.idxl");
                ::osl::Directory compiledFolder(compUrl);
                if (compiledFolder.open() != ::osl::File::E_None)
                {
                    bCompiled = false;
                    break;
                }
            }
            else
            {
                //Error
                OSL_ASSERT(false);
                bCompiled = false;
                break;
            }
        }
        if (errorNext != ::osl::File::E_NOENT
            && errorNext != ::osl::File::E_None)
        {
            //Error
            OSL_ASSERT(false);
            bCompiled = false;
        }
    }
    return bCompiled;
}


beans::Optional< beans::Ambiguous<sal_Bool> >
BackendImpl::PackageImpl::isRegistered_(
    ::osl::ResettableMutexGuard &,
    ::rtl::Reference<AbortChannel> const &,
    Reference<XCommandEnvironment> const & )
{
    BackendImpl * that = getMyBackend();

    bool bReg = false;
    if (that->hasActiveEntry(getURL()))
        bReg = true;

    return beans::Optional< beans::Ambiguous<sal_Bool> >( true, beans::Ambiguous<sal_Bool>( bReg, false ) );
}


void BackendImpl::PackageImpl::processPackage_(
    ::osl::ResettableMutexGuard &,
    bool doRegisterPackage,
    bool /* startup */,
    ::rtl::Reference<AbortChannel> const &,
    Reference<XCommandEnvironment> const & xCmdEnv )
{
    BackendImpl* that = getMyBackend();
    that->implProcessHelp( this, doRegisterPackage, xCmdEnv);
}

beans::Optional< OUString > BackendImpl::PackageImpl::getRegistrationDataURL()
{
    if (m_bRemoved)
        throw deployment::ExtensionRemovedException();

    ::std::optional<HelpBackendDb::Data> data =
          getMyBackend()->readDataFromDb(getURL());

    if (data && getMyBackend()->hasActiveEntry(getURL()))
        return beans::Optional<OUString>(true, data->dataUrl);

    return beans::Optional<OUString>(true, OUString());
}

void BackendImpl::implProcessHelp(
    PackageImpl * package, bool doRegisterPackage,
    Reference<ucb::XCommandEnvironment> const & xCmdEnv)
{
    Reference< deployment::XPackage > xPackage(package);
    OSL_ASSERT(xPackage.is());
    if (doRegisterPackage)
    {
        //revive already processed help if possible
        if ( !activateEntry(xPackage->getURL()))
        {
            HelpBackendDb::Data data;
            data.dataUrl = xPackage->getURL();
            if (!package->extensionContainsCompiledHelp())
            {
#if HAVE_FEATURE_XMLHELP
                const OUString sHelpFolder = createFolder(xCmdEnv);
                data.dataUrl = sHelpFolder;

                Reference< ucb::XSimpleFileAccess3 > xSFA = getFileAccess();
                OUString aHelpURL = xPackage->getURL();
                OUString aExpandedHelpURL = dp_misc::expandUnoRcUrl( aHelpURL );
                if( !xSFA->isFolder( aExpandedHelpURL ) )
                {
                    OUString aErrStr = DpResId( RID_STR_HELPPROCESSING_GENERAL_ERROR ) +
                        "No help folder";
                    OWeakObject* oWeakThis = this;
                    throw deployment::DeploymentException( OUString(), oWeakThis,
                                                           Any( uno::Exception( aErrStr, oWeakThis ) ) );
                }

                // Scan languages
                for (auto& aLangURL : xSFA->getFolderContents(aExpandedHelpURL, true))
                {
                    if( xSFA->isFolder( aLangURL ) )
                    {
                        std::vector< OUString > aXhpFileVector;

                        // calculate jar file URL
                        sal_Int32 indexStartSegment = aLangURL.lastIndexOf('/');
                        // for example "/en"
                        OUString langFolderURLSegment(
                            aLangURL.copy(
                                indexStartSegment + 1, aLangURL.getLength() - indexStartSegment - 1));

                        //create the folder in the "temporary folder"
                        ::ucbhelper::Content langFolderContent;
                        const OUString langFolderDest = makeURL(sHelpFolder, langFolderURLSegment);
                        const OUString langFolderDestExpanded = ::dp_misc::expandUnoRcUrl(langFolderDest);
                        ::dp_misc::create_folder(
                            &langFolderContent,
                            langFolderDest, xCmdEnv);

                        static constexpr OUString aHelpStr(u"help"_ustr);

                        OUString aJarFile(
                            makeURL(sHelpFolder, langFolderURLSegment + "/" + aHelpStr + ".jar"));
                        aJarFile = ::dp_misc::expandUnoRcUrl(aJarFile);

                        OUString aEncodedJarFilePath = rtl::Uri::encode(
                            aJarFile, rtl_UriCharClassPchar,
                            rtl_UriEncodeIgnoreEscapes,
                            RTL_TEXTENCODING_UTF8 );
                        OUString aDestBasePath = "vnd.sun.star.zip://" +
                            aEncodedJarFilePath + "/" ;

                        sal_Int32 nLenLangFolderURL = aLangURL.getLength() + 1;

                        for (auto& aSubFolderURL : xSFA->getFolderContents(aLangURL, true))
                        {
                            if( !xSFA->isFolder( aSubFolderURL ) )
                                continue;

                            implCollectXhpFiles( aSubFolderURL, aXhpFileVector );

                            // Copy to package (later: move?)
                            std::u16string_view aPureFolderName = aSubFolderURL.subView( nLenLangFolderURL );
                            OUString aDestPath = aDestBasePath + aPureFolderName;
                            xSFA->copy( aSubFolderURL, aDestPath );
                        }

                        // Call compiler
                        sal_Int32 nXhpFileCount = aXhpFileVector.size();
                        std::unique_ptr<OUString[]> pXhpFiles(new OUString[nXhpFileCount]);
                        for( sal_Int32 iXhp = 0 ; iXhp < nXhpFileCount ; ++iXhp )
                        {
                            const OUString& aXhpFile = aXhpFileVector[iXhp];
                            pXhpFiles[iXhp] = aXhpFile.copy( nLenLangFolderURL );
                        }

                        OUString aOfficeHelpPath( SvtPathOptions().GetHelpPath() );
                        OUString aOfficeHelpPathFileURL;
                        ::osl::File::getFileURLFromSystemPath( aOfficeHelpPath, aOfficeHelpPathFileURL );

                        HelpProcessingErrorInfo aErrorInfo;
                        bool bSuccess = compileExtensionHelp(
                            aOfficeHelpPathFileURL, aHelpStr, aLangURL,
                            nXhpFileCount, pXhpFiles.get(),
                            langFolderDestExpanded, aErrorInfo );

                        pXhpFiles.reset();

                        if( bSuccess )
                        {
                            OUString aLang;
                            sal_Int32 nLastSlash = aLangURL.lastIndexOf( '/' );
                            if( nLastSlash != -1 )
                                aLang = aLangURL.copy( nLastSlash + 1 );
                            else
                                aLang = "en";

                            HelpIndexer aIndexer(aLang, u"help"_ustr, langFolderDestExpanded, langFolderDestExpanded);
                            aIndexer.indexDocuments();
                        }

                        if( !bSuccess )
                        {
                            TranslateId pErrStrId;
                            switch( aErrorInfo.m_eErrorClass )
                            {
                            case HelpProcessingErrorClass::General:      pErrStrId = RID_STR_HELPPROCESSING_GENERAL_ERROR; break;
                            case HelpProcessingErrorClass::XmlParsing:   pErrStrId = RID_STR_HELPPROCESSING_XMLPARSING_ERROR; break;
                            default: ;
                            };

                            OUString aErrStr;
                            if (pErrStrId)
                            {
                                aErrStr = DpResId(pErrStrId);

                                // Remove CR/LF
                                OUString aErrMsg( aErrorInfo.m_aErrorMsg );
                                sal_Unicode const nCR = 13, nLF = 10;
                                sal_Int32 nSearchCR = aErrMsg.indexOf( nCR );
                                sal_Int32 nSearchLF = aErrMsg.indexOf( nLF );
                                if( nSearchCR != -1 || nSearchLF != -1 )
                                {
                                    sal_Int32 nCopy;
                                    if( nSearchCR == -1 )
                                        nCopy = nSearchLF;
                                    else if( nSearchLF == -1 )
                                        nCopy = nSearchCR;
                                    else
                                        nCopy = ( nSearchCR < nSearchLF ) ? nSearchCR : nSearchLF;

                                    aErrMsg = aErrMsg.copy( 0, nCopy );
                                }
                                aErrStr += aErrMsg;
                                if (pErrStrId != RID_STR_HELPPROCESSING_XMLPARSING_ERROR && !aErrorInfo.m_aXMLParsingFile.isEmpty() )
                                {
                                    aErrStr += " in ";

                                    OUString aDecodedFile = rtl::Uri::decode( aErrorInfo.m_aXMLParsingFile,
                                                                                   rtl_UriDecodeWithCharset, RTL_TEXTENCODING_UTF8 );
                                    aErrStr += aDecodedFile;
                                    if( aErrorInfo.m_nXMLParsingLine != -1 )
                                    {
                                        aErrStr += ", line " +
                                            OUString::number( aErrorInfo.m_nXMLParsingLine );
                                    }
                                }
                            }

                            OWeakObject* oWeakThis = this;
                            throw deployment::DeploymentException( OUString(), oWeakThis,
                                                                   Any( uno::Exception( aErrStr, oWeakThis ) ) );
                        }
                    }
                }
#else
                (void) xCmdEnv;
#endif
            }
            // Writing the data entry replaces writing the flag file. If we got to this
            // point the registration was successful.
            if (m_backendDb)
                m_backendDb->addEntry(xPackage->getURL(), data);
        }
    } //if (doRegisterPackage)
    else
    {
        if (m_backendDb)
            m_backendDb->revokeEntry(xPackage->getURL());
    }
}

void BackendImpl::implCollectXhpFiles( const OUString& aDir,
    std::vector< OUString >& o_rXhpFileVector )
{
    Reference< ucb::XSimpleFileAccess3 > xSFA = getFileAccess();

    // Scan xhp files recursively
    for (auto& aURL : xSFA->getFolderContents(aDir, true))
    {
        if( xSFA->isFolder( aURL ) )
        {
            implCollectXhpFiles( aURL, o_rXhpFileVector );
        }
        else
        {
            sal_Int32 nLastDot = aURL.lastIndexOf( '.' );
            if( nLastDot != -1 )
            {
                std::u16string_view aExt = aURL.subView( nLastDot + 1 );
                if( o3tl::equalsIgnoreAsciiCase( aExt, u"xhp" ) )
                    o_rXhpFileVector.push_back( aURL );
            }
        }
    }
}

Reference< ucb::XSimpleFileAccess3 > const & BackendImpl::getFileAccess()
{
    if( !m_xSFA.is() )
    {
        Reference<XComponentContext> const & xContext = getComponentContext();
        if( xContext.is() )
        {
            m_xSFA = ucb::SimpleFileAccess::create(xContext);
        }
        if( !m_xSFA.is() )
        {
            throw RuntimeException(
                u"dp_registry::backend::help::BackendImpl::getFileAccess(), "
                "could not instantiate SimpleFileAccess."_ustr  );
        }
    }
    return m_xSFA;
}

} // anon namespace

} // namespace dp_registry

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_deployment_help_PackageRegistryBackend_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& args)
{
    return cppu::acquire(new dp_registry::backend::help::BackendImpl(args, context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
